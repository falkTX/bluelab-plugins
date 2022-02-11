//
//  SimpleSpectrogramFftObj.cpp
//  BL-Ghost
//
//  Created by Pan on 02/06/18.
//
//

#include <BLSpectrogram3.h>

#include <BLUtils.h>

#include "SimpleSpectrogramFftObj.h"

SimpleSpectrogramFftObj::SimpleSpectrogramFftObj(int bufferSize,
                                                 int oversampling, int freqRes,
                                                 BL_FLOAT sampleRate)
: ProcessObj(bufferSize)
{
    mSpectrogram = new BLSpectrogram3(bufferSize/4, -1);

    ProcessObj::Reset(bufferSize, oversampling, freqRes, sampleRate);
}

SimpleSpectrogramFftObj::~SimpleSpectrogramFftObj()
{
    delete mSpectrogram;
}

void
SimpleSpectrogramFftObj::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                                          const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer)
{    
    BLUtils::TakeHalf(ioBuffer);
    
    WDL_TypedBuf<BL_FLOAT> magns;
    WDL_TypedBuf<BL_FLOAT> phases;
    BLUtils::ComplexToMagnPhase(&magns, &phases, *ioBuffer);
    
    AddSpectrogramLine(magns, phases);
}

void
SimpleSpectrogramFftObj::Reset(int bufferSize, int oversampling,
                               int freqRes, BL_FLOAT sampleRate)
{
    ProcessObj::Reset(bufferSize, oversampling, freqRes, sampleRate);
}

BLSpectrogram3 *
SimpleSpectrogramFftObj::GetSpectrogram()
{
    return mSpectrogram;
}

void
SimpleSpectrogramFftObj::AddSpectrogramLine(const WDL_TypedBuf<BL_FLOAT> &magns,
                                            const WDL_TypedBuf<BL_FLOAT> &phases)
{
    // Simple add
    mSpectrogram->AddLine(magns, phases);
}
