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
//  USTDepthProcess4.cpp
//  UST
//
//  Created by applematuer on 12/3/19.
//
//

#include <JReverb.h>
#include <DelayObj4.h>
#include <FilterSincConvoBandPass.h>
#include <FilterRBJNX.h>
#include <EarlyReflections2.h>

#include <USTStereoWidener.h>


#include <BLUtils.h>

#include "USTDepthProcess4.h"

// Like StageOne
//

// The effect is very subtle...
// Looks to add a bit depth (More like StageOne),
// but seems to make a beginning of very little metallic sound on snare test.
#define USE_PREDELAY 0 //1

// 8ms => 2.7m
//#define PREDELAY_MS 15.0 // 8ms ?
#define PREDELAY_MS 8.0 // Sounds mor like StageOne with 8ms
//#define PREDELAY_MS 4.0 // Like StateOne ?

//#define LOW_PASS_FREQ 10000.0 // like StageOne
#define LOW_PASS_FREQ 16384.0 // To manage well brillance
// For standard low pass
//#define LOW_CUT_FREQ 100.0 //500.0
// For Sinc filter
#define LOW_CUT_FREQ 100.0

// NOTE: to have a wider depth when input is mono
#define USE_POST_STEREO_WIDEN 1
#define POST_STEREO_WIDEN_FACTOR 1.0

// Volume compensation when processing one reverb by channel
// (since the reverb "dry" is not 0)
#define STEREO_FIX_DB -6.0


// Use sinc bandbass filter
// Takes 1% CPU only (with block size 64, we gain 2/3%)
#define USE_SINC_FILTER 0 //1
// Under 1024, impossible to cut at 100Hz (it rather cuts at 50Hz)
// With block size 64, it cuts at 400Hz if we ask 100Hz
// This looks normal: to have 1 perdiod of a 50Hz signal, we must have around 850 samples
// at the minimum.
#define SINC_FILTER_SIZE 64 //1024//512 //1024 // 2048

// Better to use RBJ, because with sinc, we would need a block size of 1024 at the minimum
// (to catch 100Hz low cut filter)
#define USE_RBJ_FILTERS 1 //
//#define RBJ_FILTERS_NUM_FILTERS 16
// With order 4 instead of 16, we gain 1/2%
#define RBJ_FILTERS_NUM_FILTERS 4

// Compute only reverb in JReverb, not mix with dry signal
// Re-add dry signal in the current class.
// This makes possible to avoid also filtering the dry signal
#define FIX_DRY 1

// useReverbTail, dry, wet, roomSize, width, damping, useFilter,
// useEarly, earlyRoomSize, earlyIntermicDist, earlyNormDepth, earlyOrder, earlyReflectCoeff
//BL_FLOAT _config[13] = { 1, /*1*/0, 0.180556, 0, 1, 0.333333, 1, 1, 2.8, 0.625, 0.181, 2, 1.0 };

// Only early, order 1
//BL_FLOAT _config[13] = { 0, 1, 0.180556, 0, 1, 1, 0, 1, 20, 0.1, 1, 1, 0.208333 };

// Only early, order 1 (fixed attenuation)
BL_FLOAT _config[13] = { 0, /*1*/0, 0.180556, 0, 1, 1, 1, 1, 20, 0.1, 0.25, 1, 0.868056 };

USTDepthProcess4::USTDepthProcess4(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    
    JReverb::JReverbParams revParams;
    
    // TODO: set reverb params here
    
    for (int i = 0; i < 2; i++)
    {
        mReverbs[i] = new JReverb();
        mReverbs[i]->SetSampleRate(sampleRate);
        
#if FIX_DRY
        // Set dry to 0, and manage dry in the current class
        revParams.mDryLevel = 0.0;
#endif
        
        mReverbs[i]->SetParams(revParams);
    }
    
    JReverb::JReverbParams bypassParams;
    bypassParams.mDryLevel = 1.0;
    bypassParams.mWetLevel = 0.0;
    for (int i = 0; i < 2; i++)
    {
        mBypassReverbs[i] = new JReverb();
        mBypassReverbs[i]->SetSampleRate(sampleRate);
        mBypassReverbs[i]->SetParams(bypassParams);
    }

    mPreDelays[0] = NULL;
    mPreDelays[1] = NULL;
    
#if USE_PREDELAY
    BL_FLOAT delay = (PREDELAY_MS/1000.0)*sampleRate;
    mPreDelays[0] = new DelayObj4(delay);
    mPreDelays[1] = new DelayObj4(delay);
#endif
    
#if USE_POST_STEREO_WIDEN
    mStereoWiden = new USTStereoWidener();
#endif

    for (int i = 0; i < 2; i++)
    {
        mSincBandFilters[i] = NULL;
    }
    
#if USE_SINC_FILTER
    for (int i = 0; i < 2; i++)
    {
        mSincBandFilters[i] = new SincConvoBandPassFilter();
        mSincBandFilters[i]->Init(LOW_CUT_FREQ, LOW_PASS_FREQ, sampleRate, SINC_FILTER_SIZE);
    }
#endif
  
#if USE_RBJ_FILTERS
    for (int i = 0; i < 2; i++)
    {
        // Order 16, to be very stiff (like StageOne)
        // NOTE: maybe 8 could be sufficient, needs to be tested...
        mLowPassFilters[i] = new FilterRBJNX(RBJ_FILTERS_NUM_FILTERS/*16*//*4*/, FILTER_TYPE_LOWPASS,
                                            sampleRate, LOW_PASS_FREQ);
    }

    for (int i = 0; i < 2; i++)
    {
        // NOTE: order not tested at all for low cut..
        
        // Order 16, to be very stiff (like StageOne)
        // NOTE: maybe 8 could be sufficient, needs to be tested...
        mLowCutFilters[i] = new FilterRBJNX(RBJ_FILTERS_NUM_FILTERS/*16*//*4*/, FILTER_TYPE_HIPASS,
                                           sampleRate, LOW_CUT_FREQ);
    }
#endif    
    
    for (int i = 0; i < 2; i++)
        mEarlyRef[i] = new EarlyReflections2(sampleRate);
    
    mUseFilter = false;
    mUseEarly = false;
    
    mDryGain = 1.0;
    
    mUseReverbTail = true;
    //
    LoadConfig(_config);
}

USTDepthProcess4::USTDepthProcess4(const USTDepthProcess4 &other)
{
    mSampleRate = other.mSampleRate;
    
    for (int i = 0; i < 2; i++)
    {
        mReverbs[i] = new JReverb(*other.mReverbs[i]);
    }
    
    for (int i = 0; i < 2; i++)
    {
        mBypassReverbs[i] = new JReverb(*other.mBypassReverbs[i]);
    }

    mPreDelays[0] = NULL;
    mPreDelays[1] = NULL;
    
#if USE_PREDELAY
    mPreDelays[0] = new DelayObj4(*other.mPreDelays[0]);
    mPreDelays[1] = new DelayObj4(*other.mPreDelays[1]);
#endif
    
#if USE_POST_STEREO_WIDEN
    mStereoWiden = new USTStereoWidener(*other.mStereoWiden);
#endif
    
#if USE_SINC_FILTER
    for (int i = 0; i < 2; i++)
    {
        mSincBandFilters[i] = new SincConvoBandPassFilter(*other.mSincBandFilters[i]);
    }
#endif
    
#if USE_RBJ_FILTERS
    for (int i = 0; i < 2; i++)
    {
        // Order 16, to be very stiff (like StageOne)
        // NOTE: maybe 8 could be sufficient, needs to be tested...
        mLowPassFilters[i] = new FilterRBJNX(16/*4*/, FILTER_TYPE_LOWPASS,
                                            other.mSampleRate, LOW_PASS_FREQ);
    }
    
    for (int i = 0; i < 2; i++)
    {
        // NOTE: order not tested at all for low cut..
        
        // Order 16, to be very stiff (like StageOne)
        // NOTE: maybe 8 could be sufficient, needs to be tested...
        mLowCutFilters[i] = new FilterRBJNX(16/*4*/, FILTER_TYPE_HIPASS,
                                           other.mSampleRate, LOW_CUT_FREQ);
    }
#endif
    
    mUseEarly = other.mUseEarly;
    
    for (int i = 0; i < 2; i++)
        mEarlyRef[i] = new EarlyReflections2(*other.mEarlyRef[i]);
    
    mUseFilter = other.mUseFilter;
    
    mDryGain = other.mDryGain;
    
    mUseReverbTail = other.mUseReverbTail;
}

USTDepthProcess4::~USTDepthProcess4()
{
    for (int i = 0; i < 2; i++)
    {
        delete mReverbs[i];
    }
    
    for (int i = 0; i < 2; i++)
    {
        delete mBypassReverbs[i];
    }
    
#if USE_PREDELAY
    delete mPreDelays[0];
    delete mPreDelays[1];
#endif
    
#if USE_POST_STEREO_WIDEN
    delete mStereoWiden;
#endif
    
#if USE_SINC_FILTER
    for (int i = 0; i < 2; i++)
    {
        delete mSincBandFilters[i];
    }
#endif
    
#if USE_RBJ_FILTERS
    for (int i = 0; i < 2; i++)
    {
        delete mLowPassFilters[i];
    }

    for (int i = 0; i < 2; i++)
    {
        delete mLowCutFilters[i];
    }
#endif
    
    for (int i = 0; i < 2; i++)
        delete mEarlyRef[i];
}

void
USTDepthProcess4::Reset(BL_FLOAT sampleRate, int blockSize)
{
    mSampleRate = sampleRate;
    
    for (int i = 0; i < 2; i++)
    {
        mReverbs[i]->Reset();
        mReverbs[i]->SetSampleRate(sampleRate);
    }
    
    for (int i = 0; i < 2; i++)
    {
        mBypassReverbs[i]->Reset();
        mBypassReverbs[i]->SetSampleRate(sampleRate);
    }
    
#if USE_PREDELAY
    for (int i = 0; i < 2; i++)
    {
        mPreDelays[i]->Reset();
    
        BL_FLOAT delay = (PREDELAY_MS/1000.0)*sampleRate;
        mPreDelays[i]->SetDelay(delay);
    }
#endif
    
#if USE_SINC_FILTER
    for (int i = 0; i < 2; i++)
        mSincBandFilters[i]->Reset(sampleRate, blockSize);
#endif
    
#if USE_RBJ_FILTERS
    for (int i = 0; i < 2; i++)
    {
        mLowPassFilters[i]->SetSampleRate(sampleRate);
    }

    for (int i = 0; i < 2; i++)
    {
        mLowCutFilters[i]->SetSampleRate(sampleRate);
    }
#endif
    
    for (int i = 0; i < 2; i++)
        mEarlyRef[i]->Reset(sampleRate);
}

int
USTDepthProcess4::GetLatency()
{
    int latency = 0;
    
#if USE_SINC_FILTER
    if (mUseFilter)
        latency = mSincBandFilters[0]->GetLatency();
#endif
    
    return latency;
}

// NOTE: mono method.
// This should not be used in priority.
// Prefer ProcessStereoOptim() instead.
void
USTDepthProcess4::Process(const WDL_TypedBuf<BL_FLOAT> &input,
                          WDL_TypedBuf<BL_FLOAT> *outputL,
                          WDL_TypedBuf<BL_FLOAT> *outputR)
{
    BLUtils::ResizeFillAllZeros(outputL, input.GetSize());
    BLUtils::ResizeFillAllZeros(outputR, input.GetSize());
    
    WDL_TypedBuf<BL_FLOAT> input0 = input;
    
#if USE_PREDELAY
    mPreDelays[0]->ProcessSamples(&input0);
#endif
    
    // Reverb
    if (mUseReverbTail)
        mReverbs[0]->Process(input0, outputL, outputR);
    
    if (mUseEarly)
    {
        // Gen and add early
        BL_FLOAT gain = BLUtils::DBToAmp(STEREO_FIX_DB);
        
        WDL_TypedBuf<BL_FLOAT> earlyRev[2];
        mEarlyRef[0]->Process(input0, &earlyRev[0], &earlyRev[1]);
    
        // Fix gain to stay consistent with ProcessStereoFull()
        BLUtils::MultValues(&earlyRev[0], gain);
        BLUtils::MultValues(&earlyRev[1], gain);
        
        BLUtils::AddValues(outputL, earlyRev[0]);
        BLUtils::AddValues(outputR, earlyRev[1]);
    }
    
#if USE_SINC_FILTER
    if (mUseFilter)
    {
        for (int i = 0; i < 2; i++)
        {
            if (i == 0)
                mSincBandFilters[i]->Process(outputL);
            else
                mSincBandFilters[i]->Process(outputR);
        }
    }
#endif

#if USE_RBJ_FILTERS
    if (mUseFilter)
    {
        for (int i = 0; i < 2; i++)
        {
            if (i == 0)
                mLowPassFilters[i]->Process(outputL);
            else
                mLowPassFilters[i]->Process(outputR);
        }

        for (int i = 0; i < 2; i++)
        {
            if (i == 0)
                mLowCutFilters[i]->Process(outputL);
            else
                mLowCutFilters[i]->Process(outputR);
        }
    }
#endif
    
#if USE_POST_STEREO_WIDEN
    vector<WDL_TypedBuf<BL_FLOAT> * > samples;
    samples.push_back(outputL);
    samples.push_back(outputR);
    
    mStereoWiden->StereoWiden(&samples, POST_STEREO_WIDEN_FACTOR);
#endif
    
#if FIX_DRY
    WDL_TypedBuf<BL_FLOAT> drySignal = input;
    BLUtils::MultValues(&drySignal, mDryGain);
    
    BLUtils::AddValues(outputL, drySignal);
    BLUtils::AddValues(outputR, drySignal);
#endif
}

void
USTDepthProcess4::Process(const WDL_TypedBuf<BL_FLOAT> inputs[2],
                          WDL_TypedBuf<BL_FLOAT> *outputL,
                          WDL_TypedBuf<BL_FLOAT> *outputR)
{
    // Use 2 reverb objects
    //ProcessStereoFull(inputs, outputL, outputR);
    
    // Use 1 reverb object
    ProcessStereoOptim(inputs, outputL, outputR);
}

// Use 2 reverb objects
void
USTDepthProcess4::ProcessStereoFull(const WDL_TypedBuf<BL_FLOAT> inputs[2],
                                    WDL_TypedBuf<BL_FLOAT> *outputL,
                                    WDL_TypedBuf<BL_FLOAT> *outputR)
{
    BLUtils::ResizeFillAllZeros(outputL, inputs[0].GetSize());
    BLUtils::ResizeFillAllZeros(outputR, inputs[0].GetSize());
    
    WDL_TypedBuf<BL_FLOAT> outL[2];
    WDL_TypedBuf<BL_FLOAT> outR[2];
    
    for (int i = 0; i < 2; i++)
    {
        outL[i].Resize(inputs[0].GetSize());
        BLUtils::FillAllZero(&outL[i]);
        
        outR[i].Resize(inputs[0].GetSize());
        BLUtils::FillAllZero(&outR[i]);
    }
    
    WDL_TypedBuf<BL_FLOAT> inputs0[2] = { inputs[0], inputs[1] };
    
#if USE_PREDELAY
    for (int i = 0; i < 2; i++)
    {
        mPreDelays[i]->ProcessSamples(&inputs0[i]);
    }
#endif
    
    // Reverb
    for (int i = 0; i < 2; i++)
    {
        if (mUseReverbTail)
            mReverbs[i]->Process(inputs0[i], &outL[i], &outR[i]);
        
        if (mUseEarly)
        {
            // Gen and add early
            WDL_TypedBuf<BL_FLOAT> earlyRev[2];
            mEarlyRef[i]->Process(inputs[i], &earlyRev[0], &earlyRev[1]);
            
            BLUtils::AddValues(&outL[i], earlyRev[0]);
            BLUtils::AddValues(&outR[i], earlyRev[1]);
        }
    }
    
    // Sum
    *outputL = outL[0];
    BLUtils::AddValues(outputL, outL[1]);
    
    *outputR = outR[0];
    BLUtils::AddValues(outputR, outR[1]);
    
    // Fix gain that increases in this function
    BL_FLOAT gain = BLUtils::DBToAmp(STEREO_FIX_DB);
    BLUtils::MultValues(outputL, gain);
    BLUtils::MultValues(outputR, gain);
    
#if USE_SINC_FILTER
    if (mUseFilter)
    {
        for (int i = 0; i < 2; i++)
        {
            if (i == 0)
                mSincBandFilters[i]->Process(outputL);
            else
                mSincBandFilters[i]->Process(outputR);
        }
    }
#endif
    
#if USE_RBJ_FILTERS
    if (mUseFilter)
    {
        for (int i = 0; i < 2; i++)
        {
            if (i == 0)
                mLowPassFilters[i]->Process(outputL);
            else
                mLowPassFilters[i]->Process(outputR);
        }
    
        for (int i = 0; i < 2; i++)
        {
            if (i == 0)
                mLowCutFilters[i]->Process(outputL);
            else
                mLowCutFilters[i]->Process(outputR);
        }
    }
#endif
    
#if FIX_DRY
    WDL_TypedBuf<BL_FLOAT> drySignal0 = inputs[0];
    BLUtils::MultValues(&drySignal0, mDryGain);
    
    WDL_TypedBuf<BL_FLOAT> drySignal1 = inputs[1];
    BLUtils::MultValues(&drySignal1, mDryGain);
    
    BLUtils::AddValues(outputL, drySignal0);
    BLUtils::AddValues(outputR, drySignal1);
#endif

    
#if USE_POST_STEREO_WIDEN
    vector<WDL_TypedBuf<BL_FLOAT> * > samples;
    samples.push_back(outputL);
    samples.push_back(outputR);
    
    mStereoWiden->StereoWiden(&samples, POST_STEREO_WIDEN_FACTOR);
#endif
}

// NOTE: this method should be used in priority
// (it is better tested than the mono method)
//
// Use a single reverb object
// NOTE: this seems to give almost exactly the same result as when using
// the ProcessStereoFull() method (and it uses only 1 reverb object instead of 2)
void
USTDepthProcess4::ProcessStereoOptim(const WDL_TypedBuf<BL_FLOAT> inputs[2],
                                     WDL_TypedBuf<BL_FLOAT> *outputL,
                                     WDL_TypedBuf<BL_FLOAT> *outputR)
{
    BLUtils::ResizeFillAllZeros(outputL, inputs[0].GetSize());
    BLUtils::ResizeFillAllZeros(outputR, inputs[0].GetSize());
    
    WDL_TypedBuf<BL_FLOAT> outL;
    WDL_TypedBuf<BL_FLOAT> outR;
    
    outL.Resize(inputs[0].GetSize());
    BLUtils::FillAllZero(&outL);
    
    outR.Resize(inputs[0].GetSize());
    BLUtils::FillAllZero(&outR);
    
    WDL_TypedBuf<BL_FLOAT> inputs0[2];
    inputs0[0] = inputs[0];
    inputs0[1] = inputs[1];
    
#if USE_PREDELAY
    for (int i = 0; i < 2; i++)
        mPreDelays[i]->ProcessSamples(&inputs0[i]);
#endif
    
    WDL_TypedBuf<BL_FLOAT> input0;
    BLUtils::StereoToMono(&input0, inputs0[0], inputs0[1]);
    
    // Reverb
    if (mUseReverbTail)
        mReverbs[0]->Process(input0, &outL, &outR);
        
    if (mUseEarly)
    {
        // Gen and add early, for each input channel
        BL_FLOAT gain = BLUtils::DBToAmp(STEREO_FIX_DB);
        for (int i = 0; i < 2; i++)
        {
            WDL_TypedBuf<BL_FLOAT> earlyRev[2];
            mEarlyRef[i]->Process(inputs0[i], &earlyRev[0], &earlyRev[1]);
         
            // Fix gain to stay consistent with ProcessStereoFull()
            BLUtils::MultValues(&earlyRev[0], gain);
            BLUtils::MultValues(&earlyRev[1], gain);
            
            // Apply to left and right
            BLUtils::AddValues(&outL, earlyRev[0]);
            BLUtils::AddValues(&outR, earlyRev[1]);
        }
    }
    
    // Sum
    *outputL = outL;
    *outputR = outR;
    
#if USE_SINC_FILTER
    if (mUseFilter)
    {
        for (int i = 0; i < 2; i++)
        {
            if (i == 0)
                mSincBandFilters[i]->Process(outputL);
            else
                mSincBandFilters[i]->Process(outputR);
        }
    }
#endif
    
#if USE_RBJ_FILTERS
    if (mUseFilter)
    {
        for (int i = 0; i < 2; i++)
        {
            if (i == 0)
                mLowPassFilters[i]->Process(outputL);
            else
                mLowPassFilters[i]->Process(outputR);
        }
    
        for (int i = 0; i < 2; i++)
        {
            if (i == 0)
                mLowCutFilters[i]->Process(outputL);
            else
                mLowCutFilters[i]->Process(outputR);
        }
    }
#endif
    
#if USE_POST_STEREO_WIDEN
    vector<WDL_TypedBuf<BL_FLOAT> * > samples;
    samples.push_back(outputL);
    samples.push_back(outputR);
    
    mStereoWiden->StereoWiden(&samples, POST_STEREO_WIDEN_FACTOR);
#endif
    
#if FIX_DRY
    WDL_TypedBuf<BL_FLOAT> drySignal0 = inputs[0];
    BLUtils::MultValues(&drySignal0, mDryGain);
    
    WDL_TypedBuf<BL_FLOAT> drySignal1 = inputs[1];
    BLUtils::MultValues(&drySignal1, mDryGain);
    
    BLUtils::AddValues(outputL, drySignal0);
    BLUtils::AddValues(outputR, drySignal1);
#endif
}

void
USTDepthProcess4::BypassProcess(WDL_TypedBuf<BL_FLOAT> samples[2])
{
    for (int i = 0; i < 2; i++)
    {
        WDL_TypedBuf<BL_FLOAT> outputL;
        WDL_TypedBuf<BL_FLOAT> outputR;
        
        if (mUseReverbTail)
        {
            mBypassReverbs[i]->Process(samples[i], &outputL, &outputR);
        
            samples[i] = outputL; // Ok ?
        }
        
        //WDL_TypedBuf<BL_FLOAT> monoSamples;
        //BLUtils::StereoToMono(&monoSamples, outputL, outputR);
        //samples[i] = monoSamples;
    }
}

void
USTDepthProcess4::SetUseReverbTail(bool flag)
{
    mUseReverbTail = flag;
}

void
USTDepthProcess4::SetDry(BL_FLOAT dry)
{
#if !FIX_DRY
    for (int i = 0; i < 2; i++)
    {
        JReverb::JReverbParams params = mReverbs[i]->GetParams();
        
        params.mDryLevel = dry;
        
        mReverbs[i]->SetParams(params);
    }
#else
    mDryGain = dry;
#endif
}

void
USTDepthProcess4::SetWet(BL_FLOAT wet)
{
    for (int i = 0; i < 2; i++)
    {
        JReverb::JReverbParams params = mReverbs[i]->GetParams();
        
        params.mWetLevel = wet;
        
        mReverbs[i]->SetParams(params);
    }
}

void
USTDepthProcess4::SetRoomSize(BL_FLOAT roomSize)
{
    for (int i = 0; i < 2; i++)
    {
        JReverb::JReverbParams params = mReverbs[i]->GetParams();
        
        params.mRoomSize = roomSize;
        
        mReverbs[i]->SetParams(params);
    }
}

void
USTDepthProcess4::SetWidth(BL_FLOAT width)
{
    for (int i = 0; i < 2; i++)
    {
        JReverb::JReverbParams params = mReverbs[i]->GetParams();
        
        params.mWidth = width;
        
        mReverbs[i]->SetParams(params);
    }
}

void
USTDepthProcess4::SetDamping(BL_FLOAT damping)
{
    for (int i = 0; i < 2; i++)
    {
        JReverb::JReverbParams params = mReverbs[i]->GetParams();
        
        params.mDamping = damping;
        
        mReverbs[i]->SetParams(params);
    }
}

void
USTDepthProcess4::SetUseFilter(bool flag)
{
    mUseFilter = flag;
}

void
USTDepthProcess4::SetUseEarlyReflections(bool flag)
{
    mUseEarly = flag;
}

void
USTDepthProcess4::SetEarlyRoomSize(BL_FLOAT roomSize)
{
    for (int i = 0; i < 2; i++)
        mEarlyRef[i]->SetRoomSize(roomSize);
}

void
USTDepthProcess4::SetEarlyIntermicDist(BL_FLOAT dist)
{
    for (int i = 0; i < 2; i++)
        mEarlyRef[i]->SetIntermicDist(dist);
}

void
USTDepthProcess4::SetEarlyNormDepth(BL_FLOAT depth)
{
    for (int i = 0; i < 2; i++)
        mEarlyRef[i]->SetNormDepth(depth);
}

void
USTDepthProcess4::SetEarlyOrder(int order)
{
    for (int i = 0; i < 2; i++)
        mEarlyRef[i]->SetOrder(order);
}

void
USTDepthProcess4::SetEarlyReflectCoeff(BL_FLOAT reflectCoeff)
{
    for (int i = 0; i < 2; i++)
        mEarlyRef[i]->SetReflectCoeff(reflectCoeff);
}

void
USTDepthProcess4::LoadConfig(BL_FLOAT config[13])
{
    SetUseReverbTail(config[0]);
    SetDry(config[1]);
    SetWet(config[2]);
    SetRoomSize(config[3]);
    SetWidth(config[4]);
    SetDamping(config[5]);
    SetUseFilter((int)config[6]);
    SetUseEarlyReflections((int)config[7]);
    SetEarlyRoomSize(config[8]);
    SetEarlyIntermicDist(config[9]);
    SetEarlyNormDepth(config[10]);
    SetEarlyOrder(config[11]);
    SetEarlyReflectCoeff(config[12]);
}
