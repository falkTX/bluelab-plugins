//
//  AvgHistogram2.cpp
//  EQHack
//
//  Created by Apple m'a Tuer on 10/09/17.
//
//

#include <ParamSmoother2.h>

#include "SmoothAvgHistogram2.h"

//#define RESET_VALUE -1e16


SmoothAvgHistogram2::SmoothAvgHistogram2(BL_FLOAT sampleRate, int size,
                                         BL_FLOAT smoothTimeMs,
                                         BL_FLOAT defaultValue)
{
    mData.Resize(size);

    mSampleRate = sampleRate;
    mSmoothTimeMs = smoothTimeMs;
    mSmoothCoeff = ParamSmoother2::ComputeSmoothFactor(mSmoothTimeMs, sampleRate);
    
    mDefaultValue = defaultValue;
    
    Reset(mSampleRate);
}

SmoothAvgHistogram2::~SmoothAvgHistogram2() {}

void
SmoothAvgHistogram2::AddValue(int index, BL_FLOAT val)
{
    //if (mData.Get()[index] < RESET_VALUE/2.0)
    //    // Initialize
    //    mData.Get()[index] = val;
    //else
    
    BL_FLOAT newVal = (1.0 - mSmoothCoeff) * val + mSmoothCoeff*mData.Get()[index];
    mData.Get()[index] = newVal;
}

void
SmoothAvgHistogram2::AddValues(const WDL_TypedBuf<BL_FLOAT> &values)
{
    if (values.GetSize() != mData.GetSize())
        return;

    int valuesSize = values.GetSize();
    const BL_FLOAT *valuesData = values.Get();
    BL_FLOAT *data = mData.Get();
    
    //for (int i = 0; i < values.GetSize(); i++)
    for (int i = 0; i < valuesSize; i++)
    {
        //BL_FLOAT val = values.Get()[i];
        BL_FLOAT val = valuesData[i];
        
        //if (mData.Get()[i] < RESET_VALUE / 2)
        //    // Initialize
        //    mData.Get()[i] = val;
        //else
        
        BL_FLOAT newVal = (1.0 - mSmoothCoeff) * val + mSmoothCoeff*mData.Get()[i];
        //mData.Get()[i] = newVal;
        data[i] = newVal;
    }
}

void
SmoothAvgHistogram2::GetValues(WDL_TypedBuf<BL_FLOAT> *values)
{
    values->Resize(mData.GetSize());

    int dataSize = mData.GetSize();
    BL_FLOAT *valuesData = values->Get();
    
    //for (int i = 0; i < mData.GetSize(); i++)
    for (int i = 0; i < dataSize; i++)
    {
        //BL_FLOAT val = 0.0;
        
        //if (mData.Get()[i] > RESET_VALUE/2)
        BL_FLOAT val = mData.Get()[i];
        
        //values->Get()[i] = val;
        valuesData[i] = val;
    }
}

void
SmoothAvgHistogram2::Reset(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    mSmoothCoeff = ParamSmoother2::ComputeSmoothFactor(mSmoothTimeMs, sampleRate);
    
    for (int i = 0; i < mData.GetSize(); i++)
        mData.Get()[i] = mDefaultValue;
}

void
SmoothAvgHistogram2::Reset(BL_FLOAT sampleRate, const WDL_TypedBuf<BL_FLOAT> &values)
{
    mSampleRate = sampleRate;
    mSmoothCoeff = ParamSmoother2::ComputeSmoothFactor(mSmoothTimeMs, sampleRate);
    
    if (values.GetSize() < mData.GetSize())
        return;
    
    for (int i = 0; i < mData.GetSize(); i++)
        mData.Get()[i] = values.Get()[i];
}

void
SmoothAvgHistogram2::Resize(int newSize)
{
    mData.Resize(newSize);
    
    Reset(mSampleRate);
}
