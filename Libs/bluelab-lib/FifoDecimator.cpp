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
//  FifoDecimator.cpp
//  BL-TransientShaper
//
//  Created by Pan on 11/04/18.
//
//

#include <BLUtils.h>
#include <BLUtilsDecim.h>

#include "FifoDecimator.h"

FifoDecimator::FifoDecimator(long maxSize,
                             BL_FLOAT decimFactor,
                             bool isSamples)
{
    mMaxSize = maxSize;
    mDecimFactor = decimFactor;
    mIsSamples = isSamples;
    
    mValues.Resize(mMaxSize);
    
    Reset();
}

FifoDecimator::FifoDecimator(bool isSamples)
{
    mIsSamples = isSamples;
    
    mMaxSize = 0.0;
    mDecimFactor = 1.0;
}

FifoDecimator::~FifoDecimator() {}

void
FifoDecimator::Reset()
{
    BLUtils::FillAllZero(&mValues);
}

void
FifoDecimator::SetParams(long maxSize, BL_FLOAT decimFactor)
{
    mMaxSize = maxSize;
    mDecimFactor = decimFactor;
}

void
FifoDecimator::SetParams(long maxSize, BL_FLOAT decimFactor, bool isSamples)
{
    mMaxSize = maxSize;
    mDecimFactor = decimFactor;
    
    mIsSamples = isSamples;
}

void
FifoDecimator::AddValues(const WDL_TypedBuf<BL_FLOAT> &values)
{
    WDL_TypedBuf<BL_FLOAT> decimValues;
    
    if (mIsSamples)
        BLUtilsDecim::DecimateSamples(&decimValues, values, mDecimFactor);
    else
        BLUtilsDecim::DecimateValues(&decimValues, values, mDecimFactor);
    
    BLUtils::AppendValues(&mValues, decimValues);
    
    int numValuesConsume = mValues.GetSize() - mMaxSize;
    if (numValuesConsume > 0)
    {
        BLUtils::ConsumeLeft(&mValues, numValuesConsume);
    }
}

void
FifoDecimator::GetValues(WDL_TypedBuf<BL_FLOAT> *values)
{
    *values = mValues;
}
