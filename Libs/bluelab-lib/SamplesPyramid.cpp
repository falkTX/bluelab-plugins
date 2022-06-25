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
//  SamplesPyramid.cpp
//  BL-Ghost
//
//  Created by applematuer on 9/30/18.
//
//

#include <BLUtils.h>
#include <BLUtilsDecim.h>
#include <BLUtilsMath.h>

#include "SamplesPyramid.h"

// WORKAROUND: limit the maximum pyramid depth
//
// (Tested on long file, and long capture: perfs still ok)
//
// If the level is too high, the waveform gets messy
// (we don't detect well minima and maxima)
#define MAX_PYRAMID_LEVEL 8

// FIX: Reaper, buffer size 447: when in capture mode,
// the right part of the waveform becomes glitchy
// To void this, bufferize input and output, to push
// and pop buffer of size power of two
#define PUSH_POP_POW_TWO 1


SamplesPyramid::SamplesPyramid()
{
    mRemainToPop = 0;
}

SamplesPyramid::~SamplesPyramid() {}

void
SamplesPyramid::Reset()
{
#if PUSH_POP_POW_TWO
    mPushBuf.Resize(0);
    
    mRemainToPop = 0;
#endif
    
    mSamplesPyramid.clear();
}

void
SamplesPyramid::SetValues(const WDL_TypedBuf<BL_FLOAT> &samples)
{
#if PUSH_POP_POW_TWO
    mPushBuf.Resize(0);
    
    mRemainToPop = 0;
#endif

    // Clear, in case we previously build the pyramid
    mSamplesPyramid.clear();
    
    // Gen the pyramid
    mSamplesPyramid.resize(1);
    mSamplesPyramid[0] = samples;
    
    while(true)
    {
        int pyramidLevel = mSamplesPyramid.size();
        mSamplesPyramid.resize(pyramidLevel + 1);
        
        const WDL_TypedBuf<BL_FLOAT> &prevLevel = mSamplesPyramid[pyramidLevel - 1];
        if (prevLevel.GetSize() < 2)
            break;
        
        BLUtilsDecim::DecimateSamples(&mSamplesPyramid[pyramidLevel],
                                      prevLevel, (BL_FLOAT)0.5);
        
        if (pyramidLevel >= MAX_PYRAMID_LEVEL)
            break;
    }
}

void
SamplesPyramid::PushValues(const WDL_TypedBuf<BL_FLOAT> &samples)
{
#if PUSH_POP_POW_TWO
    // Bufferize pushed buffers to manage only power of two buffers
    // Otherwise there will be artefacts in the waveform
    // (Ghost-X, capture mode, Reaper, buffer size 447)
    //
    mPushBuf.Add(samples.Get(), samples.GetSize());
    
    int numToAdd = mPushBuf.GetSize();
    numToAdd = BLUtilsMath::NextPowerOfTwo(numToAdd);
    numToAdd /= 2;
    
    WDL_TypedBuf<BL_FLOAT> samples0;
    samples0.Add(mPushBuf.Get(), numToAdd);
    
    BLUtils::ConsumeLeft(&mPushBuf, numToAdd);
#else
    WDL_TypedBuf<BL_FLOAT> samples0 = samples;
#endif
    
    
    // Num samples to overlap at level 0,
    // to avoid discontinuities
    // (will decrease when going deeper in the pyramid)
#define NUM_SAMPLES_OVERLAP 0 //512
    
    if (mSamplesPyramid.empty())
        // First time, create pyramid
        mSamplesPyramid.resize(1);
    
    mSamplesPyramid[0].Add(samples0.Get(), samples0.GetSize());
    
    int pyramidLevel = 0;
    int numSamplesOverlap = NUM_SAMPLES_OVERLAP;
    int numSamplesAdd = samples0.GetSize();
    
    while(true)
    {
        if (mSamplesPyramid[pyramidLevel].GetSize() < 2)
            // Top of the pyramid
            break;
        
        pyramidLevel++;
        
        // Must go up
        if (mSamplesPyramid.size() < pyramidLevel + 1)
            // Grow the pyramid if necessary
            mSamplesPyramid.resize(pyramidLevel + 1);
        
        const WDL_TypedBuf<BL_FLOAT> &currentLevel = mSamplesPyramid[pyramidLevel - 1];
        
        // Take twice the size of the input buffer, to avoid discontinuities
        int start = currentLevel.GetSize() - numSamplesAdd - numSamplesOverlap;
        if (start < 0)
            start = 0;
        int end = currentLevel.GetSize();
        int size = end - start;
        
        WDL_TypedBuf<BL_FLOAT> samplesCurrentLevel;
        samplesCurrentLevel.Add(&currentLevel.Get()[start], size);
        
        WDL_TypedBuf<BL_FLOAT> samplesNextLevel;
        BLUtilsDecim::DecimateSamples(&samplesNextLevel,
                                      samplesCurrentLevel, (BL_FLOAT)0.5);
        
        numSamplesOverlap /= 2;
        
        // Take only the second half of the buffer
        // (because we previously tooke twice the buffer size)
        BLUtils::ConsumeLeft(&samplesNextLevel, numSamplesOverlap);
        
        mSamplesPyramid[pyramidLevel].Add(samplesNextLevel.Get(), samplesNextLevel.GetSize());
        
        numSamplesAdd /= 2;
        
        if (pyramidLevel >= MAX_PYRAMID_LEVEL)
            break;
    }
}

void
SamplesPyramid::PopValues(long numSamples)
{
#if PUSH_POP_POW_TWO
    // Because we bufferize when pushing, we must
    // bufferize when poping too, to avoid shifting
    // (Otherwise in Ghost-X view mode as plugins, the waveform
    // will not scroll correclly).
    long numSamples0 = numSamples + mRemainToPop;
    
    numSamples0 = BLUtilsMath::NextPowerOfTwo(numSamples0);
    numSamples0 /= 2;
    
    if (mSamplesPyramid.empty() ||
        (mSamplesPyramid[0].GetSize() < numSamples0))
    {
        mRemainToPop += numSamples;
            
        return;
    }
    
    mRemainToPop += numSamples - numSamples0;
    
    numSamples = numSamples0;
#endif
    
    // NOTE: not sur BL_FLOAT type is really useful
    
    BL_FLOAT numSamplesD = numSamples;
    
    for (int i = 0; i < mSamplesPyramid.size(); i++)
    {
        BLUtils::ConsumeLeft(&mSamplesPyramid[i], numSamplesD);
        
        numSamplesD /= 2.0;
        
        if (mSamplesPyramid[i].GetSize() < 2)
        {
            // Throw away the top of the pyramid
            // (useless because we have less data)
            mSamplesPyramid.resize(i + 1);
            
            break;
        }
    }
}

void
SamplesPyramid::ReplaceValues(long start, const WDL_TypedBuf<BL_FLOAT> &samples)
{
    int pyramidLevel = 0;
    
    WDL_TypedBuf<BL_FLOAT> samples0 = samples;
    BL_FLOAT startD = start;
    
    // Iterate over the pyramid levels and replace
    while(true)
    {
        if (pyramidLevel >= mSamplesPyramid.size())
        {
            break;
        }
        
        // Replace
        BLUtils::Replace(&mSamplesPyramid[pyramidLevel], startD, samples0);
        
        WDL_TypedBuf<BL_FLOAT> tmp = samples0;
        BLUtilsDecim::DecimateSamples(&samples0, tmp, (BL_FLOAT)0.5);
        
        startD /= 2.0;
        pyramidLevel++;
    }
}

void
SamplesPyramid::GetValues(long start, long end, long numValues,
                          WDL_TypedBuf<BL_FLOAT> *samples)
{
    // Check if we must add zeros at the beginning
    int numZerosBegin = 0;
    if (start < 0)
    {
        numZerosBegin = -start;
        start = 0;
    }
    
    // Check if we must add zeros at the end
    int numZerosEnd = 0;
    if (!mSamplesPyramid.empty() && (end > mSamplesPyramid[0].GetSize()))
    {
        numZerosEnd = end - mSamplesPyramid[0].GetSize();
        end = mSamplesPyramid[0].GetSize();
    }
    
    // Get the data
    int pyramidLevel = 0;
    
    // Find the correct pyramid level
    while(true)
    {
        long size = end - start;
        
        // We must start at the first bigger interval
        if ((pyramidLevel >= mSamplesPyramid.size()) || (size < numValues))
        {
            pyramidLevel--;
            if (pyramidLevel < 0)
                pyramidLevel = 0;
            
            start *= 2;
            end *= 2;
            
            numZerosBegin *= 2;
            numZerosEnd *= 2;
            
            break;
        }
        
        start /= 2;
        end /= 2;
        
        numZerosBegin /= 2;
        numZerosEnd /= 2;
        
        pyramidLevel++;
    }
    
    const WDL_TypedBuf<BL_FLOAT> &currentPyramidLevel = mSamplesPyramid[pyramidLevel];
    
    WDL_TypedBuf<BL_FLOAT> level;
    int size = end - start;
    level.Add(&currentPyramidLevel.Get()[(long)start], size);
    
    // Add zeros if necessary
    if (numZerosBegin > 0)
    {
        BLUtils::PadZerosLeft(&level, numZerosBegin);
    }
    
    if (numZerosEnd > 0)
    {
        BLUtils::PadZerosRight(&level, numZerosEnd);
    }
    
    // Update the size with the zeros added
    size = level.GetSize();
    
    // Decimate
    BL_FLOAT decimFactor = ((BL_FLOAT)numValues)/size;
    BLUtilsDecim::DecimateSamples(samples, level, decimFactor);
}

