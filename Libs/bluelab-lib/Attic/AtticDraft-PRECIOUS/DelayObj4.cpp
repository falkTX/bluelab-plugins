//
//  DelayObj4.cpp
//  BL-Precedence
//
//  Created by applematuer on 3/12/19.
//
//

#include <Utils.h>

#include "DelayObj4.h"

// When setting the delay to 0, then increasing,
// there is often a click at the beginning
#define FIX_CLICK_FROM_ZERO 1

DelayObj4::DelayObj4(double delay)
{
    mReadPos = 0;
    
    int delayI = ceil(delay);
    mDelayLine.Resize(delayI);
    Utils::FillAllZero(&mDelayLine);
   
    mWritePos = mDelayLine.GetSize() - 1;
    if (mWritePos < 0)
        mWritePos = 0;
    
    SetDelay(delay);
}

DelayObj4::~DelayObj4() {}

void
DelayObj4::Reset()
{
    Utils::FillAllZero(&mDelayLine);
    
    mReadPos = 0;
    mWritePos = mDelayLine.GetSize() - 1;
    if (mWritePos < 0)
        mWritePos = 0;
}

void
DelayObj4::SetDelay(double delay)
{
    mDelay = delay;
 
    int delayI = ceil(delay);
    
#if FIX_CLICK_FROM_ZERO
    if (delayI == 0)
        delayI = 1;
#endif
    
    double lastValue = 0.0;
    if (mDelayLine.GetSize() > 0)
        lastValue = mDelayLine.Get()[mWritePos];
    
    int delayLinePrevSize = mDelayLine.GetSize();
    if (delayI > delayLinePrevSize)
    {
        Utils::InsertValues(&mDelayLine, mWritePos,
                            delayI - delayLinePrevSize, lastValue);
        
#if 1
        if (mReadPos > mWritePos)
            mReadPos = (mReadPos + delayI - delayLinePrevSize) % mDelayLine.GetSize();
        
        //STRANGE (should not be commented)
        //mWritePos = (mWritePos + delayI - delayLinePrevSize) % mDelayLine.GetSize();
#endif
    }
    
    if (delayI < delayLinePrevSize)
    {
        Utils::RemoveValuesCyclic2(&mDelayLine,
                                   mWritePos - (delayLinePrevSize - delayI),
                                   delayLinePrevSize - delayI);
        
#if 1
        if (mReadPos > mWritePos)
        {
            mReadPos -= delayLinePrevSize - delayI;
            if (mReadPos < 0)
                mReadPos += mDelayLine.GetSize();
            mReadPos = mReadPos % mDelayLine.GetSize();
        }
        
        mWritePos -= delayLinePrevSize - delayI;
        if (mWritePos < 0)
            mWritePos += mDelayLine.GetSize();
        mWritePos = mWritePos % mDelayLine.GetSize();
#endif
    }
    
    // Fill new rooms with last value
    //Utils::ResizeFillValue2(&mDelayLine, delayI, lastValue);
}

double
DelayObj4::ProcessSample(double sample)
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
    double result0 = mDelayLine.Get()[mReadPos];
    double result1 = mDelayLine.Get()[(mReadPos + 1) % mDelayLine.GetSize()];
    
    double t = mDelay - (int)mDelay;
    
    double result = (1.0 - t)*result0 + t*result1;
    
    // Write
    mDelayLine.Get()[mWritePos] = sample;
    
    mReadPos = (mReadPos + 1) % mDelayLine.GetSize();
    mWritePos = (mWritePos + 1) % mDelayLine.GetSize();
    
    return result;
}
