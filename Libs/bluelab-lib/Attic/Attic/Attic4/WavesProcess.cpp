//
//  StereoVizProcess3.cpp
//  BL-PitchShift
//
//  Created by Pan on 20/04/18.
//
//

#include <BLUtils.h>
#include <DebugGraph.h>

#include <WavesRender.h>

#include "WavesProcess.h"


WavesProcess::WavesProcess(int bufferSize,
                           BL_FLOAT overlapping, BL_FLOAT oversampling,
                           BL_FLOAT sampleRate)
: ProcessObj(bufferSize)
{
    mBufferSize = bufferSize;
    mOverlapping = overlapping;
    mOversampling = oversampling;
    
    mSampleRate = sampleRate;
    
    mWavesRender = NULL;
}

WavesProcess::~WavesProcess() {}

void
WavesProcess::Reset()
{
    Reset(mBufferSize, mOverlapping, mOversampling, mSampleRate);
}

void
WavesProcess::Reset(int bufferSize, int overlapping, int oversampling,
                    BL_FLOAT sampleRate)
{
    mBufferSize = bufferSize;
    
    mOverlapping = overlapping;
    mOversampling = oversampling;
    
    mSampleRate = sampleRate;
    
    mValues.Resize(0);
}

void
WavesProcess::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                               const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer)

{
    WDL_TypedBuf<WDL_FFT_COMPLEX> fftSamples = *ioBuffer;
    
    // Take half of the complexes
    BLUtils::TakeHalf(&fftSamples);
    
    WDL_TypedBuf<BL_FLOAT> magns;
    BLUtils::ComplexToMagn(&magns, fftSamples);
    
    if (mWavesRender != NULL)
    {
        mWavesRender->AddMagns(magns);
    }
}

void
WavesProcess::SetWavesRender(WavesRender *wavesRender)
{
    mWavesRender = wavesRender;
}
