//
//  FilterRBJNX.cpp
//  UST
//
//  Created by applematuer on 8/25/19.
//
//

#include <BLUtils.h>

#include "FilterRBJNX.h"

// Was just a test
#define FIX_BANDPASS 0 //1

FilterRBJNX::FilterRBJNX(int numFilters,
                       int type,
                       BL_FLOAT sampleRate,
                       BL_FLOAT cutoffFreq,
                       BL_FLOAT QFactor,
                       BL_FLOAT gain)
{
    mNumFilters = numFilters;
    mType = type;
    mSampleRate = sampleRate;
    mCutoffFreq = cutoffFreq;
    mQFactor = QFactor;
    mGain = gain;
    
    for (int i = 0; i < numFilters; i++)
    {
        CFxRbjFilter *filter = new CFxRbjFilter();
        mFilters.push_back(filter);
    }
    
    CalcFilterCoeffs();
}

FilterRBJNX::FilterRBJNX(const FilterRBJNX &other)
{
    mNumFilters = other.mNumFilters;
    mType = other.mType;
    mSampleRate = other.mSampleRate;
    mCutoffFreq = other.mCutoffFreq;
    mQFactor = other.mQFactor;
    mGain = other.mGain;
    
    for (int i = 0; i < mNumFilters; i++)
    {
        CFxRbjFilter *filter = new CFxRbjFilter();
        mFilters.push_back(filter);
    }
    
    CalcFilterCoeffs();
}

FilterRBJNX::~FilterRBJNX()
{
    for (int i = 0; i < mFilters.size(); i++)
    {
        CFxRbjFilter *filter = mFilters[i];
        delete filter;
    }
}

void
FilterRBJNX::SetCutoffFreq(BL_FLOAT freq)
{
    mCutoffFreq = freq;
    
    CalcFilterCoeffs();
}

void
FilterRBJNX::SetQFactor(BL_FLOAT q)
{
    mQFactor = q;
    
    CalcFilterCoeffs();
}

void
FilterRBJNX::SetSampleRate(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    
    CalcFilterCoeffs();
}

BL_FLOAT
FilterRBJNX::Process(BL_FLOAT sample)
{
    // FIX_FLT_DENORMAL(sample);
    for (int i = 0; i < mFilters.size(); i++)
    {
        CFxRbjFilter *filter = mFilters[i];
        sample = filter->filter(sample);
        
        //FIX_FLT_DENORMAL(sample);
    }
    
    return sample;
}

void
FilterRBJNX::CalcFilterCoeffs()
{
    // For flat crossover
    //BL_FLOAT QFactor = 0.707; // For crossover
    BL_FLOAT QFactor = mQFactor;
    
    // NOTE: set to != 0 for shelves
    //BL_FLOAT dbGain = 0.0; // For crossover
    BL_FLOAT dbGain = mGain;
    bool qIsBandwidth = false;
    
#if FIX_BANDPASS
    if ((mType == FILTER_TYPE_BANDPASS_CSG) ||
        (mType == FILTER_TYPE_BANDPASS_CZPG))
    {
        qIsBandwidth = true;
    }
#endif
    
    for (int i = 0; i < mFilters.size(); i++)
    {
        CFxRbjFilter *filter = mFilters[i];
        filter->calc_filter_coeffs(mType, mCutoffFreq, mSampleRate,
                                  QFactor, dbGain, qIsBandwidth);
    }
}

#if 0
void
FilterRBJNX::Process(WDL_TypedBuf<BL_FLOAT> *result,
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
FilterRBJNX::Process(WDL_TypedBuf<BL_FLOAT> *ioSamples)
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
FilterRBJNX::Process(WDL_TypedBuf<BL_FLOAT> *ioSamples)
{
    // FIX_FLT_DENORMAL(sample);
    for (int i = 0; i < mFilters.size(); i++)
    {
        CFxRbjFilter *filter = mFilters[i];
        filter->filter(ioSamples->Get(), ioSamples->GetSize());
        
        //FIX_FLT_DENORMAL(sample);
    }
}
#endif

#if 0
void
FilterRBJNX::Process(WDL_TypedBuf<BL_FLOAT> *samples)
{
    WDL_TypedBuf<BL_FLOAT> copy = *samples;
    Process(samples, copy);
}
#endif
