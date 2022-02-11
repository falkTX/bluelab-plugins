//
//  FilterRBJFeed.cpp
//  BL-InfraSynth
//
//  Created by applematuer on 9/15/19.
//
//

#include <BLUtils.h>

#include "FilterRBJFeed.h"


FilterRBJFeed::FilterRBJFeed(FilterRBJ *filter,
                             int numPrevSamples)
{
    mFilter = filter;
    
    mNumPrevSamples = numPrevSamples;
}

FilterRBJFeed::FilterRBJFeed(const FilterRBJFeed &other)
{
    mFilter = other.mFilter;
}

FilterRBJFeed::~FilterRBJFeed() {}

void
FilterRBJFeed::SetCutoffFreq(BL_FLOAT freq)
{
    mFilter->SetCutoffFreq(freq);
    
    FeedWithPrevSamples();
}

void
FilterRBJFeed::SetQFactor(BL_FLOAT q)
{
    mFilter->SetQFactor(q);
    
    FeedWithPrevSamples();
}

void
FilterRBJFeed::SetSampleRate(BL_FLOAT sampleRate)
{
    mFilter->SetSampleRate(sampleRate);
    
    FeedWithPrevSamples();
}

BL_FLOAT
FilterRBJFeed::Process(BL_FLOAT sample)
{
    BL_FLOAT res = mFilter->Process(sample);
    
    mPrevSamples.Add(sample);
    int numToRemove = mPrevSamples.GetSize() - mNumPrevSamples;
    
    // OPTIM: Test with mNumPrevSamples instead of 0
    if (numToRemove > mNumPrevSamples/*0*/)
        BLUtils::ConsumeLeft(&mPrevSamples, mNumPrevSamples/*numToRemove*/);
    
    return res;
}

#if 0
void
FilterRBJFeed::Process(WDL_TypedBuf<BL_FLOAT> *result,
                       const WDL_TypedBuf<BL_FLOAT> &samples)
{
    mFilter->Process(result, samples);
    
    mPrevSamples.Add(samples.Get(), samples.GetSize());
    int numToRemove = mPrevSamples.GetSize() - mNumPrevSamples;
    if (numToRemove > 0)
        BLUtils::ConsumeLeft(&mPrevSamples, numToRemove);
}
#endif

#if 1
void
FilterRBJFeed::Process(WDL_TypedBuf<BL_FLOAT> *ioSamples)
{
    WDL_TypedBuf<BL_FLOAT> samples = *ioSamples;
    
    mFilter->Process(&samples);
    
    mPrevSamples.Add(samples.Get(), samples.GetSize());
    int numToRemove = mPrevSamples.GetSize() - mNumPrevSamples;
    if (numToRemove > 0)
        BLUtils::ConsumeLeft(&mPrevSamples, numToRemove);
}
#endif

void
FilterRBJFeed::FeedWithPrevSamples()
{
    //WDL_TypedBuf<BL_FLOAT> dummy;
    //mFilter->Process(&dummy, mPrevSamples);
    
    WDL_TypedBuf<BL_FLOAT> dummy = mPrevSamples;
    mFilter->Process(&dummy);
}
