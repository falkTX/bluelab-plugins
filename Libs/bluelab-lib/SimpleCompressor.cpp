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
//  SimpleCompressor.cpp
//  UST
//
//  Created by applematuer on 2/29/20.
//
//

#include "SimpleCompressor.h"

SimpleCompressor::SimpleCompressor(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    
    mPregain = 0.0;
    mThreshold = 0.0;
    mKnee = 0.0;
    mRatio = 1.0;
    
    mAttack = 0.0;
    mRelease = 0.0;
    
    mState = new sf_compressor_state_st;
    
    Init();
}

SimpleCompressor::~SimpleCompressor()
{
    delete mState;
}

void
SimpleCompressor::Reset(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    
    Init();
}

void
SimpleCompressor::SetParameters(BL_FLOAT pregain, BL_FLOAT threshold, BL_FLOAT knee,
                                BL_FLOAT ratio, BL_FLOAT attack, BL_FLOAT release)
{
    mPregain = pregain;
    mThreshold = threshold;
    mKnee = knee;
    mRatio = ratio;
    mAttack = attack;
    mRelease = release;
    
    Init();
}

#if 0
void
SimpleCompressor::Process(BL_FLOAT *l, BL_FLOAT *r)
{
    sf_sample_st input;
    input.L = *l;
    input.R = *r;
    
    sf_sample_st output;
    
    sf_compressor_process(mState, 1, &input, &output);
    
    *l = output.L;
    *r = output.R;
}
#endif

void
SimpleCompressor::Process(WDL_TypedBuf<BL_FLOAT> *l, WDL_TypedBuf<BL_FLOAT> *r)
{
    sf_sample_st *input = new sf_sample_st[l->GetSize()];
    for (int i = 0; i < l->GetSize(); i++)
    {
        input[i].L = l->Get()[i];
        input[i].R = r->Get()[i];
    }
    
    sf_sample_st *output = new sf_sample_st[l->GetSize()];
    
    //
    sf_compressor_process(mState, l->GetSize(), input, output);
    
    for (int i = 0; i < l->GetSize(); i++)
    {
        l->Get()[i] = output[i].L;
        r->Get()[i] = output[i].R;
    }
    
    delete input;
    delete output;
}

void
SimpleCompressor::Init()
{
    sf_simplecomp(mState, mSampleRate, mPregain,
                  mThreshold, mKnee, mRatio,
                  mAttack, mRelease);
}
