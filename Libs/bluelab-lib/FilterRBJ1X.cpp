//
//  FilterRBJ1X.cpp
//  UST
//
//  Created by applematuer on 8/25/19.
//
//

#include <BLUtils.h>

#include "FilterRBJ1X.h"

// Without this, in UST, the horizontal line indicating the sum of the filters
// makes oscillations
#define FIX_RBJ_ALLPASS 0 //1

FilterRBJ1X::FilterRBJ1X(int type,
                         BL_FLOAT sampleRate,
                         BL_FLOAT cutoffFreq)
{
    mType = type;
    mSampleRate = sampleRate;
    mCutoffFreq = cutoffFreq;
    
    mQFactor = 0.707;
    
    mFilter = new CFxRbjFilter();
    
    CalcFilterCoeffs();
}

FilterRBJ1X::FilterRBJ1X(const FilterRBJ1X &other)
{
    mType = other.mType;
    mSampleRate = other.mSampleRate;
    mCutoffFreq = other.mCutoffFreq;

    mQFactor = 0.707;
    
    mFilter = new CFxRbjFilter();
    
    CalcFilterCoeffs();
}

FilterRBJ1X::~FilterRBJ1X()
{
    delete mFilter;
}

void
FilterRBJ1X::SetCutoffFreq(BL_FLOAT freq)
{
    mCutoffFreq = freq;
    
    CalcFilterCoeffs();
}

void
FilterRBJ1X::SetSampleRate(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    
    CalcFilterCoeffs();
}

void
FilterRBJ1X::SetQFactor(BL_FLOAT q)
{
    mQFactor = q;
    
    CalcFilterCoeffs();
}

BL_FLOAT
FilterRBJ1X::Process(BL_FLOAT sample)
{
    //FIX_FLT_DENORMAL(sample);
    
    sample = mFilter->filter(sample);
    
    //FIX_FLT_DENORMAL(sample)
    
    return sample;
}

void
FilterRBJ1X::CalcFilterCoeffs()
{
    // For flat crossover
    //BL_FLOAT QFactor = 0.707;
    BL_FLOAT QFactor = mQFactor;
    
    BL_FLOAT dbGain = 0.0;
    bool qIsBandwidth = false;
    
#if FIX_RBJ_ALLPASS
    if (mType == FILTER_TYPE_ALLPASS)
        qIsBandwidth = true;
#endif
    
    mFilter->calc_filter_coeffs(mType, mCutoffFreq, mSampleRate,
                                QFactor, dbGain, qIsBandwidth);
}

void
FilterRBJ1X::Process(WDL_TypedBuf<BL_FLOAT> *ioSamples)
{
    //FIX_FLT_DENORMAL(sample);
    
    mFilter->filter(ioSamples->Get(), ioSamples->GetSize());
    
    //FIX_FLT_DENORMAL(sample)
}
