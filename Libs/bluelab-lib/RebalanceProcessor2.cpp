//
//  RebalanceProcessor2.cpp
//  BL-Rebalance-macOS
//
//  Created by applematuer on 10/14/20.
//
//

#include <FftProcessObj16.h>
#include <RebalanceDumpFftObj2.h>
#include <RebalanceMaskPredictor8.h>
#include <RebalanceProcessFftObjComp4.h>
#include <SpectrogramDisplayScroll4.h>

#include <RebalanceProcessFftObjCompStereo.h>

#include <BLUtils.h>
#include <BLDebug.h>

#include "RebalanceProcessor2.h"

RebalanceProcessor2::RebalanceProcessor2(BL_FLOAT sampleRate,
                                         BL_FLOAT targetSampleRate,
                                         int bufferSize, int targetBufferSize,
                                         int overlapping,
                                         int numSpectroCols,
                                         bool stereoMode)
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

    mStereoMode = stereoMode;
    mDetectProcessObjStereo = NULL;
}

RebalanceProcessor2::~RebalanceProcessor2()
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

    if (mMaskPred != NULL)
        delete mMaskPred;

    if (mDetectProcessObjStereo != NULL)
        delete mDetectProcessObjStereo;
}

void
RebalanceProcessor2::InitDetect(const IPluginBase &plug)
{
    mMaskPred = new RebalanceMaskPredictor8(mTargetBufferSize, mOverlapping,
                                            mTargetSampleRate,
                                            mNumSpectroCols,
                                            plug);
    mMaskPred->SetPredictModuloNum(REBALANCE_PREDICT_MODULO_NUM);
    //mMaskPred->SetPredictModuloNum(0);

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
        vector<ProcessObj*> processObjs;

        if (!mStereoMode)
        {
            for (int i = 0; i < numChannels; i++)
            {
                RebalanceProcessFftObjComp4* obj =
                    new RebalanceProcessFftObjComp4(mBufferSize, mOverlapping,
                                                    mSampleRate,
                                                    mMaskPred,
                                                    mNumSpectroCols,
                                                    NUM_STEM_SOURCES);
                
                mDetectProcessObjs[i] = obj;
                
                processObjs.push_back(obj);
            }
        }

        mNativeFftObj = new FftProcessObj16(processObjs,
                                            numChannels, numScInputs,
                                            mTargetBufferSize,
                                            mOverlapping, 1,
                                            mTargetSampleRate);

        if (mStereoMode)
        {
            mDetectProcessObjStereo =
                new RebalanceProcessFftObjCompStereo(mBufferSize, mOverlapping,
                                                     mSampleRate,
                                                     mMaskPred,
                                                     mNumSpectroCols,
                                                     NUM_STEM_SOURCES);
            
            mNativeFftObj->AddMultichannelProcess(mDetectProcessObjStereo);
        }
            
        mNativeFftObj->SetAnalysisWindow(FftProcessObj16::ALL_CHANNELS,
                                         FftProcessObj16::WindowHanning);
        mNativeFftObj->SetSynthesisWindow(FftProcessObj16::ALL_CHANNELS,
                                          FftProcessObj16::WindowHanning);

        mNativeFftObj->SetKeepSynthesisEnergy(FftProcessObj16::ALL_CHANNELS, 0);
    }
}

void
RebalanceProcessor2::InitDump(int dumpOverlap)
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
                                            mNumSpectroCols,
                                            dumpOverlap);
        
        mTargetFftObj->AddMultichannelProcess(mDumpObj);
        
        // Optim
        mTargetFftObj->SetSkipIFft(-1, true);
    }
}

void
RebalanceProcessor2::Reset(BL_FLOAT sampleRate, int blockSize)
{
    ResampProcessObj::Reset(sampleRate, blockSize);
    
    mBlockSize = blockSize;
    
    if (mTargetFftObj != NULL)
        mTargetFftObj->Reset();

    if (mNativeFftObj != NULL)
        mNativeFftObj->Reset();
}

int
RebalanceProcessor2::GetLatency()
{
    int lat0 = ResampProcessObj::GetLatency();
    int lat1 = 0;
    if (mNativeFftObj != NULL)
        lat1 = mNativeFftObj->ComputeLatency(mBlockSize);
    int lat2 = 0;
    if (mMaskPred != NULL)
        lat2 = mMaskPred->GetLatency();

    int lat3 = 0;
    if (mDetectProcessObjs[0] != NULL)
        lat3 = mDetectProcessObjs[0]->GetLatency();
    else if (mDetectProcessObjStereo != NULL)
        lat3 = mDetectProcessObjStereo->GetLatency();
    
    int latency = lat0 + lat1 + lat2 + lat3;
    
    return latency;
}

void
RebalanceProcessor2::SetVocal(BL_FLOAT vocal)
{
    if (!mStereoMode)
    {
        for (int i = 0; i < 2; i++)
        {
            if (mDetectProcessObjs[i] != NULL)
                mDetectProcessObjs[i]->SetVocal(vocal);
        }
    }
    else
    {
        if (mDetectProcessObjStereo != NULL)
            mDetectProcessObjStereo->SetVocal(vocal);
    }
}

void
RebalanceProcessor2::SetBass(BL_FLOAT bass)
{
    if (!mStereoMode)
    {
        for (int i = 0; i < 2; i++)
        {
            if (mDetectProcessObjs[i] != NULL)
                mDetectProcessObjs[i]->SetBass(bass);
        }
    }
    else
    {
        if (mDetectProcessObjStereo != NULL)
            mDetectProcessObjStereo->SetBass(bass);
    }
}

void
RebalanceProcessor2::SetDrums(BL_FLOAT drums)
{
    if (!mStereoMode)
    {
        for (int i = 0; i < 2; i++)
        {
            if (mDetectProcessObjs[i] != NULL)
                mDetectProcessObjs[i]->SetDrums(drums);
        }
    }
    else
    {
        if (mDetectProcessObjStereo != NULL)
            mDetectProcessObjStereo->SetDrums(drums);
    }
}

void
RebalanceProcessor2::SetOther(BL_FLOAT other)
{
    if (!mStereoMode)
    {
        for (int i = 0; i < 2; i++)
        {
            if (mDetectProcessObjs[i] != NULL)
                mDetectProcessObjs[i]->SetOther(other);
        }
    }
    else
    {
        if (mDetectProcessObjStereo != NULL)
            mDetectProcessObjStereo->SetOther(other);
    }
}

void
RebalanceProcessor2::SetMasksContrast(BL_FLOAT contrast)
{
    if (!mStereoMode)
    {
        for (int i = 0; i < 2; i++)
        {
            if (mDetectProcessObjs[i] != NULL)
                mDetectProcessObjs[i]->SetContrast(contrast);
        }
    }
    else
    {
        if (mDetectProcessObjStereo != NULL)
            mDetectProcessObjStereo->SetContrast(contrast);
    }
}

void
RebalanceProcessor2::SetVocalSensitivity(BL_FLOAT vocalSensitivity)
{
    if (!mStereoMode)
    {
        for (int i = 0; i < 2; i++)
        {
            if (mDetectProcessObjs[i] != NULL)
                mDetectProcessObjs[i]->SetVocalSensitivity(vocalSensitivity);
        }
    }
    else
    {
        if (mDetectProcessObjStereo != NULL)
            mDetectProcessObjStereo->SetVocalSensitivity(vocalSensitivity);
    }
}

void
RebalanceProcessor2::SetBassSensitivity(BL_FLOAT bassSensitivity)
{
    if (!mStereoMode)
    {
        for (int i = 0; i < 2; i++)
        {
            if (mDetectProcessObjs[i] != NULL)
                mDetectProcessObjs[i]->SetBassSensitivity(bassSensitivity);
        }
    }
    else
    {
        if (mDetectProcessObjStereo != NULL)
            mDetectProcessObjStereo->SetBassSensitivity(bassSensitivity);
    }
}

void
RebalanceProcessor2::SetDrumsSensitivity(BL_FLOAT drumsSensitivity)
{
    if (!mStereoMode)
    {
        for (int i = 0; i < 2; i++)
        {
            if (mDetectProcessObjs[i] != NULL)
                mDetectProcessObjs[i]->SetDrumsSensitivity(drumsSensitivity);
        }
    }
    else
    {
        if (mDetectProcessObjStereo != NULL)
            mDetectProcessObjStereo->SetDrumsSensitivity(drumsSensitivity);
    }
}

void
RebalanceProcessor2::SetOtherSensitivity(BL_FLOAT otherSensitivity)
{
    if (!mStereoMode)
    {
        for (int i = 0; i < 2; i++)
        {
            if (mDetectProcessObjs[i] != NULL)
                mDetectProcessObjs[i]->SetOtherSensitivity(otherSensitivity);
        }
    }
    else
    {
        if (mDetectProcessObjStereo != NULL)
            mDetectProcessObjStereo->SetOtherSensitivity(otherSensitivity);
    }
}

void
RebalanceProcessor2::SetWidthVocal(BL_FLOAT widthVocal)
{
    if (mDetectProcessObjStereo != NULL)
        mDetectProcessObjStereo->SetWidthVocal(widthVocal);
}

void
RebalanceProcessor2::SetWidthBass(BL_FLOAT widthBass)
{
    if (mDetectProcessObjStereo != NULL)
        mDetectProcessObjStereo->SetWidthBass(widthBass);
}

void
RebalanceProcessor2::SetWidthDrums(BL_FLOAT widthDrums)
{
    if (mDetectProcessObjStereo != NULL)
        mDetectProcessObjStereo->SetWidthDrums(widthDrums);
}

void
RebalanceProcessor2::SetWidthOther(BL_FLOAT widthOther)
{
    if (mDetectProcessObjStereo != NULL)
        mDetectProcessObjStereo->SetWidthOther(widthOther);
}

void
RebalanceProcessor2::SetPanVocal(BL_FLOAT panVocal)
{
    if (mDetectProcessObjStereo != NULL)
        mDetectProcessObjStereo->SetPanVocal(panVocal);
}

void
RebalanceProcessor2::SetPanBass(BL_FLOAT panBass)
{
    if (mDetectProcessObjStereo != NULL)
        mDetectProcessObjStereo->SetPanBass(panBass);
}

void
RebalanceProcessor2::SetPanDrums(BL_FLOAT panDrums)
{
    if (mDetectProcessObjStereo != NULL)
        mDetectProcessObjStereo->SetPanDrums(panDrums);
}

void
RebalanceProcessor2::SetPanOther(BL_FLOAT panOther)
{
    if (mDetectProcessObjStereo != NULL)
        mDetectProcessObjStereo->SetPanOther(panOther);
}

void
RebalanceProcessor2::SetWidthBoost(bool flag)
{
    if (mDetectProcessObjStereo != NULL)
        mDetectProcessObjStereo->SetWidthBoost(flag);
}

bool
RebalanceProcessor2::HasEnoughDumpData()
{
    if (mDumpObj != NULL)
    {
        bool result = mDumpObj->HasEnoughData();
        
        return result;
    }
    
    return false;
}

void
RebalanceProcessor2::
GetSpectroData(WDL_TypedBuf<BL_FLOAT> data[REBALANCE_NUM_SPECTRO_COLS])
{
    if (mDumpObj != NULL)
        mDumpObj->GetSpectrogramData(data);
}

void
RebalanceProcessor2::
GetStereoData(WDL_TypedBuf<BL_FLOAT> data[REBALANCE_NUM_SPECTRO_COLS])
{
    if (mDumpObj != NULL)
        mDumpObj->GetStereoData(data);
}

BLSpectrogram4 *
RebalanceProcessor2::GetSpectrogram()
{
    if (mDetectProcessObjs[0] != NULL)
    {
        BLSpectrogram4 *spect = mDetectProcessObjs[0]->GetSpectrogram();

        return spect;
    }
    else if (mDetectProcessObjStereo != NULL)
    {
        BLSpectrogram4 *spect = mDetectProcessObjStereo->GetSpectrogram();

        return spect;
    }

    return NULL;
}

void
RebalanceProcessor2::
SetSpectrogramDisplay(SpectrogramDisplayScroll4 *spectroDisplay)
{
    if (mDetectProcessObjs[0] != NULL)
        mDetectProcessObjs[0]->SetSpectrogramDisplay(spectroDisplay);
    else if (mDetectProcessObjStereo != NULL)
        mDetectProcessObjStereo->SetSpectrogramDisplay(spectroDisplay);
}

void
RebalanceProcessor2::RecomputeSpectrogram(bool recomputeMasks)
{
    if (mDetectProcessObjs[0] != NULL)
        mDetectProcessObjs[0]->RecomputeSpectrogram(recomputeMasks);
    else if (mDetectProcessObjStereo != NULL)
        mDetectProcessObjStereo->RecomputeSpectrogram(recomputeMasks);
}

void
RebalanceProcessor2::SetModelNum(int modelNum)
{
    if (mMaskPred != NULL)
        mMaskPred->SetModelNum(modelNum);
}

void
RebalanceProcessor2::SetPredictModuloNum(int modNum)
{
    if (mMaskPred != NULL)
        mMaskPred->SetPredictModuloNum(modNum);
}

void
RebalanceProcessor2::SetDbgThreshold(BL_FLOAT thrs)
{
#if 0
    if (mMaskPred != NULL)
        mMaskPred->SetDbgThreshold(thrs);
#endif
}

bool
RebalanceProcessor2::
ProcessSamplesBuffers(vector<WDL_TypedBuf<BL_FLOAT> > *ioBuffers,
                      vector<WDL_TypedBuf<BL_FLOAT> > *ioResampBuffer)
{
    if (mDumpObj != NULL)
    // Dump mode
    {
        vector<WDL_TypedBuf<BL_FLOAT> > &monoBuffer = mTmpBuf0;
        monoBuffer.resize(1);
        //monoBuffer = *ioResampBuffer;
        //BLUtils::StereoToMono(&monoBuffer);
        //monoBuffer.resize(1);
        BLUtils::StereoToMono(&monoBuffer[0], *ioResampBuffer);
        
        // Here, we fill the dump obj with the spectrogram cols
        vector<WDL_TypedBuf<BL_FLOAT> > &scIn = mTmpBuf1;
        mTargetFftObj->Process(monoBuffer, scIn, NULL);
        
        return false;
    }
    
    if ((mDetectProcessObjs[0] != NULL) ||
        (mDetectProcessObjStereo != NULL))
    {
        vector<WDL_TypedBuf<BL_FLOAT> > &monoBuffer = mTmpBuf2;
        monoBuffer.resize(1);
        //monoBuffer = *ioResampBuffer;
        //BLUtils::StereoToMono(&monoBuffer);
        //monoBuffer.resize(1);
        BLUtils::StereoToMono(&monoBuffer[0], *ioResampBuffer);
        
        // Here, we detect mask
        vector<WDL_TypedBuf<BL_FLOAT> > &scIn0 = mTmpBuf3;
        mTargetFftObj->Process(monoBuffer, scIn0, NULL);
        
        // Here, we apply the detected mask (after having scaled it)
        vector<WDL_TypedBuf<BL_FLOAT> > &scIn1 = mTmpBuf4;
        
        vector<WDL_TypedBuf<BL_FLOAT> > &outResult = mTmpBuf5;
        outResult = *ioBuffers;
        BLUtils::FillAllZero(&outResult);
        
        mNativeFftObj->Process(*ioBuffers, scIn1, &outResult);
        
        *ioBuffers = outResult;
        
        return false;
    }
    
    return false;
}
