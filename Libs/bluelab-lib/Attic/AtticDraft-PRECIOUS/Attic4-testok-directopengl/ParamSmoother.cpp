//
//  ParamSmoother.cpp
//  StereoPan
//
//  Created by Apple m'a Tuer on 09/04/17.
//
//

#include <math.h>

#include "ParamSmoother.h"

ParamSmoother::ParamSmoother(double value, double smoothCoeff, int precision)
{
    mNewValue = value;
    mCurrentValue = value;
    
    mSmoothCoeff = smoothCoeff;
    
    mFirstValueSet = true;
    
    mIsStable = false;
    mPrecision = precision;
    
    mFirstUpdate = true;
}

ParamSmoother::ParamSmoother()
{
    mNewValue = 0.0;
    mCurrentValue = 0.0;
    
    mSmoothCoeff = DEFAULT_SMOOTH_COEFF;
    
    mFirstValueSet = false;
    
    mIsStable = false;
    mPrecision = DEFAULT_PRECISION;
    
    mFirstUpdate = true;
}

ParamSmoother::~ParamSmoother() {}

double
ParamSmoother::GetCurrentValue()
{
    return mCurrentValue;
}

void
ParamSmoother::SetSmoothCoeff(double smoothCoeff)
{
    mSmoothCoeff = smoothCoeff;
}

void
ParamSmoother::SetNewValue(double value)
{
    mNewValue = value;
    
    if (mFirstUpdate)
        mCurrentValue = mNewValue;
}

void
ParamSmoother::Update()
{
    double prevValue = mCurrentValue;
    
    if (!mFirstValueSet)
        mCurrentValue = mNewValue;
    else
        mCurrentValue = mSmoothCoeff*mCurrentValue + (1.0 - mSmoothCoeff)*mNewValue;
    
    mFirstValueSet = true;
    
    // Stability and precision
    double eps = pow(10.0, -mPrecision);
    
    double diff = fabs(mCurrentValue - prevValue);
    mIsStable = (diff < eps);
    
    if (mIsStable)
        mCurrentValue = mNewValue;
    
    mFirstUpdate = false;
}

void
ParamSmoother::Reset(double newValue)
{
    mNewValue = newValue;
    mCurrentValue = newValue;
}

bool
ParamSmoother::IsStable()
{
    return mIsStable;
}