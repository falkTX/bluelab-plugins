//
//  AvgHistogram.cpp
//  EQHack
//
//  Created by Apple m'a Tuer on 10/09/17.
//
//

#include "SmoothAvgHistogram.h"

//#define RESET_VALUE -1e16


SmoothAvgHistogram::SmoothAvgHistogram(int size, double smoothCoeff, double defaultValue)
{
    mData.Resize(size);
    
    mSmoothCoeff = smoothCoeff;
    
    mDefaultValue = defaultValue;
    
    Reset();
}

SmoothAvgHistogram::~SmoothAvgHistogram() {}

void
SmoothAvgHistogram::AddValue(int index, double val)
{
    //if (mData.Get()[index] < RESET_VALUE/2.0)
    //    // Initialize
    //    mData.Get()[index] = val;
    //else
    
    double newVal = (1.0 - mSmoothCoeff) * val + mSmoothCoeff*mData.Get()[index];
    mData.Get()[index] = newVal;
}

void
SmoothAvgHistogram::AddValues(WDL_TypedBuf<double> *values)
{
    if (values->GetSize() > mData.GetSize())
        return;
    
    for (int i = 0; i < values->GetSize(); i++)
    {
        double val = values->Get()[i];
        
        //if (mData.Get()[i] < RESET_VALUE / 2)
        //    // Initialize
        //    mData.Get()[i] = val;
        //else
        
        double newVal = (1.0 - mSmoothCoeff) * val + mSmoothCoeff*mData.Get()[i];
        mData.Get()[i] = newVal;
    }
}

void
SmoothAvgHistogram::GetValues(WDL_TypedBuf<double> *values)
{
    values->Resize(mData.GetSize());
    
    for (int i = 0; i < mData.GetSize(); i++)
    {
        //double val = 0.0;
        
        //if (mData.Get()[i] > RESET_VALUE/2)
        double val = mData.Get()[i];
        
        values->Get()[i] = val;
    }
}

void
SmoothAvgHistogram::Reset()
{
    for (int i = 0; i < mData.GetSize(); i++)
        mData.Get()[i] = mDefaultValue;
}
