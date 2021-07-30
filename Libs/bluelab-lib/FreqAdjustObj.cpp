//
//  FreqAdjustObj.cpp
//  PitchShift
//
//  Created by Apple m'a Tuer on 30/10/17.
//
//

#include <stdlib.h>
#include <memory.h>
#include <math.h>

#include <BLUtils.h>
#include <BLUtilsMath.h>

#include "FreqAdjustObj.h"


FreqAdjustObj::FreqAdjustObj(int bufferSize, int oversampling, BL_FLOAT sampleRate)
{
    mBufferSize = bufferSize;
    mOversampling = oversampling;
    mSampleRate = sampleRate;
    
    Reset();
}

FreqAdjustObj::~FreqAdjustObj() {}

void
FreqAdjustObj::Reset()
{
    mLastPhases.Resize(mBufferSize/2 + 1);
        
    for (int i = 0; i < mLastPhases.GetSize(); i++)
        mLastPhases.Get()[i] = 0.0; //ioPhases.Get()[i];
    
    mSumPhases.Resize(mBufferSize/2 + 1);
        
    for (int i = 0; i < mSumPhases.GetSize(); i++)
        mSumPhases.Get()[i] = 0.0;
}

void
FreqAdjustObj::ComputeRealFrequencies(const WDL_TypedBuf<BL_FLOAT> &ioPhases,
                                      WDL_TypedBuf<BL_FLOAT> *outRealFreqs)
{
    outRealFreqs->Resize(mBufferSize/2 + 1);
    
    // Oversamplig factor for fft
    // e.g: 2 for an overlap of 50%
    BL_FLOAT osamp = mOversampling;
    
    BL_FLOAT stepSize = ((BL_FLOAT)ioPhases.GetSize())/osamp;
    BL_FLOAT expct = 2.0*M_PI*(BL_FLOAT)stepSize/(BL_FLOAT)ioPhases.GetSize();
    BL_FLOAT freqPerBin = mSampleRate/(BL_FLOAT)ioPhases.GetSize();
    
    for (int i = 0; i <= ioPhases.GetSize()/2; i++) // "<=" to cover to not miss the last value
    {
        BL_FLOAT phase = ioPhases.Get()[i];
        
        /* compute phase difference */
        BL_FLOAT tmp = phase - mLastPhases.Get()[i];
        
#if 0 // Doesn't work for leakage
        // See: http://blogs.zynaptiq.com/bernsee/pitch-shifting-using-the-ft/#comment-138
        // When using smbFft, this is exactly the same thing ! 
        tmp = -tmp;
#endif
        
        mLastPhases.Get()[i] = phase;
        
        /* subtract expected phase difference */
        tmp -= (BL_FLOAT)i*expct;
        
        // Exactly same effect as above
        tmp = MapToPi(tmp);
        
        /* get deviation from bin frequency from the +/- Pi interval */
        tmp = osamp*tmp/(2.*M_PI);
        
        /* compute the k-th partials' true frequency */
        tmp = (BL_FLOAT)i*freqPerBin + tmp*freqPerBin;
        
        /* store true frequency in analysis arrays */
        outRealFreqs->Get()[i] = tmp;
    }
}

void
FreqAdjustObj::ComputePhases(WDL_TypedBuf<BL_FLOAT> *ioPhases, const WDL_TypedBuf<BL_FLOAT> &realFreqs)
{
    // Oversamplig factor for fft
    // e.g: 2 for an overlap of 50%
    BL_FLOAT osamp = mOversampling;
    
    //BL_FLOAT osamp = ioPhases->GetSize();
    BL_FLOAT stepSize = ((BL_FLOAT)ioPhases->GetSize())/osamp;
    BL_FLOAT expct = 2.0*M_PI*(BL_FLOAT)stepSize/(BL_FLOAT)ioPhases->GetSize();
    BL_FLOAT freqPerBin = mSampleRate/(BL_FLOAT)ioPhases->GetSize();
    
    for (int i = 0; i <= ioPhases->GetSize()/2; i++) // <= !!
    {
        /* get true frequency from synthesis arrays */
        BL_FLOAT tmp = realFreqs.Get()[i];
        
        /* subtract bin mid frequency */
        tmp -= (BL_FLOAT)i*freqPerBin;
        
        /* get bin deviation from freq deviation */
        tmp /= freqPerBin;
        
        /* take osamp into account */
        tmp = 2.*M_PI*tmp/osamp;
        
        /* add the overlap phase advance back in */
        tmp += (BL_FLOAT)i*expct;
        
#if 0 // Can be uesed for debugging
        tmp = MapToPi(tmp);
#endif
        
        /* accumulate delta phase to get bin phase */
        mSumPhases.Get()[i] += tmp;
        BL_FLOAT phase = mSumPhases.Get()[i];
        
        ioPhases->Get()[i] = phase;
    }
}

BL_FLOAT
FreqAdjustObj::MapToPi(BL_FLOAT val)
{
    /* map delta phase into +/- Pi interval */
    long qpd = val/M_PI;
    if (qpd >= 0)
        qpd += qpd&1;
    else
        qpd -= qpd&1;

    val -= M_PI*(BL_FLOAT)qpd;
    
    return val;
}
