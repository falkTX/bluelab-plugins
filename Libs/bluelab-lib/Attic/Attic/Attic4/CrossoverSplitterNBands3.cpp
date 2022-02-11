//
//  CrossoverSplitterNBands3.cpp
//  UST
//
//  Created by applematuer on 8/24/19.
//
//

#include <FilterRBJNX.h>
#include <FilterTransparentRBJNX.h>

#include "CrossoverSplitterNBands3.h"

// 2 for double
#define FILTER_ORDER 2

// GOOD !
// Compensate the delays by adding transparent filters.
//
// With that, the sum will be straight,
// event when moving the bands a lot.
// (to test, draw the sum curve on the multiband graph)
//
// See: https://music-dsp.music.columbia.narkive.com/V34CpSZy/crossover-filtering-for-multiband-application:i.4.1.full
// and: https://music-dsp.music.columbia.narkive.com/V34CpSZy/crossover-filtering-for-multiband-application
#define ADD_TRANSPARENT_FILTERS 1

// BAD
// Doesn't work
#define ADD_ALLPASS_FILTERS 0


CrossoverSplitterNBands3::CrossoverSplitterNBands3(int numBands,
                                                   BL_FLOAT cutoffFreqs[],
                                                   BL_FLOAT sampleRate)
{
    mNumBands = numBands;
    
    mSampleRate = sampleRate;
    
    if (numBands > 1)
    {
        mFilterChains.resize(mNumBands);
        
        mCutoffFreqs.resize(mNumBands - 1);
        for (int i = 0; i < mNumBands - 1; i++)
            mCutoffFreqs[i] = cutoffFreqs[i];
        
        CreateFilters(mSampleRate);
    }
    
#if OPTIM_AVOID_NEW
    mTmpResultCross = new BL_FLOAT[mNumBands];
    mTmpResultCross2 = new BL_FLOAT[mNumBands];
#endif
}

CrossoverSplitterNBands3::CrossoverSplitterNBands3(const CrossoverSplitterNBands3 &other)
{
    mNumBands = other.mNumBands;
    mSampleRate = other.mSampleRate;
    
    if (mNumBands > 1)
    {
        mFilterChains.resize(mNumBands);
        
        mCutoffFreqs.resize(mNumBands - 1);
        for (int i = 0; i < mNumBands - 1; i++)
            mCutoffFreqs[i] = other.mCutoffFreqs[i];
        
        CreateFilters(mSampleRate);
    }
    
#if OPTIM_AVOID_NEW
    mTmpResultCross = new BL_FLOAT[mNumBands];
    mTmpResultCross2 = new BL_FLOAT[mNumBands];
#endif
}

CrossoverSplitterNBands3::~CrossoverSplitterNBands3()
{
    for (int i = 0; i < mFilterChains.size(); i++)
    {
        const vector<FilterRBJ *> &chain = mFilterChains[i];
        for (int j = 0; j < chain.size(); j++)
        {
            FilterRBJ *filter = chain[j];
            
            delete filter;
        }
    }
    
#if OPTIM_AVOID_NEW
    delete mTmpResultCross;
    delete mTmpResultCross2;
#endif
}

void
CrossoverSplitterNBands3::Reset(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    
    SetFiltersValues();
}

void
CrossoverSplitterNBands3::SetCutoffFreqs(BL_FLOAT freqs[])
{
    for (int j = 0; j < mNumBands - 1; j++)
        mCutoffFreqs[j] = freqs[j];
    
    SetFiltersValues();
}

int
CrossoverSplitterNBands3::GetNumBands()
{
    return mNumBands;
}

BL_FLOAT
CrossoverSplitterNBands3::GetCutoffFreq(int freqNum)
{
    if (freqNum >= mNumBands - 1)
        return -1.0;
    
    BL_FLOAT freq = mCutoffFreqs[freqNum];
    
    return freq;
}

void
CrossoverSplitterNBands3::SetCutoffFreq(int freqNum, BL_FLOAT freq)
{
    if (freqNum >= mNumBands - 1)
        return;
    
    mCutoffFreqs[freqNum] = freq;
    
    // Not optimized, set the values for all the filters
    // (and not only the ones with cutoff)
    SetFiltersValues();
}

void
CrossoverSplitterNBands3::Split(BL_FLOAT sample, BL_FLOAT result[])
{
#if !OPTIM_AVOID_NEW
    BL_FLOAT *resultCross = new BL_FLOAT[mNumBands];
#endif
    
    for (int i = 0; i < mFilterChains.size(); i++)
    {
        const vector<FilterRBJ *> &chain = mFilterChains[i];
        
        for (int j = 0; j < chain.size(); j++)
        {
            if ((i == 0) && (j == 0))
                // First chain, first filter
            {
#if !OPTIM_AVOID_NEW
                resultCross[i] = sample;
#else
                mTmpResultCross[i] = sample;
#endif
                
                FilterRBJ *filter = mFilterChains[i][j];
                result[i] = filter->Process(sample);
            }
            else if (j == 0)
                // Other chains, first filter
            {
                FilterRBJ *filter = mFilterChains[i][j];
#if !OPTIM_AVOID_NEW
                result[i] = filter->Process(resultCross[i - 1]);
                resultCross[i] = result[i];
#else
                result[i] = filter->Process(mTmpResultCross[i - 1]);
                mTmpResultCross[i] = result[i];
#endif
            }
            else
                // Next filters
            {
                FilterRBJ *filter = mFilterChains[i][j];
                result[i] = filter->Process(result[i]);
            }
        }
    }
    
#if !OPTIM_AVOID_NEW
    delete resultCross;
#endif
    
#if 0 // TEST
    for (int k = 1; k < mNumBands; k += 2)
    {
        result[k] = -result[k];
    }
#endif
}

void
CrossoverSplitterNBands3::Split(const WDL_TypedBuf<BL_FLOAT> &samples,
                                WDL_TypedBuf<BL_FLOAT> result[])
{
    for (int i = 0; i < mNumBands; i++)
    {
        result[i].Resize(samples.GetSize());
    }
    
#if !OPTIM_AVOID_NEW
    BL_FLOAT *r = new BL_FLOAT[mNumBands];
#endif
    
    for (int i = 0; i < samples.GetSize(); i++)
    {
        BL_FLOAT s = samples.Get()[i];
        
#if !OPTIM_AVOID_NEW
        Split(s, r);
#else
        Split(s, mTmpResultCross2);
#endif
        
        for (int k = 0; k < mNumBands; k++)
        {
#if !OPTIM_AVOID_NEW
            result[k].Get()[i] = r[k];
#else
            result[k].Get()[i] = mTmpResultCross2[k];
#endif
        }
    }

#if !OPTIM_AVOID_NEW
    delete r;
#endif
}

void
CrossoverSplitterNBands3::CreateFilters(BL_FLOAT sampleRate)
{
    for (int i = 0; i < mNumBands; i++)
    {
        if (i == 0)
        {
            FilterRBJNX *filter =
                    new FilterRBJNX(FILTER_ORDER,
                                   FILTER_TYPE_LOWPASS,
                                   mSampleRate,
                                   mCutoffFreqs[i]);
            mFilterChains[i].push_back(filter);
        }
        else if (i == mNumBands - 1)
        {
            FilterRBJNX *filter =
                    new FilterRBJNX(FILTER_ORDER,
                                   FILTER_TYPE_HIPASS,
                                   mSampleRate,
                                   mCutoffFreqs[i - 1]);
            mFilterChains[i].push_back(filter);
        }
        else
        {
            FilterRBJNX *filter =
                    new FilterRBJNX(FILTER_ORDER,
                                   FILTER_TYPE_HIPASS,
                                   mSampleRate,
                                   mCutoffFreqs[i - 1]);
            mFilterChains[i].push_back(filter);
            
            FilterRBJNX *filter2 =
                    new FilterRBJNX(FILTER_ORDER,
                                   FILTER_TYPE_LOWPASS,
                                   mSampleRate,
                                   mCutoffFreqs[i]);
            mFilterChains[i].push_back(filter2);
        }
    }
    
    // GOOD
    // With that, we got flat curve when injecting IRs
#if ADD_TRANSPARENT_FILTERS
    for (int i = 0; i < mNumBands; i++)
    {
        int numTransparentFilters = mNumBands - i - 2;
        if (numTransparentFilters < 0)
            numTransparentFilters = 0;

        for (int j = 0; j < numTransparentFilters; j++)
        {
            FilterTransparentRBJNX *filter =
                new FilterTransparentRBJNX(FILTER_ORDER,
                                         mSampleRate,
                                         mCutoffFreqs[i + j + 1]);
            
            mFilterChains[i].push_back(filter);
        }
    }
#endif
    
    // BAD: doesn't work
#if ADD_ALLPASS_FILTERS
    for (int i = 0; i < mNumBands; i++)
    {
        int numAllPassFilters = mNumBands - i - 2;
        if (numAllPassFilters < 0)
            numAllPassFilters = 0;
        
        for (int j = 0; j < numAllPassFilters; j++)
        {
            FilterRBJNX *filter =
            new FilterRBJNX(FILTER_ORDER,
                           FILTER_TYPE_ALLPASS,
                           mSampleRate,
                           mCutoffFreqs[i + j + 1]);
            
            mFilterChains[i].push_back(filter);
        }
    }
#endif
}

void
CrossoverSplitterNBands3::SetFiltersValues()
{
    for (int i = 0; i < mNumBands; i++)
    {
        if (i == 0)
        {
            FilterRBJ *filter = mFilterChains[i][0];
            filter->SetCutoffFreq(mCutoffFreqs[i]);
            filter->SetSampleRate(mSampleRate);
        }
        else if (i == mNumBands - 1)
        {
            FilterRBJ *filter = mFilterChains[i][0];
            filter->SetCutoffFreq(mCutoffFreqs[i - 1]);
            filter->SetSampleRate(mSampleRate);
        }
        else
        {
            FilterRBJ *filter = mFilterChains[i][0];
            filter->SetCutoffFreq(mCutoffFreqs[i - 1]);
            filter->SetSampleRate(mSampleRate);
      
            FilterRBJ *filter2 = mFilterChains[i][1];
            filter2->SetCutoffFreq(mCutoffFreqs[i]);
            filter2->SetSampleRate(mSampleRate);
        }
    }
    
    // GOOD:
    // Without this, we got flat response with injecting IR,
    // but had holes when filtering white noise.
    // With that, the process is transparent for white noise too.
#if ADD_TRANSPARENT_FILTERS
    for (int i = 0; i < mNumBands; i++)
    {
        int numTransparentFilters = mNumBands - i - 2;
        if (numTransparentFilters < 0)
            numTransparentFilters = 0;
        
        for (int j = 0; j < numTransparentFilters; j++)
        {
            int index = mFilterChains[i].size() - 1 - j;
            FilterRBJ *filter = mFilterChains[i][index];
            
            int freqIndex = mCutoffFreqs.size() - 1 - j;
            filter->SetCutoffFreq(mCutoffFreqs[freqIndex]);
            filter->SetSampleRate(mSampleRate);
        }
    }
#endif
}
