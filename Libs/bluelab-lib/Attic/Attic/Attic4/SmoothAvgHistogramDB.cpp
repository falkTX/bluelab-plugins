//
//  AvgHistogram.cpp
//  EQHack
//
//  Created by Apple m'a Tuer on 10/09/17.
//
//

#include "BLUtils.h"
#include "SmoothAvgHistogramDB.h"


SmoothAvgHistogramDB::SmoothAvgHistogramDB(int size, BL_FLOAT smoothCoeff, BL_FLOAT defaultValue,
                                           BL_FLOAT mindB, BL_FLOAT maxdB)
{
    mData.Resize(size);
    
    mSmoothCoeff = smoothCoeff;
    
    mMindB = mindB;
    mMaxdB = maxdB;
    
    BL_FLOAT defaultValueDB = BLUtils::NormalizedYTodB(defaultValue, mMindB, mMaxdB);
    
    mDefaultValue = defaultValueDB;
    
    Reset();
}

SmoothAvgHistogramDB::~SmoothAvgHistogramDB() {}

void
SmoothAvgHistogramDB::AddValue(int index, BL_FLOAT val)
{
    val = BLUtils::NormalizedYTodB(val, mMindB, mMaxdB);
    
    BL_FLOAT newVal = (1.0 - mSmoothCoeff) * val + mSmoothCoeff*mData.Get()[index];
    
    mData.Get()[index] = newVal;
}

void
SmoothAvgHistogramDB::AddValues(const WDL_TypedBuf<BL_FLOAT> &values)
{
    if (values.GetSize() != mData.GetSize())
        return;
    
    for (int i = 0; i < values.GetSize(); i++)
    {
        BL_FLOAT val = values.Get()[i];
        
        AddValue(i, val);
    }
}

void
SmoothAvgHistogramDB::GetValues(WDL_TypedBuf<BL_FLOAT> *values)
{
    values->Resize(mData.GetSize());
    
    for (int i = 0; i < mData.GetSize(); i++)
    {
        BL_FLOAT val = mData.Get()[i];
        
        val = BLUtils::NormalizedYTodBInv(val, mMindB, mMaxdB);
        
        values->Get()[i] = val;
    }
}

void
SmoothAvgHistogramDB::SetValues(const WDL_TypedBuf<BL_FLOAT> *values)
{
    mData = *values;
}

void
SmoothAvgHistogramDB::Reset()
{
    for (int i = 0; i < mData.GetSize(); i++)
        mData.Get()[i] = mDefaultValue;
}
