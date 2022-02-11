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
