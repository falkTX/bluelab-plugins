//
//  FilterTransparentRBJ2X.cpp
//  UST
//
//  Created by applematuer on 8/25/19.
//
//

#include <FilterRBJ2X.h>
#include <FilterRBJ2X2.h>

#include <BLUtils.h>

#include "FilterTransparentRBJ2X.h"

FilterTransparentRBJ2X::FilterTransparentRBJ2X(BL_FLOAT sampleRate,
                                               BL_FLOAT cutoffFreq)
{
    mSampleRate = sampleRate;
    mCutoffFreq = cutoffFreq;
    
    mFilters[0] = new FILTER_2X_CLASS(FILTER_TYPE_LOWPASS,
                                  sampleRate, cutoffFreq);
    
    mFilters[1] = new FILTER_2X_CLASS(FILTER_TYPE_HIPASS,
                                  sampleRate, cutoffFreq);
}

FilterTransparentRBJ2X::FilterTransparentRBJ2X(const FilterTransparentRBJ2X &other)
{
    mSampleRate = other.mSampleRate;
    mCutoffFreq = other.mCutoffFreq;
    
    mFilters[0] = new FILTER_2X_CLASS(FILTER_TYPE_LOWPASS,
                                  mSampleRate, mCutoffFreq);
    
    mFilters[1] = new FILTER_2X_CLASS(FILTER_TYPE_HIPASS,
                                  mSampleRate, mCutoffFreq);
}

FilterTransparentRBJ2X::~FilterTransparentRBJ2X()
{
    for (int i = 0; i < 2; i++)
    {
        FILTER_2X_CLASS *filter = mFilters[i];
        delete filter;
    }
}

void
FilterTransparentRBJ2X::SetCutoffFreq(BL_FLOAT freq)
{
    mCutoffFreq = freq;
    
    for (int i = 0; i < 2; i++)
    {
        FILTER_2X_CLASS *filter = mFilters[i];
        filter->SetCutoffFreq(freq);
    }
}

// NEW
void
FilterTransparentRBJ2X::SetQFactor(BL_FLOAT q)
{
    for (int i = 0; i < 2; i++)
    {
        FILTER_2X_CLASS *filter = mFilters[i];
        filter->SetQFactor(q);
    }
}

void
FilterTransparentRBJ2X::SetSampleRate(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    
    for (int i = 0; i < 2; i++)
    {
        FILTER_2X_CLASS *filter = mFilters[i];
        filter->SetSampleRate(sampleRate);
    }
}

BL_FLOAT
FilterTransparentRBJ2X::Process(BL_FLOAT sample)
{
    BL_FLOAT sum = 0.0;
    for (int i = 0; i < 2; i++)
    {
        FILTER_2X_CLASS *filter = mFilters[i];
        BL_FLOAT result = filter->Process(sample);
        
        sum += result;
    }
    
    return sum;
}

#if 0
void
FilterTransparentRBJ2X::Process(WDL_TypedBuf<BL_FLOAT> *result,
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
FilterTransparentRBJ2X::Process(WDL_TypedBuf<BL_FLOAT> *ioSamples)
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
FilterTransparentRBJ2X::Process(WDL_TypedBuf<BL_FLOAT> *ioSamples)
{
    WDL_TypedBuf<BL_FLOAT> sum;
    sum.Resize(ioSamples->GetSize());
    BLUtils::FillAllZero(&sum);
    
    for (int i = 0; i < 2; i++)
    {
        FILTER_2X_CLASS *filter = mFilters[i];
        
        WDL_TypedBuf<BL_FLOAT> samples = *ioSamples;
        filter->Process(&samples);
        
        BLUtils::AddValues(&sum, samples);
    }
    
    *ioSamples = sum;
}
#endif
