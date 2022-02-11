//
//  DelayObj.cpp
//  BL-Precedence
//
//  Created by applematuer on 3/12/19.
//
//

#include <BLUtils.h>

#include "DelayObj2.h"


DelayObj2::DelayObj2(int delay)
{
    SetDelay(delay);
    
    mWriteAddress = 0;
}

DelayObj2::~DelayObj2() {}

void
DelayObj2::Reset()
{
    BLUtils::FillAllZero(&mDelayLine);
    
    mWriteAddress = 0;
}

void
DelayObj2::SetDelay(int delay)
{
    int prevDelay = mDelay;
    mDelay = delay;
    
    if (prevDelay < mDelay)
        // Increase the line
    {
        // Add at tail
        int index = mWriteAddress - 1;
        if (index < 0)
            index += mDelayLine.GetSize();
        
        //
        if (index < 0)
            index = 0;
        
        BLUtils::InsertZeros(&mDelayLine, index, mDelay - prevDelay);
    }
    
    if (prevDelay > mDelay)
    // Decrease the line
    {
        // Remove oldest values
        int index = mWriteAddress;
        BLUtils::RemoveValuesCyclic(&mDelayLine, index, prevDelay - mDelay);
    }
}

BL_FLOAT
DelayObj2::ProcessSample(BL_FLOAT sample)
{
    if (mDelayLine.GetSize() == 0)
        return sample;
    
    //
    mWriteAddress = mWriteAddress % mDelay;
    
    // Read
    BL_FLOAT result = mDelayLine.Get()[mWriteAddress];
    
    // Write
    mDelayLine.Get()[mWriteAddress] = sample;
    
    // Increment
    mWriteAddress++;
    mWriteAddress = mWriteAddress % mDelay;
    
    return result;
}
