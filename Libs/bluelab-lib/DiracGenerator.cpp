//
//  DiracGenerator.cpp
//  Impulse
//
//  Created by Pan on 04/12/17.
//
//

#include "DiracGenerator.h"

// Generate the first dirac just at the beginning,
// or after having waited for the first interval?
#define FIRST_SAMPLE_IS_DIRAC 1

DiracGenerator::DiracGenerator(BL_FLOAT sampleRate, BL_FLOAT frequency,
                               BL_FLOAT value,
                               long sampleLatency)
{
    mSampleRate = sampleRate;
    
    mFrequency = frequency;
    
    mValue = value;
    
    mSampleLatency = sampleLatency;
    
    mSampleNum = -mSampleLatency;
    
    mFirstDiracGenerated = false;
}

DiracGenerator::~DiracGenerator() {}

int
DiracGenerator::Process(BL_FLOAT *outSamples, int numSamples)
{
    int res = -1;
    for (int i = 0; i < numSamples; i++)
    {
        outSamples[i] = 0.0;

#if !FIRST_SAMPLE_IS_DIRAC
        if (mSampleNum >= mSampleRate/mFrequency)
        {
            mSampleNum = 0;
            
            // Dirac !
            outSamples[i] = mValue;
            
            res = i;
            
            mFirstDiracGenerated = true;
        }
#else
        if (mSampleNum >= mSampleRate/mFrequency)
            mSampleNum = 0;

        if (mSampleNum == 0)
        {
            // Dirac !
            outSamples[i] = mValue;
            
            res = i;
            
            mFirstDiracGenerated = true;
        }
#endif
        
        mSampleNum++;
    }
    
    return res;
}

void
DiracGenerator::Reset(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    
    mSampleNum = -mSampleLatency;
    
    mFirstDiracGenerated = false;
}

void
DiracGenerator::SetFrequency(BL_FLOAT frequency)
{
    mFrequency = frequency;
    
    Reset(mSampleRate);
}

bool
DiracGenerator::FirstDiracGenerated()
{
    return mFirstDiracGenerated;
}
