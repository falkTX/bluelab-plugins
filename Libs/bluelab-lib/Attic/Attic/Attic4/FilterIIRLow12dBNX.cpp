//
//  FilterIIRLow12dBNX.cpp
//  UST
//
//  Created by applematuer on 8/10/20.
//
//

#include <FilterIIRLow12dB.h>

#include "FilterIIRLow12dBNX.h"

FilterIIRLow12dBNX::FilterIIRLow12dBNX(int numFilters)
{
    mFilters.resize(numFilters);
    for (int i = 0; i < mFilters.size(); i++)
    {
        mFilters[i] = new FilterIIRLow12dB();
    }
    
    Init(400.0, 44100.0);
}

FilterIIRLow12dBNX::~FilterIIRLow12dBNX()
{
    for (int i = 0; i < mFilters.size(); i++)
        delete mFilters[i];
}

void
FilterIIRLow12dBNX::Init(BL_FLOAT resoFreq, BL_FLOAT sampleRate)
{
    for (int i = 0; i < mFilters.size(); i++)
    {
        FilterIIRLow12dB *filter = mFilters[i];
        
        filter->Init(resoFreq, sampleRate);
    }
}

BL_FLOAT
FilterIIRLow12dBNX::Process(BL_FLOAT sample)
{
    for (int i = 0; i < mFilters.size(); i++)
    {
        FilterIIRLow12dB *filter = mFilters[i];
        
        sample = filter->Process(sample);
    }
    
    return sample;
}

void
FilterIIRLow12dBNX::Process(WDL_TypedBuf<BL_FLOAT> *result,
                           const WDL_TypedBuf<BL_FLOAT> &samples)
{
    result->Resize(samples.GetSize());
    
    for (int i = 0; i < samples.GetSize(); i++)
    {
        BL_FLOAT sample = samples.Get()[i];
        
        for (int j = 0; j < mFilters.size(); j++)
        {
            FilterIIRLow12dB *filter = mFilters[j];
            
            sample = filter->Process(sample);
        }
        
        result->Get()[i] = sample;
    }
}
