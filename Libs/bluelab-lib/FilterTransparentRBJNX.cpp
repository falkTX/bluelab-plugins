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
//  FilterTransparentRBJNX.cpp
//  UST
//
//  Created by applematuer on 8/25/19.
//
//

#include <FilterRBJNX.h>

#include <BLUtils.h>

#include "FilterTransparentRBJNX.h"

FilterTransparentRBJNX::FilterTransparentRBJNX(int numFilters,
                                           BL_FLOAT sampleRate,
                                           BL_FLOAT cutoffFreq)
{
    mNumFilters = numFilters;
    mSampleRate = sampleRate;
    mCutoffFreq = cutoffFreq;
    
    mFilters[0] = new FilterRBJNX(numFilters, FILTER_TYPE_LOWPASS,
                                 sampleRate, cutoffFreq);
    
    mFilters[1] = new FilterRBJNX(numFilters, FILTER_TYPE_HIPASS,
                                 sampleRate, cutoffFreq);
}

FilterTransparentRBJNX::FilterTransparentRBJNX(const FilterTransparentRBJNX &other)
{
    mNumFilters = other.mNumFilters;
    mSampleRate = other.mSampleRate;
    mCutoffFreq = other.mCutoffFreq;
    
    mFilters[0] = new FilterRBJNX(mNumFilters, FILTER_TYPE_LOWPASS,
                                 mSampleRate, mCutoffFreq);
    
    mFilters[1] = new FilterRBJNX(mNumFilters, FILTER_TYPE_HIPASS,
                                 mSampleRate, mCutoffFreq);
}

FilterTransparentRBJNX::~FilterTransparentRBJNX()
{
    for (int i = 0; i < 2; i++)
    {
        FilterRBJNX *filter = mFilters[i];
        delete filter;
    }
}

void
FilterTransparentRBJNX::SetCutoffFreq(BL_FLOAT freq)
{
    mCutoffFreq = freq;
    
    for (int i = 0; i < 2; i++)
    {
        FilterRBJNX *filter = mFilters[i];
        filter->SetCutoffFreq(freq);
    }
}

// NEW
void
FilterTransparentRBJNX::SetQFactor(BL_FLOAT q)
{
    for (int i = 0; i < 2; i++)
    {
        FilterRBJNX *filter = mFilters[i];
        filter->SetQFactor(q);
    }
}

void
FilterTransparentRBJNX::SetSampleRate(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    
    for (int i = 0; i < 2; i++)
    {
        FilterRBJNX *filter = mFilters[i];
        filter->SetSampleRate(sampleRate);
    }
}

BL_FLOAT
FilterTransparentRBJNX::Process(BL_FLOAT sample)
{
    BL_FLOAT sum = 0.0;
    for (int i = 0; i < 2; i++)
    {
        FilterRBJNX *filter = mFilters[i];
        BL_FLOAT result = filter->Process(sample);
        
        sum += result;
    }
    
    return sum;
}

void
FilterTransparentRBJNX::Process(WDL_TypedBuf<BL_FLOAT> *ioSamples)
{
    WDL_TypedBuf<BL_FLOAT> sum;
    sum.Resize(ioSamples->GetSize());
    BLUtils::FillAllZero(&sum);

    for (int i = 0; i < 2; i++)
    {
        FilterRBJNX *filter = mFilters[i];
        
        WDL_TypedBuf<BL_FLOAT> samples = *ioSamples;
        filter->Process(&samples);
        
        BLUtils::AddValues(&sum, samples);
    }
    
    *ioSamples = sum;
}

