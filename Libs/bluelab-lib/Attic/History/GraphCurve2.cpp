//
//  GraphCurve.cpp
//  EQHack
//
//  Created by Apple m'a Tuer on 10/09/17.
//
//

#include <BLUtils.h>
#include "GraphCurve2.h"

GraphCurve2::GraphCurve2(int numValues)
{
    mYdBScale = false;
    mMinY = 0.0;
    mMaxY = 1.0;
    
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

GraphCurve2::~GraphCurve2() {}

void
GraphCurve2::SetYScale(bool dBFlag, BL_FLOAT minY, BL_FLOAT maxY)
{
    mYdBScale = dBFlag;
    mMinY = minY;
    mMaxY = maxY;
}

void
GraphCurve2::FillAllValues(BL_FLOAT val)
{
    for (int i = 0; i < mValues.GetSize(); i++)
    {
        mValues.Get()[i] = val;
    }
}

void
GraphCurve2::SetValues(const WDL_TypedBuf<BL_FLOAT> *values)
{
    if (mValues.GetSize() != values->GetSize())
        return;
    
    mValues = *values;
}

void
GraphCurve2::SetValue(BL_FLOAT t, BL_FLOAT val)
{
    int pos = t*mNumValues;
    
    if ((pos  < 0) || (pos >= mNumValues))
        return;
    
    mValues.Get()[pos] = val;
}

void
GraphCurve2::PushValue(BL_FLOAT val)
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
        WDL_TypedBuf<BL_FLOAT> newValues;
        for (int i = 1; i < mNumValues + 1; i++)
        {
            BL_FLOAT val = mValues.Get()[i];
            newValues.Add(val);
        }
        
        mValues = newValues;
    }
}
