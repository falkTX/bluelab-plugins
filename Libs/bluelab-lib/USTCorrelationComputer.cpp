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
//  USTCorrelationComputer.cpp
//  UST
//
//  Created by applematuer on 1/2/20.
//
//

#include "USTCorrelationComputer.h"

// Looks to work well with 44100Hz and 88200Hz
#define DEFAULT_SMOOTH_COEFF 0.9999

USTCorrelationComputer::USTCorrelationComputer()
{
    mSmoothCoeff = DEFAULT_SMOOTH_COEFF;
    
    Reset();
}

USTCorrelationComputer::~USTCorrelationComputer() {}

void
USTCorrelationComputer::Reset()
{
    mCurrentL = 0.0;
    mCurrentR = 0.0;
    
    mCurrentL2 = 0.0;
    mCurrentR2 = 0.0;
    
    mCurrentCorrelation = 0.0;
    
    // Save state
    mCurrentLSave = 0.0;
    mCurrentRSave = 0.0;
    
    mCurrentL2Save = 0.0;
    mCurrentR2Save = 0.0;

}
void
USTCorrelationComputer::SetSmoothCoeff(BL_FLOAT smoothCoeff)
{
    mSmoothCoeff = smoothCoeff;
}

void
USTCorrelationComputer::Process(const WDL_TypedBuf<BL_FLOAT> samples[2])
{
#define EPS 1e-15
    
    for (int i = 0; i < samples[0].GetSize(); i++)
    {
        BL_FLOAT l = samples[0].Get()[i];
        BL_FLOAT r = samples[1].Get()[i];
        
        BL_FLOAT l2 = l*l;
        BL_FLOAT r2 = r*r;
        
        BL_FLOAT newL = mSmoothCoeff*mCurrentL + (1.0 - mSmoothCoeff)*l;
        BL_FLOAT newR = mSmoothCoeff*mCurrentR + (1.0 - mSmoothCoeff)*r;
        
        BL_FLOAT newL2 = mSmoothCoeff*mCurrentL2 + (1.0 - mSmoothCoeff)*l2;
        BL_FLOAT newR2 = mSmoothCoeff*mCurrentR2 + (1.0 - mSmoothCoeff)*r2;
        
        mCurrentL = newL;
        mCurrentR = newR;
        
        mCurrentL2 = newL2;
        mCurrentR2 = newR2;
    }
    
    mCurrentCorrelation = 0.0;
    
    BL_FLOAT denom = std::sqrt(mCurrentL2*mCurrentR2);
    if (denom > EPS)
        mCurrentCorrelation = mCurrentL*mCurrentR/denom;
}

void
USTCorrelationComputer::Process(BL_FLOAT l, BL_FLOAT r)
{
    BL_FLOAT l2 = l*l;
    BL_FLOAT r2 = r*r;
    
    BL_FLOAT newL = mSmoothCoeff*mCurrentL + (1.0 - mSmoothCoeff)*l;
    BL_FLOAT newR = mSmoothCoeff*mCurrentR + (1.0 - mSmoothCoeff)*r;
    
    BL_FLOAT newL2 = mSmoothCoeff*mCurrentL2 + (1.0 - mSmoothCoeff)*l2;
    BL_FLOAT newR2 = mSmoothCoeff*mCurrentR2 + (1.0 - mSmoothCoeff)*r2;
    
    mCurrentL = newL;
    mCurrentR = newR;
    
    mCurrentL2 = newL2;
    mCurrentR2 = newR2;

    mCurrentCorrelation = 0.0;

    BL_FLOAT denom = std::sqrt(mCurrentL2*mCurrentR2);
    if (denom > EPS)
        mCurrentCorrelation = mCurrentL*mCurrentR/denom;
}

BL_FLOAT
USTCorrelationComputer::GetCorrelation()
{
    return mCurrentCorrelation;
}

void
USTCorrelationComputer::SaveState()
{
    mCurrentLSave = mCurrentL;
    mCurrentRSave = mCurrentR;
    
    mCurrentL2Save = mCurrentL2;
    mCurrentR2Save = mCurrentR2;
}

void
USTCorrelationComputer::RestoreState()
{
    mCurrentL = mCurrentLSave;
    mCurrentR = mCurrentRSave;
    
    mCurrentL2 = mCurrentL2Save;
    mCurrentR2 = mCurrentR2Save;
}
