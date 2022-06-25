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
//  Oversampler2.cpp
//  Denoiser
//
//  Created by Apple m'a Tuer on 26/04/17.
//
//


#include "Resampler2.h"

Resampler2::Resampler2(BL_FLOAT inSampleRate, BL_FLOAT outSampleRate)
{
    mInSampleRate = inSampleRate;
    mOutSampleRate = outSampleRate;
    
    Reset(mInSampleRate, mOutSampleRate);
}

Resampler2::~Resampler2() {}

void
Resampler2::Resample(const WDL_TypedBuf<BL_FLOAT> *input,
                     WDL_TypedBuf<BL_FLOAT> *output)
{
    if (mOutSampleRate == mInSampleRate)
    {
        *output = *input;
        
        return;
    }
    
    int inSize = input->GetSize();
    int outSize = input->GetSize()*(mOutSampleRate/mInSampleRate);
    
    output->Resize(outSize);
    
    Resample(input->Get(), inSize, output->Get(), outSize);
}

void
Resampler2::Resample2(const WDL_TypedBuf<BL_FLOAT> *input,
                       WDL_TypedBuf<BL_FLOAT> *output)
{
    if (mOutSampleRate == mInSampleRate)
    {
        *output = *input;
        
        return;
    }
    
    if (mInSampleRate == 0)
        return;
    
    int inSize = input->GetSize();
    int outSize = input->GetSize()*(mOutSampleRate/mInSampleRate);
    
    output->Resize(outSize);
    
    Resample2(input->Get(), inSize, output->Get(), outSize);
}

void
Resampler2::Reset(BL_FLOAT inSampleRate, BL_FLOAT outSampleRate)
{
    mInSampleRate = inSampleRate;
    mOutSampleRate = outSampleRate;
    
    mResampler.SetMode(true, 1, false, 0, 0);
    mResampler.SetFeedMode(true);
    
    if ((mInSampleRate > 0) && (mOutSampleRate > 0))
        mResampler.SetRates(mInSampleRate, mOutSampleRate);
}

void
Resampler2::Resample(const WDL_TypedBuf<BL_FLOAT> *input,
                     WDL_TypedBuf<BL_FLOAT> *output,
                     BL_FLOAT inSampleRate, BL_FLOAT outSampleRate)
{
    Resampler2 resampler(inSampleRate, outSampleRate);
    
    resampler.Resample(input, output);
}

void
Resampler2::Resample2(const WDL_TypedBuf<BL_FLOAT> *input,
                      WDL_TypedBuf<BL_FLOAT> *output,
                      BL_FLOAT inSampleRate, BL_FLOAT outSampleRate)
{
    Resampler2 resampler(inSampleRate, outSampleRate);
    
    resampler.Resample2(input, output);
}

#if 0 // Buggy, not tested
void
Resampler2::Resample(const WDL_TypedBuf<BL_FLOAT> *input,
                     WDL_TypedBuf<BL_FLOAT> *output,
                     int srcSize, int dstSize)
{
    if (input->GetSize() != srcSize)
        return;
    
    output->Resize(dstSize);
    
    Resampler2 resampler(-1, -1);
    
    resampler.Resample(input->Get(), srcSize, output->Get(), dstSize);
}
#endif

void
Resampler2::Resample(const BL_FLOAT *src, int srcSize, BL_FLOAT *dst, int dstSize)
{
    WDL_ResampleSample* p;
    int numSamples = mResampler.ResamplePrepare(srcSize, 1, &p);
        
    for (int i = 0; i < numSamples; ++i)
        *p++ = (WDL_ResampleSample)*src++;
        
    int numOutSamples = mResampler.ResampleOut(dst, srcSize, dstSize, 1);
    if (numOutSamples != dstSize)
        // Something failed
        memset(dst, 0, dstSize*sizeof(BL_FLOAT));
}

// For Spatializer
void
Resampler2::Resample2(const BL_FLOAT *src, int srcSize, BL_FLOAT *dst, int dstSize)
{
    WDL_ResampleSample* p;
    int numSamples = mResampler.ResamplePrepare(srcSize, 1, &p);
    
    for (int i = 0; i < numSamples; ++i)
        *p++ = (WDL_ResampleSample)*src++;
    
    int numOutSamples = mResampler.ResampleOut(dst, srcSize, dstSize, 1);
    if (numOutSamples != dstSize)
    {
        // Something failed ?
        int numZeros = dstSize - numOutSamples;
        if (numZeros > 0)
        {
            memset(&dst[numOutSamples], 0, numZeros*sizeof(BL_FLOAT));
        }
    }
}

