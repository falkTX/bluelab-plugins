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
//  AvgHistogram2.cpp
//  EQHack
//
//  Created by Apple m'a Tuer on 10/09/17.
//
//

#include <ParamSmoother2.h>

#include "SmoothAvgHistogram2.h"

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
    
    for (int i = 0; i < valuesSize; i++)
    {
        BL_FLOAT val = valuesData[i];
        
        BL_FLOAT newVal = (1.0 - mSmoothCoeff) * val + mSmoothCoeff*mData.Get()[i];
        
        data[i] = newVal;
    }
}

void
SmoothAvgHistogram2::GetValues(WDL_TypedBuf<BL_FLOAT> *values)
{
    values->Resize(mData.GetSize());

    int dataSize = mData.GetSize();
    BL_FLOAT *valuesData = values->Get();
    
    for (int i = 0; i < dataSize; i++)
    {
        BL_FLOAT val = mData.Get()[i];
        
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

void
SmoothAvgHistogram2::SetSmoothTimeMs(BL_FLOAT smoothTimeMs)
{
    mSmoothTimeMs = smoothTimeMs;
    mSmoothCoeff =
        ParamSmoother2::ComputeSmoothFactor(mSmoothTimeMs, mSampleRate);

    Reset(mSampleRate);
}
