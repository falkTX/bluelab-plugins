//
//  LUFTMeter.cpp
//  UST
//
//  Created by applematuer on 8/14/19.
//
//

#include <Oversampler4.h>

#include "LUFSMeter.h"

#define MIN_LOUDNESS -90.0

// See: https://designingsound.org/2013/02/27/does-bit-or-sample-rate-reduction-affect-loudness-measurements/
// And after tests, sampling rat edoes not change LUFS a lot
// NOTE: BAD: after resampling, we have a difference of around 3dB
#define OPTIM_DOWNSAMPLE 0 //1
#define OPTIM_DOWNSAMPLE_SR 11025.0

// GOOD: same LUFS result, and good performance gain
#define OPTIM_USE_FLOAT 1


LUFSMeter::LUFSMeter(int numChannels, BL_FLOAT sampleRate)
{
    mState = NULL;
    
    mNumChannels = numChannels;

    for (int i = 0; i < NUM_DOWNSAMPLERS; i++)
        mDownsamplers[i] = NULL;
    
    Reset(sampleRate);
}

LUFSMeter::~LUFSMeter()
{
    ebur128_destroy(&mState);
}

void
LUFSMeter::Reset(BL_FLOAT sampleRate)
{
#if OPTIM_DOWNSAMPLE
    
#define EPS 1e-15
    BL_FLOAT oversampling = sampleRate/OPTIM_DOWNSAMPLE_SR;
    if (oversampling < 1.0)
    {
        if (oversampling > EPS)
            oversampling = 1.0/oversampling;
    }
        
    for (int i = 0; i < NUM_DOWNSAMPLERS; i++)
    {
        if (mDownsamplers[i] != NULL)
        {
            delete mDownsamplers[i];
        }
        
        mDownsamplers[i] = new Oversampler4(oversampling, false);
    }
    
    sampleRate = OPTIM_DOWNSAMPLE_SR;
#endif

    if (mState != NULL)
    {
        ebur128_destroy(&mState);
        mState = NULL;
    }
    
#if !USE_SINGLE_CHANNEL
    int nChannels = 2;
#else
    int nChannels = 1;
#endif
    
#if ENABLE_SHORT_TERM
    mState = ebur128_init(mNumChannels/*nChannels*/, sampleRate, EBUR128_MODE_S);
#else
    // Avoid unnecessary compitung in libebur128
    mState = ebur128_init(mNumChannels/*nChannels*/, sampleRate, EBUR128_MODE_M);
#endif
}

#if !USE_SINGLE_CHANNEL

#if !OPTIM_USE_FLOAT // Use BL_FLOAT
void
LUFSMeter::AddSamples(const vector<WDL_TypedBuf<BL_FLOAT> > &samples)
{
    if (samples.size() != mNumChannels)
        return;
    
    vector<WDL_TypedBuf<BL_FLOAT> > samples0 = samples;
    
#if OPTIM_DOWNSAMPLE
    for (int i = 0; i < samples0.size(); i++)
    {
        WDL_TypedBuf<BL_FLOAT> &buf = samples0[i];
        if (i < NUM_DOWNSAMPLERS)
        {
            // Quick and dirty downsampling, using WDL_Resampler
            // and no Nyquist filtering
            mDownsamplers[i]->Resample(&buf);
        }
    }
#endif
    
    WDL_TypedBuf<BL_FLOAT> buf;
    buf.Resize(samples0[0].GetSize()*mNumChannels);
    
    // Interleave
    for (int i = 0; i < samples0[0].GetSize(); i++)
    {
        for (int k = 0; k < mNumChannels; k++)
        {
            buf.Get()[i*mNumChannels + k] = samples0[k].Get()[i];
        }
    }
    
    ebur128_add_frames_BL_FLOAT(mState, buf.Get(), samples0[0].GetSize());
}
#else // Use float
void
LUFSMeter::AddSamples(const vector<WDL_TypedBuf<BL_FLOAT> > &samples)
{
    if (samples.size() != mNumChannels)
        return;
    
    WDL_TypedBuf<float> buf;
    buf.Resize(samples[0].GetSize()*mNumChannels);
    
    // Interleave
    for (int i = 0; i < samples[0].GetSize(); i++)
    {
        for (int k = 0; k < mNumChannels; k++)
        {
            buf.Get()[i*mNumChannels + k] = samples[k].Get()[i];
        }
    }
    
    ebur128_add_frames_float(mState, buf.Get(), samples[0].GetSize());
}
#endif

#else
void
LUFSMeter::AddSamples(const WDL_TypedBuf<BL_FLOAT> &samples)
{
    ebur128_add_frames_BL_FLOAT(mState, samples.Get(), samples.GetSize());
}
#endif

void
LUFSMeter::GetLoudnessMomentary(BL_FLOAT *ioLoudness)
{
    double loudness;
    
    int status = ebur128_loudness_momentary(mState, &loudness);
    if (status == EBUR128_SUCCESS)
    {
        // Avoid -inf
        if (loudness > MIN_LOUDNESS)
            *ioLoudness = loudness;
    }
}

#if ENABLE_SHORT_TERM
void
LUFSMeter::GetLoudnessShortTerm(BL_FLOAT *ioLoudness)
{
    double loudness;
    
    int status = ebur128_loudness_shortterm(mState, &loudness);
    if (status == EBUR128_SUCCESS)
    {
        // Avoid -inf
        if (loudness > MIN_LOUDNESS)
            *ioLoudness = loudness;
    }
}
#endif
