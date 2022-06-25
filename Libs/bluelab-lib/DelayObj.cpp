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
//  DelayObj.cpp
//  BL-Precedence
//
//  Created by applematuer on 3/12/19.
//
//

#include <BLUtils.h>

#include "DelayObj.h"


#define FADE_RATIO 0.25


DelayObj::DelayObj(int maxDelaySamples, BL_FLOAT sampleRate)
{
    mMaxDelaySamples = maxDelaySamples;
    
    mSampleRate = sampleRate;
    
    // Current
    mDelay = 0;
    
    // Prev
    mPrevDelay = 0;
    
    // To avoid clicks after reset
    mPrevReset = true;
}

DelayObj::~DelayObj() {}

void
DelayObj::Reset(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    
    // Reset the delay lines
    mDelayLine.Resize(0);
    mPrevDelayLine.Resize(0);
    
    mPrevReset = true;
}

void
DelayObj::SetDelay(int delay)
{
    mDelay = delay;
}

void
DelayObj::AddSamples(const WDL_TypedBuf<BL_FLOAT> &inBuf)
{
    // Current
    mDelayLine.Add(inBuf.Get(), inBuf.GetSize());
    
    // Prev
    mPrevDelayLine.Add(inBuf.Get(), inBuf.GetSize());
}

void
DelayObj::GetSamples(const WDL_TypedBuf<BL_FLOAT> &inBuf,
                     WDL_TypedBuf<BL_FLOAT> *outBuf)
{
    int nFrames = outBuf->GetSize();
    if (inBuf.GetSize() != nFrames)
        return;
    
    // Simple copy if possible
    // (because just after, we may return if the delay line is not full)
    //
    
    // Left
    if (mDelay == 0)
    {
        for (int i = 0; i < nFrames; i++)
        {
            outBuf->Get()[i] = inBuf.Get()[i];
        }
        
        mPrevReset = false;
    }
    
    // Check
    if (mDelayLine.GetSize() < mDelay + nFrames)
    {
        mPrevDelay = mDelay;
        
        // We don't have enough samples, return
        
        return;
    }
    
    // Set the out samples
    //
    // Version avoiding blank zones when delay changes
    // (avoid buffer underrun by keeping previous samples)
    bool prevReset = mPrevReset;
    for (int i = 0; i < nFrames; i++)
    {
        // Avoid click after reset, when first time playing a delay line with non-zero delay
        int numSamplesFade = nFrames*FADE_RATIO;
        BL_FLOAT t = ((BL_FLOAT)i)/numSamplesFade;
        if (t < 0.0)
            t = 0.0;
        if (t > 1.0)
            t = 1.0;
        
        int startIdx = mDelayLine.GetSize() - nFrames - mDelay;
        if ((startIdx >= 0) && (startIdx + i < mDelayLine.GetSize()))
        {
            BL_FLOAT sample = mDelayLine.Get()[startIdx + i];
            
            if (prevReset)
            {
                sample *= t;
                
                mPrevReset = false;
            }
            
            outBuf->Get()[i] = sample;
        }
    }
    
    // Avoid clicks, by fading between prev delay lines
    int numSamplesFade = nFrames*FADE_RATIO;
    
    if (mPrevDelay != mDelay)
    // Update only if the delay changed
    {
        int startIdx = mDelayLine.GetSize() - nFrames - mDelay;
        int prevStartIdx = mPrevDelayLine.GetSize() - nFrames - mPrevDelay;
        
        if ((mDelayLine.GetSize() > numSamplesFade) &&
            (mPrevDelayLine.GetSize() > numSamplesFade))
        {
            for (int i = 0; i < numSamplesFade; i++)
            {
                BL_FLOAT t = 0.0;
                if (numSamplesFade >= 2)
                    t = ((BL_FLOAT)i)/(numSamplesFade - 1);
                
                if ((numSamplesFade < mDelayLine.GetSize()) &&
                    (numSamplesFade < mPrevDelayLine.GetSize()))
                {
                    if ((startIdx >= 0) &&
                        (startIdx + i < mDelayLine.GetSize()) &&
                        (prevStartIdx >= 0) &&
                        (prevStartIdx + i < mPrevDelayLine.GetSize()))
                    {
                        BL_FLOAT current = mDelayLine.Get()[i + startIdx];
                        BL_FLOAT prev = mPrevDelayLine.Get()[i + prevStartIdx];
                        
                        BL_FLOAT val = (1.0 - t)*prev + t*current;
                        
                        if (numSamplesFade < outBuf->GetSize())
                            outBuf->Get()[i] = val;
                    }
                }
            }
        }
    }
}

void
DelayObj::ConsumeBuffer()
{
    // Current
    int numToConsumeCurrent = mDelayLine.GetSize() - mMaxDelaySamples;
    if (numToConsumeCurrent > 0)
    {
        BLUtils::ConsumeLeft(&mDelayLine, numToConsumeCurrent);
    }
    
    // Prev
    int numToConsumePrev = mPrevDelayLine.GetSize() - mMaxDelaySamples;
    if (numToConsumePrev > 0)
    {
        BLUtils::ConsumeLeft(&mPrevDelayLine, numToConsumePrev);
    }
    
    // Update
    mPrevDelay = mDelay;
}
