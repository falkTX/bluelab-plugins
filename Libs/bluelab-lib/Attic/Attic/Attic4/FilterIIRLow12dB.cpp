//
//  FilterIIRLow12dB.cpp
//  BL-Infra
//
//  Created by applematuer on 7/9/19.
//
//

#include <math.h>

#include "FilterIIRLow12dB.h"

#ifndef M_PI
#define M_PI 3.1415926535897932384
#endif

// GOOD
// In Infra, avoid clipping the sub octave frequency
#define SKIP_CLIP 1

// Optimize a very little by avoiding the function call in the loop
#define OPTIM_FUN_CALL 1

// See: http://www.musicdsp.org/en/latest/Filters/27-resonant-iir-lowpass-12db-oct.html

FilterIIRLow12dB::FilterIIRLow12dB() {}

FilterIIRLow12dB::~FilterIIRLow12dB() {}

void
FilterIIRLow12dB::Init(BL_FLOAT resoFreq, BL_FLOAT sampleRate)
{
    BL_FLOAT w = 2.0*M_PI*resoFreq/sampleRate; // Pole angle
    BL_FLOAT q = 1.0-w/(2.0*(AMP+0.5/(1.0+w))+w-2.0); // Pole magnitude
    
    mR = q*q;
    mC = mR+1.0-2.0*cos(w)*q;
    mVibraPos = 0.0;
    mVibraSpeed = 0.0;
}

BL_FLOAT
FilterIIRLow12dB::Process(BL_FLOAT sample)
{
    /* Accelerate vibra by signal-vibra, multiplied by lowpasscutoff */
    mVibraSpeed += (sample - mVibraPos) * mC;
    
    /* Add velocity to vibra's position */
    mVibraPos += mVibraSpeed;
    
    /* Attenuate/amplify vibra's velocity by resonance */
    mVibraSpeed *= mR;
    
    // Check clipping
    BL_FLOAT result = mVibraPos;
    
#if !SKIP_CLIP
    if (result < -1.0)
        result = -1.0;
    if (result > 1.0)
        result = 1.0;
#endif
    
    return result;
}

void
FilterIIRLow12dB::Process(WDL_TypedBuf<BL_FLOAT> *result,
                          const WDL_TypedBuf<BL_FLOAT> &samples)
{
    result->Resize(samples.GetSize());
                   
    for (int i = 0; i < samples.GetSize(); i++)
    {
        BL_FLOAT sample = samples.Get()[i];
        
#if !OPTIM_FUN_CALL
        BL_FLOAT sampleRes = Process(sample);
#else
        /* Accelerate vibra by signal-vibra, multiplied by lowpasscutoff */
        mVibraSpeed += (sample - mVibraPos) * mC;
        
        /* Add velocity to vibra's position */
        mVibraPos += mVibraSpeed;
        
        /* Attenuate/amplify vibra's velocity by resonance */
        mVibraSpeed *= mR;
        
        // Check clipping
        BL_FLOAT sampleRes = mVibraPos;
#endif
        
        result->Get()[i] = sampleRes;
    }
}
