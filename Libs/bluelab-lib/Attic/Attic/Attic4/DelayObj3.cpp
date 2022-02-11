//
//  DelayObj3.cpp
//  BL-Precedence
//
//  Created by applematuer on 3/12/19.
//
//

#include <BLUtils.h>

#include "DelayObj3.h"

// When setting the delay to 0, then increasing,
// there is often a click at the beginning
#define FIX_CLICK_FROM_ZERO 1

DelayObj3::DelayObj3(BL_FLOAT delay)
{
    SetDelay(delay);
}

DelayObj3::~DelayObj3() {}

void
DelayObj3::Reset()
{
    BLUtils::FillAllZero(&mDelayLine);
}

void
DelayObj3::SetDelay(BL_FLOAT delay)
{
    mDelay = delay;
 
    int delayI = ceil(delay) /*+ 1*/;
    
#if FIX_CLICK_FROM_ZERO
    if (delayI == 0)
        delayI = 1;
#endif
    
    BL_FLOAT lastValue = 0.0;
    if (mDelayLine.GetSize() > 0)
        lastValue = mDelayLine.Get()[mDelayLine.GetSize() - 1];
    
    // Fill new rooms with last value
    BLUtils::ResizeFillValue2(&mDelayLine, delayI, lastValue);
}

BL_FLOAT
DelayObj3::ProcessSample(BL_FLOAT sample)
{
    if (mDelayLine.GetSize() == 0)
        return sample;
    
    // Read
    if (mDelayLine.GetSize() <= 1)
    {
#if FIX_CLICK_FROM_ZERO
        mDelayLine.Get()[mDelayLine.GetSize() - 1] = sample;
#endif
        
        return sample;
    }
    
    // Read by interpolating, to manage non integer delays
    // GOOD: efficient when using a ParamSmoother for the delay parameter
    BL_FLOAT result0 = mDelayLine.Get()[0];
    BL_FLOAT result1 = mDelayLine.Get()[1];
    
    BL_FLOAT t = mDelay - (int)mDelay;
    
    BL_FLOAT result = (1.0 - t)*result0 + t*result1;
    
    BLUtils::ConsumeLeft(&mDelayLine, 1);
    
    // Write
    mDelayLine.Resize(mDelayLine.GetSize() + 1);
    mDelayLine.Get()[mDelayLine.GetSize() - 1] = sample;
    
    return result;
}
