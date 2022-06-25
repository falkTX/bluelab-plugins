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
//  FilterTransparentRBJ2X2.cpp
//  UST
//
//  Created by applematuer on 8/25/19.
//
//

#include <FilterRBJ1X.h>
#include <FilterRBJ2X.h>
#include <FilterRBJ2X2.h>

#include <BLUtils.h>

#include "FilterTransparentRBJ2X2.h"

FilterTransparentRBJ2X2::FilterTransparentRBJ2X2(BL_FLOAT sampleRate,
                                               BL_FLOAT cutoffFreq)
{
    mSampleRate = sampleRate;
    mCutoffFreq = cutoffFreq;
    
    mFilter = new TRANSPARENT_RBJ_2X2_FILTER_2X_CLASS(FILTER_TYPE_ALLPASS,
                                                      sampleRate, cutoffFreq);
}

FilterTransparentRBJ2X2::FilterTransparentRBJ2X2(const FilterTransparentRBJ2X2 &other)
{
    mSampleRate = other.mSampleRate;
    mCutoffFreq = other.mCutoffFreq;
    
    mFilter = new TRANSPARENT_RBJ_2X2_FILTER_2X_CLASS(FILTER_TYPE_ALLPASS,
                                                      mSampleRate, mCutoffFreq);
}

FilterTransparentRBJ2X2::~FilterTransparentRBJ2X2()
{
    delete mFilter;
}

void
FilterTransparentRBJ2X2::SetCutoffFreq(BL_FLOAT freq)
{
    mCutoffFreq = freq;
    
    mFilter->SetCutoffFreq(freq);
}

// NEW
void
FilterTransparentRBJ2X2::SetQFactor(BL_FLOAT q)
{
    mFilter->SetQFactor(q);
}

void
FilterTransparentRBJ2X2::SetSampleRate(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    
    mFilter->SetSampleRate(sampleRate);
}

BL_FLOAT
FilterTransparentRBJ2X2::Process(BL_FLOAT sample)
{
    BL_FLOAT result = mFilter->Process(sample);
    
    return result;
}

void
FilterTransparentRBJ2X2::Process(WDL_TypedBuf<BL_FLOAT> *ioSamples)
{    
    mFilter->Process(ioSamples);
}
