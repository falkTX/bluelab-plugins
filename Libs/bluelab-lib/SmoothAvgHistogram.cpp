/* Copyright (C) 2022 Nicolas Dittlo <deadlab.plugins@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this software; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
 
//
//  AvgHistogram.cpp
//  EQHack
//
//  Created by Apple m'a Tuer on 10/09/17.
//
//

#include "SmoothAvgHistogram.h"

//#define RESET_VALUE -1e16


SmoothAvgHistogram::SmoothAvgHistogram(int size, BL_FLOAT smoothCoeff, BL_FLOAT defaultValue)
{
    mData.Resize(size);
    
    mSmoothCoeff = smoothCoeff;
    
    mDefaultValue = defaultValue;
    
    Reset();
}

SmoothAvgHistogram::~SmoothAvgHistogram() {}

void
SmoothAvgHistogram::AddValue(int index, BL_FLOAT val)
{
    //if (mData.Get()[index] < RESET_VALUE/2.0)
    //    // Initialize
    //    mData.Get()[index] = val;
    //else
    
    BL_FLOAT newVal = (1.0 - mSmoothCoeff) * val + mSmoothCoeff*mData.Get()[index];
    mData.Get()[index] = newVal;
}

void
SmoothAvgHistogram::AddValues(const WDL_TypedBuf<BL_FLOAT> &values)
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
SmoothAvgHistogram::GetValues(WDL_TypedBuf<BL_FLOAT> *values)
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
SmoothAvgHistogram::Reset()
{
    for (int i = 0; i < mData.GetSize(); i++)
        mData.Get()[i] = mDefaultValue;
}

void
SmoothAvgHistogram::Reset(const WDL_TypedBuf<BL_FLOAT> &values)
{
    if (values.GetSize() < mData.GetSize())
        return;
    
    for (int i = 0; i < mData.GetSize(); i++)
        mData.Get()[i] = values.Get()[i];
}

void
SmoothAvgHistogram::Resize(int newSize)
{
    mData.Resize(newSize);
    
    Reset();
}
