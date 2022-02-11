//
//  FilterTransparentRBJNX.cpp
//  UST
//
//  Created by applematuer on 8/25/19.
//
//

#include <FilterRBJNX.h>

#include <BLUtils.h>

#include "FilterTransparentRBJNX.h"

FilterTransparentRBJNX::FilterTransparentRBJNX(int numFilters,
                                           BL_FLOAT sampleRate,
                                           BL_FLOAT cutoffFreq)
{
    mNumFilters = numFilters;
    mSampleRate = sampleRate;
    mCutoffFreq = cutoffFreq;
    
    mFilters[0] = new FilterRBJNX(numFilters, FILTER_TYPE_LOWPASS,
                                 sampleRate, cutoffFreq);
    
    mFilters[1] = new FilterRBJNX(numFilters, FILTER_TYPE_HIPASS,
                                 sampleRate, cutoffFreq);
}

FilterTransparentRBJNX::FilterTransparentRBJNX(const FilterTransparentRBJNX &other)
{
    mNumFilters = other.mNumFilters;
    mSampleRate = other.mSampleRate;
    mCutoffFreq = other.mCutoffFreq;
    
    mFilters[0] = new FilterRBJNX(mNumFilters, FILTER_TYPE_LOWPASS,
                                 mSampleRate, mCutoffFreq);
    
    mFilters[1] = new FilterRBJNX(mNumFilters, FILTER_TYPE_HIPASS,
                                 mSampleRate, mCutoffFreq);
}

FilterTransparentRBJNX::~FilterTransparentRBJNX()
{
    for (int i = 0; i < 2; i++)
    {
        FilterRBJNX *filter = mFilters[i];
        delete filter;
    }
}

void
FilterTransparentRBJNX::SetCutoffFreq(BL_FLOAT freq)
{
    mCutoffFreq = freq;
    
    for (int i = 0; i < 2; i++)
    {
        FilterRBJNX *filter = mFilters[i];
        filter->SetCutoffFreq(freq);
    }
}

// NEW
void
FilterTransparentRBJNX::SetQFactor(BL_FLOAT q)
{
    for (int i = 0; i < 2; i++)
    {
        FilterRBJNX *filter = mFilters[i];
        filter->SetQFactor(q);
    }
}

void
FilterTransparentRBJNX::SetSampleRate(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    
    for (int i = 0; i < 2; i++)
    {
        FilterRBJNX *filter = mFilters[i];
        filter->SetSampleRate(sampleRate);
    }
}

BL_FLOAT
FilterTransparentRBJNX::Process(BL_FLOAT sample)
{
    BL_FLOAT sum = 0.0;
    for (int i = 0; i < 2; i++)
    {
        FilterRBJNX *filter = mFilters[i];
        BL_FLOAT result = filter->Process(sample);
        
        sum += result;
    }
    
    return sum;
}

#if 0
void
FilterTransparentRBJNX::Process(WDL_TypedBuf<BL_FLOAT> *result,
                              const WDL_TypedBuf<BL_FLOAT> &samples)
{
    result->Resize(samples.GetSize());
    
    for (int i = 0; i < samples.GetSize(); i++)
    {
        BL_FLOAT sample = samples.Get()[i];
        
        BL_FLOAT sampleRes = Process(sample);
        
        result->Get()[i] = sampleRes;
    }
}
#endif

#if 0 //1
void
FilterTransparentRBJNX::Process(WDL_TypedBuf<BL_FLOAT> *ioSamples)
{
    for (int i = 0; i < ioSamples->GetSize(); i++)
    {
        BL_FLOAT sample = ioSamples->Get()[i];
        
        BL_FLOAT sampleRes = Process(sample);
        
        ioSamples->Get()[i] = sampleRes;
    }
}
#endif

#if 1
void
FilterTransparentRBJNX::Process(WDL_TypedBuf<BL_FLOAT> *ioSamples)
{
    WDL_TypedBuf<BL_FLOAT> sum;
    sum.Resize(ioSamples->GetSize());
    BLUtils::FillAllZero(&sum);

    for (int i = 0; i < 2; i++)
    {
        FilterRBJNX *filter = mFilters[i];
        
        WDL_TypedBuf<BL_FLOAT> samples = *ioSamples;
        filter->Process(&samples);
        
        BLUtils::AddValues(&sum, samples);
    }
    
    *ioSamples = sum;
}
#endif
