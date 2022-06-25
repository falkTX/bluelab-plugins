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
//  DualDelayLine.cpp
//  BL-Bat
//
//  Created by applematuer on 12/14/19.
//
//

#include <BLUtils.h>

#include "DualDelayLine.h"

#define FIX_CLICK_FROM_ZERO 1


DualDelayLine::DualDelayLine(BL_FLOAT sampleRate, BL_FLOAT maxDelay, BL_FLOAT delay)
{
    mSampleRate = sampleRate;
    mMaxDelay = maxDelay;
    mDelay = delay;
    
    Init();
}

DualDelayLine::~DualDelayLine() {}

void
DualDelayLine::Reset(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    
    Init();
}

WDL_FFT_COMPLEX
DualDelayLine::ProcessSample(WDL_FFT_COMPLEX sample)
{
    if (mDelayLine.GetSize() == 0)
        return sample;
    
    // Read
    if (mDelayLine.GetSize() == 1)
    {
#if FIX_CLICK_FROM_ZERO
        mDelayLine.Get()[mDelayLine.GetSize() - 1] = sample;
#endif
        
        return sample;
    }
    
    WDL_FFT_COMPLEX result = mDelayLine.Get()[mReadPos];
    
    // Write
    mDelayLine.Get()[mWritePos] = sample;
    
    mReadPos = (mReadPos + 1) % mDelayLine.GetSize();
    mWritePos = (mWritePos + 1) % mDelayLine.GetSize();
    
    return result;
}

void
DualDelayLine::Init()
{
    mReadPos = 0;
    
    BL_FLOAT delay = mDelay + mMaxDelay/2.0;
    
    if (delay < 0.0)
    {
        // Error
    }
    
    long delaySamples = delay*mSampleRate;
    
    mDelayLine.Resize(delaySamples);
    BLUtils::FillAllZero(&mDelayLine);
    
    mWritePos = mDelayLine.GetSize() - 1;
    if (mWritePos < 0)
        mWritePos = 0;
}
