//
//  AvgHistogram.cpp
//  EQHack
//
//  Created by Apple m'a Tuer on 10/09/17.
//
//

#include <BLUtils.h>
#include <BLDebug.h>

#include "SmoothAvgHistogramDB.h"


SmoothAvgHistogramDB::SmoothAvgHistogramDB(int size, BL_FLOAT smoothCoeff,
                                           BL_FLOAT defaultValue,
                                           BL_FLOAT mindB, BL_FLOAT maxdB)
{
    mData.Resize(size);
    
    mSmoothCoeff = smoothCoeff;
    
    mMindB = mindB;
    mMaxdB = maxdB;

#if 0 // Origin.
      // For Air, after reset this makes the curve "fall from the top" 
    BL_FLOAT defaultValueDB = BLUtils::NormalizedYTodB(defaultValue, mMindB, mMaxdB);
    mDefaultValue = defaultValueDB;
#endif

#if 1 // For Air e.g, so the default value is 0 and not > 1
      // For Air, this makes the curve raise from the bottom (which is what we want)

    // NOTE: defaultValue, minDB and maxDB are already in DB
    // so no need to call AmpToDB!
    //
    // NOTE: mData is in "normalized DB" i.e db scale, but normalzied inside [0, 1]
    //
    mDefaultValue = (defaultValue - mMindB)/(mMaxdB - mMindB);
#endif
    
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

#if 0 // Origin version
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
#endif

#if 1 // Optimized version
void
SmoothAvgHistogramDB::AddValues(const WDL_TypedBuf<BL_FLOAT> &values)
{
    if (values.GetSize() != mData.GetSize())
        return;

    WDL_TypedBuf<BL_FLOAT> &normY = mTmpBuf0;
    normY.Resize(values.GetSize());

    BLUtils::NormalizedYTodB(values, mMindB, mMaxdB, &normY);
    
    for (int i = 0; i < values.GetSize(); i++)
    {
        BL_FLOAT valDB = normY.Get()[i];
    
        BL_FLOAT newVal =
            (1.0 - mSmoothCoeff) * valDB + mSmoothCoeff*mData.Get()[i];
    
        mData.Get()[i] = newVal;
    }
}
#endif

int
SmoothAvgHistogramDB::GetNumValues()
{
    return mData.GetSize();
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
SmoothAvgHistogramDB::GetValuesDB(WDL_TypedBuf<BL_FLOAT> *values)
{
    values->Resize(mData.GetSize());
    
    for (int i = 0; i < mData.GetSize(); i++)
    {
        BL_FLOAT val = mData.Get()[i];
        
        values->Get()[i] = val;
    }
}

void
SmoothAvgHistogramDB::SetValues(const WDL_TypedBuf<BL_FLOAT> *values,
                                bool convertToDB)
{
    if (!convertToDB)
        mData = *values;
    else
        BLUtils::NormalizedYTodB(*values, mMindB, mMaxdB, &mData);
}

void
SmoothAvgHistogramDB::Reset(BL_FLOAT smoothCoeff)
{
    if (smoothCoeff > 0.0)
        mSmoothCoeff = smoothCoeff;
            
    for (int i = 0; i < mData.GetSize(); i++)
        mData.Get()[i] = mDefaultValue;
}
