//
//  AvgHistogram.cpp
//  EQHack
//
//  Created by Apple m'a Tuer on 10/09/17.
//
//

#include "AvgHistogram.h"

AvgHistogram::AvgHistogram(int size)
{
    mData.Resize(size);
    mNumData.Resize(size);
    
    Reset();
}

AvgHistogram::~AvgHistogram() {}

void
AvgHistogram::AddValue(int index, BL_FLOAT val)
{
    mData.Get()[index] += val;
    mNumData.Get()[index] = mNumData.Get()[index] + 1;
}

void
AvgHistogram::AddValues(WDL_TypedBuf<BL_FLOAT> *values)
{
    if (values->GetSize() > mData.GetSize())
        return;
    
    for (int i = 0; i < values->GetSize(); i++)
    {
        mData.Get()[i] += values->Get()[i];
        mNumData.Get()[i] = mNumData.Get()[i] + 1;
    }
}

void
AvgHistogram::GetValues(WDL_TypedBuf<BL_FLOAT> *values)
{
    *values = mData;
    
    for (int i = 0; i < mData.GetSize(); i++)
    {
        int numData = mNumData.Get()[i];
        if (numData > 0)
            values->Get()[i] /= numData;
    }
}

void
AvgHistogram::Reset()
{
    for (int i = 0; i < mData.GetSize(); i++)
        mData.Get()[i] = 0.0;
    
    for (int i = 0; i < mNumData.GetSize(); i++)
        mNumData.Get()[i] = 0;
}
