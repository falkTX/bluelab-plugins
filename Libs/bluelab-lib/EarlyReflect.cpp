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
//  EarlyReflect.cpp
//  UST
//
//  Created by applematuer on 1/16/20.
//
//

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <math.h>

#include <EarlyReflect.h>

EarlyReflect::EarlyReflect(BL_FLOAT sampleRate, BL_FLOAT factor, BL_FLOAT width)
{
    mSampleRate = sampleRate;
    mFactor = factor;
    mWidth = width;
    
    Reset(mSampleRate);
}

EarlyReflect::~EarlyReflect() {}

void
EarlyReflect::Reset(BL_FLOAT sampleRate)
{
    // input sample rate (samples per second)
    int rate = mSampleRate;
    
    // early reflection factor [0.5 to 2.5]
    BL_FLOAT ereffactor = mFactor; //1.0;
    
    // early reflection width [-1 to 1]
    BL_FLOAT erefwidth = mWidth; //0.0;
    
    sf_earlyref(&mRev, rate, ereffactor, erefwidth);
}

void
EarlyReflect::Process(const WDL_TypedBuf<BL_FLOAT> &input,
                      WDL_TypedBuf<BL_FLOAT> *outputL,
                      WDL_TypedBuf<BL_FLOAT> *outputR)
{
    int size = input.GetSize();
    
    sf_sample_st *input0;
    input0 = (sf_sample_st *)malloc(sizeof(sf_sample_st)*size);
    for (int i = 0; i < size; i++)
    {
        // Mono
        BL_FLOAT val = input.Get()[i];
        input0[i].L = val;
        input0[i].R = val;
    }
    
    sf_sample_st *output0;
    output0 = (sf_sample_st *)malloc(sizeof(sf_sample_st)*size);
    
    sf_earlyref_process(&mRev, size, input0, output0);

    for (int i = 0; i < size; i++)
    {
        outputL->Get()[i] = output0[i].L;
        outputR->Get()[i] = output0[i].R;
    }
}
