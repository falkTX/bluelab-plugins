/* Copyright (C) 2022 Nicolas Dittlo <deadlab.plugins@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this software; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
 
//
//  RebalanceProcessor.cpp
//  BL-Rebalance-macOS
//
//  Created by applematuer on 10/14/20.
//
//

#include <FftProcessObj16.h>
#include <RebalanceDumpFftObj2.h>
#include <RebalanceMaskPredictorComp6.h>
#include <RebalanceProcessFftObjComp3.h>
#include <BLUtils.h>
#include <BLDebug.h>

#include "RebalanceProcessor.h"

RebalanceProcessor::RebalanceProcessor(BL_FLOAT sampleRate, BL_FLOAT targetSampleRate,
                                       int bufferSize, int targetBufferSize,
                                       int overlapping,
                                       int numSpectroCols)
: ResampProcessObj(targetSampleRate, sampleRate, true)
{
    mNativeFftObj = NULL;
    mTargetFftObj = NULL;
    
    mDumpObj = NULL;
    
    mMaskPred = NULL;
    
    for (int i = 0; i < 2; i++)
        mDetectProcessObjs[i] = NULL;
    
    mBufferSize = bufferSize;
    mTargetBufferSize = targetBufferSize;
    
    mOverlapping = overlapping;
    
    mNumSpectroCols = numSpectroCols;
    
    mBlockSize = bufferSize;
}

RebalanceProcessor::~RebalanceProcessor()
{
    if (mNativeFftObj != NULL)
        delete mNativeFftObj;
    
    if (mTargetFftObj != NULL)
        delete mTargetFftObj;
    
    if (mDumpObj != NULL)
        delete mDumpObj;
    
    for (int i = 0; i < 2; i++)
    {
        if (mDetectProcessObjs[i] != NULL)
            delete mDetectProcessObjs[i];
    }
}

void
RebalanceProcessor::InitDetect(const IPluginBase &plug,
                               IGraphics &graphics)
{
    mMaskPred = new RebalanceMaskPredictorComp6(mTargetBufferSize, mOverlapping,
                                                mTargetSampleRate,
                                                mNumSpectroCols,
                                                plug, graphics);
    mMaskPred->SetPredictModuloNum(REBALANCE_PREDICT_MODULO_NUM);
    
    if (mTargetFftObj == NULL)
    {
        int numChannels = 2;
        int numScInputs = 1;
        
        vector<ProcessObj *> processObjs;
        mTargetFftObj = new FftProcessObj16(processObjs,
                                            numChannels, numScInputs,
                                            mTargetBufferSize,
                                            mOverlapping, 1,
                                            mTargetSampleRate);
        
        mTargetFftObj->SetAnalysisWindow(FftProcessObj16::ALL_CHANNELS,
                                         FftProcessObj16::WindowHanning);
        mTargetFftObj->SetSynthesisWindow(FftProcessObj16::ALL_CHANNELS,
                                          FftProcessObj16::WindowHanning);
        
        mTargetFftObj->SetKeepSynthesisEnergy(FftProcessObj16::ALL_CHANNELS, 0);
        
        // For detection, add MaskPredictor (multichannel process)
        mTargetFftObj->AddMultichannelProcess(mMaskPred);
        
        // Optim
        mTargetFftObj->SetSkipIFft(-1, true);
    }
    
    if (mNativeFftObj == NULL)
    {
        int numChannels = 2;
        int numScInputs = 1;
        
        // For applying the mask, use RebalanceProcessFftObjComp3
        vector<ProcessObj *> processObjs;
        for (int i = 0; i < numChannels; i++)
        {
            RebalanceProcessFftObjComp3 *obj =
                new RebalanceProcessFftObjComp3(mBufferSize,
                                                mMaskPred,
                                                mNumSpectroCols,
                                                NUM_STEM_SOURCES);
            
            mDetectProcessObjs[i] = obj;
            
            processObjs.push_back(obj);
        }
        
        mNativeFftObj = new FftProcessObj16(processObjs,
                                            numChannels, numScInputs,
                                            mTargetBufferSize,
                                            mOverlapping, 1,
                                            mTargetSampleRate);
        
        mNativeFftObj->SetAnalysisWindow(FftProcessObj16::ALL_CHANNELS,
                                         FftProcessObj16::WindowHanning);
        mNativeFftObj->SetSynthesisWindow(FftProcessObj16::ALL_CHANNELS,
                                          FftProcessObj16::WindowHanning);
        
        mNativeFftObj->SetKeepSynthesisEnergy(FftProcessObj16::ALL_CHANNELS, 0);
    }
}

void
RebalanceProcessor::InitDump()
{
    if (mTargetFftObj == NULL)
    {
        int numChannels = 2;
        int numScInputs = 1;
        
        vector<ProcessObj *> processObjs;
        mTargetFftObj = new FftProcessObj16(processObjs,
                                            numChannels, numScInputs,
                                            mTargetBufferSize,
                                            mOverlapping, 1,
                                            mTargetSampleRate);
            
        mTargetFftObj->SetAnalysisWindow(FftProcessObj16::ALL_CHANNELS,
                                         FftProcessObj16::WindowHanning);
        mTargetFftObj->SetSynthesisWindow(FftProcessObj16::ALL_CHANNELS,
                                          FftProcessObj16::WindowHanning);
            
        mTargetFftObj->SetKeepSynthesisEnergy(FftProcessObj16::ALL_CHANNELS, 0);
        
        // Dump stereo to mono signal
        mDumpObj = new RebalanceDumpFftObj2(mTargetBufferSize,
                                            mTargetSampleRate,
                                            mNumSpectroCols);
        
        mTargetFftObj->AddMultichannelProcess(mDumpObj);
        
        // Optim
        mTargetFftObj->SetSkipIFft(-1, true);
    }
}

void
RebalanceProcessor::Reset(BL_FLOAT sampleRate, int blockSize)
{
    ResampProcessObj::Reset(sampleRate, blockSize);
    
    mBlockSize = blockSize;
    
    if (mTargetFftObj != NULL)
        mTargetFftObj->Reset();
}

int
RebalanceProcessor::GetLatency()
{
    int lat0 = ResampProcessObj::GetLatency();
    int lat1 = mNativeFftObj->ComputeLatency(mBlockSize);
    int lat2 = mMaskPred->GetLatency();
    
    int latency = lat0 + lat1 + lat2;
    
    return latency;
}

void
RebalanceProcessor::SetVocal(BL_FLOAT vocal)
{
    for (int i = 0; i < 2; i++)
        mDetectProcessObjs[i]->SetVocal(vocal);
}

void
RebalanceProcessor::SetBass(BL_FLOAT bass)
{
    for (int i = 0; i < 2; i++)
        mDetectProcessObjs[i]->SetBass(bass);
}

void
RebalanceProcessor::SetDrums(BL_FLOAT drums)
{
    for (int i = 0; i < 2; i++)
        mDetectProcessObjs[i]->SetDrums(drums);
}

void
RebalanceProcessor::SetOther(BL_FLOAT other)
{
    for (int i = 0; i < 2; i++)
        mDetectProcessObjs[i]->SetOther(other);
}

void
RebalanceProcessor::SetMasksContrast(BL_FLOAT contrast)
{
    for (int i = 0; i < 2; i++)
        mDetectProcessObjs[i]->SetMasksContrast(contrast);
}

void
RebalanceProcessor::SetVocalSensitivity(BL_FLOAT vocalSensitivity)
{
    mMaskPred->SetVocalSensitivity(vocalSensitivity);
}

void
RebalanceProcessor::SetBassSensitivity(BL_FLOAT bassSensitivity)
{
    mMaskPred->SetBassSensitivity(bassSensitivity);
}

void
RebalanceProcessor::SetDrumsSensitivity(BL_FLOAT drumsSensitivity)
{
    mMaskPred->SetDrumsSensitivity(drumsSensitivity);
}

void
RebalanceProcessor::SetOtherSensitivity(BL_FLOAT otherSensitivity)
{
    mMaskPred->SetOtherSensitivity(otherSensitivity);
}

bool
RebalanceProcessor::HasEnoughDumpData()
{
    if (mDumpObj != NULL)
    {
        bool result = mDumpObj->HasEnoughData();
        
        return result;
    }
    
    return false;
}

void
RebalanceProcessor::GetSpectroData(WDL_TypedBuf<BL_FLOAT> data[REBALANCE_NUM_SPECTRO_COLS])
{
    if (mDumpObj != NULL)
        mDumpObj->GetSpectrogramData(data);
}

void
RebalanceProcessor::GetStereoData(WDL_TypedBuf<BL_FLOAT> data[REBALANCE_NUM_SPECTRO_COLS])
{
    if (mDumpObj != NULL)
        mDumpObj->GetStereoData(data);
}

void
RebalanceProcessor::SetDbgThreshold(BL_FLOAT thrs)
{
#if 0
    mMaskPred->SetDbgThreshold(thrs);
#endif
}

bool
RebalanceProcessor::ProcessSamplesBuffers(vector<WDL_TypedBuf<BL_FLOAT> > *ioBuffers,
                                          vector<WDL_TypedBuf<BL_FLOAT> > *ioResampBuffer)
{
    if (mDumpObj != NULL)
    // Dump mode
    {
        vector<WDL_TypedBuf<BL_FLOAT> > monoBuffer = *ioResampBuffer;
        BLUtils::StereoToMono(&monoBuffer);
        monoBuffer.resize(1);
        
        // Here, we fill the dump obj with the spectrogram cols
        vector<WDL_TypedBuf<BL_FLOAT> > scIn;
        mTargetFftObj->Process(monoBuffer, scIn, NULL);
        
        return false;
    }
    
    if (mDetectProcessObjs[0] != NULL)
    {
        vector<WDL_TypedBuf<BL_FLOAT> > monoBuffer = *ioResampBuffer;
        BLUtils::StereoToMono(&monoBuffer);
        monoBuffer.resize(1);
        
        // Here, we detect mask
        vector<WDL_TypedBuf<BL_FLOAT> > scIn0;
        mTargetFftObj->Process(monoBuffer, scIn0, NULL);
        
        // Here, we apply the detected mask (after having scaled it)
        vector<WDL_TypedBuf<BL_FLOAT> > scIn1;
        
        vector<WDL_TypedBuf<BL_FLOAT> > outResult;
        outResult = *ioBuffers;
        BLUtils::FillAllZero(&outResult);
        
        mNativeFftObj->Process(*ioBuffers, scIn1, &outResult);
        
        *ioBuffers = outResult;
        
        return false;
    }
    
    return false;
}
