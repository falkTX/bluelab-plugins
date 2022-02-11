//
//  USTCorrelationComputer4.cpp
//  UST
//
//  Created by applematuer on 1/2/20.
//
//


#include "USTCorrelationComputer4.h"

USTCorrelationComputer4::USTCorrelationComputer4(BL_FLOAT sampleRate,
                                                 BL_FLOAT smoothTimeMs)
{
    mSampleRate = sampleRate;
    mSmoothTimeMs = smoothTimeMs;
    
    Reset(sampleRate);
}

USTCorrelationComputer4::~USTCorrelationComputer4() {}

void
USTCorrelationComputer4::Reset(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    mCorrelation = 0.0;
    
    mHistorySize = mSmoothTimeMs*0.001*mSampleRate;
    
    mXLXR.clear();
    mXL2.clear();
    mXR2.clear();
    
    // Fill the histories with zeros
    mXLXR.resize(mHistorySize);
    mXL2.resize(mHistorySize);
    mXR2.resize(mHistorySize);
    
    for (int i = 0; i < mHistorySize; i++)
    {
        mXLXR[i] = 0.0;
        mXL2[i] = 0.0;
        mXR2[i] = 0.0;
    }
    
    mSumXLXR = 0.0;
    mSumXL2 = 0.0;
    mSumXR2 = 0.0;
}

void
USTCorrelationComputer4::Reset()

{
    Reset(mSampleRate);
}

void
USTCorrelationComputer4::Process(const WDL_TypedBuf<BL_FLOAT> samples[2])
{
    // Fill the history
    for (int i = 0; i < samples[0].GetSize(); i++)
    {
        BL_FLOAT l = samples[0].Get()[i];
        BL_FLOAT r = samples[1].Get()[i];

        BL_FLOAT xLxR = l*r;
        mXLXR.push_back(xLxR);
        
        BL_FLOAT xL2 = l*l;
        mXL2.push_back(xL2);
        
        BL_FLOAT xR2 = r*r;
        mXR2.push_back(xR2);
        
        if (mXLXR.size() >= 2)
        {
            mSumXLXR += mXLXR[mXLXR.size() - 1] - mXLXR[0];
            mSumXL2 += mXL2[mXL2.size() - 1] - mXL2[0];
            mSumXR2 += mXR2[mXR2.size() - 1] - mXR2[0];
        }
        
        while(mXLXR.size() > mHistorySize)
            mXLXR.pop_front();
        while(mXL2.size() > mHistorySize)
            mXL2.pop_front();
        while(mXR2.size() > mHistorySize)
            mXR2.pop_front();
    }
    
    // Compute the expectation (aka the averages)
    BL_FLOAT ExLxR = 0.0;
    BL_FLOAT ExL2 = 0.0;
    BL_FLOAT ExR2 = 0.0;
    
    if (mXLXR.size() > 0.0)
    {
        ExLxR = mSumXLXR/mXLXR.size();
        ExL2 = mSumXL2/mXL2.size();
        ExR2 = mSumXR2/mXR2.size();
    }
    
    // Compute the correlation
    BL_FLOAT corr = 0.0;
    if (ExL2*ExR2 > 0.0)
        corr = ExLxR/sqrt(ExL2*ExR2);
    
    mCorrelation = corr;
}

void
USTCorrelationComputer4::Process(BL_FLOAT l, BL_FLOAT r)
{
    // Fill the history
    BL_FLOAT xLxR = l*r;
    mXLXR.push_back(xLxR);
        
    BL_FLOAT xL2 = l*l;
    mXL2.push_back(xL2);
        
    BL_FLOAT xR2 = r*r;
    mXR2.push_back(xR2);
    
    if (mXLXR.size() >= 2)
    {
        mSumXLXR += mXLXR[mXLXR.size() - 1] - mXLXR[0];
        mSumXL2 += mXL2[mXL2.size() - 1] - mXL2[0];
        mSumXR2 += mXR2[mXR2.size() - 1] - mXR2[0];
    }
    
    while(mXLXR.size() > mHistorySize)
        mXLXR.pop_front();
    while(mXL2.size() > mHistorySize)
        mXL2.pop_front();
    while(mXR2.size() > mHistorySize)
        mXR2.pop_front();
    
    // Compute the expectation (aka the averages)
    BL_FLOAT ExLxR = 0.0;
    BL_FLOAT ExL2 = 0.0;
    BL_FLOAT ExR2 = 0.0;
    
    if (mXLXR.size() > 0.0)
    {
        ExLxR = mSumXLXR/mXLXR.size();
        ExL2 = mSumXL2/mXL2.size();
        ExR2 = mSumXR2/mXR2.size();
    }
    
    // Compute the correlation
    BL_FLOAT corr = 0.0;
    if (ExL2*ExR2 > 0.0)
        corr = ExLxR/sqrt(ExL2*ExR2);
    
    mCorrelation = corr;
}

BL_FLOAT
USTCorrelationComputer4::GetCorrelation()
{
    return mCorrelation;
}

BL_FLOAT
USTCorrelationComputer4::GetSmoothWindowMs()
{
    return mSmoothTimeMs;
}
