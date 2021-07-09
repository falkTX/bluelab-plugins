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

#include "FreqAdjustObj3.h"

#include "resource.h"

#define BILL_FARMER_HACK 0

// BAD
// With that activated, the stereo phase diagram
// becomes and stays strange when we move the pitch factor
// In this case, to retrieve a good state, the playback must be restarted
#define USE_REINIT_PHASES 0

// Just for testing
// Set to 0 at the origin
// Need to be unit-tested
// Does't seem to really optimize
#define USE_SMB_MAPTOPI 0

#define M_PI_INV 0.318309886183791

FreqAdjustObj3::FreqAdjustObj3(int bufferSize, int overlapping, int oversampling,
                               BL_FLOAT sampleRate)
{
    mMustSetLastPhases = true;
    
    Reset(bufferSize, overlapping, oversampling, sampleRate);
}

FreqAdjustObj3::~FreqAdjustObj3() {}

void
FreqAdjustObj3::Reset(int bufferSize, int overlapping, int oversampling,
                      BL_FLOAT sampleRate)
{
    mBufferSize = bufferSize;
    mOverlapping = overlapping;
    mOversampling = oversampling;
    mSampleRate = sampleRate;
    
    mLastPhases.Resize(mBufferSize/2);
    
    for (int i = 0; i < mLastPhases.GetSize(); i++)
        mLastPhases.Get()[i] = 0.0;
    
    mSumPhases.Resize(mBufferSize/2);
    
    for (int i = 0; i < mSumPhases.GetSize(); i++)
        mSumPhases.Get()[i] = 0.0;
    
    // Makes negative frequencies...
#if BAD_COMPUTE_INITIAL_PHASES
    // Good ! For having correct frequencies from ComputeRealFrequencies()
    // Doesn't work for overlapping = 32
    ComputeInitialLastPhases();
    
    ComputeInitialSumPhases();
#endif
    
#if USE_REINIT_PHASES
    mMustSetLastPhases = true;
#endif
}

#if DEBUG_STEREO_TEST
void
FreqAdjustObj3::SetPhases(const WDL_TypedBuf<BL_FLOAT> &phases,
                          const WDL_TypedBuf<BL_FLOAT> &prevPhases)
{
#if 0 // orig
    mLastPhases = phases;
    
    WDL_TypedBuf<BL_FLOAT> diff = phases;
    BLUtils::SubstractValues(&diff, prevPhases);
    
    BLUtils::SubstractValues(&mSumPhases, diff);
#endif
    
#if 1 // reversed (works with identity !!)
    mSumPhases = phases;
    
    WDL_TypedBuf<BL_FLOAT> &diff = mTmpBuf0;
    diff = phases;
    BLUtils::SubstractValues(&diff, prevPhases);
    
    BLUtils::SubstractValues(&mLastPhases, diff);
#endif
}
#endif


#define DEBUG_ADJUST 0
#define DEBUG_NUM "ZAR"

void
FreqAdjustObj3::ComputeRealFrequencies(const WDL_TypedBuf<BL_FLOAT> &ioPhases,
                                       WDL_TypedBuf<BL_FLOAT> *outRealFreqs)
{
    // TEST
    if (ioPhases.GetSize() != mLastPhases.GetSize())
        return;
    // Will be set just after... //??
    
#if USE_REINIT_PHASES
    if (mMustSetLastPhases)
    {
        // Copy the first half
        mLastPhases.Resize(ioPhases.GetSize()/*/2*/);
        for (int i = 0; i < mLastPhases.GetSize(); i++)
            mLastPhases.Get()[i] = ioPhases.Get()[i];
        
        mMustSetLastPhases = false;
    }
#endif
    
    outRealFreqs->Resize(mLastPhases.GetSize());
    
    BL_FLOAT osamp = mOverlapping*mOversampling;
    
    // Doesn't work ...
    // This avoids negative frequencies, and makes identity in case of tempered !
    // 
    // See: https://github.com/JorenSix/TarsosDSP/blob/master/src/core/be/tarsos/dsp/PitchShifter.java
    //BL_FLOAT osamp = ((BL_FLOAT)mBufferSize)/(mBufferSize - mBufferSize/mOverlapping);
    
    BL_FLOAT expct = 2.0*M_PI/osamp;
    BL_FLOAT expctInv = 1.0/expct;
    
    BL_FLOAT freqPerBin = mSampleRate/(BL_FLOAT)mBufferSize;
    
#if DEBUG_ADJUST
    BLDebug::DumpPhases("step0-0-"DEBUG_NUM".txt", ioPhases.Get(), numPhases);
    
    WDL_TypedBuf<BL_FLOAT> step1;
    WDL_TypedBuf<BL_FLOAT> step2;
    WDL_TypedBuf<BL_FLOAT> step3;
    WDL_TypedBuf<BL_FLOAT> step4;
#endif
    
    for (int i = 0; i < mLastPhases.GetSize()/*numPhases*/; i++)
    {
        BL_FLOAT phase = ioPhases.Get()[i];
        
        /* compute phase difference */
        BL_FLOAT dphase = phase - mLastPhases.Get()[i];
        mLastPhases.Get()[i] = phase;
        
        BL_FLOAT tmp = dphase;
        
#if BILL_FARMER_HACK
        // See: http://blogs.zynaptiq.com/bernsee/pitch-shifting-using-the-ft/#comment-138
        tmp = -tmp;
#endif
        
#if DEBUG_ADJUST
        step1.Add(tmp);
#endif
        
        /* subtract expected phase difference */
        tmp -= (BL_FLOAT)i*expct;
        
#if DEBUG_ADJUST
        step2.Add(tmp);
#endif
        
        // Must keep this !
        
#if 0
        tmp = MapToPi2(tmp);
#endif
        
#if 1
        
#if !USE_SMB_MAPTOPI
        tmp = MapToPi(tmp);
#else
        tmp = SMB_MapToPi(tmp);
#endif
        
#endif
        
#if DEBUG_ADJUST
        step3.Add(tmp);
#endif
        
        /* get deviation from bin frequency from the +/- Pi interval */
        //tmp /= expct;
        tmp *= expctInv;
        
#if DEBUG_ADJUST
        step4.Add(tmp);
#endif
        
        /* compute the k-th partials' true frequency */
        tmp = ((BL_FLOAT)i + tmp)*freqPerBin;
        
        /* store true frequency in analysis arrays */
        outRealFreqs->Get()[i] = tmp;
    }
    
#if DEBUG_ADJUST
    BLDebug::DumpData("step0-1-"DEBUG_NUM".txt", step1.Get(), step1.GetSize());
    BLDebug::DumpData("step0-2-"DEBUG_NUM".txt", step2.Get(), step2.GetSize());
    BLDebug::DumpData("step0-3-"DEBUG_NUM".txt", step3.Get(), step3.GetSize());
    BLDebug::DumpData("step0-4-"DEBUG_NUM".txt", step4.Get(), step4.GetSize());
    
    BLDebug::DumpData("step0-5-"DEBUG_NUM".txt", outRealFreqs->Get(), outRealFreqs->GetSize());
#endif
}

void
FreqAdjustObj3::ComputePhases(WDL_TypedBuf<BL_FLOAT> *outPhases,
                              const WDL_TypedBuf<BL_FLOAT> &realFreqs)
{
    if (realFreqs.GetSize() != mSumPhases.GetSize())
        return;
    
    outPhases->Resize(mSumPhases.GetSize());
    
    BL_FLOAT osamp = mOverlapping*mOversampling;
    BL_FLOAT expct = 2.0*M_PI/osamp;
    BL_FLOAT freqPerBin = mSampleRate/(BL_FLOAT)mBufferSize;
    
#if DEBUG_ADJUST
    BLDebug::DumpData("step1-5-"DEBUG_NUM".txt", realFreqs.Get(), realFreqs.GetSize());
    
    WDL_TypedBuf<BL_FLOAT> step4;
    WDL_TypedBuf<BL_FLOAT> step3;
    WDL_TypedBuf<BL_FLOAT> step1;
    WDL_TypedBuf<BL_FLOAT> step2;
#endif
    
    for (int i = 0; i < mSumPhases.GetSize(); i++)
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
        
#if BILL_FARMER_HACK
        tmp = -tmp;
#endif
        
        /* accumulate delta phase to get bin phase */
        mSumPhases.Get()[i] = mSumPhases.Get()[i] + tmp;
        BL_FLOAT phase = mSumPhases.Get()[i];
        
        // Niko: better to have "standard" phase bounds
        // (useful for debugging)
        // (not changing anything in the resulting sound !)
#if !USE_SMB_MAPTOPI
        phase = MapToPi(phase);
#else
        phase = SMB_MapToPi(phase);
#endif

        outPhases->Get()[i] = phase;
    }
    
#if DEBUG_ADJUST
    BLDebug::DumpData("step1-4-"DEBUG_NUM".txt", step4.Get(), step4.GetSize());
    BLDebug::DumpData("step1-3-"DEBUG_NUM".txt", step3.Get(), step3.GetSize());
    BLDebug::DumpData("step1-2-"DEBUG_NUM".txt", step2.Get(), step2.GetSize());
    BLDebug::DumpData("step1-1-"DEBUG_NUM".txt", step1.Get(), step1.GetSize());
    
    BLDebug::DumpData("step1-0-"DEBUG_NUM".txt", ioPhases->Get(), numPhases);
    
    exit(0);
#endif
}

#if BAD_COMPUTE_INITIAL_PHASES
void
FreqAdjustObj3::ComputeInitialLastPhases()
{
    BL_FLOAT osamp = mOverlapping*mOversampling;
    BL_FLOAT expct = 2.0*M_PI/osamp;
    
    for (int i = 0; i < mLastPhases.GetSize(); i++)
    {
        // Subtract expected phase difference
        mLastPhases.Get()[i] = -i*expct;
    }
}

void
FreqAdjustObj3::ComputeInitialSumPhases()
{
    BL_FLOAT osamp = mOverlapping*mOversampling;
    BL_FLOAT expct = 2.0*M_PI/osamp;
    
    for (int i = 0; i < mSumPhases.GetSize(); i++)
    {
        // Subtract expected phase difference
        mSumPhases.Get()[i] = -i*expct;
    }
}
#endif

BL_FLOAT
FreqAdjustObj3::SMB_MapToPi(BL_FLOAT val)
{ 
    /* map delta phase into +/- Pi interval */
    //long qpd = val/M_PI;
    long qpd = val*M_PI_INV;
    
    if (qpd >= 0)
        qpd += qpd&1;
    else
        qpd -= qpd&1;

    val -= M_PI*(BL_FLOAT)qpd;
    
    return val;
}

#if 0 // Origin
// With this one, we will prefer +PI to -PI is we have the choice
// (to avoid negative frequencies)
BL_FLOAT
FreqAdjustObj3::MapToPi(BL_FLOAT val)
{
    /* Map delta phase into +/- Pi interval */
    val =  fmod(val, 2.0*M_PI);
    if (val <= -M_PI)
        val += 2.0*M_PI;
    if (val > M_PI)
        val -= 2.0*M_PI;
    
    return val;
}
#endif

#if 1 // Optim
// With this one, we will prefer +PI to -PI is we have the choice
// (to avoid negative frequencies)
BL_FLOAT
FreqAdjustObj3::MapToPi(BL_FLOAT val)
{
    /* Map delta phase into +/- Pi interval */
    val =  fmod(val, TWO_PI);
    if (val <= -M_PI)
        val += TWO_PI;
    if (val > M_PI)
        val -= TWO_PI;
    
    return val;
}
#endif

// Between 0 and 2 PI
// With that, we don't have negative frequencies anymore !
// And with over32, we have identity !
BL_FLOAT
FreqAdjustObj3::MapToPi2(BL_FLOAT val)
{
    /* Map delta phase into +/- Pi interval */
    val =  fmod(val, 2.0*M_PI);
    if (val < 0.0)
        val += 2.0*M_PI;
    if (val > 2.0*M_PI)
        val -= 2.0*M_PI;
    
    return val;
}

#if 0
// The two versions are not exactly the same
// at the bounds
// < / <= etc.
//
// This makes a difference when viewing spectrograms with Quality 1
// (some new horizontal frequency lines appear)
//
static void
DebugTestMapToPi()
{
#define NUM_VALUES 1024
        
#define MIN_BOUND -4.0*M_PI
#define MAX_BOUND 4.0*M_PI
        
        for (int i = 0; i < NUM_VALUES; i++)
        {
            BL_FLOAT t = ((BL_FLOAT)i)/NUM_VALUES;
            
            BL_FLOAT val = MIN_BOUND + t*(MAX_BOUND - MIN_BOUND);
            
            BLDebug::AppendValue("vals.txt", val);
            
            BL_FLOAT my = MapToPi(val);
            BLDebug::AppendValue("my.txt", my);
            
            BL_FLOAT smb = SMB_MapToPi(val);
            BLDebug::AppendValue("smb.txt", smb);
        }
        
        exit(0);
}
#endif
