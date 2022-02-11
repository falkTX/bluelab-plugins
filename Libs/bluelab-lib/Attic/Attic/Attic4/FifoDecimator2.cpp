//
//  FifoDecimator2.cpp
//  BL-TransientShaper
//
//  Created by Pan on 11/04/18.
//
//

#include <BLUtils.h>
#include "FifoDecimator2.h"

FifoDecimator2::FifoDecimator2(long maxSize,
                             BL_FLOAT decimFactor,
                             bool isSamples)
{
    mMaxSize = maxSize;
    mDecimFactor = decimFactor;
    mIsSamples = isSamples;
    
    mValues.Resize(mMaxSize);
    
    Reset();
}

FifoDecimator2::FifoDecimator2(bool isSamples)
{
    mIsSamples = isSamples;
    
    mMaxSize = 0.0;
    mDecimFactor = 1.0;
}

FifoDecimator2::~FifoDecimator2() {}

void
FifoDecimator2::Reset()
{
    BLUtils::FillAllZero(&mValues);
}

void
FifoDecimator2::SetParams(long maxSize, BL_FLOAT decimFactor)
{
    mMaxSize = maxSize;
    mDecimFactor = decimFactor;
}

void
FifoDecimator2::SetParams(long maxSize, BL_FLOAT decimFactor, bool isSamples)
{
    mMaxSize = maxSize;
    mDecimFactor = decimFactor;
    
    mIsSamples = isSamples;
}

void
FifoDecimator2::AddValues(const WDL_TypedBuf<BL_FLOAT> &values)
{
    BLUtils::AppendValues(&mValues, values);
    
    int numValuesConsume = mValues.GetSize() - mMaxSize;
    if (numValuesConsume > 0)
    {
        BLUtils::ConsumeLeft(&mValues, numValuesConsume);
    }
}

void
FifoDecimator2::GetValues(WDL_TypedBuf<BL_FLOAT> *values)
{
    if (mIsSamples)
        BLUtils::DecimateSamples(values, mValues, mDecimFactor);
        //BLUtils::DecimateSamplesFast(values, mValues, mDecimFactor);
    else
        BLUtils::DecimateValues(values, mValues, mDecimFactor);
}
