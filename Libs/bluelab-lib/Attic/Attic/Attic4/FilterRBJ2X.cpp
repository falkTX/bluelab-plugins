//
//  FilterRBJ2X.cpp
//  UST
//
//  Created by applematuer on 8/25/19.
//
//

#include <BLUtils.h>

#include "FilterRBJ2X.h"


FilterRBJ2X::FilterRBJ2X(int type,
                         BL_FLOAT sampleRate,
                         BL_FLOAT cutoffFreq)
{
    mType = type;
    mSampleRate = sampleRate;
    mCutoffFreq = cutoffFreq;
    
    mQFactor = 0.707;
    
    mFilters[0] = new CFxRbjFilter();
    mFilters[1] = new CFxRbjFilter();
    
    CalcFilterCoeffs();
}

FilterRBJ2X::FilterRBJ2X(const FilterRBJ2X &other)
{
    mType = other.mType;
    mSampleRate = other.mSampleRate;
    mCutoffFreq = other.mCutoffFreq;
    
    mFilters[0] = new CFxRbjFilter();
    mFilters[1] = new CFxRbjFilter();
    
    CalcFilterCoeffs();
}

FilterRBJ2X::~FilterRBJ2X()
{
    delete mFilters[0];
    delete mFilters[1];
}

void
FilterRBJ2X::SetCutoffFreq(BL_FLOAT freq)
{
    mCutoffFreq = freq;
    
    CalcFilterCoeffs();
}

void
FilterRBJ2X::SetSampleRate(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    
    CalcFilterCoeffs();
}

void
FilterRBJ2X::SetQFactor(BL_FLOAT q)
{
    mQFactor = q;
    
    CalcFilterCoeffs();
}

BL_FLOAT
FilterRBJ2X::Process(BL_FLOAT sample)
{
    //FIX_FLT_DENORMAL(sample);
    
    sample = mFilters[0]->filter(sample);
    
    //FIX_FLT_DENORMAL(sample)
    
    sample = mFilters[1]->filter(sample);
    
    //FIX_FLT_DENORMAL(sample)
    
    return sample;
}

void
FilterRBJ2X::CalcFilterCoeffs()
{
    // For flat crossover
    //BL_FLOAT QFactor = 0.707;
    BL_FLOAT QFactor = mQFactor;
    
    BL_FLOAT dbGain = 0.0;
    bool qIsBandwidth = false;
    
    for (int i = 0; i < 2; i++)
        mFilters[i]->calc_filter_coeffs(mType, mCutoffFreq, mSampleRate,
                                        QFactor, dbGain, qIsBandwidth);
}

#if 0
void
FilterRBJ2X::Process(WDL_TypedBuf<BL_FLOAT> *result,
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
FilterRBJ2X::Process(WDL_TypedBuf<BL_FLOAT> *ioSamples)
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
FilterRBJ2X::Process(WDL_TypedBuf<BL_FLOAT> *ioSamples)
{
    //FIX_FLT_DENORMAL(sample);
    
    mFilters[0]->filter(ioSamples->Get(), ioSamples->GetSize());
    
    //FIX_FLT_DENORMAL(sample)
    
    mFilters[1]->filter(ioSamples->Get(), ioSamples->GetSize());
    
    //FIX_FLT_DENORMAL(sample)
}
#endif
