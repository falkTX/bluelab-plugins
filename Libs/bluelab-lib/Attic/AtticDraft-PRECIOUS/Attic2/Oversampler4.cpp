//
//  Oversampler2.cpp
//  Denoiser
//
//  Created by Apple m'a Tuer on 26/04/17.
//
//

#include <Utils.h>

#include "Oversampler4.h"

// Useless (we always have the good sample count)
#define FIX_ADJUST_IN_RESAMPLING 0 //1

// Re-inject some previous input values before processing,
// to avoid linear interpolation discontinuity between buffers
// (in WDL_Resampler, at each buffer, linear interpolation continuity is lost)
// this attenuates a lot the vertical bars in the spectrogram
#define FIX_CONTINUITY_LINERP 1

Oversampler4::Oversampler4(int oversampling, bool upSample)
{
    mOversampling = oversampling;
    
    mUpSample = upSample;
    
    mSampleRate = 0;
    
    mRemainingSamples = 0.0;
    
    //
    for (int i = 0; i < CONTINUITY_LERP_HISTO_SIZE; i++)
        mPrevSampValues[i] = 0.0;
}

Oversampler4::~Oversampler4() {}

int
Oversampler4::GetOversampling()
{
    return mOversampling;
}

double *
Oversampler4::GetOutEmptyBuffer()
{
    return mOutEmptyBuf.Get();
}

void
Oversampler4::Reset(double sampleRate)
{
    mSampleRate = sampleRate;
    
    mResampler.Reset(); //
    
    // NOTE: a problem was detected during UST Clipper
    // When using on a pure sine wave, clipper gain 18dB
    // => there are very light vertical bars on the spectrogram, every ~2048 sample
    // (otherwise it looks to work well)
    
    // Default: Makes very light vertical bars on spectorgram, every ~2048 samples
    //mResampler.SetMode(true, 1, false, 0, 0);
    
    // Better than default (but still vertical bars)
    // NOTE: with FIX_CONTINUITY_LINERP added, this removes all the vertical bars
    // and the quality when used in UST is better than SirClipper
    mResampler.SetMode(true, 2, false, 0, 0);
    
    // Does not make vertical bars, but the resampling is not enough "clean"
    //mResampler.SetMode(false, 1, false, 0, 0);
    
    // Makes vertical bars, and resampling better than default
    //mResampler.SetMode(true, 3/*1*/, false, 0, 0);
    
    // Makes vertical bars, and resampling better than default
    //mResampler.SetMode(false, 0, true, 64, 32);
    
    mResampler.SetFilterParms();
    
    mResampler.SetFeedMode(true);
    
    double rateOut = mUpSample ? sampleRate*mOversampling : sampleRate/mOversampling;
    
    mResampler.SetRates(sampleRate, rateOut);
    
    mRemainingSamples = 0.0;
    
    for (int i = 0; i < CONTINUITY_LERP_HISTO_SIZE; i++)
        mPrevSampValues[i] = 0.0;
    
    //int latency = mResampler.GetCurrentLatency();
}

void
Oversampler4::Resample(WDL_TypedBuf<double> *ioBuf)
{
    // TODO: optimize this!
    // Create a temporary buffer of bigger size
    // and copy the elements one by one at the correct place, to avoid
    // re-allocating each time
#if FIX_CONTINUITY_LINERP
    if (mUpSample)
    {
        // Add the previous value at the beginning
        WDL_TypedBuf<double> tmpBuf;
        
        for (int i = 0; i < CONTINUITY_LERP_HISTO_SIZE; i++)
            tmpBuf.Add(mPrevSampValues[i]);
        tmpBuf.Add(ioBuf->Get(), ioBuf->GetSize());
        *ioBuf = tmpBuf;
        
        // Keep the last value
        for (int i = 0; i < CONTINUITY_LERP_HISTO_SIZE; i++)
        {
            mPrevSampValues[i] =
                ioBuf->Get()[ioBuf->GetSize() - CONTINUITY_LERP_HISTO_SIZE + i];
        }
    }
#endif
    
    int srcSize = ioBuf->GetSize();
    int dstSize = mUpSample ? srcSize*mOversampling : srcSize/mOversampling;
    
    // int desiredSamples = dstSize;
    
    int desiredSamples = mUpSample ? dstSize : srcSize;
    
#if FIX_ADJUST_IN_RESAMPLING
    // Adjust
    if (mRemainingSamples >= 1.0)
    {
        int subSamples = floor(mRemainingSamples);
        mRemainingSamples -= subSamples;
        
        desiredSamples -= subSamples;
    }
#endif
     
    WDL_ResampleSample* p;
    int numSamples = mResampler.ResamplePrepare(desiredSamples/*srcSize*/, 1, &p);
    
    double *src = ioBuf->Get();
    for (int i = 0; i < numSamples; ++i)
    {
        //if (i >= srcSize)
            //break;
        
        if (i >= ioBuf->GetSize())
        {
            *p++ = 0.0;
            
            continue;
        }
        
        *p++ = (WDL_ResampleSample)*src++;
    }
    
    WDL_TypedBuf<double> dstBuf;
    dstBuf.Resize(dstSize);
    Utils::FillAllZero(&dstBuf);
    
    double *dst = dstBuf.Get();
    
    int numOutSamples = mResampler.ResampleOut(dst, srcSize, dstSize, 1);
    
    *ioBuf = dstBuf;
    
#if FIX_ADJUST_IN_RESAMPLING
    // Adjust
    double remaining = numOutSamples*srcSize/dstSize - desiredSamples;
    mRemainingSamples += remaining;
#endif
    
#if FIX_CONTINUITY_LINERP
    if (mUpSample)
    {
        Utils::ConsumeLeft(ioBuf, mOversampling*CONTINUITY_LERP_HISTO_SIZE);
    }
#endif
}
