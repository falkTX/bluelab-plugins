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

#include "SmoothAvgHistogram3.h"

SmoothAvgHistogram3::SmoothAvgHistogram3(BL_FLOAT sampleRate, int size,
                                         BL_FLOAT smoothTimeMs,
                                         BL_FLOAT defaultValue)
{
    mData.resize(size);

    mSampleRate = sampleRate;
    mSmoothTimeMs = smoothTimeMs;
    mSmoothCoeff =
        ParamSmoother2::ComputeSmoothFactor(mSmoothTimeMs, sampleRate);
    
    mDefaultValue = defaultValue;
    
    Reset(mSampleRate);
}

SmoothAvgHistogram3::~SmoothAvgHistogram3() {}

void
SmoothAvgHistogram3::AddValue(int index, BL_FLOAT val)
{
    BL_FLOAT newVal =
        (1.0 - mSmoothCoeff) * val + mSmoothCoeff*mData[index];
    mData[index] = newVal;
}

void
SmoothAvgHistogram3::AddValues(const vector<BL_FLOAT> &values)
{
    if (values.size() != mData.size())
        return;

    int valuesSize = values.size();
    for (int i = 0; i < valuesSize; i++)
    {
        BL_FLOAT val = values[i];
        
        BL_FLOAT newVal = (1.0 - mSmoothCoeff) * val +
            mSmoothCoeff*mData[i];
        
        mData[i] = newVal;
    }
}

void
SmoothAvgHistogram3::GetValues(vector<BL_FLOAT> *values)
{
    values->resize(mData.size());

    int dataSize = mData.size();    
    for (int i = 0; i < dataSize; i++)
    {
        BL_FLOAT val = mData[i];
        
        (*values)[i] = val;
    }
}

void
SmoothAvgHistogram3::Reset(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    mSmoothCoeff =
        ParamSmoother2::ComputeSmoothFactor(mSmoothTimeMs, sampleRate);
    
    for (int i = 0; i < mData.size(); i++)
        mData[i] = mDefaultValue;
}

void
SmoothAvgHistogram3::Reset(BL_FLOAT sampleRate,
                           const vector<BL_FLOAT> &values)
{
    mSampleRate = sampleRate;
    mSmoothCoeff =
        ParamSmoother2::ComputeSmoothFactor(mSmoothTimeMs, sampleRate);
    
    if (values.size() < mData.size())
        return;
    
    for (int i = 0; i < mData.size(); i++)
        mData[i] = values[i];
}

void
SmoothAvgHistogram3::Resize(int newSize)
{
    mData.resize(newSize);
    
    Reset(mSampleRate);
}

void
SmoothAvgHistogram3::SetSmoothTimeMs(BL_FLOAT smoothTimeMs,
                                     bool reset)
{
    mSmoothTimeMs = smoothTimeMs;
    mSmoothCoeff =
        ParamSmoother2::ComputeSmoothFactor(mSmoothTimeMs, mSampleRate);

    if (reset)
        Reset(mSampleRate);
}
