//
//  PitchShiftPrusaFftObj.cpp
//  BL-PitchShift
//
//  Created by Pan on 16/04/18.
//
//

#include <BLUtils.h>
#include <BLUtilsComp.h>
#include <BLUtilsFft.h>

#include "PitchShiftPrusaFftObj.h"

PitchShiftPrusaFftObj::PitchShiftPrusaFftObj(int bufferSize,
                                             int oversampling,
                                             int freqRes,
                                             BL_FLOAT sampleRate)
: ProcessObj(bufferSize)
{
    ProcessObj::Reset(bufferSize, oversampling, freqRes, sampleRate);
    
    mFactor = 1.0;
}

PitchShiftPrusaFftObj::~PitchShiftPrusaFftObj() {}

void
PitchShiftPrusaFftObj::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer0,
                                        const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer)
{
    WDL_TypedBuf<WDL_FFT_COMPLEX> &ioBuffer = mTmpBuf6;
    BLUtils::TakeHalf(*ioBuffer0, &ioBuffer);
    
    WDL_TypedBuf<BL_FLOAT> &magns = mTmpBuf0;
    WDL_TypedBuf<BL_FLOAT> &phases = mTmpBuf1;
    BLUtilsComp::ComplexToMagnPhase(&magns, &phases, ioBuffer);
    
    //
    // Convert
    //
    Convert(&magns, &phases, mFactor);
        
    BLUtilsComp::MagnPhaseToComplex(&ioBuffer, magns, phases);
    
    BLUtils::SetBuf(ioBuffer0, ioBuffer);
    
    BLUtilsFft::FillSecondFftHalf(ioBuffer0);
}

void
PitchShiftPrusaFftObj::SetFactor(BL_FLOAT factor)
{
    mFactor = factor;
}

void
PitchShiftPrusaFftObj::Reset(int bufferSize, int oversampling,
                             int freqRes, BL_FLOAT sampleRate)
{
    ProcessObj::Reset(bufferSize, oversampling, freqRes, sampleRate);
    
    ResetPitchShift();
}

void
PitchShiftPrusaFftObj::Reset()
{
    //ProcessObj::Reset();
    ProcessObj::Reset(mBufferSize, mOverlapping, mFreqRes, mSampleRate);
    
    ResetPitchShift();
}


void
PitchShiftPrusaFftObj::Convert(WDL_TypedBuf<BL_FLOAT> *magns,
                               WDL_TypedBuf<BL_FLOAT> *phases,
                               BL_FLOAT factor)
{
    // TODO
}

void
PitchShiftPrusaFftObj::ResetPitchShift()
{
    // TODO
}
