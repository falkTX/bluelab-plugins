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
//  CMAParamSmooth.cpp
//  UST
//
//  Created by applematuer on 2/29/20.
//
//

#include "CMAParamSmooth.h"

CMAParamSmooth::CMAParamSmooth(BL_FLOAT smoothingTimeMs, BL_FLOAT samplingRate)
{
    mSmoothingTimeMs = smoothingTimeMs;
    mSampleRate = samplingRate;
    
    mWindowSize = smoothingTimeMs*0.001*samplingRate;
    
    mCurrentAvg = 0.0;
    
    Reset(samplingRate);
}

CMAParamSmooth::~CMAParamSmooth() {}

void
CMAParamSmooth::Reset(BL_FLOAT samplingRate)
{
    Reset(samplingRate, 0.0);
}

void
CMAParamSmooth::Reset(BL_FLOAT samplingRate, BL_FLOAT val)
{
    mSampleRate = samplingRate;
    mWindowSize = mSmoothingTimeMs*0.001*samplingRate;
    
    mCurrentAvg = val;
    
    mPrevValues.clear();
    
    // TEST
    Process(val);
}

void
CMAParamSmooth::SetSmoothTimeMs(BL_FLOAT smoothingTimeMs)
{
    mSmoothingTimeMs = smoothingTimeMs;
    
    Reset(mSampleRate);
}

BL_FLOAT
CMAParamSmooth::Process(BL_FLOAT inVal)
{
    if (mPrevValues.empty())
    {
        for (int i = 0; i < mWindowSize; i++)
            mPrevValues.push_back(inVal);
        
        return inVal;
    }
    
    mPrevValues.push_back(inVal);
    
    //if (mPrevValues.size() == 1)
    //    return inVal;
    
    if (!mPrevValues.empty())
    {
        BL_FLOAT firstValue = mPrevValues[0];
        mCurrentAvg += (1.0/mPrevValues.size())*(inVal - firstValue);
        
        if (mPrevValues.size() >= mWindowSize)
        {
            mPrevValues.pop_front();
        }
    }
    
    return mCurrentAvg;
}
