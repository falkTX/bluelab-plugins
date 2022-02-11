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

#include "FreqAdjustObj2.h"


FreqAdjustObj2::FreqAdjustObj2(int bufferSize, int overlapping, int oversampling,
                               BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    
    Reset(bufferSize, overlapping, oversampling);
}

FreqAdjustObj2::~FreqAdjustObj2() {}

void
FreqAdjustObj2::Reset(int bufferSize, int overlapping, int oversampling)
{
    mBufferSize = bufferSize;
    mOverlapping = overlapping;
    mOversampling = oversampling;
    
    mLastPhases.Resize(mBufferSize/2 + 1);
    
    for (int i = 0; i < mLastPhases.GetSize(); i++)
        mLastPhases.Get()[i] = 0.0;
    
    mSumPhases.Resize(mBufferSize/2 + 1);
    
    for (int i = 0; i < mSumPhases.GetSize(); i++)
        mSumPhases.Get()[i] = 0.0;
}

#define DEBUG_ADJUST 0
#define DEBUG_NUM "ZAR"

void
FreqAdjustObj2::ComputeRealFrequencies(const WDL_TypedBuf<BL_FLOAT> &ioPhases,
                                       WDL_TypedBuf<BL_FLOAT> *outRealFreqs)
{
    int numPhases = mBufferSize/2 + 1;
    if (ioPhases.GetSize() < numPhases)
        return;
    
    outRealFreqs->Resize(numPhases);

    BL_FLOAT osamp = mOverlapping*mOversampling;
    BL_FLOAT expct = 2.0*M_PI/osamp;
    BL_FLOAT freqPerBin = mSampleRate/(BL_FLOAT)mBufferSize;
    
#if DEBUG_ADJUST
    BLDebug::DumpPhases("step0-0-"DEBUG_NUM".txt", ioPhases.Get(), numPhases);
    
    WDL_TypedBuf<BL_FLOAT> step1;
    WDL_TypedBuf<BL_FLOAT> step2;
    WDL_TypedBuf<BL_FLOAT> step3;
    WDL_TypedBuf<BL_FLOAT> step4;
#endif
    
    for (int i = 0; i < numPhases; i++)
    {
        BL_FLOAT phase = ioPhases.Get()[i];
        
        /* compute phase difference */
        BL_FLOAT tmp = phase - mLastPhases.Get()[i];
        mLastPhases.Get()[i] = phase;

#if DEBUG_ADJUST
        step1.Add(tmp);
#endif
        
        /* subtract expected phase difference */
        tmp -= (BL_FLOAT)i*expct;
        
#if DEBUG_ADJUST
        step2.Add(tmp);
#endif
        
        // Must keep this !
        tmp = MapToPi(tmp);
        
#if DEBUG_ADJUST
        step3.Add(tmp);
#endif
        
        /* get deviation from bin frequency from the +/- Pi interval */
        tmp /= expct;
        
#if DEBUG_ADJUST
        step4.Add(tmp);
#endif
        
        /* compute the k-th partials' true frequency */
        tmp = (i + tmp)*freqPerBin;
        
        /* store true frequency in analysis arrays */
        outRealFreqs->Get()[i] = tmp;
    }
    
#if DEBUG_ADJUST
    BLDebug::DumpPhases("step0-1-"DEBUG_NUM".txt", step1.Get(), step1.GetSize());
    BLDebug::DumpPhases("step0-2-"DEBUG_NUM".txt", step2.Get(), step2.GetSize());
    BLDebug::DumpPhases("step0-3-"DEBUG_NUM".txt", step3.Get(), step3.GetSize());
    BLDebug::DumpPhases("step0-4-"DEBUG_NUM".txt", step4.Get(), step4.GetSize());
    
    BLDebug::DumpData("step0-5-"DEBUG_NUM".txt", outRealFreqs->Get(), outRealFreqs->GetSize());
#endif
}

void
FreqAdjustObj2::ComputePhases(WDL_TypedBuf<BL_FLOAT> *ioPhases,
                              const WDL_TypedBuf<BL_FLOAT> &realFreqs)
{
    int numPhases = mBufferSize/2 + 1;
    if (ioPhases->GetSize() < numPhases)
        return;
    
    if (realFreqs.GetSize() < numPhases)
        return;
    
#if 1 // ORIGIN
    BL_FLOAT osamp = mOverlapping;
    
    // That was for a fix, to be checked...
    if (mOverlapping > 1)
        osamp *= mOversampling;
#endif
    
    // TEST
    //BL_FLOAT osamp = mOverlapping*mOversampling;
    
    BL_FLOAT expct = 2.0*M_PI/osamp;
    BL_FLOAT freqPerBin = mSampleRate/(BL_FLOAT)mBufferSize;
    
#if DEBUG_ADJUST
    BLDebug::DumpData("step1-5-"DEBUG_NUM".txt", realFreqs.Get(), realFreqs.GetSize());
    
    WDL_TypedBuf<BL_FLOAT> step4;
    WDL_TypedBuf<BL_FLOAT> step3;
    WDL_TypedBuf<BL_FLOAT> step1;
    WDL_TypedBuf<BL_FLOAT> step2;
#endif
    
    for (int i = 0; i < numPhases; i++)
    {
        /* get true frequency from synthesis arrays */
        BL_FLOAT tmp = realFreqs.Get()[i];
      
        /* subtract bin mid frequency and
           get bin deviation from freq deviation */
        tmp = tmp/freqPerBin - i;
        
#if DEBUG_ADJUST
        step4.Add(tmp);
#endif
        
        /* take osamp into account */
        tmp *= expct;

#if DEBUG_ADJUST
        step3.Add(tmp);
#endif
        
        // Here, in the inverse method,
        // was mapped to PI
        
#if DEBUG_ADJUST
        step2.Add(tmp);
#endif
        
        /* add the overlap phase advance back in */
        tmp += (BL_FLOAT)i*expct;
        
#if DEBUG_ADJUST
        step1.Add(tmp);
#endif
        
        /* accumulate delta phase to get bin phase */
        mSumPhases.Get()[i] += tmp;
        BL_FLOAT phase = mSumPhases.Get()[i];
        
        ioPhases->Get()[i] = phase;
    }
    
#if DEBUG_ADJUST
    BLDebug::DumpPhases("step1-4-"DEBUG_NUM".txt", step4.Get(), step4.GetSize());
    BLDebug::DumpPhases("step1-3-"DEBUG_NUM".txt", step3.Get(), step3.GetSize());
    BLDebug::DumpPhases("step1-2-"DEBUG_NUM".txt", step2.Get(), step2.GetSize());
    BLDebug::DumpPhases("step1-1-"DEBUG_NUM".txt", step1.Get(), step1.GetSize());
    
    BLDebug::DumpPhases("step1-0-"DEBUG_NUM".txt", ioPhases->Get(), numPhases);
    
    exit(0);
#endif
}

BL_FLOAT
FreqAdjustObj2::MapToPi(BL_FLOAT val)
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

#if 0 // THAT WAS A TEST
BL_FLOAT
FreqAdjustObj2::MapToPi(BL_FLOAT val)
{
    /* map delta phase into +/- Pi interval */
    val =  fmod(val, 2.0*M_PI);
    if (val < 0.0)
        val += 2.0*M_PI;
    val -= M_PI;
    
    return val;
}
#endif
