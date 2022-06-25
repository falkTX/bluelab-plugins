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
//  DelayObj4.cpp
//  BL-Precedence
//
//  Created by applematuer on 3/12/19.
//
//

#include <BLUtils.h>

#include "DelayObj4.h"

// When setting the delay to 0, then increasing,
// there is often a click at the beginning
#define FIX_CLICK_FROM_ZERO 1

// For UST
#define OPTIM_MODULUS 1

DelayObj4::DelayObj4(BL_FLOAT delay)
{
    mReadPos = 0;
    
    int delayI = ceil(delay);
    mDelayLine.Resize(delayI);
    BLUtils::FillAllZero(&mDelayLine);
   
    mWritePos = mDelayLine.GetSize() - 1;
    if (mWritePos < 0)
        mWritePos = 0;

    SetDelay(delay);
}

DelayObj4::DelayObj4(const DelayObj4 &other)
{
    BL_FLOAT delay = other.mDelay;
    
    mReadPos = 0;
    
    int delayI = ceil(delay);
    mDelayLine.Resize(delayI);
    BLUtils::FillAllZero(&mDelayLine);
    
    mWritePos = mDelayLine.GetSize() - 1;
    if (mWritePos < 0)
        mWritePos = 0;
    
    SetDelay(delay);
}

DelayObj4::~DelayObj4() {}

void
DelayObj4::Reset()
{
    BLUtils::FillAllZero(&mDelayLine);
    
    mReadPos = 0;
    mWritePos = mDelayLine.GetSize() - 1;
    if (mWritePos < 0)
        mWritePos = 0;
}

void
DelayObj4::SetDelay(BL_FLOAT delay)
{
    mDelay = delay;
 
    int delayI = ceil(delay);
    
#if FIX_CLICK_FROM_ZERO
    if (delayI == 0)
        delayI = 1;
#endif
    
    BL_FLOAT lastValue = 0.0;
    if (mDelayLine.GetSize() > 0)
    {
        // Must take the value of mWritePos - 1
        // this is the last written value
        //
        // If we took mWritePos, that made problems,
        // because this is the position of the future value to be written.
        int writeIndex = mWritePos - 1;
        if (writeIndex < 0)
            writeIndex += mDelayLine.GetSize();
        writeIndex = writeIndex % mDelayLine.GetSize();
        
        lastValue = mDelayLine.Get()[writeIndex];
    }
    
    int delayLinePrevSize = mDelayLine.GetSize();
    if (delayI > delayLinePrevSize)
    {
        // Insert values
        int numToInsert = delayI - delayLinePrevSize;
        BLUtils::InsertValues(&mDelayLine, mWritePos,
                            numToInsert, lastValue);
        
        // Adjust read and write positions
        if (mReadPos > mWritePos)
            mReadPos = (mReadPos + numToInsert) % mDelayLine.GetSize();
        
        mWritePos = (mWritePos + numToInsert) % mDelayLine.GetSize();
    }
    
    if (delayI < delayLinePrevSize)
    {
        // Remove values
        int numToRemove = delayLinePrevSize - delayI;
        BLUtils::RemoveValuesCyclic2(&mDelayLine, mWritePos, numToRemove);
        
        // Adjust the read an write positions
        if (mReadPos > mWritePos)
        {
            mReadPos -= numToRemove;
            if (mReadPos < 0)
                mReadPos += mDelayLine.GetSize();
            mReadPos = mReadPos % mDelayLine.GetSize();
        }
        
        mWritePos -= numToRemove;
        if (mWritePos < 0)
            mWritePos += mDelayLine.GetSize();
        mWritePos = mWritePos % mDelayLine.GetSize();
    }
}

#if !OPTIM_MODULUS
BL_FLOAT
DelayObj4::ProcessSample(BL_FLOAT sample)
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
    
    // Read by interpolating, to manage non integer delays
    // GOOD: efficient when using a ParamSmoother for the delay parameter
    BL_FLOAT result0 = mDelayLine.Get()[mReadPos];
    BL_FLOAT result1 = mDelayLine.Get()[(mReadPos + 1) % mDelayLine.GetSize()];
    
    BL_FLOAT t = mDelay - (int)mDelay;
    
    BL_FLOAT result = (1.0 - t)*result0 + t*result1;
    
    // Write
    mDelayLine.Get()[mWritePos] = sample;
    
    mReadPos = (mReadPos + 1) % mDelayLine.GetSize();
    mWritePos = (mWritePos + 1) % mDelayLine.GetSize();
    
    return result;
}
#endif

#if OPTIM_MODULUS
BL_FLOAT
DelayObj4::ProcessSample(BL_FLOAT sample)
{
    int delaySize = mDelayLine.GetSize();
    if (delaySize == 0)
        return sample;
    
    BL_FLOAT *delayBuf = mDelayLine.Get();
    
    // Read
    if (delaySize == 1)
    {
#if FIX_CLICK_FROM_ZERO
        delayBuf[delaySize - 1] = sample;
#endif
        
        return sample;
    }
    
    // Read by interpolating, to manage non integer delays
    // GOOD: efficient when using a ParamSmoother for the delay parameter
    BL_FLOAT result0 = delayBuf[mReadPos];
    
    mReadPos++;
    if (mReadPos >= delaySize)
        mReadPos -= delaySize;
    
    BL_FLOAT result1 = delayBuf[mReadPos];
    
    BL_FLOAT t = mDelay - (int)mDelay;
    
    BL_FLOAT result = (1.0 - t)*result0 + t*result1;
    
    // Write
    delayBuf[mWritePos] = sample;
    
    mWritePos++;
    if (mWritePos >= delaySize)
        mWritePos -= delaySize;
    
    return result;
}
#endif

void
DelayObj4::ProcessSamples(WDL_TypedBuf<BL_FLOAT> *samples)
{
    for (int i = 0; i < samples->GetSize(); i++)
    {
        BL_FLOAT samp0 = samples->Get()[i];
        BL_FLOAT samp1 = ProcessSample(samp0);
        samples->Get()[i] = samp1;
    }
}
