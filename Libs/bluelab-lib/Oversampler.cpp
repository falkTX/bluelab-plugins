//
//  Oversampler.cpp
//  Denoiser
//
//  Created by Apple m'a Tuer on 26/04/17.
//
//


#include "Oversampler.h"

Oversampler::Oversampler(int oversampling)
{
    mOversampling = oversampling;
    
    mAntiAlias.Calc(0.5 / (BL_FLOAT)mOversampling);
    mUpsample.Reset();
    mDownsample.Reset();
}

Oversampler::~Oversampler() {}

const BL_FLOAT *
Oversampler::Upsample(BL_FLOAT *inBuf, int size)
{
    mUpsample.Reset();
    
    if (mResultBuf.GetSize() < size*mOversampling)
    {
        mResultBuf.Resize(size*mOversampling);
        mOutEmptyBuf.Resize(size*mOversampling);
    }
    
    BL_FLOAT *outBuf = mResultBuf.Get();
    
    for (int i  = 0; i < size; i++)
    {
        BL_FLOAT sample = inBuf[i];
        
        for (int j = 0; j < mOversampling; j++)
        {
            // Upsample
            if (j > 0)
                sample = 0.;
        
            mUpsample.Process(sample, mAntiAlias.Coeffs());
            sample = (BL_FLOAT)mUpsample.Output()*mOversampling;
        
            outBuf[i*mOversampling + j] = sample;
        }
    }
    
#if 0
    // Debug
    FILE *file0 = fopen("/Volumes/HDD/Share/original.txt", "w");
    for (int i = 0; i < size; i++)
        fprintf(file0, "%f ", inBuf[i]);
    fclose(file0);
    
    FILE *file1 = fopen("/Volumes/HDD/Share/result.txt", "w");
    for (int i = 0; i < size*mOversampling; i++)
        fprintf(file1, "%f ", outBuf[i]);
    fclose(file1);
#endif
    
    return outBuf;
}

const BL_FLOAT *
Oversampler::Downsample(BL_FLOAT *inBuf, int size)
{
    mDownsample.Reset();
    
    if (mResultBuf.GetSize() < size*mOversampling)
    {
        mResultBuf.Resize(size);
        mOutEmptyBuf.Resize(size);
    }
    
    BL_FLOAT *outBuf = mResultBuf.Get();
    
    for (int i  = 0; i < size; i++)
    {
        for (int j = 0; j < mOversampling; j++)
        {
            BL_FLOAT sample = inBuf[i*mOversampling + j];
            
            // Downsample
            mDownsample.Process(sample, mAntiAlias.Coeffs());
            if (j == 0)
                outBuf[i] = mDownsample.Output();
        }
    }
    
    return outBuf;
}

BL_FLOAT *
Oversampler::GetOutEmptyBuffer()
{
    return mOutEmptyBuf.Get();
}
