//
//  AvgHistogram.cpp
//  EQHack
//
//  Created by Apple m'a Tuer on 10/09/17.
//
//

#include "Utils.h"
#include "SmoothAvgHistogramDB.h"


SmoothAvgHistogramDB::SmoothAvgHistogramDB(int size, double smoothCoeff, double defaultValue,
                                           double mindB, double maxdB)
{
    mData.Resize(size);
    
    mSmoothCoeff = smoothCoeff;
    
    mMindB = mindB;
    mMaxdB = maxdB;
    
    double defaultValueDB = Utils::NormalizedYTodB(defaultValue, mMindB, mMaxdB);
    
    mDefaultValue = defaultValueDB;
    
    Reset();
}

SmoothAvgHistogramDB::~SmoothAvgHistogramDB() {}

void
SmoothAvgHistogramDB::AddValue(int index, double val)
{
    val = Utils::NormalizedYTodB(val, mMindB, mMaxdB);
    
    double newVal = (1.0 - mSmoothCoeff) * val + mSmoothCoeff*mData.Get()[index];
    
    mData.Get()[index] = newVal;
}

void
SmoothAvgHistogramDB::AddValues(WDL_TypedBuf<double> *values)
{
    if (values->GetSize() != mData.GetSize())
        return;
    
    for (int i = 0; i < values->GetSize(); i++)
    {
        double val = values->Get()[i];
        
        AddValue(i, val);
    }
}

void
SmoothAvgHistogramDB::GetValues(WDL_TypedBuf<double> *values)
{
    values->Resize(mData.GetSize());
    
    for (int i = 0; i < mData.GetSize(); i++)
    {
        double val = mData.Get()[i];
        
        val = Utils::NormalizedYTodBInv(val, mMindB, mMaxdB);
        
        values->Get()[i] = val;
    }
}

void
SmoothAvgHistogramDB::SetValues(const WDL_TypedBuf<double> *values)
{
    mData = *values;
}

void
SmoothAvgHistogramDB::Reset()
{
    for (int i = 0; i < mData.GetSize(); i++)
        mData.Get()[i] = mDefaultValue;
}
