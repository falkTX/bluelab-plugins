//
//  CrossoverSplitterNBands2.cpp
//  UST
//
//  Created by applematuer on 8/24/19.
//
//

#include <FilterRBJ2X.h>
#include <FilterRBJ2X2.h>

#include "CrossoverSplitterNBands2.h"

CrossoverSplitterNBands2::CrossoverSplitterNBands2(int numBands,
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

CrossoverSplitterNBands2::CrossoverSplitterNBands2(const CrossoverSplitterNBands2 &other)
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

CrossoverSplitterNBands2::~CrossoverSplitterNBands2()
{
    for (int i = 0; i < mFilterChains.size(); i++)
    {
        const vector<FILTER_2X_CLASS *> &chain = mFilterChains[i];
        for (int j = 0; j < chain.size(); j++)
        {
            FILTER_2X_CLASS *filter = chain[j];
            
            delete filter;
        }
    }
    
#if OPTIM_AVOID_NEW
    delete mTmpResultCross;
    delete mTmpResultCross2;
#endif
}

void
CrossoverSplitterNBands2::Reset(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    
    SetFiltersValues();
}

void
CrossoverSplitterNBands2::SetCutoffFreqs(BL_FLOAT freqs[])
{
    for (int j = 0; j < mNumBands - 1; j++)
        mCutoffFreqs[j] = freqs[j];
    
    SetFiltersValues();
}

int
CrossoverSplitterNBands2::GetNumBands()
{
    return mNumBands;
}

BL_FLOAT
CrossoverSplitterNBands2::GetCutoffFreq(int freqNum)
{
    if (freqNum >= mNumBands - 1)
        return -1.0;
    
    BL_FLOAT freq = mCutoffFreqs[freqNum];
    
    return freq;
}

void
CrossoverSplitterNBands2::SetCutoffFreq(int freqNum, BL_FLOAT freq)
{
    if (freqNum >= mNumBands - 1)
        return;
    
    mCutoffFreqs[freqNum] = freq;
    
    // Not optimized, set the values for all the filters
    // (and not only the ones with cutoff)
    SetFiltersValues();
}

void
CrossoverSplitterNBands2::Split(BL_FLOAT sample, BL_FLOAT result[])
{
#if !OPTIM_AVOID_NEW
    BL_FLOAT *resultCross = new BL_FLOAT[mNumBands];
#endif
    
    for (int i = 0; i < mFilterChains.size(); i++)
    {
        const vector<FILTER_2X_CLASS *> &chain = mFilterChains[i];
        
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
                
                FILTER_2X_CLASS *filter = mFilterChains[i][j];
                result[i] = filter->Process(sample);
            }
            else if (j == 0)
                // Other chains, first filter
            {
                FILTER_2X_CLASS *filter = mFilterChains[i][j];
                
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
                FILTER_2X_CLASS *filter = mFilterChains[i][j];
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
CrossoverSplitterNBands2::Split(const WDL_TypedBuf<BL_FLOAT> &samples,
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
CrossoverSplitterNBands2::CreateFilters(BL_FLOAT sampleRate)
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
}

void
CrossoverSplitterNBands2::SetFiltersValues()
{
    for (int i = 0; i < mNumBands; i++)
    {
        if (i == 0)
        {
            FILTER_2X_CLASS *filter = mFilterChains[i][0];
            filter->SetCutoffFreq(mCutoffFreqs[i]);
            filter->SetSampleRate(mSampleRate);
        }
        else if (i == mNumBands - 1)
        {
            FILTER_2X_CLASS *filter = mFilterChains[i][0];
            filter->SetCutoffFreq(mCutoffFreqs[i - 1]);
            filter->SetSampleRate(mSampleRate);
        }
        else
        {
            FILTER_2X_CLASS *filter = mFilterChains[i][0];
            filter->SetCutoffFreq(mCutoffFreqs[i - 1]);
            filter->SetSampleRate(mSampleRate);
            
            FILTER_2X_CLASS *filter2 = mFilterChains[i][1];
            filter2->SetCutoffFreq(mCutoffFreqs[i]);
            filter2->SetSampleRate(mSampleRate);
        }
    }
}
