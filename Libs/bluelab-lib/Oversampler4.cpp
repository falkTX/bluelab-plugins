//
//  Oversampler2.cpp
//  Denoiser
//
//  Created by Apple m'a Tuer on 26/04/17.
//
//

#include <BLUtils.h>

#include "Oversampler4.h"

// Useless (we always have the good sample count)
#define FIX_ADJUST_IN_RESAMPLING 0 //1

// Re-inject some previous input values before processing,
// to avoid linear interpolation discontinuity between buffers
// (in WDL_Resampler, at each buffer, linear interpolation continuity is lost)
// this attenuates a lot the vertical bars in the spectrogram
//
// Optim
#define FIX_CONTINUITY_LINERP 1

// Avoid that WDL low pass filter the signal
// The filter used has a very unsteep slope, and it cuts too many high frequencies
// NOTE: we will filter somewhere else, with steeper filter
#define DISABLE_WDL_LOW_PASS_FILTER 1

Oversampler4::Oversampler4(BL_FLOAT oversampling, bool upSample)
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

BL_FLOAT
Oversampler4::GetOversampling()
{
    return mOversampling;
}

BL_FLOAT *
Oversampler4::GetOutEmptyBuffer()
{
    return mOutEmptyBuf.Get();
}

void
Oversampler4::Reset(BL_FLOAT sampleRate, int blockSize)
{
    mSampleRate = sampleRate;
    
    mResampler.Reset(); //
    
    // NOTE: a problem was detected during UST Clipper
    // When using on a pure sine wave, clipper gain 18dB
    // => there are very light vertical bars on the spectrogram, every ~2048 sample
    // (otherwise it looks to work well)
    
    // Default: Makes very light vertical bars on spectorgram, every ~2048 samples
    //mResampler.SetMode(true, 1, false, 0, 0);
    
#if !DISABLE_WDL_LOW_PASS_FILTER
    // Low pass filter the signal, which is not good
    // the low pass has not a steep slope at all
    
    // Better than default (but still vertical bars)
    // NOTE: with FIX_CONTINUITY_LINERP added, this removes all the vertical bars
    // and the quality when used in UST is better than SirClipper
    mResampler.SetMode(true, 2, false, 0, 0);
#else
    if (!mUpSample)
        mResampler.SetMode(true, 0, false, 0, 0);
    else
    {
        // Without this in the case of upsampling, there are vertical bars on the spectrogram
        //
        
        // NOTE: with this, filter=1, we remove a little some high frequencies, but so few...
        // NOTE: stil have some vertical bars with pure sine with filter=1
        //mResampler.SetMode(true, 1/*2*/, false, 0, 0);
        
        // filter=2
        // NOTE: - no vertical bars on sine wave test
        //       - remove a bit more high frequencies (very slight)
        //       - less aliasing with sine sweep
        mResampler.SetMode(true, 2, false, 0, 0);
        //mResampler.SetMode(false, 0, true, 64, 32);
    }
#endif
    
    // Does not make vertical bars, but the resampling is not enough "clean"
    //mResampler.SetMode(false, 1, false, 0, 0);
    
    // Makes vertical bars, and resampling better than default
    //mResampler.SetMode(true, 3/*1*/, false, 0, 0);
    
    // Makes vertical bars, and resampling better than default
    //mResampler.SetMode(false, 0, true, 64, 32);
    
    mResampler.SetFilterParms();
    
    mResampler.SetFeedMode(true);
    
    BL_FLOAT rateOut = mUpSample ? sampleRate*mOversampling : sampleRate/mOversampling;
    
    mResampler.SetRates(sampleRate, rateOut);
    
    mRemainingSamples = 0.0;
    
    for (int i = 0; i < CONTINUITY_LERP_HISTO_SIZE; i++)
        mPrevSampValues[i] = 0.0;
}

int
Oversampler4::GetLatency()
{
    int latency = mResampler.GetCurrentLatency();
    
    return latency;
}

void
Oversampler4::Resample(WDL_TypedBuf<BL_FLOAT> *ioBuf)
{
    // Current buf
    WDL_TypedBuf<BL_FLOAT> &buf = *ioBuf;
    
#if FIX_CONTINUITY_LINERP
    if (mUpSample)
    {
        // Add the previous value at the beginning
        int size = ioBuf->GetSize() + CONTINUITY_LERP_HISTO_SIZE;
        if (mTmpContinuityBuf.GetSize() != size)
            mTmpContinuityBuf.Resize(size);
        
        for (int i = 0; i < CONTINUITY_LERP_HISTO_SIZE; i++)
            mTmpContinuityBuf.Get()[i] = mPrevSampValues[i];
        
        for (int i = 0; i < ioBuf->GetSize(); i++)
            mTmpContinuityBuf.Get()[i + CONTINUITY_LERP_HISTO_SIZE] = ioBuf->Get()[i];
        
        // Keep the last value
        for (int i = 0; i < CONTINUITY_LERP_HISTO_SIZE; i++)
        {
            mPrevSampValues[i] =
                ioBuf->Get()[ioBuf->GetSize() - CONTINUITY_LERP_HISTO_SIZE + i];
        }
        
        buf = mTmpContinuityBuf;
    }
#endif

    int srcSize = buf.GetSize();
    
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
    
    BL_FLOAT *src = buf.Get();
    for (int i = 0; i < numSamples; ++i)
    {
        //if (i >= srcSize)
            //break;
        
        if (i >= buf.GetSize())
        {
            *p++ = 0.0;
            
            continue;
        }
        
        *p++ = (WDL_ResampleSample)*src++;
    }
    
    WDL_TypedBuf<BL_FLOAT> dstBuf;
    dstBuf.Resize(dstSize);
    BLUtils::FillAllZero(&dstBuf);
    
    BL_FLOAT *dst = dstBuf.Get();
    
    /*int numOutSamples =*/ mResampler.ResampleOut(dst, srcSize, dstSize, 1);
    
#if FIX_ADJUST_IN_RESAMPLING
    // Adjust
    BL_FLOAT remaining = numOutSamples*srcSize/dstSize - desiredSamples;
    mRemainingSamples += remaining;
#endif
    
    // By default
    *ioBuf = dstBuf;
    
#if FIX_CONTINUITY_LINERP
    if (mUpSample)
    {
        int size2 = dstBuf.GetSize() - mOversampling*CONTINUITY_LERP_HISTO_SIZE;
        if (size2 >= 0)
            ioBuf->Resize(size2);
    
        for (int i = 0; i < ioBuf->GetSize(); i++)
        {
            ioBuf->Get()[i] = dstBuf.Get()[i + (int)(mOversampling*CONTINUITY_LERP_HISTO_SIZE)];
        }
    }
#endif
}
