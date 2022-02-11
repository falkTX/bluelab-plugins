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
    
    mAntiAlias.Calc(0.5 / (double)mOversampling);
    mUpsample.Reset();
    mDownsample.Reset();
}

Oversampler::~Oversampler() {}

const double *
Oversampler::Upsample(double *inBuf, int size)
{
    mUpsample.Reset();
    
    if (mResultBuf.GetSize() < size*mOversampling)
    {
        mResultBuf.Resize(size*mOversampling);
        mOutEmptyBuf.Resize(size*mOversampling);
    }
    
    double *outBuf = mResultBuf.Get();
    
    for (int i  = 0; i < size; i++)
    {
        double sample = inBuf[i];
        
        for (int j = 0; j < mOversampling; j++)
        {
            // Upsample
            if (j > 0)
                sample = 0.;
        
            mUpsample.Process(sample, mAntiAlias.Coeffs());
            sample = (double)mUpsample.Output()*mOversampling;
        
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

const double *
Oversampler::Downsample(double *inBuf, int size)
{
    mDownsample.Reset();
    
    if (mResultBuf.GetSize() < size*mOversampling)
    {
        mResultBuf.Resize(size);
        mOutEmptyBuf.Resize(size);
    }
    
    double *outBuf = mResultBuf.Get();
    
    for (int i  = 0; i < size; i++)
    {
        for (int j = 0; j < mOversampling; j++)
        {
            double sample = inBuf[i*mOversampling + j];
            
            // Downsample
            mDownsample.Process(sample, mAntiAlias.Coeffs());
            if (j == 0)
                outBuf[i] = mDownsample.Output();
        }
    }
    
    return outBuf;
}

double *
Oversampler::GetOutEmptyBuffer()
{
    return mOutEmptyBuf.Get();
}
