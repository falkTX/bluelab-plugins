//
//  DelayLinePhaseShift2.cpp
//  BL-Bat
//
//  Created by applematuer on 12/14/19.
//
//

#include <BLUtils.h>
#include <BLUtilsComp.h>
#include <BLUtilsFft.h>

#include <BLDebug.h>

// For DBG_Test
#include <FftProcessObj16.h>

#include "DelayLinePhaseShift2.h"

#define FIX_CLICK_FROM_ZERO 1

// Use complex multiplication for phase shift instead of
// converting to magns and phases (avoids sin, cos and atan)
#define OPTIM_PHASE_SHIFT_COMPLEX 1

DelayLinePhaseShift2::DelayLinePhaseShift2(BL_FLOAT sampleRate, int bufferSize, int binNum,
                                           BL_FLOAT maxDelay, BL_FLOAT delay)
{
    mSampleRate = sampleRate;
    mBufferSize = bufferSize;
    
    mBinNum = binNum;
    
    mMaxDelay = maxDelay;
    mDelay = delay;
    
    Init();
}

DelayLinePhaseShift2::~DelayLinePhaseShift2() {}

void
DelayLinePhaseShift2::Reset(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    
    Init();
}

#if !OPTIM_PHASE_SHIFT_COMPLEX
WDL_FFT_COMPLEX
DelayLinePhaseShift2::ProcessSample(WDL_FFT_COMPLEX sample)
{
    BL_FLOAT magn = COMP_MAGN(sample);
    BL_FLOAT phase = COMP_PHASE(sample);
    
    phase += mDPhase;
    
    WDL_FFT_COMPLEX result;
    MAGN_PHASE_COMP(magn, phase, result);
    
    return result;
}
#else
// Optimized: avoid sin, cos and atan
WDL_FFT_COMPLEX
DelayLinePhaseShift2::ProcessSample(WDL_FFT_COMPLEX sample)
{
    // "Multiplying by a complex number causes a rescaling by the magnitude and a rotation by the angle."
    // See: https://dsp.stackexchange.com/questions/61236/amplify-a-signal-and-phase-shift-it-by-multiplying-by-a-complex-number
    
    WDL_FFT_COMPLEX result;

    COMP_MULT(sample, mPhaseShiftComplex, result);
    
    return result;
}
#endif

void
DelayLinePhaseShift2::Init()
{
    BL_FLOAT delay = mDelay + mMaxDelay/2.0;
    
    if (delay < 0.0)
    {
        // Error
    }
    
    BL_FLOAT hzPerBin = mSampleRate/mBufferSize;
    BL_FLOAT freq = mBinNum*hzPerBin;

    //BL_FLOAT T = 1.0/freq;
    //BL_FLOAT t = delay/T;
    
    BL_FLOAT t = delay*freq;
    
    mDPhase = t*2.0*M_PI;
    
#if OPTIM_PHASE_SHIFT_COMPLEX
    BL_FLOAT magn = 1.0;
    BL_FLOAT phase = mDPhase;
    
    MAGN_PHASE_COMP(magn, phase, mPhaseShiftComplex);
#endif
}

void
DelayLinePhaseShift2::DBG_Test(int bufferSize, BL_FLOAT sampleRate,
                              const WDL_TypedBuf<WDL_FFT_COMPLEX> &fftSamples)
{
    WDL_TypedBuf<WDL_FFT_COMPLEX> fftSamples0 = fftSamples;
    fftSamples0.Resize(fftSamples0.GetSize()*2);
    BLUtilsFft::FillSecondFftHalf(&fftSamples0);
    
    WDL_TypedBuf<BL_FLOAT> samples0;
    FftProcessObj16::FftToSamples(fftSamples0, &samples0);
    
    BLDebug::DumpData("samples0.txt", samples0);
    
    // Create the delay lines
#define DELAY 0.0005 // 0.5ms
    
    vector<DelayLinePhaseShift2 *> phaseShifts;
    phaseShifts.resize(bufferSize/2.0);
    for (int i = 0; i < bufferSize/2.0; i++)
    {
        DelayLinePhaseShift2 *phaseShift =
            new DelayLinePhaseShift2(sampleRate, bufferSize, i, 0.0, DELAY);
        phaseShifts[i] = phaseShift;
    }
    
    // Apply the delay
    WDL_TypedBuf<WDL_FFT_COMPLEX> fftSamples1;
    fftSamples1.Resize(fftSamples.GetSize());
    
    for (int i = 0; i < bufferSize/2.0; i++)
    {
        WDL_FFT_COMPLEX val = fftSamples.Get()[i];
        WDL_FFT_COMPLEX val1 = phaseShifts[i]->ProcessSample(val);
        
        fftSamples1.Get()[i] = val1;
    }
    
    // Convert to samples and dump
    fftSamples1.Resize(fftSamples1.GetSize()*2);
    BLUtilsFft::FillSecondFftHalf(&fftSamples1);
    
    WDL_TypedBuf<BL_FLOAT> samples1;
    FftProcessObj16::FftToSamples(fftSamples1, &samples1);
    
    BLDebug::DumpData("samples1.txt", samples1);
    
    // Clean
    for (int i = 0; i < bufferSize/2.0; i++)
    {
        DelayLinePhaseShift2 *phaseShift = phaseShifts[i];
        delete phaseShift;
    }
}
