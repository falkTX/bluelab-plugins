//
//  GraphCurve.cpp
//  EQHack
//
//  Created by Apple m'a Tuer on 10/09/17.
//
//

#include "Utils.h"
#include "GraphCurve.h"

GraphCurve::GraphCurve(int numValues)
{
    mYdBScale = false;
    mMinYdB = 0.0;
    mMaxYdB = 1.0;
    
    SET_COLOR_FROM_INT(mColor, 0, 0, 0, 255);
    mAlpha = 1.0;
    
    mLineWidth = 1.0;
    
    mCurveFill = false;
    mFillAlpha = FILL_CURVE_ALPHA;
    
    mSingleValue = false;
    
    mNumValues = numValues;
    mValues.Resize(mNumValues);
    FillAllValues(0.0);
    
    mPushCounter = 0;
    mTmpPushValue = 0.0;
}

GraphCurve::~GraphCurve() {}

void
GraphCurve::SetYdBScale(bool flag, double minYdB, double maxYdB)
{
    mYdBScale = flag;
    mMinYdB = minYdB;
    mMaxYdB = maxYdB;
}

void
GraphCurve::FillAllValues(double val)
{
    for (int i = 0; i < mValues.GetSize(); i++)
    {
        mValues.Get()[i] = val;
    }
}

void
GraphCurve::SetValues(const WDL_TypedBuf<double> *values)
{
    if (mValues.GetSize() != values->GetSize())
        return;
    
    mValues = *values;
}

void
GraphCurve::SetValue(double t, double val)
{
    int pos = t*mNumValues;
    
    if ((pos  < 0) || (pos >= mNumValues))
        return;
    
    mValues.Get()[pos] = val;
}

void
GraphCurve::PushValue(double val)
{
    if (mDoSmooth)
    {
        mParamSmoother.SetNewValue(val);
        mParamSmoother.Update();
        val = mParamSmoother.GetCurrentValue();
    }
    
    mValues.Add(val);

    if (mValues.GetSize() > mNumValues)
    {
        WDL_TypedBuf<double> newValues;
        for (int i = 1; i < mNumValues + 1; i++)
        {
            double val = mValues.Get()[i];
            newValues.Add(val);
        }
        
        mValues = newValues;
    }
}

