//
//  AvgHistogram.cpp
//  EQHack
//
//  Created by Apple m'a Tuer on 10/09/17.
//
//

#include "Utils.h"
#include "MinMAxAvgHistogramDB.h"


MinMaxAvgHistogramDB::MinMaxAvgHistogramDB(int size, double smoothCoeff, double defaultValue,
                                           double mindB, double maxdB)
{
    mMinData.Resize(size);
    mMaxData.Resize(size);
    
    mSmoothCoeff = smoothCoeff;
    
    mDefaultValue = defaultValue;
    
    mMindB = mindB;
    mMaxdB = maxdB;
    
    Reset();
}

MinMaxAvgHistogramDB::~MinMaxAvgHistogramDB() {}

void
MinMaxAvgHistogramDB::AddValue(int index, double val)
{
    val = Utils::NormalizedYTodB(val, mMindB, mMaxdB);
    
    // min
    double minVal = mMinData.Get()[index];
    if (val < minVal)
    {
        double newVal = (1.0 - mSmoothCoeff) * val + mSmoothCoeff*mMinData.Get()[index];
        mMinData.Get()[index] = newVal;
    }
    
    // max
    double maxVal = mMaxData.Get()[index];
    if (val > maxVal)
    {
        double newVal = (1.0 - mSmoothCoeff) * val + mSmoothCoeff*mMaxData.Get()[index];
        mMaxData.Get()[index] = newVal;
    }
}

void
MinMaxAvgHistogramDB::AddValues(WDL_TypedBuf<double> *values)
{
    if (values->GetSize() > mMinData.GetSize())
        return;
    
    for (int i = 0; i < values->GetSize(); i++)
    {
        double val = values->Get()[i];
        
        AddValue(i, val);
    }
}

void
MinMaxAvgHistogramDB::GetMinValues(WDL_TypedBuf<double> *values)
{
    values->Resize(mMinData.GetSize());
    
    for (int i = 0; i < mMinData.GetSize(); i++)
    {
        double val = mMinData.Get()[i];
        
        Utils::NormalizedYTodBInv(val, mMindB, mMaxdB);
        
        values->Get()[i] = val;
    }
}

void
MinMaxAvgHistogramDB::GetMaxValues(WDL_TypedBuf<double> *values)
{
    values->Resize(mMaxData.GetSize());
    
    for (int i = 0; i < mMaxData.GetSize(); i++)
    {
        double val = mMaxData.Get()[i];
        
        Utils::NormalizedYTodBInv(val, mMindB, mMaxdB);
        
        values->Get()[i] = val;
    }
}

void
MinMaxAvgHistogramDB::GetAvgValues(WDL_TypedBuf<double> *values)
{
    values->Resize(mMaxData.GetSize());
    
    for (int i = 0; i < mMaxData.GetSize(); i++)
    {
        double minVal = mMinData.Get()[i];
        double maxVal = mMaxData.Get()[i];
        
        double val = (minVal + maxVal)/2.0;
        
        //double val = Utils::AverageYDB(minVal, maxVal, mMindB, mMaxdB);
        
        Utils::NormalizedYTodBInv(val, mMindB, mMaxdB);
        
        values->Get()[i] = val;
    }
}

void
MinMaxAvgHistogramDB::Reset()
{
    for (int i = 0; i < mMinData.GetSize(); i++)
    {
        mMinData.Get()[i] = mDefaultValue;
        mMaxData.Get()[i] = mDefaultValue;
    }
}
