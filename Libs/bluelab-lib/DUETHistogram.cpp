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
//  DUETHistogram.cpp
//  BL-DUET
//
//  Created by applematuer on 5/3/20.
//
//

#include <BLUtils.h>

#include "DUETHistogram.h"

DUETHistogram::DUETHistogram(int width, int height, BL_FLOAT maxValue)
{
    mWidth = width;
    mHeight = height;
    
    mMaxValue = maxValue;
    
    mData.Resize(mWidth*mHeight);
    //mPrevData.Resize(mWidth*mHeight);
    mIndices.resize(mWidth*mHeight);
    
    mSmoothFactor = 0.0;
    
    Clear();
}

DUETHistogram::~DUETHistogram() {}

void
DUETHistogram::Reset()
{
    mPrevData.Resize(0);
}

void
DUETHistogram::Clear()
{
    BLUtils::FillAllZero(&mData);
    
    //BLUtils::FillAllZero(&mPrevData);
    //mPrevData.Resize(0);
    
    for (int i = 0; i < mIndices.size(); i++)
    {
        mIndices[i].clear();
    }
}

void
DUETHistogram::AddValue(BL_FLOAT u, BL_FLOAT v, BL_FLOAT value,
                        int sampleIndex)
{
    int x = u*(mWidth - 1);
    int y = v*(mHeight - 1);
    
    if ((x >= 0) && (x < mWidth) &&
        (y >= 0) && (y < mHeight))
    {
        mData.Get()[x + y*mWidth] += value;
        
        if (sampleIndex != -1)
        {
            mIndices[x + y*mWidth].push_back(sampleIndex);
        }
    }
}

void
DUETHistogram::Process()
{
    if (mPrevData.GetSize() != mData.GetSize())
        mPrevData = mData;
    
    BLUtils::Smooth(&mData, &mPrevData, mSmoothFactor);
}

int
DUETHistogram::GetWidth()
{
    return mWidth;
}

int
DUETHistogram::GetHeight()
{
    return mHeight;
}

void
DUETHistogram::GetData(WDL_TypedBuf<BL_FLOAT> *data)
{
    *data = mData;
    
    BL_FLOAT coeff = 1.0/mMaxValue;
    BLUtils::MultValues(data, coeff);
    
    BLUtils::ClipMax(data, (BL_FLOAT)1.0);
}

void
DUETHistogram::GetIndices(int histoIndex, vector<int> *indices)
{
    indices->resize(0);
    
    if (histoIndex < mIndices.size())
    {
        *indices = mIndices[histoIndex];
    }
}

void
DUETHistogram::SetTimeSmooth(BL_FLOAT smoothFactor)
{
    mSmoothFactor = smoothFactor;
}
