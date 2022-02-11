//
//  FifoDecimator.cpp
//  BL-TransientShaper
//
//  Created by Pan on 11/04/18.
//
//

#include <BLUtils.h>
#include "FifoDecimator.h"

FifoDecimator::FifoDecimator(long maxSize,
                             BL_FLOAT decimFactor,
                             bool isSamples)
{
    mMaxSize = maxSize;
    mDecimFactor = decimFactor;
    mIsSamples = isSamples;
    
    mValues.Resize(mMaxSize);
    
    Reset();
}

FifoDecimator::FifoDecimator(bool isSamples)
{
    mIsSamples = isSamples;
    
    mMaxSize = 0.0;
    mDecimFactor = 1.0;
}

FifoDecimator::~FifoDecimator() {}

void
FifoDecimator::Reset()
{
    BLUtils::FillAllZero(&mValues);
}

void
FifoDecimator::SetParams(long maxSize, BL_FLOAT decimFactor)
{
    mMaxSize = maxSize;
    mDecimFactor = decimFactor;
}

void
FifoDecimator::SetParams(long maxSize, BL_FLOAT decimFactor, bool isSamples)
{
    mMaxSize = maxSize;
    mDecimFactor = decimFactor;
    
    mIsSamples = isSamples;
}

void
FifoDecimator::AddValues(const WDL_TypedBuf<BL_FLOAT> &values)
{
    WDL_TypedBuf<BL_FLOAT> decimValues;
    
    if (mIsSamples)
        BLUtils::DecimateSamples(&decimValues, values, mDecimFactor);
    else
        BLUtils::DecimateValues(&decimValues, values, mDecimFactor);
    
    BLUtils::AppendValues(&mValues, decimValues);
    
    int numValuesConsume = mValues.GetSize() - mMaxSize;
    if (numValuesConsume > 0)
    {
        BLUtils::ConsumeLeft(&mValues, numValuesConsume);
    }
}

void
FifoDecimator::GetValues(WDL_TypedBuf<BL_FLOAT> *values)
{
    *values = mValues;
}
