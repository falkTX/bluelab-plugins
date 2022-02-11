//
//  AvgHistogram.cpp
//  EQHack
//
//  Created by Apple m'a Tuer on 10/09/17.
//
//

#include "MinMAxAvgHistogram.h"


MinMaxAvgHistogram::MinMaxAvgHistogram(int size, BL_FLOAT smoothCoeff,
				       BL_FLOAT defaultValue)
{
    mMinData.Resize(size);
    mMaxData.Resize(size);
    
    mSmoothCoeff = smoothCoeff;

    mDefaultValue = defaultValue;
    
    Reset();
}

MinMaxAvgHistogram::~MinMaxAvgHistogram() {}

void
MinMaxAvgHistogram::AddValue(int index, BL_FLOAT val)
{
    // min
    BL_FLOAT minVal = mMinData.Get()[index];
    if (val < minVal)
    {
        BL_FLOAT newVal = (1.0 - mSmoothCoeff) * val + mSmoothCoeff*mMinData.Get()[index];
        mMinData.Get()[index] = newVal;
    }
    
    // max
    BL_FLOAT maxVal = mMaxData.Get()[index];
    if (val > maxVal)
    {
        BL_FLOAT newVal = (1.0 - mSmoothCoeff) * val + mSmoothCoeff*mMaxData.Get()[index];
        mMaxData.Get()[index] = newVal;
    }
}

void
MinMaxAvgHistogram::AddValues(WDL_TypedBuf<BL_FLOAT> *values)
{
    if (values->GetSize() > mMinData.GetSize())
        return;
    
    for (int i = 0; i < values->GetSize(); i++)
    {
        BL_FLOAT val = values->Get()[i];
        
        AddValue(i, val);
    }
}

void
MinMaxAvgHistogram::GetMinValues(WDL_TypedBuf<BL_FLOAT> *values)
{
    values->Resize(mMinData.GetSize());
    
    for (int i = 0; i < mMinData.GetSize(); i++)
    {
        BL_FLOAT val = mMinData.Get()[i];
        
        values->Get()[i] = val;
    }
}

void
MinMaxAvgHistogram::GetMaxValues(WDL_TypedBuf<BL_FLOAT> *values)
{
    values->Resize(mMaxData.GetSize());
    
    for (int i = 0; i < mMaxData.GetSize(); i++)
    {
        BL_FLOAT val = mMaxData.Get()[i];
        
        values->Get()[i] = val;
    }
}

void
MinMaxAvgHistogram::GetAvgValues(WDL_TypedBuf<BL_FLOAT> *values)
{
    values->Resize(mMaxData.GetSize());
    
    for (int i = 0; i < mMaxData.GetSize(); i++)
    {
        BL_FLOAT minVal = mMinData.Get()[i];
        BL_FLOAT maxVal = mMaxData.Get()[i];
        
        BL_FLOAT val = (minVal + maxVal) / 2.0;
        values->Get()[i] = val;
    }
}

void
MinMaxAvgHistogram::Reset()
{
    for (int i = 0; i < mMinData.GetSize(); i++)
    {
        mMinData.Get()[i] = mDefaultValue;
        mMaxData.Get()[i] = mDefaultValue;
    }
}
