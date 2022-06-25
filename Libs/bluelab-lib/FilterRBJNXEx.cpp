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
//  FilterRBJNXEx.cpp
//  UST
//
//  Created by applematuer on 8/25/19.
//
//

#include <BLUtils.h>

#include "FilterRBJNXEx.h"

// Was just a test
#define FIX_BANDPASS 0 //1

FilterRBJNXEx::FilterRBJNXEx(int numFilters,
                           int type,
                           BL_FLOAT sampleRate,
                           BL_FLOAT cutoffFreq,
                           BL_FLOAT QFactor,
                           BL_FLOAT gain)
{
    mNumFilters = numFilters;
    mType = type;
    mSampleRate = sampleRate;
    mCutoffFreq = cutoffFreq;
    mQFactor = QFactor;
    mGain = gain;
    
    mMix = 1.0;
    
    for (int i = 0; i < numFilters; i++)
    {
        CFxRbjFilter *filter = new CFxRbjFilter();
        mFilters.push_back(filter);
    }
    
    for (int i = 0; i < numFilters; i++)
    {
        CFxRbjFilter *filter = new CFxRbjFilter();
        mBypassFilters.push_back(filter);
    }
    
    CalcFilterCoeffs();
}

FilterRBJNXEx::FilterRBJNXEx(const FilterRBJNXEx &other)
{
    mNumFilters = other.mNumFilters;
    mType = other.mType;
    mSampleRate = other.mSampleRate;
    mCutoffFreq = other.mCutoffFreq;
    mQFactor = other.mQFactor;
    mGain = other.mGain;
    mMix = other.mMix;
    
    for (int i = 0; i < mNumFilters; i++)
    {
        CFxRbjFilter *filter = new CFxRbjFilter();
        mFilters.push_back(filter);
    }
    
    for (int i = 0; i < mNumFilters; i++)
    {
        CFxRbjFilter *filter = new CFxRbjFilter();
        mBypassFilters.push_back(filter);
    }
    
    CalcFilterCoeffs();
}

FilterRBJNXEx::~FilterRBJNXEx()
{
    for (int i = 0; i < mFilters.size(); i++)
    {
        CFxRbjFilter *filter = mFilters[i];
        delete filter;
    }
    
    for (int i = 0; i < mBypassFilters.size(); i++)
    {
        CFxRbjFilter *filter = mBypassFilters[i];
        delete filter;
    }
}

void
FilterRBJNXEx::SetCutoffFreq(BL_FLOAT freq)
{
    mCutoffFreq = freq;
    
    CalcFilterCoeffs();
}

void
FilterRBJNXEx::SetQFactor(BL_FLOAT q)
{
    mQFactor = q;
    
    CalcFilterCoeffs();
}

void
FilterRBJNXEx::SetMix(BL_FLOAT mix)
{
    mMix = mix;
}

void
FilterRBJNXEx::SetSampleRate(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    
    CalcFilterCoeffs();
}

BL_FLOAT
FilterRBJNXEx::Process(BL_FLOAT sample)
{
    BL_FLOAT sample0 = sample;
    
    // FIX_FLT_DENORMAL(sample0);
    for (int i = 0; i < mFilters.size(); i++)
    {
        CFxRbjFilter *filter = mFilters[i];
        sample0 = filter->filter(sample0);
        
        //FIX_FLT_DENORMAL(sample0);
    }
    
    BL_FLOAT sample1 = sample;
    
    // FIX_FLT_DENORMAL(sample1);
    for (int i = 0; i < mBypassFilters.size(); i++)
    {
        CFxRbjFilter *filter = mBypassFilters[i];
        sample1 = filter->filter(sample1);
        
        //FIX_FLT_DENORMAL(sample1);
    }
    
    BL_FLOAT res = (1.0 - mMix)*sample1 + mMix*sample0;
    
    return res;
}

void
FilterRBJNXEx::CalcFilterCoeffs()
{
    // For flat crossover
    //BL_FLOAT QFactor = 0.707; // For crossover
    BL_FLOAT QFactor = mQFactor;
    
    // NOTE: set to != 0 for shelves
    //BL_FLOAT dbGain = 0.0; // For crossover
    BL_FLOAT dbGain = mGain;
    bool qIsBandwidth = false;
    
#if FIX_BANDPASS
    if ((mType == FILTER_TYPE_BANDPASS_CSG) ||
        (mType == FILTER_TYPE_BANDPASS_CZPG))
    {
        qIsBandwidth = true;
    }
#endif
    
    for (int i = 0; i < mFilters.size(); i++)
    {
        CFxRbjFilter *filter = mFilters[i];
        filter->calc_filter_coeffs(mType, mCutoffFreq, mSampleRate,
                                  QFactor, dbGain, qIsBandwidth);
    }
    
    // Bypass filters
    for (int i = 0; i < mBypassFilters.size(); i++)
    {
        CFxRbjFilter *filter = mBypassFilters[i];
        filter->calc_filter_coeffs(FILTER_TYPE_ALLPASS, mCutoffFreq, mSampleRate,
                                   QFactor, dbGain, qIsBandwidth);
    }
}

void
FilterRBJNXEx::Process(WDL_TypedBuf<BL_FLOAT> *ioSamples)
{
    WDL_TypedBuf<BL_FLOAT> samples0 = *ioSamples;
    
    // FIX_FLT_DENORMAL(sample0);
    for (int i = 0; i < mFilters.size(); i++)
    {
        CFxRbjFilter *filter = mFilters[i];
        filter->filter(samples0.Get(), samples0.GetSize());
        
        //FIX_FLT_DENORMAL(sample0);
    }
    
    WDL_TypedBuf<BL_FLOAT> samples1 = samples0;
    
    // FIX_FLT_DENORMAL(sample1);
    for (int i = 0; i < mBypassFilters.size(); i++)
    {
        CFxRbjFilter *filter = mBypassFilters[i];
        filter->filter(samples1.Get(), samples1.GetSize());
        
        //FIX_FLT_DENORMAL(sample1);
    }
    
    for (int i = 0; i < ioSamples->GetSize(); i++)
    {
        BL_FLOAT sample0 = samples0.Get()[i];
        BL_FLOAT sample1 = samples1.Get()[i];
        
        BL_FLOAT res = (1.0 - mMix)*sample1 + mMix*sample0;
        
        ioSamples->Get()[i] = res;
    }
}

