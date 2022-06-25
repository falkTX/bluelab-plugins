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
//  StereoDeReverbProcess.cpp
//  BL-Pano
//
//  Created by Pan on 02/06/18.
//
//

#include <BLSpectrogram4.h>
#include <GraphControl11.h>
#include <SpectrogramDisplay.h>
#include <ImageDisplay.h>
#include <DUETSeparator3.h>

#include <BLUtils.h>
#include <BLUtilsComp.h>
#include <BLUtilsFft.h>

#include "StereoDeReverbProcess.h"

// Paper: 35x50
#define HISTO_SIZE 32 //64

#define TIME_SMOOTH 0.75
#define ALPHA_ZOOM 1.0
#define DELTA_ZOOM 1.0

//#define SOFT_MASK_SIZE 8
#define SOFT_MASK_SIZE 16

#define USE_GRADIENT_MASKS 1

//
#define OVERMIX_FACTOR 4.0 //8.0

#define PROCESS_MASK_CENTERED 1


StereoDeReverbProcess::StereoDeReverbProcess(int bufferSize, int oversampling,
                                             int freqRes, BL_FLOAT sampleRate)
: MultichannelProcess()
{
    MultichannelProcess::Reset(bufferSize, oversampling, freqRes, sampleRate);
    
    //
    mBufferSize = bufferSize;
    mOverlapping = oversampling;
    mFreqRes = freqRes;
    mSampleRate = sampleRate;
    
    //
    mSeparator = new DUETSeparator3(HISTO_SIZE, mSampleRate,
                                    bufferSize, oversampling,
                                    SOFT_MASK_SIZE);
    
#if 0
    mUsePhaseAliasingCorrection = false;
#endif
    
    mSeparator->SetTimeSmooth(TIME_SMOOTH);
    mSeparator->SetAlphaZoom(ALPHA_ZOOM);
    mSeparator->SetDeltaZoom(DELTA_ZOOM);
    
    mSeparator->SetThresholdFloor(0.0);
    
    mSeparator->SetUsePhaseAliasingCorrection(false);
    
    mSeparator->SetThresholdPeaks(0.0);
    mSeparator->SetThresholdPeaksWidth(0.0);
    
    mSeparator->SetUseKernelSmooth(true);
    
    mSeparator->SetSoftMaskSize(SOFT_MASK_SIZE);
    
    mUseSoftMasksComp = true;
    mSeparator->SetUseSoftMasksComp(true);
    //mSeparator->SetUseSoftMasks(true);
    
    mSeparator->SetUseGradientMasks(USE_GRADIENT_MASKS);
    mSeparator->SetThresholdAll(true);
    
    mSeparator->SetInvertPickSelection(false);
    
    //
    mOutputReverbOnly = false;
    mMix = 0.5;
}

StereoDeReverbProcess::~StereoDeReverbProcess()
{
    delete mSeparator;
}

void
StereoDeReverbProcess::ProcessInputFft(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > *ioFftSamples,
                                       const vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > *scBuffer)
{
    if (ioFftSamples->size() != 2)
        return;
    
    //
    WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffers0 = mTmpBuf0;
    ioBuffers0[0] = *(*ioFftSamples)[0];
    ioBuffers0[1] = *(*ioFftSamples)[1];

    WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffers = mTmpBuf1;
    
    //BLUtils::TakeHalf(&ioBuffers[0]);
    //BLUtils::TakeHalf(&ioBuffers[1]);
    BLUtils::TakeHalf(ioBuffers0[0], &ioBuffers[0]);
    BLUtils::TakeHalf(ioBuffers0[1], &ioBuffers[1]);
    
    
    WDL_TypedBuf<BL_FLOAT> *magns = mTmpBuf2;
    WDL_TypedBuf<BL_FLOAT> *phases = mTmpBuf3;
    BLUtilsComp::ComplexToMagnPhase(&magns[0], &phases[0], ioBuffers[0]);
    BLUtilsComp::ComplexToMagnPhase(&magns[1], &phases[1], ioBuffers[1]);
    
    //
    // Must set magns anyway
    mSeparator->SetInputData(magns, phases);
    
    if (mUseSoftMasksComp)
        mSeparator->SetInputDataComp(ioBuffers);
    
    Process();
    
    
    // Complex soft masks!
    WDL_TypedBuf<WDL_FFT_COMPLEX> *deReverb = mTmpBuf4;
    mSeparator->GetOutputDataComp(deReverb);
    
#if PROCESS_MASK_CENTERED
    mSeparator->GetDelayedInputDataComp(ioBuffers);
#endif
    
    //WDL_TypedBuf<WDL_FFT_COMPLEX> reverb[2] = { ioBuffers[0], ioBuffers[1] };
    WDL_TypedBuf<WDL_FFT_COMPLEX> *reverb = mTmpBuf5;
    reverb[0] = ioBuffers[0];
    reverb[1] = ioBuffers[1];
    
    for (int i = 0; i < 2; i++)
    {
        BLUtils::SubstractValues(&reverb[i], deReverb[i]);
    }
        
    if (mOutputReverbOnly)
    {
        ioBuffers[0] = reverb[0];
        ioBuffers[1] = reverb[1];
    }        
    else
    {
        BL_FLOAT revFactor = 1.0;
        if (mMix < 0.0)
        {
            revFactor = 1.0 + mMix;
        }
        else
        {
            revFactor = 1.0 + mMix*(OVERMIX_FACTOR - 1.0);
        }
            
        for (int i = 0; i < 2; i++)
        {
            BLUtils::MultValues(&reverb[i], revFactor);
        }
        
        for (int i = 0; i < 2; i++)
        {
            for (int k = 0; k < ioBuffers[i].GetSize(); k++)
            {
                WDL_FFT_COMPLEX deRevVal = deReverb[i].Get()[k];
                WDL_FFT_COMPLEX revVal = reverb[i].Get()[k];
                
                WDL_FFT_COMPLEX result;
                result.re = deRevVal.re + revVal.re;
                result.im = deRevVal.im + revVal.im;
                
                ioBuffers[i].Get()[k] = result;
            }
        }
    }
        
    //
    //BLUtils::ResizeFillZeros(&ioBuffers[0], ioBuffers[0].GetSize()*2);
    //BLUtils::ResizeFillZeros(&ioBuffers[1], ioBuffers[1].GetSize()*2);

    BLUtils::SetBuf(&ioBuffers0[0], ioBuffers[0]);
    BLUtils::SetBuf(&ioBuffers0[1], ioBuffers[1]);
    
    BLUtilsFft::FillSecondFftHalf(&ioBuffers0[0]);
    BLUtilsFft::FillSecondFftHalf(&ioBuffers0[1]);
    
    *(*ioFftSamples)[0] = ioBuffers0[0];
    *(*ioFftSamples)[1] = ioBuffers0[1];
}

#if 0
void
StereoDeReverbProcess::ProcessInputSamplesWin(vector<WDL_TypedBuf<BL_FLOAT> * > *ioSamples,
                                              const vector<WDL_TypedBuf<BL_FLOAT> > *scBuffer)
{
    if (!mUsePhaseAliasingCorrection)
        return;
    
    if (ioSamples->size() != 2)
        return;
    
    WDL_TypedBuf<BL_FLOAT> samples[2] = { *(*ioSamples)[0], *(*ioSamples)[1] };
    
    int oversampling = mSeparator->GetPACOversampling();
    for (int i = 0; i < 2; i++)
    {
        BLUtils::ResizeFillZeros(&samples[i], samples[i].GetSize()*oversampling);
        FftProcessObj16::ComputeFft(samples[i], &mPACOversampledFft[i], oversampling);
        
        BLUtils::TakeHalf(&mPACOversampledFft[i]);
        BLUtils::MultValues(&mPACOversampledFft[i],  0.5); // Hack
    }
    
    mSeparator->SetPACOversamplesFft(mPACOversampledFft);
}
#endif

void
StereoDeReverbProcess::Reset(int bufferSize, int oversampling, int freqRes, BL_FLOAT sampleRate)
{
    MultichannelProcess::Reset(bufferSize, oversampling, freqRes, sampleRate);
    
    mBufferSize = bufferSize;
    mOverlapping = oversampling;
    mFreqRes = freqRes;
    mSampleRate = sampleRate;
    
    mSeparator->Reset(sampleRate, bufferSize, oversampling);
    
#if 0
    for (int i = 0; i < 2; i++)
        mPACOversampledFft[i].Resize(0);
#endif
}

void
StereoDeReverbProcess::SetMix(BL_FLOAT mix)
{
    mMix = mix;
}

void
StereoDeReverbProcess::SetOutputReverbOnly(bool flag)
{
    mOutputReverbOnly = flag;
}

void
StereoDeReverbProcess::SetThreshold(BL_FLOAT threshold)
{
    mSeparator->SetThresholdFloor(threshold);
}

#if 0
void
StereoDeReverbProcess::SetUsePhaseAliasingCorrection(bool flag)
{
    mUsePhaseAliasingCorrection = flag;
    mSeparator->SetUsePhaseAliasingCorrection(flag);
}
#endif

int
StereoDeReverbProcess::GetAdditionalLatency()
{
    int lat = mSeparator->GetAdditionalLatency();
    
    return lat;
}

void
StereoDeReverbProcess::Process()
{
    mSeparator->Process();
}
