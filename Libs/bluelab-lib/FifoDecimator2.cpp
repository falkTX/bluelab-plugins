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
