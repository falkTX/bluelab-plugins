//
//  CrossoverSplitterNBands4.cpp
//  UST
//
//  Created by applematuer on 8/24/19.
//
//

#include <FilterRBJ2X.h>
#include <FilterRBJ2X2.h>

#include <FilterTransparentRBJ2X.h>
#include <FilterTransparentRBJ2X2.h>

#include "CrossoverSplitterNBands4.h"

//#define FILTER_2X_CLASS FilterRBJ2X
#define FILTER_2X_CLASS FilterRBJ2X2

// Two filters, LP and HP, then sum
//#define TRANSPARENT_FILTER_CLASS FilterTransparentRBJ2X

// A single allpass filter
// NOTE: This seems to work!
// And it uses only 1 filter instead of 2x2 (2xLP + 2xHP)
#define TRANSPARENT_FILTER_CLASS FilterTransparentRBJ2X2

// For a good crossover, we know we must cahin 2 filters, not less
//#define FILTER_ORDER 2

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

#define OPTIM_PARALLEL_FILTERS 1

CrossoverSplitterNBands4::CrossoverSplitterNBands4(int numBands,
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

CrossoverSplitterNBands4::CrossoverSplitterNBands4(const CrossoverSplitterNBands4 &other)
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

CrossoverSplitterNBands4::~CrossoverSplitterNBands4()
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
    delete []mTmpResultCross;
    delete []mTmpResultCross2;
#endif
}

void
CrossoverSplitterNBands4::Reset(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    
    SetFiltersValues();
}

void
CrossoverSplitterNBands4::SetCutoffFreqs(BL_FLOAT freqs[])
{
    for (int j = 0; j < mNumBands - 1; j++)
        mCutoffFreqs[j] = freqs[j];
    
    SetFiltersValues();
}

int
CrossoverSplitterNBands4::GetNumBands()
{
    return mNumBands;
}

BL_FLOAT
CrossoverSplitterNBands4::GetCutoffFreq(int freqNum)
{
    if (freqNum >= mNumBands - 1)
        return -1.0;
    
    BL_FLOAT freq = mCutoffFreqs[freqNum];
    
    return freq;
}

void
CrossoverSplitterNBands4::SetCutoffFreq(int freqNum, BL_FLOAT freq)
{
    if (freqNum >= mNumBands - 1)
        return;
    
    mCutoffFreqs[freqNum] = freq;
    
    // Not optimized, set the values for all the filters
    // (and not only the ones with cutoff)
    SetFiltersValues();
}

void
CrossoverSplitterNBands4::Split(BL_FLOAT sample, BL_FLOAT result[])
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
    delete []resultCross;
#endif
    
#if 0 // TEST
    for (int k = 1; k < mNumBands; k += 2)
    {
        result[k] = -result[k];
    }
#endif
}

#if !OPTIM_PARALLEL_FILTERS
void
CrossoverSplitterNBands4::Split(const WDL_TypedBuf<BL_FLOAT> &samples,
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
    delete []r;
#endif
}
#endif

#if OPTIM_PARALLEL_FILTERS
void
CrossoverSplitterNBands4::Split(const WDL_TypedBuf<BL_FLOAT> &samples,
                                WDL_TypedBuf<BL_FLOAT> result[])
{
    for (int i = 0; i < mNumBands; i++)
    {
        result[i].Resize(samples.GetSize());
    }
    
    //WDL_TypedBuf<BL_FLOAT> resultCross[mNumBands];
    vector<WDL_TypedBuf<BL_FLOAT> > &resultCross = mTmpBuf0;
    resultCross.resize(mNumBands);

    for (int i = 0; i < mFilterChains.size(); i++)
    {
        const vector<FilterRBJ *> &chain = mFilterChains[i];
            
        for (int j = 0; j < chain.size(); j++)
        {
            if ((i == 0) && (j == 0))
                // First chain, first filter
            {
                resultCross[i] = samples;
                
                FilterRBJ *filter = mFilterChains[i][j];
                
                result[i] = samples; //
                filter->Process(&result[i]);
            }
            else if (j == 0)
                // Other chains, first filter
            {
                FilterRBJ *filter = mFilterChains[i][j];
                
                result[i] = resultCross[i - 1];
                
                filter->Process(&result[i]);
                
                resultCross[i] = result[i];
            }
            else
                // Next filters
            {
                FilterRBJ *filter = mFilterChains[i][j];
                filter->Process(&result[i]);
            }
        }
    }
}
#endif

void
CrossoverSplitterNBands4::CreateFilters(BL_FLOAT sampleRate)
{
    for (int i = 0; i < mNumBands; i++)
    {
        if (i == 0)
        {
            FILTER_2X_CLASS *filter =
                new FILTER_2X_CLASS(FILTER_TYPE_LOWPASS,
                                    mSampleRate,
                                    mCutoffFreqs[i]);
            mFilterChains[i].push_back(filter);
        }
        else if (i == mNumBands - 1)
        {
            FILTER_2X_CLASS *filter =
                new FILTER_2X_CLASS(FILTER_TYPE_HIPASS,
                                    mSampleRate,
                                    mCutoffFreqs[i - 1]);
            mFilterChains[i].push_back(filter);
        }
        else
        {
            FILTER_2X_CLASS *filter =
                new FILTER_2X_CLASS(FILTER_TYPE_HIPASS,
                                    mSampleRate,
                                    mCutoffFreqs[i - 1]);
            mFilterChains[i].push_back(filter);
            
            FILTER_2X_CLASS *filter2 =
                new FILTER_2X_CLASS(FILTER_TYPE_LOWPASS,
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
            TRANSPARENT_FILTER_CLASS *filter =
                new TRANSPARENT_FILTER_CLASS(mSampleRate,
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
                new FILTER_2X_CLASS(FILTER_TYPE_ALLPASS,
                                    mSampleRate,
                                    mCutoffFreqs[i + j + 1]);
            
            mFilterChains[i].push_back(filter);
        }
    }
#endif
}

void
CrossoverSplitterNBands4::SetFiltersValues()
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
