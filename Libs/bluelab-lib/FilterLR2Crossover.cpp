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
//  FilterLR2Crossover.cpp
//  UST
//
//  Created by applematuer on 7/28/19.
//
//

#include "FilterLR2Crossover.h"

#ifndef M_PI
#define M_PI 3.141592653589793
#endif

FilterLR2Crossover::FilterLR2Crossover(BL_FLOAT cutoffFreq, BL_FLOAT sampleRate)
{
    mCutoffFreq = cutoffFreq;
    mSampleRate = sampleRate;
    
    Init();
}

FilterLR2Crossover::~FilterLR2Crossover() {}

void
FilterLR2Crossover::Reset(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    
    Init();
}

void
FilterLR2Crossover::SetCutoffFreq(BL_FLOAT freq)
{
    mCutoffFreq = freq;
    
    Init();
}

void
FilterLR2Crossover::Reset(BL_FLOAT cutoffFreq, BL_FLOAT sampleRate)
{
    mCutoffFreq = cutoffFreq;
    mSampleRate = sampleRate;
    
    Init();
}

void
FilterLR2Crossover::Process(BL_FLOAT inSample,
                            BL_FLOAT *lpOutSample,
                            BL_FLOAT *hpOutSample)
{
    //=========================
    // sample loop, in -> input
    //=========================
    
    //---lp
    BL_FLOAT lp_out = mA0_lp*inSample + mLp_xm0;
    mLp_xm0 = mA1_lp*inSample - mB1*lp_out + mLp_xm1;
    mLp_xm1 = mA2_lp*inSample - mB2*lp_out;
    
    //---hp
    BL_FLOAT hp_out = mA0_hp*inSample + mHp_xm0;
    mHp_xm0 = mA1_hp*inSample - mB1*hp_out + mHp_xm1;
    mHp_xm1 = mA2_hp*inSample - mB2*hp_out;
    
    // the two are with 180 degrees phase shift,
    // so you need to invert the phase of one.
    //out = lp_out + hp_out*(-1);
    
    //result is allpass at Fc
    
    *lpOutSample = lp_out;
    *hpOutSample = hp_out;
}

void
FilterLR2Crossover::Process(const WDL_TypedBuf<BL_FLOAT> &inSamples,
                            WDL_TypedBuf<BL_FLOAT> *lpOutSamples,
                            WDL_TypedBuf<BL_FLOAT> *hpOutSamples)
{
    lpOutSamples->Resize(inSamples.GetSize());
    hpOutSamples->Resize(inSamples.GetSize());
    
    for (int i = 0; i < inSamples.GetSize(); i++)
    {
        BL_FLOAT s = inSamples.Get()[i];
    
        BL_FLOAT lpS;
        BL_FLOAT hpS;
        
        Process(s, &lpS, &hpS);
        
        lpOutSamples->Get()[i] = lpS;
        hpOutSamples->Get()[i] = hpS;
    }
}

void
FilterLR2Crossover::Init()
{
    //------------------------------
    // LR2
    // fc -> cutoff frequency
    // pi -> 3.14285714285714
    // srate -> sample rate
    //------------------------------
    BL_FLOAT fpi = M_PI*mCutoffFreq;
    BL_FLOAT wc = 2.0*fpi;
    BL_FLOAT wc2 = wc*wc;
    BL_FLOAT wc22 = 2*wc2;
    BL_FLOAT k = wc/tan(fpi/mSampleRate);
    BL_FLOAT k2 = k*k;
    BL_FLOAT k22 = 2*k2;
    BL_FLOAT wck2 = 2*wc*k;
    BL_FLOAT tmpk = (k2+wc2+wck2);
    
    //b shared
    mB1 = (-k22+wc22)/tmpk;
    mB2 = (-wck2+k2+wc2)/tmpk;
    
    //---------------
    // low-pass
    //---------------
    mA0_lp = (wc2)/tmpk;
    mA1_lp = (wc22)/tmpk;
    mA2_lp = (wc2)/tmpk;
    
    //----------------
    // high-pass
    //----------------
    mA0_hp = (k2)/tmpk;
    mA1_hp = (-k22)/tmpk;
    mA2_hp = (k2)/tmpk;
    
    // Niko
    mLp_xm0 = 0.0;
    mLp_xm1 = 0.0;
    
    mHp_xm0 = 0.0;
    mHp_xm1 = 0.0;
}
