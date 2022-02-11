//
//  FifoDecimator3.cpp
//  BL-TransientShaper
//
//  Created by Pan on 11/04/18.
//
//

#include <BLUtils.h>
#include <BLUtilsDecim.h>

#include "FifoDecimator3.h"

#define MAX_BUF_SIZE 8192

FifoDecimator3::FifoDecimator3(long maxSize,
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

FifoDecimator3::FifoDecimator3(bool isSamples)
{
    mIsSamples = isSamples;
    
    mMaxSize = 0.0;
    mDecimFactor = 1.0;
}

FifoDecimator3::~FifoDecimator3() {}

void
FifoDecimator3::Reset()
{
    WDL_TypedBuf<BL_FLOAT> &zeros = mTmpBuf0;
    zeros.Resize(mValues.Available());
    BLUtils::FillAllZero(&zeros);

    mValues.SetFromBuf(0, zeros.Get(), zeros.GetSize());
}

void
FifoDecimator3::SetParams(long maxSize, BL_FLOAT decimFactor)
{
    mMaxSize = maxSize;
    mDecimFactor = decimFactor;
}

void
FifoDecimator3::SetParams(long maxSize, BL_FLOAT decimFactor, bool isSamples)
{
    mMaxSize = maxSize;
    mDecimFactor = decimFactor;
    
    mIsSamples = isSamples;
}

void
FifoDecimator3::AddValues(const WDL_TypedBuf<BL_FLOAT> &values)
{    
    mValues.Add(values.Get(), values.GetSize());
    
    int numValuesConsume = mValues.Available() - MAX_BUF_SIZE;
    if (numValuesConsume > 0)
        mValues.Advance(numValuesConsume);
}

void
FifoDecimator3::GetValues(WDL_TypedBuf<BL_FLOAT> *values)
{
    if (mValues.Available() < mMaxSize)
    {
        values->Resize(mMaxSize);
        BLUtils::FillAllZero(values);

        return;
    }
    
    WDL_TypedBuf<BL_FLOAT> &buf = mTmpBuf1;
    buf.Resize(mMaxSize);
    mValues.GetToBuf(0, buf.Get(), buf.GetSize());

    mValues.Advance(mMaxSize);
    
    if (mIsSamples)
        BLUtilsDecim::DecimateSamples(values, buf, mDecimFactor);
    else
        BLUtilsDecim::DecimateValues(values, buf, mDecimFactor);
}
