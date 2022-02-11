//
//  CrossoverSplitter2Bands.cpp
//  UST
//
//  Created by applematuer on 7/28/19.
//
//

#include <FilterLR2Crossover.h>
#include <FilterLR4Crossover.h>

#include <BLUtils.h>

#include "CrossoverSplitter2Bands.h"

// Fixes crackles when moving cutoff freqs quickly
#define FIX_CRACKLES 1

#define FEED_PREV_SAMPLES_SIZE 44100 //4096

CrossoverSplitter2Bands::CrossoverSplitter2Bands(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    
    mFilter = new FILTER_CLASS(mSampleRate, 200.0);
    
#if FIX_CRACKLES
    mFeedPrevSamples = false;
#endif
}

CrossoverSplitter2Bands::~CrossoverSplitter2Bands()
{
    delete mFilter;
}

void
CrossoverSplitter2Bands::Reset(BL_FLOAT sampleRate)
{
    mFilter->Reset(mSampleRate);
}

void
CrossoverSplitter2Bands::SetCutoffFreq(BL_FLOAT freq)
{
    mFilter->SetCutoffFreq(freq);
    
#if FIX_CRACKLES
    // Feed the filters with previous samples, to ensure continuity
    FeedPrevSamples();
#endif
}

void
CrossoverSplitter2Bands::Split(BL_FLOAT sample, BL_FLOAT result[2])
{
    // See: https://gist.github.com/130db/6001343
    // ??
    
    mFilter->Process(sample, &result[0], &result[1]);
}

void
CrossoverSplitter2Bands::Split(const WDL_TypedBuf<BL_FLOAT> &samples,
                               WDL_TypedBuf<BL_FLOAT> result[2])
{
    for (int i = 0; i < 2; i++)
    {
        result[i].Resize(samples.GetSize());
    }
    
    for (int i = 0; i < samples.GetSize(); i++)
    {
        BL_FLOAT s = samples.Get()[i];
        BL_FLOAT r[2];
        Split(s, r);
        
        for (int k = 0; k < 2; k++)
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

void
CrossoverSplitter2Bands::FeedPrevSamples()
{
    mFeedPrevSamples = true;
    
    WDL_TypedBuf<BL_FLOAT> result[2];
    Split(mPrevSamples, result);
    
    mFeedPrevSamples = false;
}

