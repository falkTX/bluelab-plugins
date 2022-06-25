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
//  ParamSmoother.cpp
//  StereoPan
//
//  Created by Apple m'a Tuer on 09/04/17.
//
//

#include <math.h>
#include <cmath>

#include "ParamSmoother.h"

ParamSmoother::ParamSmoother(BL_FLOAT value, BL_FLOAT smoothCoeff, int precision)
{
    mSmoothCoeff = smoothCoeff;
    mPrecision = precision;
    
    Reset(value);
}

ParamSmoother::ParamSmoother()
{
    mSmoothCoeff = DEFAULT_SMOOTH_COEFF;
    mPrecision = DEFAULT_PRECISION;
    
    Reset();
}

ParamSmoother::~ParamSmoother() {}

BL_FLOAT
ParamSmoother::GetCurrentValue()
{
    return mCurrentValue;
}

void
ParamSmoother::SetSmoothCoeff(BL_FLOAT smoothCoeff)
{
    mSmoothCoeff = smoothCoeff;
}

void
ParamSmoother::SetNewValue(BL_FLOAT value)
{
    mNewValue = value;
    
    if (mFirstUpdate)
        mCurrentValue = mNewValue;
}

void
ParamSmoother::Update()
{
    BL_FLOAT prevValue = mCurrentValue;
    
    if (!mFirstValueSet)
        mCurrentValue = mNewValue;
    else
        mCurrentValue = mSmoothCoeff*mCurrentValue + (1.0 - mSmoothCoeff)*mNewValue;
    
    mFirstValueSet = true;
    
    // Stability and precision
    BL_FLOAT eps = std::pow((SAMPLE_TYPE)10.0, -mPrecision);
    
    BL_FLOAT diff = std::fabs(mCurrentValue - prevValue);
    mIsStable = (diff < eps);
    
    if (mIsStable)
        mCurrentValue = mNewValue;
    
    mFirstUpdate = false;
}

void
ParamSmoother::Reset(BL_FLOAT newValue)
{
    mNewValue = newValue;
    mCurrentValue = newValue;
    
    mFirstValueSet = true;
    
    mIsStable = false;
    
    mFirstUpdate = true;
}

void
ParamSmoother::Reset()
{
    mNewValue = 0.0;
    mCurrentValue = 0.0;
    
    mFirstValueSet = false;
    
    mIsStable = false;
    
    mFirstUpdate = true;
}

bool
ParamSmoother::IsStable()
{
    return mIsStable;
}
