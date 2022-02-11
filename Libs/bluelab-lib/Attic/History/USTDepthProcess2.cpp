//
//  USTDepthProcess2.cpp
//  UST
//
//  Created by applematuer on 12/3/19.
//
//

#include <BLReverb.h>
#include <DelayObj4.h>
#include <NRBJFilter.h>

#include "USTDepthProcess2.h"

// Like StageOne
//#define MIN_DB 0.0
//#define MAX_DB 9.0
//#define MAX_DB 9.0 //18.0 // TEST

#define USE_PREDELAY 0 //1
// 8ms => 2.7m
#define PREDELAY_MS 15.0 // 8ms ?

#define USE_LOW_PASS_FILTER 0 //1
#define LOW_PASS_FREQ 10000.0

// 0.11ms => 38mm
//#define DELAY_L_R_MS 0.11

//BL_FLOAT REVERB_PRESET[] = { 2, 0, -16.6667, -70, 1.125, 0, 0, -0.555553, 0.1, 0, 0, 18000, 50, 8784.1, 7624.39, 0.1, 0 };
//BL_FLOAT REVERB_PRESET[] = { 2, 0, -12.2222, 0.0/*-5.55556*/, 1.125, 1, 1, 10, 0.1, 0, 0, 18000, 50, 8784.1, 7624.39, 0.1, 0 };
BL_FLOAT REVERB_PRESET[] = { 2, 0, -12.2222, -5.55556, 1.125, 1, 1, 10, 0.1, 0, 0, 18000, 50, 8784.1, 7624.39, 0.1, 0 };

// Transparent: { 2, 0, -70, -6, 1.125, 1, 1, -70, 0.1, 0, 0, 18000, 50, 8784.1, 7624.39, 0.1, 0 }


USTDepthProcess2::USTDepthProcess2(BL_FLOAT sampleRate)
{
    mReverb = new BLReverb(sampleRate);
    
    mReverb->ApplyPreset(REVERB_PRESET);
    
#if USE_PREDELAY
    BL_FLOAT delay = (PREDELAY_MS/1000.0)*sampleRate;
    mPreDelay = new DelayObj4(delay);
#endif
    
#if USE_LOW_PASS_FILTER
    for (int i = 0; i < 2; i++)
    {
        mLowPassFilters[i] = new NRBJFilter(4, FILTER_TYPE_LOWPASS,
                                            sampleRate, LOW_PASS_FREQ);
    }
#endif
}

USTDepthProcess2::~USTDepthProcess2()
{
    delete mReverb;
    
#if USE_PREDELAY
    delete mPreDelay;
#endif
    
#if USE_LOW_PASS_FILTER
    for (int i = 0; i < 2; i++)
    {
        delete mLowPassFilters[i];
    }
#endif
}

void
USTDepthProcess2::Reset(BL_FLOAT sampleRate)
{
    mReverb->Reset(sampleRate);
    
#if USE_PREDELAY
    mPreDelay->Reset();
    
    BL_FLOAT delay = (PREDELAY_MS/1000.0)*sampleRate;
    mPreDelay->SetDelay(delay);
#endif
    
#if USE_LOW_PASS_FILTER
    for (int i = 0; i < 2; i++)
    {
        mLowPassFilters[i]->SetSampleRate(sampleRate);
    }
#endif
}

void
USTDepthProcess2::Process(const WDL_TypedBuf<BL_FLOAT> &input,
                          WDL_TypedBuf<BL_FLOAT> *outputL,
                          WDL_TypedBuf<BL_FLOAT> *outputR
                          /*BL_FLOAT depth*/)
{
    outputL->Resize(input.GetSize());
    outputR->Resize(input.GetSize());
    
    WDL_TypedBuf<BL_FLOAT> input0 = input;
    
#if USE_PREDELAY
    mPreDelay->ProcessSamples(&input0);
#endif
    
    // Reverb
    mReverb->Process(input0, outputL, outputR);
    
#if USE_LOW_PASS_FILTER
    for (int i = 0; i < 2; i++)
    {
        if (i == 0)
            mLowPassFilters[i]->Process(outputL);
        else
            mLowPassFilters[i]->Process(outputR);
    }
#endif
        
    // Gain
    //BL_FLOAT gainDB = (1.0 - depth)*MIN_DB + depth*MAX_DB;
    //BL_FLOAT gainDB = depth*MAX_DB;
    //BL_FLOAT gain = DBToAmp(gainDB);

    //gain = gain - 1.0;
    //if (gain < 0.0)
    //    gain = 0.0;
    
    //BL_FLOAT gain = depth*DBToAmp(MAX_DB);
    
#if 0
    for (int i = 0; i < input0.GetSize(); i++)
    {
        BL_FLOAT in = input0.Get()[i];
        
        BL_FLOAT outL = outputL->Get()[i];
        BL_FLOAT outR = outputR->Get()[i];
        
        outL = (1.0 - depth)*in + depth*outL;
        outR = (1.0 - depth)*in + depth*outR;
        
        //outL *= gain;
        //outR *= gain;
        
        outputL->Get()[i] = outL;
        outputR->Get()[i] = outR;
    }
#endif
}
