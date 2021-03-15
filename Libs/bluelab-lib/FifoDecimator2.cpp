//
//  FifoDecimator2.cpp
//  BL-TransientShaper
//
//  Created by Pan on 11/04/18.
//
//

#include <BLUtils.h>
#include <BLUtilsDecim.h>

#include "FifoDecimator2.h"

FifoDecimator2::FifoDecimator2(long maxSize,
                             BL_FLOAT decimFactor,
                             bool isSamples)
{
    mMaxSize = maxSize;
    mDecimFactor = decimFactor;
    mIsSamples = isSamples;
    
    //mValues.Resize(mMaxSize);
    mValues.Add(0, mMaxSize);
    
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
    //BLUtils::FillAllZero(&mValues);

    WDL_TypedBuf<BL_FLOAT> &zeros = mTmpBuf0;
    zeros.Resize(mValues.Available());
    BLUtils::FillAllZero(&zeros);

    mValues.SetFromBuf(0, zeros.Get(), zeros.GetSize());
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
    //BLUtils::AppendValues(&mValues, values);
    mValues.Add(values.Get(), values.GetSize());
    
    //int numValuesConsume = mValues.GetSize() - mMaxSize;
    int numValuesConsume = mValues.Available() - mMaxSize;
    if (numValuesConsume > 0)
    {
        //BLUtils::ConsumeLeft(&mValues, numValuesConsume);
        mValues.Advance(numValuesConsume);
    }
}

void
FifoDecimator2::GetValues(WDL_TypedBuf<BL_FLOAT> *values)
{
    WDL_TypedBuf<BL_FLOAT> &buf = mTmpBuf1;
    buf.Resize(mValues.Available());
    mValues.GetToBuf(0, buf.Get(), buf.GetSize());
                     
    if (mIsSamples)
        BLUtilsDecim::DecimateSamples(values, buf/*mValues*/, mDecimFactor);
        //BLUtils::DecimateSamplesFast(values, mValues, mDecimFactor);
    else
        BLUtilsDecim::DecimateValues(values, buf/*mValues*/, mDecimFactor);
}
