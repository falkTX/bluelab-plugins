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
