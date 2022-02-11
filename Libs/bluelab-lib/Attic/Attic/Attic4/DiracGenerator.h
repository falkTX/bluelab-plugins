//
//  DiracGenerator.h
//  Impulse
//
//  Created by Pan on 04/12/17.
//
//

#ifndef __Impulse__DiracGenerator__
#define __Impulse__DiracGenerator__

#include <BLTypes.h>

class DiracGenerator
{
public:
    DiracGenerator(BL_FLOAT sampleRate, BL_FLOAT frequency, BL_FLOAT value,
                   long sampleLatency);
    
    virtual ~DiracGenerator();
    
    // Return the index where a dirac was generated -1 otherwise
    // Assuming frequency is not too high
    // (i.e 1 dirac generated at the maximum during numSamples).
    int Process(BL_FLOAT *outSamples, int numSamples);
    
    void Reset(BL_FLOAT sampleRate);
    
    void SetFrequency(BL_FLOAT frequency);
    
    bool FirstDiracGenerated();
    
protected:
    BL_FLOAT mSampleRate;
    
    BL_FLOAT mFrequency;
    
    BL_FLOAT mValue;
    
    long mSampleLatency;
    
    long mSampleNum;
    
    bool mFirstDiracGenerated;
};

#endif /* defined(__Impulse__DiracGenerator__) */
