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
