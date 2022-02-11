//
//  DUETHistogram2.cpp
//  BL-DUET
//
//  Created by applematuer on 5/3/20.
//
//

#include <BLUtils.h>

#include "DUETHistogram2.h"


// BAD: filling with 1 is too high, compared to the values used for filling after
//
// By default fill with 1 instead of 0
// So when we smooth, the first values will be considered as dry signal,
// and ot reverb signal (as it should be if we fill with 0)
#define DEFAULT_VALUE_ONE 0 //1

// WON'T WORK...
// For the first value, do not interpolate
#define FORCE_FIRST_VALUE 0 //1


DUETHistogram2::DUETHistogram2(int width, int height, BL_FLOAT maxValue)
{
    mWidth = width;
    mHeight = height;
    
    mMaxValue = maxValue;
    
    mData.Resize(mWidth*mHeight);
    mIndices.resize(mWidth*mHeight);
    
    mSmoothFactor = 0.0;
    
    Clear();
}

DUETHistogram2::~DUETHistogram2() {}

void
DUETHistogram2::Reset()
{
    mPrevData.Resize(0);
}

void
DUETHistogram2::Reset(int width, int height, BL_FLOAT maxValue)
{
    mWidth = width;
    mHeight = height;
    
    mMaxValue = maxValue;
    
#if !FORCE_FIRST_VALUE
    mData.Resize(mWidth*mHeight);
    mIndices.resize(mWidth*mHeight);
#else
    mData.Resize(0);
    mIndices.resize(0);
#endif
    
    Clear();
    
    Reset();
}


void
DUETHistogram2::Clear()
{
#if !FORCE_FIRST_VALUE
    
#if !DEFAULT_VALUE_ONE
    BLUtils::FillAllZero(&mData);
#else
    BLUtils::FillAllValue(&mData, 1.0);
#endif
    
    for (int i = 0; i < mIndices.size(); i++)
    {
        mIndices[i].clear();
    }
#else
    mData.Resize(0);
    mIndices.resize(0);
#endif
}

void
DUETHistogram2::AddValue(BL_FLOAT u, BL_FLOAT v, BL_FLOAT value,
                        int sampleIndex)
{
    int x = u*(mWidth - 1);
    int y = v*(mHeight - 1);
    
    if ((x >= 0) && (x < mWidth) &&
        (y >= 0) && (y < mHeight))
    {
        int index = x + y*mWidth;
        mData.Get()[index] += value;
        
        if (sampleIndex != -1)
        {
            mIndices[index].push_back(sampleIndex);
        }
    }
}

void
DUETHistogram2::Process()
{
    if (mPrevData.GetSize() != mData.GetSize())
        mPrevData = mData;
    
    BLUtils::Smooth(&mData, &mPrevData, mSmoothFactor);
}

int
DUETHistogram2::GetWidth()
{
    return mWidth;
}

int
DUETHistogram2::GetHeight()
{
    return mHeight;
}

void
DUETHistogram2::GetData(WDL_TypedBuf<BL_FLOAT> *data)
{
    *data = mData;
    
    BL_FLOAT coeff = 1.0/mMaxValue;
    BLUtils::MultValues(data, coeff);
    
    BLUtils::ClipMax(data, (BL_FLOAT)1.0);
}

void
DUETHistogram2::GetIndices(int histoIndex, vector<int> *indices)
{
    indices->resize(0);
    
    if (histoIndex < mIndices.size())
    {
        *indices = mIndices[histoIndex];
    }
}

void
DUETHistogram2::SetTimeSmooth(BL_FLOAT smoothFactor)
{
    mSmoothFactor = smoothFactor;
}
