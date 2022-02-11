//
//  USTDepthProcess4.cpp
//  UST
//
//  Created by applematuer on 12/3/19.
//
//

#include <JReverb.h>
#include <DelayObj4.h>
#include <SincConvoBandPassFilter.h>
#include <EarlyReflections.h>

#include <NRBJFilter.h>
#include <USTStereoWidener.h>


#include <Utils.h>

#include "USTDepthProcess4.h"

// Like StageOne
//

// NOTE: StageOne reverb duration is 270ms, UST one is a bit longer

//#define MAX_DB 9.0

// 0.11ms => 38mm
//#define DELAY_L_R_MS 0.11

// ORIGIN: 1
#define USE_PREDELAY 0 //1

// 8ms => 2.7m
//#define PREDELAY_MS 15.0 // 8ms ?
#define PREDELAY_MS 8.0 // Sounds mor like StageOne with 8ms
//#define PREDELAY_MS 4.0 // Like StateOne ?

// ORIGIN: 1
#define USE_LOW_PASS_FILTER 0 //1

//#define LOW_PASS_FREQ 10000.0 // like StageOne
#define LOW_PASS_FREQ 16384.0 // To manage well brillance

// ORIGIN: 1
#define USE_LOW_CUT_FILTER 0 //1
#define LOW_CUT_FREQ 100.0 //500.0

// ORIGIN: 1
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

// Use sinc bandbass filter
// Takes 1% CPU only
#define USE_SINC_FILTER 1

// Compute only reverb in JReverb, not mix with dry signal
// Re-add dry signal in the current class.
// This makes possible to avoid also filtering the dry signal
#define FIX_DRY 1

#if 0
NOTE: without all the filters, the sound of JReverb sounds a lot less metallic

NOTE: there are 2 reverb objects for stereo processing => use only 1 to optimize ?
#endif


USTDepthProcess4::USTDepthProcess4(BL_FLOAT sampleRate)
{
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
        mLowPassFilters[i] = new NRBJFilter(16/*4*/, FILTER_TYPE_LOWPASS,
                                            sampleRate, LOW_PASS_FREQ);
    }
#endif

#if USE_LOW_CUT_FILTER
    for (int i = 0; i < 2; i++)
    {
        // NOTE: order not tested at all for low cut..
        
        // Order 16, to be very stiff (like StageOne)
        // NOTE: maybe 8 could be sufficient, needs to be tested...
        mLowCutFilters[i] = new NRBJFilter(16/*4*/, FILTER_TYPE_HIPASS,
                                            sampleRate, LOW_CUT_FREQ);
    }
#endif
    
#if USE_POST_STEREO_WIDEN
    mStereoWiden = new USTStereoWidener();
#endif
    
#if USE_SINC_FILTER
    for (int i = 0; i < 2; i++)
    {
        mSincBandFilters[i] = new SincConvoBandPassFilter();
        mSincBandFilters[i]->Init(LOW_CUT_FREQ, LOW_PASS_FREQ, sampleRate);
    }
#endif
    
    for (int i = 0; i < 2; i++)
        mEarlyRef[i] = new EarlyReflections(sampleRate);
    
    mDbgUseFilter = false;
    mDbgUseEarly = false;
    
    mDryGain = 1.0;
}

USTDepthProcess4::USTDepthProcess4(const USTDepthProcess4 &other)
{
    for (int i = 0; i < 2; i++)
    {
        mReverbs[i] = new JReverb(*other.mReverbs[i]);
    }
    
    for (int i = 0; i < 2; i++)
    {
        mBypassReverbs[i] = new JReverb(*other.mBypassReverbs[i]);
    }
    
#if USE_PREDELAY
    mPreDelays[0] = new DelayObj4(*other.mPreDelays[0]);
    mPreDelays[1] = new DelayObj4(*other.mPreDelays[1]);
#endif
    
#if USE_LOW_PASS_FILTER
    for (int i = 0; i < 2; i++)
    {
        mLowPassFilters[i] = new NRBJFilter(*other.mLowPassFilters[i]);
    }
#endif
    
#if USE_LOW_CUT_FILTER
    for (int i = 0; i < 2; i++)
    {
        mLowCutFilters[i] = new NRBJFilter(*other.mLowCutFilters[i]);
    }
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
    
    mDbgUseEarly = other.mDbgUseEarly;
    
    for (int i = 0; i < 2; i++)
        mEarlyRef[i] = new EarlyReflections(*other.mEarlyRef[i]);
    
    mDbgUseFilter = other.mDbgUseFilter;
    
    mDryGain = other.mDryGain;
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
    
#if USE_SINC_FILTER
    for (int i = 0; i < 2; i++)
    {
        delete mSincBandFilters[i];
    }
#endif
    
    for (int i = 0; i < 2; i++)
        delete mEarlyRef[i];
}

void
USTDepthProcess4::Reset(BL_FLOAT sampleRate, int blockSize)
{
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
    
#if USE_SINC_FILTER
    for (int i = 0; i < 2; i++)
        mSincBandFilters[i]->Reset(sampleRate, blockSize);
#endif
    
    for (int i = 0; i < 2; i++)
        mEarlyRef[i]->Reset(sampleRate);
}

int
USTDepthProcess4::GetLatency()
{
    int latency = 0;
    
#if USE_SINC_FILTER
    if (mDbgUseFilter)
        latency = mSincBandFilters[0]->GetLatency();
#endif
    
    return latency;
}

void
USTDepthProcess4::Process(const WDL_TypedBuf<BL_FLOAT> &input,
                          WDL_TypedBuf<BL_FLOAT> *outputL,
                          WDL_TypedBuf<BL_FLOAT> *outputR)
{
    Utils::ResizeFillZeros(outputL, input.GetSize());
    Utils::ResizeFillZeros(outputR, input.GetSize());
    
#if DBG_BYPASS
    return;
#endif
    
    WDL_TypedBuf<BL_FLOAT> input0 = input;
    
#if USE_PREDELAY
    mPreDelays[0]->ProcessSamples(&input0);
#endif
    
    // Reverb
    mReverbs[0]->Process(input0, outputL, outputR);
    
    if (mDbgUseEarly)
    {
        // Gen and add early
        WDL_TypedBuf<BL_FLOAT> earlyRev[2];
        mEarlyRef[0]->Process(input0, &earlyRev[0], &earlyRev[1]);
    
        Utils::AddValues(outputL, earlyRev[0]);
        Utils::AddValues(outputR, earlyRev[1]);
    }
    
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
    
#if USE_SINC_FILTER
    if (mDbgUseFilter)
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

#if FIX_DRY
    WDL_TypedBuf<BL_FLOAT> drySignal = input;
    Utils::MultValues(&drySignal, mDryGain);
    Utils::AddValues(outputL, drySignal);
    Utils::AddValues(outputR, drySignal);
#endif
    
#if USE_POST_STEREO_WIDEN
    vector<WDL_TypedBuf<BL_FLOAT> * > samples;
    samples.push_back(outputL);
    samples.push_back(outputR);
    
    mStereoWiden->StereoWiden(&samples, POST_STEREO_WIDEN_FACTOR);
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
    Utils::ResizeFillZeros(outputL, inputs[0].GetSize());
    Utils::ResizeFillZeros(outputR, inputs[0].GetSize());
    
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
    for (int i = 0; i < 2; i++)
    {
        mReverbs[i]->Process(inputs0[i], &outL[i], &outR[i]);
        
        if (mDbgUseEarly)
        {
            // Gen and add early
            WDL_TypedBuf<BL_FLOAT> earlyRev[2];
            mEarlyRef[i]->Process(inputs[i], &earlyRev[0], &earlyRev[1]);
            
            Utils::AddValues(&outL[i], earlyRev[0]);
            Utils::AddValues(&outR[i], earlyRev[1]);
        }
    }
    
    // Sum
#if !STEREO_SUM2
    *outputL = outL[0];
    Utils::AddValues(outputL, outL[1]);
    
    *outputR = outR[0];
    Utils::AddValues(outputR, outR[1]);
#else
    *outputL = outL[0];
    Utils::AddValues(outputL, outL[1]);
    
    *outputR = outR[0];
    Utils::AddValues(outputR, outR[1]);
#endif
    
    // Fix gain that increases in this function
    BL_FLOAT gain = DBToAmp(STEREO_FIX_DB);
    Utils::MultValues(outputL, gain);
    Utils::MultValues(outputR, gain);
    
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
    
#if USE_SINC_FILTER
    if (mDbgUseFilter)
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
    
#if FIX_DRY
    WDL_TypedBuf<BL_FLOAT> drySignal0 = inputs[0];
    Utils::MultValues(&drySignal0, mDryGain);
    
    WDL_TypedBuf<BL_FLOAT> drySignal1 = inputs[1];
    Utils::MultValues(&drySignal1, mDryGain);
    
    Utils::AddValues(outputL, drySignal0);
    Utils::AddValues(outputR, drySignal1);
#endif

    
#if USE_POST_STEREO_WIDEN
    vector<WDL_TypedBuf<BL_FLOAT> * > samples;
    samples.push_back(outputL);
    samples.push_back(outputR);
    
    mStereoWiden->StereoWiden(&samples, POST_STEREO_WIDEN_FACTOR);
#endif
}

// Use 1 reverb object
// NOTE: this seems to give almost exactly the same result as when using
// the ProcessStereoFull() method (and it uses only 1 reverb object instead of 2)
void
USTDepthProcess4::ProcessStereoOptim(const WDL_TypedBuf<BL_FLOAT> inputs[2],
                                     WDL_TypedBuf<BL_FLOAT> *outputL,
                                     WDL_TypedBuf<BL_FLOAT> *outputR)
{
    Utils::ResizeFillZeros(outputL, inputs[0].GetSize());
    Utils::ResizeFillZeros(outputR, inputs[0].GetSize());
    
#if DBG_BYPASS
    return;
#endif
    
    WDL_TypedBuf<BL_FLOAT> outL;
    WDL_TypedBuf<BL_FLOAT> outR;
    
    outL.Resize(inputs[0].GetSize());
    outR.Resize(inputs[0].GetSize());
    
    WDL_TypedBuf<BL_FLOAT> input0;
    Utils::StereoToMono(&input0, inputs[0], inputs[1]);
    
#if USE_PREDELAY
    for (int i = 0; i < 2; i++)
    {
        mPreDelays[i]->ProcessSamples(&inputs0[i]);
    }
#endif
    
    // Reverb
    mReverbs[0]->Process(input0, &outL, &outR);
        
    if (mDbgUseEarly)
    {
        // Gen and add early, for each input channel
        BL_FLOAT gain = DBToAmp(STEREO_FIX_DB);
        for (int i = 0; i < 2; i++)
        {
            WDL_TypedBuf<BL_FLOAT> earlyRev[2];
            mEarlyRef[i]->Process(inputs[i], &earlyRev[0], &earlyRev[1]);
         
            // Fix gain to stay consistent with ProcessStereoFull()
            Utils::MultValues(&earlyRev[0], gain);
            Utils::MultValues(&earlyRev[1], gain);
            
            // Apply to left and right
            Utils::AddValues(&outL, earlyRev[0]);
            Utils::AddValues(&outR, earlyRev[1]);
        }
    }
    
    // Sum
    *outputL = outL;
    *outputR = outR;
    
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
    
#if USE_SINC_FILTER
    if (mDbgUseFilter)
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
    
#if USE_POST_STEREO_WIDEN
    vector<WDL_TypedBuf<BL_FLOAT> * > samples;
    samples.push_back(outputL);
    samples.push_back(outputR);
    
    mStereoWiden->StereoWiden(&samples, POST_STEREO_WIDEN_FACTOR);
#endif
    
#if FIX_DRY
    WDL_TypedBuf<BL_FLOAT> drySignal0 = inputs[0];
    Utils::MultValues(&drySignal0, mDryGain);
    
    WDL_TypedBuf<BL_FLOAT> drySignal1 = inputs[1];
    Utils::MultValues(&drySignal1, mDryGain);
    
    Utils::AddValues(outputL, drySignal0);
    Utils::AddValues(outputR, drySignal1);
#endif
}

void
USTDepthProcess4::BypassProcess(WDL_TypedBuf<BL_FLOAT> samples[2])
{
    for (int i = 0; i < 2; i++)
    {
        WDL_TypedBuf<BL_FLOAT> outputL;
        WDL_TypedBuf<BL_FLOAT> outputR;
        
        mBypassReverbs[i]->Process(samples[i], &outputL, &outputR);
        
        samples[i] = outputL; // Ok ?
        
        //WDL_TypedBuf<BL_FLOAT> monoSamples;
        //Utils::StereoToMono(&monoSamples, outputL, outputR);
        //samples[i] = monoSamples;
    }
}

void
USTDepthProcess4::DBG_SetDry(BL_FLOAT dry)
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
USTDepthProcess4::DBG_SetWet(BL_FLOAT wet)
{
    for (int i = 0; i < 2; i++)
    {
        JReverb::JReverbParams params = mReverbs[i]->GetParams();
        
        params.mWetLevel = wet;
        
        mReverbs[i]->SetParams(params);
    }
}

void
USTDepthProcess4::DBG_SetRoomSize(BL_FLOAT roomSize)
{
    for (int i = 0; i < 2; i++)
    {
        JReverb::JReverbParams params = mReverbs[i]->GetParams();
        
        params.mRoomSize = roomSize;
        
        mReverbs[i]->SetParams(params);
    }
}

void
USTDepthProcess4::DBG_SetWidth(BL_FLOAT width)
{
    for (int i = 0; i < 2; i++)
    {
        JReverb::JReverbParams params = mReverbs[i]->GetParams();
        
        params.mWidth = width;
        
        mReverbs[i]->SetParams(params);
    }
}

void
USTDepthProcess4::DBG_SetDamping(BL_FLOAT damping)
{
    for (int i = 0; i < 2; i++)
    {
        JReverb::JReverbParams params = mReverbs[i]->GetParams();
        
        params.mDamping = damping;
        
        mReverbs[i]->SetParams(params);
    }
}

void
USTDepthProcess4::DBG_SetUseFilter(bool flag)
{
    mDbgUseFilter = flag;
}

void
USTDepthProcess4::DBG_SetUseEarlyReflections(bool flag)
{
    mDbgUseEarly = flag;
}

void
USTDepthProcess4::DBG_SetEarlyRoomSize(BL_FLOAT roomSize)
{
    for (int i = 0; i < 2; i++)
        mEarlyRef[i]->SetRoomSize(roomSize);
}

void
USTDepthProcess4::DBG_SetEarlyIntermicDist(BL_FLOAT dist)
{
    for (int i = 0; i < 2; i++)
        mEarlyRef[i]->SetIntermicDist(dist);
}

void
USTDepthProcess4::DBG_SetEarlyNormDepth(BL_FLOAT depth)
{
    for (int i = 0; i < 2; i++)
        mEarlyRef[i]->SetNormDepth(depth);
}
