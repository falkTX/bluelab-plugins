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
//  CrossoverSplitter5Bands.cpp
//  UST
//
//  Created by applematuer on 7/28/19.
//
//

#include <FilterLR2Crossover.h>
#include <FilterLR4Crossover.h>

#include <BLUtils.h>

#include "CrossoverSplitter5Bands.h"

// Fixes crackles when moving cutoff freqs quickly
#define FIX_CRACKLES 1
// #define FEED_PREV_SAMPLES_SIZE 512 // still crackles
#define FEED_PREV_SAMPLES_SIZE 4096


CrossoverSplitter5Bands::CrossoverSplitter5Bands(BL_FLOAT cutoffFreqs[CS5B_NUM_FILTERS],
                                                 BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    
    for (int i = 0; i < CS5B_NUM_FILTERS; i++)
        mFilters[i] = new FILTER_CLASS(cutoffFreqs[i], mSampleRate);
    
#if FIX_CRACKLES
    mFeedPrevSamples = false;
#endif
}

CrossoverSplitter5Bands::~CrossoverSplitter5Bands()
{
    for (int i = 0; i < CS5B_NUM_FILTERS; i++)
        delete mFilters[i];
}

void
CrossoverSplitter5Bands::Reset(BL_FLOAT sampleRate)
{
    for (int i = 0; i < CS5B_NUM_FILTERS; i++)
        mFilters[i]->Reset(mSampleRate);
}

void
CrossoverSplitter5Bands::SetCutoffFreqs(BL_FLOAT freqs[4])
{
    for (int i = 0; i < CS5B_NUM_FILTERS; i++)
        mFilters[i]->SetCutoffFreq(freqs[i]);
    
#if FIX_CRACKLES
    // Feed the filters with previous samples, to ensure continuity
    FeedPrevSamples();
#endif
}

BL_FLOAT
CrossoverSplitter5Bands::GetCutoffFreq(int freqNum)
{
    if (freqNum > CS5B_NUM_FILTERS - 1)
        return -1.0;
    
    BL_FLOAT freq = mFilters[freqNum]->GetCutoffFreq();
    
    return freq;
}

void
CrossoverSplitter5Bands::SetCutoffFreq(int freqNum, BL_FLOAT freq)
{
    if (freqNum > CS5B_NUM_FILTERS - 1)
        return;
    
    mFilters[freqNum]->SetCutoffFreq(freq);
    
#if FIX_CRACKLES
    // Feed the filters with previous samples, to ensure continuity
    FeedPrevSamples();
#endif
}

void
CrossoverSplitter5Bands::Split(BL_FLOAT sample, BL_FLOAT result[5])
{
    // See: https://gist.github.com/130db/6001343
    // ??
    
    BL_FLOAT rest;
    for (int i = 0; i < CS5B_NUM_FILTERS; i++)
    {
        mFilters[i]->Process(sample, &result[i], &rest);
        
        sample = rest;
    }
    
    result[4] = rest;
}

void
CrossoverSplitter5Bands::Split(const WDL_TypedBuf<BL_FLOAT> &samples,
                               WDL_TypedBuf<BL_FLOAT> result[5])
{
    for (int i = 0; i < 5; i++)
    {
        result[i].Resize(samples.GetSize());
    }
    
    for (int i = 0; i < samples.GetSize(); i++)
    {
        BL_FLOAT s = samples.Get()[i];
        BL_FLOAT r[5];
        Split(s, r);
        
        for (int k = 0; k < 5; k++)
        {
            result[k].Get()[i] = r[k];
        }
    }
    
#if FIX_CRACKLES
    if (!mFeedPrevSamples)
    {
        mPrevSamples.Add(samples.Get(), samples.GetSize());
        int numToConsume = mPrevSamples.GetSize() - FEED_PREV_SAMPLES_SIZE;
        if (numToConsume > 0)
            BLUtils::ConsumeLeft(&mPrevSamples, numToConsume);
    }
#endif
}

int
CrossoverSplitter5Bands::GetNumFilters()
{
    return CS5B_NUM_FILTERS;
}

// CRASHES
//void
//CrossoverSplitter5Bands::GetFilter(int index, FILTER_CLASS *filter)
//{
//    if (index >= CS5B_NUM_FILTERS)
//        return;
//
//    *filter = *mFilters[index];
//}

const FILTER_CLASS *
CrossoverSplitter5Bands::GetFilter(int index)
{
    if (index >= CS5B_NUM_FILTERS)
        return NULL;
    
    return mFilters[index];
}

void
CrossoverSplitter5Bands::FeedPrevSamples()
{
    mFeedPrevSamples = true;
    
    WDL_TypedBuf<BL_FLOAT> result[5];
    Split(mPrevSamples, result);
    
    mFeedPrevSamples = false;
}
