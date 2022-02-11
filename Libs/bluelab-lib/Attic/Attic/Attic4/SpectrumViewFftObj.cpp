//
//  SpectrumViewFftObj.cpp
//  BL-SpectrumView
//
//  Created by applematuer on 7/8/19.
//
//

#include <BLUtils.h>

#include "SpectrumViewFftObj.h"


SpectrumViewFftObj::SpectrumViewFftObj(int bufferSize, int oversampling, int freqRes)
: ProcessObj(bufferSize) {}

SpectrumViewFftObj::~SpectrumViewFftObj() {}

void
SpectrumViewFftObj::Reset(int bufferSize, int oversampling, int freqRes, BL_FLOAT sampleRate)
{
    ProcessObj::Reset(bufferSize, oversampling, freqRes, sampleRate);
    
    //
    mSignalBuf.Resize(0);
}

void
SpectrumViewFftObj::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                                     const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer)
{
    BLUtils::TakeHalf(ioBuffer);
    
    WDL_TypedBuf<BL_FLOAT> sigMagns;
    WDL_TypedBuf<BL_FLOAT> phases;
    BLUtils::ComplexToMagnPhase(&sigMagns, &phases, *ioBuffer);
    
    mSignalBuf = sigMagns;
}

void
SpectrumViewFftObj::GetSignalBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer)
{
    *ioBuffer = mSignalBuf;
}
