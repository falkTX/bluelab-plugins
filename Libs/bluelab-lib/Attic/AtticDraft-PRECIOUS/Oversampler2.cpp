//
//  Oversampler2.cpp
//  Denoiser
//
//  Created by Apple m'a Tuer on 26/04/17.
//
//


#include "Oversampler2.h"

#define BLOCK_LENGTH 64

Oversampler2::Oversampler2(int oversampling)
{
    mOversampling = oversampling;
    mSampleRate = 0;
}

Oversampler2::~Oversampler2() {}

const double *
Oversampler2::Upsample(double *inBuf, int size)
{
    if (mResultBuf.GetSize() < size*mOversampling)
    {
        mResultBuf.Resize(size*mOversampling);
        mOutEmptyBuf.Resize(size*mOversampling);
    }
    
    double *outBuf = mResultBuf.Get();
    
    Resample(inBuf, size, mSampleRate,
             outBuf, size*mOversampling, mSampleRate*mOversampling);
    
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
Oversampler2::Downsample(double *inBuf, int size)
{
    if (mResultBuf.GetSize() < size*mOversampling)
    {
        mResultBuf.Resize(size);
        mOutEmptyBuf.Resize(size);
    }
    
    double *outBuf = mResultBuf.Get();
    
    Resample(inBuf, size*mOversampling, mSampleRate*mOversampling,
             outBuf, size, mSampleRate);
    
    return outBuf;
}

int
Oversampler2::GetOversampling()
{
    return mOversampling;
}

double *
Oversampler2::GetOutEmptyBuffer()
{
    return mOutEmptyBuf.Get();
}

void
Oversampler2::Reset(double sampleRate)
{
    if (sampleRate != mSampleRate)
    {
        mSampleRate = sampleRate;
        
#if 0
        mResampler.SetMode(false, 0, true); // Sinc, default size
        mResampler.SetFeedMode(true); // Input driven
#endif
        //mResampler.SetMode(true, 1, false, 0, 0);
        mResampler.SetMode(false, 1, false, 0, 0);
        mResampler.SetFeedMode(true);
    }
}

void
Oversampler2::Resample(const double *src, int srcSize, double srcRate,
                       double *dst, int dstSize, double dstRate)
{
    if (srcSize == dstSize)
    {
        memcpy(dst, src, srcSize*sizeof(double));
        
        return;
    }
    
    mResampler.SetRates(srcRate, dstRate);
#if 0 // With that, the scale will be applied to the output gain
      // This is not correct
    double scale = srcRate / dstRate;
#endif
    
    while (dstSize > 0)
    {
        WDL_ResampleSample* p;
        int n = mResampler.ResamplePrepare(BLOCK_LENGTH, 1, &p);
        int m = n;
        
        if (n > srcSize)
            n = srcSize;
        
        for (int i = 0; i < n; ++i)
            *p++ = (WDL_ResampleSample)*src++;
        
        if (n < m)
            memset(p, 0, (m - n) * sizeof(WDL_ResampleSample));
        
        srcSize -= n;
            
        WDL_ResampleSample buf[BLOCK_LENGTH];
        
        n = mResampler.ResampleOut(buf, m, m, 1);
        if (n > dstSize)
            n = dstSize;
        
        p = buf;
        for (int i = 0; i < n; ++i)
            *dst++ = (double)(*p++);
#if 0
            *dst++ = (double)(scale * *p++);
#endif
        
        dstSize -= n;
    }
   
    mResampler.Reset();
}