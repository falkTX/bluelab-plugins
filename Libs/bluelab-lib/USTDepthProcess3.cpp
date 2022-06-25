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
//  USTDepthProcess3.cpp
//  UST
//
//  Created by applematuer on 12/3/19.
//
//

#include <BLReverbSndF.h>
#include <DelayObj4.h>
#include <FilterRBJNX.h>

#include <BLReverbIR.h>

#include <BLUtils.h>

#include <USTStereoWidener.h>

#include "USTDepthProcess3.h"

// Like StageOne
//

// NOTE: StageOne reverb duration is 270ms, UST one is a bit longer

//#define MAX_DB 9.0

// 0.11ms => 38mm
//#define DELAY_L_R_MS 0.11

#define USE_PREDELAY 1 //0 //1
// 8ms => 2.7m
//#define PREDELAY_MS 15.0 // 8ms ?
#define PREDELAY_MS 8.0 // Sounds mor like StageOne with 8ms
//#define PREDELAY_MS 4.0 // Like StateOne ?

#define USE_LOW_PASS_FILTER 1 //0 //1
//#define LOW_PASS_FREQ 10000.0 // like StageOne
#define LOW_PASS_FREQ 16384.0 // To manage well brillance

#define USE_LOW_CUT_FILTER 1 //0 //1
#define LOW_CUT_FREQ 100.0 //500.0

#define USE_POST_STEREO_WIDEN 1
#define POST_STEREO_WIDEN_FACTOR 1.0 // with 1, that goes out of correlation


// Volume compensation when processing one reverb by channel
// (since the reverb "dry" is not 0)
#define STEREO_FIX_DB -6.0

// Test: to manage well input that is not mono-compatible
// => avoid summing left and right input parts
//
// NOTE: doesn't seem to fix (if the input is not mono compatible)
// and may diminish the space feeling
#define STEREO_SUM2 0 //1

// for profiling
#define DBG_BYPASS 0 //1

// Does not really optimize
#define USE_REVER_IRS 0 //1
// 0.5 seconds
#define IR_LENGTH_SECONDS 0.5

// Do not compute if parameters are zero
//
// NOTE: does not optimize a lot (and risk of sound mistake)
#define REVERB_OPTIM 0 //1

// Attenuate the early reflecctions over the time
#define REVERB_ATTENUATE_EARLY 1


// TEST OPTIM
// Takes 3%
/*#define USE_PREDELAY 0
#define USE_LOW_PASS_FILTER 0
#define USE_LOW_CUT_FILTER 0
#define USE_POST_STEREO_WIDEN 0*/

// Presets
//

// OSF ERtoLt ERWet Dry ERFac ERWdth Wdth Wet Wander BassB Spin InpLP BasLP DmpLP OutLP RT60  Delay

// Good
//BL_FLOAT REVERB_PRESET[] = { 2, 0, -12.2222, -5.55556, 1.125, 1, 1, 10, 0.1, 0, 0, 18000, 50, 8784.1, 7624.39, 0.1, 0 };

// Optimized preset
#if 0 // Makes metallic noises with snare from Emrah
      // This was due to reverb tail that enhanced some particular frequencies
      // (Some freqs took a longer time to disappear with RT60, and that sounded metallic)
BL_FLOAT REVERB_PRESET[] = { 1/**/, 0, -12.2222, -5.55556, 1.125, 1, 1, 10, 0.1, 0, 0, 18000, 50, 8784.1, 7624.39, 0.1, 0 };

BL_FLOAT BYPASS_PRESET[] = { 2, 0, -70, -6, 1.125, 1, 1, -70, 0.1, 0, 0, 18000, 50, 8784.1, 7624.39, 0.1, 0 };
#endif

#if 1 // Use only early reflections + longer early (to be similar to Stage One)
BL_FLOAT REVERB_PRESET[] = {  1, 0,
                            -12.2222/*-7.22222*/, // Early wet
                            -120.0/*Mute dry*//*-6.0*//*-3*//*Keep constant level with or without depth*/,
                            /*1.125*/ 2.5, // Early factor
                            /*4.0*/ /*2.0*/1 /* > 1 -> sound coloration !*/, // Early width
                            0, -70, 0.1, 0, 0, 18000, 50, 18000,
                            /* So cuts early at 10K, to do so, se to to 5K here*//*5000.0*/18000,
                            // Out low pass (doesn't work well)
                            0.1, 0 };

BL_FLOAT BYPASS_PRESET[] = { 1/*2*/, 0, -120.0/*-70*/, -6, 1.125, 1, 1, -120.0/*-70*/, 0.1, 0, 0, 18000, 50, 8784.1, 7624.39, 0.1, 0 };
#endif


USTDepthProcess3::USTDepthProcess3(BL_FLOAT sampleRate)
{
    for (int i = 0; i < 2; i++)
    {
        mReverbs[i] = new BLReverbSndF(sampleRate, REVERB_OPTIM);
        mReverbs[i]->ApplyPreset(REVERB_PRESET);
    }
    
    for (int i = 0; i < 2; i++)
    {
        mBypassReverbs[i] = new BLReverbSndF(sampleRate, REVERB_OPTIM);
        mBypassReverbs[i]->ApplyPreset(BYPASS_PRESET);
    }

    mReverbIRs[0] = NULL;
    mReverbIRs[1] = NULL;
    
#if USE_REVER_IRS
    for (int i = 0; i < 2; i++)
    {
        mReverbIRs[i] = new BLReverbIR(mReverbs[i], sampleRate,
                                       IR_LENGTH_SECONDS);
        
        mReverbIRs[i]->UpdateIRs();
    }
#endif
    
#if USE_PREDELAY
    BL_FLOAT delay = (PREDELAY_MS/1000.0)*sampleRate;
    mPreDelays[0] = new DelayObj4(delay);
    mPreDelays[1] = new DelayObj4(delay);
#endif
    
#if USE_LOW_PASS_FILTER
    for (int i = 0; i < 2; i++)
    {
        // Order 16, to be very stiff (like StageOne)
        // NOTE: maybe 8 could be sufficient, needs to be tested...
        mLowPassFilters[i] = new FilterRBJNX(16/*4*/, FILTER_TYPE_LOWPASS,
                                            sampleRate, LOW_PASS_FREQ);
    }
#endif

#if USE_LOW_CUT_FILTER
    for (int i = 0; i < 2; i++)
    {
        // NOTE: order not tested at all for low cut..
        
        // Order 16, to be very stiff (like StageOne)
        // NOTE: maybe 8 could be sufficient, needs to be tested...
        mLowCutFilters[i] = new FilterRBJNX(16/*4*/, FILTER_TYPE_HIPASS,
                                            sampleRate, LOW_CUT_FREQ);
    }
#endif
    
#if REVERB_ATTENUATE_EARLY
    sf_set_niko_attenuate_early(1);
#endif
    
#if USE_POST_STEREO_WIDEN
    mStereoWiden = new USTStereoWidener();
#endif
}

USTDepthProcess3::~USTDepthProcess3()
{
    for (int i = 0; i < 2; i++)
    {
        delete mReverbs[i];
    }
    
    for (int i = 0; i < 2; i++)
    {
        delete mBypassReverbs[i];
    }
    
#if USE_REVER_IRS
    for (int i = 0; i < 2; i++)
    {
        delete mReverbIRs[i];
    }
#endif
    
#if USE_PREDELAY
    delete mPreDelays[0];
    delete mPreDelays[1];
#endif
    
#if USE_LOW_PASS_FILTER
    for (int i = 0; i < 2; i++)
    {
        delete mLowPassFilters[i];
    }
#endif

#if USE_LOW_CUT_FILTER
    for (int i = 0; i < 2; i++)
    {
        delete mLowCutFilters[i];
    }
#endif
    
#if USE_POST_STEREO_WIDEN
    delete mStereoWiden;
#endif
}

void
USTDepthProcess3::Reset(BL_FLOAT sampleRate, int blockSize)
{
    for (int i = 0; i < 2; i++)
    {
        mReverbs[i]->Reset(sampleRate, blockSize);
    }
    
    for (int i = 0; i < 2; i++)
    {
        mBypassReverbs[i]->Reset(sampleRate, blockSize);
    }
    
#if USE_REVER_IRS
    for (int i = 0; i < 2; i++)
    {
        mReverbIRs[i]->Reset(sampleRate, blockSize);
    }
#endif

    
#if USE_PREDELAY
    for (int i = 0; i < 2; i++)
    {
        mPreDelays[i]->Reset();
    
        BL_FLOAT delay = (PREDELAY_MS/1000.0)*sampleRate;
        mPreDelays[i]->SetDelay(delay);
    }
#endif
    
#if USE_LOW_PASS_FILTER
    for (int i = 0; i < 2; i++)
    {
        mLowPassFilters[i]->SetSampleRate(sampleRate);
    }
#endif
    
#if USE_LOW_CUT_FILTER
    for (int i = 0; i < 2; i++)
    {
        mLowCutFilters[i]->SetSampleRate(sampleRate);
    }
#endif
}

int
USTDepthProcess3::GetLatency()
{
    int latency = 0;
    
#if USE_REVER_IRS
    latency = mReverbIRs[0]->GetLatency();
#endif
    
    return latency;
}

void
USTDepthProcess3::Process(const WDL_TypedBuf<BL_FLOAT> &input,
                          WDL_TypedBuf<BL_FLOAT> *outputL,
                          WDL_TypedBuf<BL_FLOAT> *outputR)
{
    BLUtils::ResizeFillZeros(outputL, input.GetSize());
    BLUtils::ResizeFillZeros(outputR, input.GetSize());
    
#if DBG_BYPASS
    return;
#endif
    
    WDL_TypedBuf<BL_FLOAT> input0 = input;
    
#if USE_PREDELAY
    mPreDelays[0]->ProcessSamples(&input0);
#endif
    
    // Reverb
#if !USE_REVER_IRS
    mReverbs[0]->Process(input0, outputL, outputR);
#else
    mReverbIRs[0]->Process(input0, outputL, outputR);
#endif
        
#if USE_LOW_PASS_FILTER
    for (int i = 0; i < 2; i++)
    {
        if (i == 0)
            mLowPassFilters[i]->Process(outputL);
        else
            mLowPassFilters[i]->Process(outputR);
    }
#endif
    
#if USE_LOW_CUT_FILTER
    for (int i = 0; i < 2; i++)
    {
        if (i == 0)
            mLowCutFilters[i]->Process(outputL);
        else
            mLowCutFilters[i]->Process(outputR);
    }
#endif
    
#if USE_POST_STEREO_WIDEN
    vector<WDL_TypedBuf<BL_FLOAT> * > samples;
    samples.push_back(outputL);
    samples.push_back(outputR);
    
    mStereoWiden->StereoWiden(&samples, POST_STEREO_WIDEN_FACTOR);
#endif
}

void
USTDepthProcess3::Process(const WDL_TypedBuf<BL_FLOAT> inputs[2],
                          WDL_TypedBuf<BL_FLOAT> *outputL,
                          WDL_TypedBuf<BL_FLOAT> *outputR)
{
    BLUtils::ResizeFillZeros(outputL, inputs[0].GetSize());
    BLUtils::ResizeFillZeros(outputR, inputs[0].GetSize());
    
#if DBG_BYPASS
    return;
#endif
    
    WDL_TypedBuf<BL_FLOAT> outL[2];
    WDL_TypedBuf<BL_FLOAT> outR[2];
    
    for (int i = 0; i < 2; i++)
    {
        outL[i].Resize(inputs[0].GetSize());
        outR[i].Resize(inputs[0].GetSize());
    }
    
    WDL_TypedBuf<BL_FLOAT> inputs0[2] = { inputs[0], inputs[1] };
    
#if USE_PREDELAY
    for (int i = 0; i < 2; i++)
    {
        mPreDelays[i]->ProcessSamples(&inputs0[i]);
    }
#endif
    
    // Reverb
#if !USE_REVER_IRS
    for (int i = 0; i < 2; i++)
    {
        mReverbs[i]->Process(inputs0[i], &outL[i], &outR[i]);
    }
#else
    for (int i = 0; i < 2; i++)
    {
        mReverbIRs[i]->Process(inputs0[i], &outL[i], &outR[i]);
    }
#endif
    
    // Sum
#if !STEREO_SUM2
    *outputL = outL[0];
    BLUtils::AddValues(outputL, outL[1]);
    
    *outputR = outR[0];
    BLUtils::AddValues(outputR, outR[1]);
#else
    *outputL = outL[0];
    BLUtils::AddValues(outputL, outL[1]);
    
    *outputR = outR[0];
    BLUtils::AddValues(outputR, outR[1]);
#endif
    
    // Fix gain that increases in this function
    BL_FLOAT gain = BLUtils::DBToAmp(STEREO_FIX_DB);
    BLUtils::MultValues(outputL, gain);
    BLUtils::MultValues(outputR, gain);
    
#if USE_LOW_PASS_FILTER
    for (int i = 0; i < 2; i++)
    {
        if (i == 0)
            mLowPassFilters[i]->Process(outputL);
        else
            mLowPassFilters[i]->Process(outputR);
    }
#endif
    
#if USE_LOW_CUT_FILTER
    for (int i = 0; i < 2; i++)
    {
        if (i == 0)
            mLowCutFilters[i]->Process(outputL);
        else
            mLowCutFilters[i]->Process(outputR);
    }
#endif
    
#if USE_POST_STEREO_WIDEN
    vector<WDL_TypedBuf<BL_FLOAT> * > samples;
    samples.push_back(outputL);
    samples.push_back(outputR);
    
    mStereoWiden->StereoWiden(&samples, POST_STEREO_WIDEN_FACTOR);
#endif
}

void
USTDepthProcess3::BypassProcess(WDL_TypedBuf<BL_FLOAT> samples[2])
{
    for (int i = 0; i < 2; i++)
    {
        WDL_TypedBuf<BL_FLOAT> outputL;
        WDL_TypedBuf<BL_FLOAT> outputR;
        
        mBypassReverbs[i]->Process(samples[i], &outputL, &outputR);
        
        samples[i] = outputL; // Ok ?
        
        //WDL_TypedBuf<BL_FLOAT> monoSamples;
        //BLUtils::StereoToMono(&monoSamples, outputL, outputR);
        //samples[i] = monoSamples;
    }
}
