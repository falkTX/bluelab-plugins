//
//  Oversampler2.cpp
//  Denoiser
//
//  Created by Apple m'a Tuer on 26/04/17.
//
//


#include "Oversampler3.h"

Oversampler3::Oversampler3(int oversampling, bool upSample)
{
    mOversampling = oversampling;
    
    mUpSample = upSample;
    
    mSampleRate = 0;
}

Oversampler3::~Oversampler3() {}

const BL_FLOAT *
Oversampler3::Resample(BL_FLOAT *inBuf, int size)
{
    int resultSize = mUpSample ? size*mOversampling : size;
    
    if (mResultBuf.GetSize() < resultSize)
    {
        mResultBuf.Resize(resultSize);
        mOutEmptyBuf.Resize(resultSize);
    }
    
    BL_FLOAT *outBuf = mResultBuf.Get();
    
    if (mUpSample)
        Resample(inBuf, size, outBuf, size*mOversampling);
    else
        Resample(inBuf, size*mOversampling, outBuf, size);
    
    return outBuf;
}

void
Oversampler3::Resample(WDL_TypedBuf<BL_FLOAT> *ioBuf)
{
    int size = ioBuf->GetSize();
    const BL_FLOAT *newData = Resample2(ioBuf->Get(), size);
    
    int resultSize = mUpSample ? size*mOversampling : size/mOversampling;
    
    ioBuf->Resize(resultSize);
    memcpy(ioBuf->Get(), newData, resultSize*sizeof(BL_FLOAT));
}

int
Oversampler3::GetOversampling()
{
    return mOversampling;
}

BL_FLOAT *
Oversampler3::GetOutEmptyBuffer()
{
    return mOutEmptyBuf.Get();
}

void
Oversampler3::Reset(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    
    // Modifs after IPlugResampler
    mResampler.SetMode(true, 1, false, 0, 0);
    
    //mResampler.SetFilterParms();
    
    mResampler.SetFeedMode(true);
    
    BL_FLOAT rateOut = mUpSample ? sampleRate*mOversampling : sampleRate/mOversampling;
    
    mResampler.SetRates(sampleRate, rateOut);
}

void
Oversampler3::Resample(const BL_FLOAT *src, int srcSize, BL_FLOAT *dst, int dstSize)
{
    WDL_ResampleSample* p;
    int numSamples = mResampler.ResamplePrepare(srcSize, 1, &p);
        
    for (int i = 0; i < numSamples; ++i)
        *p++ = (WDL_ResampleSample)*src++;
        
    int numOutSamples = mResampler.ResampleOut(dst, srcSize, dstSize, 1);
    
#if 0 // Ignore resample error (commenter for Rebalance)
    if (numOutSamples != dstSize)
        // Something failed
        memset(dst, 0, dstSize*sizeof(BL_FLOAT));
#endif
}

// New method, for Rebalance
// (because the original method looked quite buggy)
const BL_FLOAT *
Oversampler3::Resample2(BL_FLOAT *inBuf, int size)
{
    int resultSize = mUpSample ? size*mOversampling : size/mOversampling;
    
    if (mResultBuf.GetSize() < resultSize)
    {
        mResultBuf.Resize(resultSize);
        mOutEmptyBuf.Resize(resultSize);
    }
    
    BL_FLOAT *outBuf = mResultBuf.Get();
    
    mResampler.Reset();
    
    // Set an arbitrary sample rate, to avoid Nyquist test
    Reset(176400.0);
    
    Resample(inBuf, size, outBuf, resultSize);
    
    return outBuf;
}
