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
//  FilterRBJ2X.cpp
//  UST
//
//  Created by applematuer on 8/25/19.
//
//

#include <BLUtils.h>

#include "FilterRBJ2X.h"

// Without this, in UST, the horizontal line indicating the sum of the filters
// makes oscillations
#define FIX_RBJ_ALLPASS 0 //1

FilterRBJ2X::FilterRBJ2X(int type,
                         BL_FLOAT sampleRate,
                         BL_FLOAT cutoffFreq)
{
    mType = type;
    mSampleRate = sampleRate;
    mCutoffFreq = cutoffFreq;
    
    mQFactor = 0.707;
    
    mFilters[0] = new CFxRbjFilter();
    mFilters[1] = new CFxRbjFilter();
    
    CalcFilterCoeffs();
}

FilterRBJ2X::FilterRBJ2X(const FilterRBJ2X &other)
{
    mType = other.mType;
    mSampleRate = other.mSampleRate;
    mCutoffFreq = other.mCutoffFreq;

    mQFactor = 0.707;
    
    mFilters[0] = new CFxRbjFilter();
    mFilters[1] = new CFxRbjFilter();
    
    CalcFilterCoeffs();
}

FilterRBJ2X::~FilterRBJ2X()
{
    delete mFilters[0];
    delete mFilters[1];
}

void
FilterRBJ2X::SetCutoffFreq(BL_FLOAT freq)
{
    mCutoffFreq = freq;
    
    CalcFilterCoeffs();
}

void
FilterRBJ2X::SetSampleRate(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    
    CalcFilterCoeffs();
}

void
FilterRBJ2X::SetQFactor(BL_FLOAT q)
{
    mQFactor = q;
    
    CalcFilterCoeffs();
}

BL_FLOAT
FilterRBJ2X::Process(BL_FLOAT sample)
{
    //FIX_FLT_DENORMAL(sample);
    
    sample = mFilters[0]->filter(sample);
    
    //FIX_FLT_DENORMAL(sample)
    
    sample = mFilters[1]->filter(sample);
    
    //FIX_FLT_DENORMAL(sample)
    
    return sample;
}

void
FilterRBJ2X::CalcFilterCoeffs()
{
    // For flat crossover
    //BL_FLOAT QFactor = 0.707;
    BL_FLOAT QFactor = mQFactor;
    
    BL_FLOAT dbGain = 0.0;
    bool qIsBandwidth = false;
    
#if FIX_RBJ_ALLPASS
    if (mType == FILTER_TYPE_ALLPASS)
        qIsBandwidth = true;
#endif
    
    for (int i = 0; i < 2; i++)
        mFilters[i]->calc_filter_coeffs(mType, mCutoffFreq, mSampleRate,
                                        QFactor, dbGain, qIsBandwidth);
}

void
FilterRBJ2X::Process(WDL_TypedBuf<BL_FLOAT> *ioSamples)
{
    //FIX_FLT_DENORMAL(sample);
    
    mFilters[0]->filter(ioSamples->Get(), ioSamples->GetSize());
    
    //FIX_FLT_DENORMAL(sample)
    
    mFilters[1]->filter(ioSamples->Get(), ioSamples->GetSize());
    
    //FIX_FLT_DENORMAL(sample)
}
