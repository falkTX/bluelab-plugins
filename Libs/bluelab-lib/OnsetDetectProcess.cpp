//
//  OnsetDetectProcess.cpp
//  BL-Air
//
//  Created by Pan on 20/04/18.
//
//

#include <BLUtils.h>
#include <BLUtilsComp.h>

#include <DebugGraph.h>

#include <OnsetDetector.h>

#include "OnsetDetectProcess.h"


OnsetDetectProcess::OnsetDetectProcess(int bufferSize,
                                       BL_FLOAT overlapping, BL_FLOAT oversampling,
                                       BL_FLOAT sampleRate)
: ProcessObj(bufferSize)
{
    mBufferSize = bufferSize;
    mOverlapping = overlapping;
    mOversampling = oversampling;
    
    mSampleRate = sampleRate;
    
    mOnsetDetector = new OnsetDetector();
}

OnsetDetectProcess::~OnsetDetectProcess()
{
    delete mOnsetDetector;
}

void
OnsetDetectProcess::Reset()
{
    Reset(mBufferSize, mOverlapping, mOversampling, mSampleRate);
}

void
OnsetDetectProcess::Reset(int bufferSize, int overlapping, int oversampling,
                          BL_FLOAT sampleRate)
{
    mBufferSize = bufferSize;
    
    mOverlapping = overlapping;
    mOversampling = oversampling;
    
    mSampleRate = sampleRate;
}

void
OnsetDetectProcess::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                                     const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer)

{    
    WDL_TypedBuf<WDL_FFT_COMPLEX> fftSamples = *ioBuffer;
    
    // Take half of the complexes
    BLUtils::TakeHalf(&fftSamples);
    
    WDL_TypedBuf<BL_FLOAT> magns;
    WDL_TypedBuf<BL_FLOAT> phases;
    BLUtilsComp::ComplexToMagnPhase(&magns, &phases, fftSamples);
    
    mOnsetDetector->Detect(magns);
}

void
OnsetDetectProcess::ProcessSamplesPost(WDL_TypedBuf<BL_FLOAT> *ioBuffer)
{
    BL_FLOAT onsetValue = mOnsetDetector->GetCurrentOnsetValue();
    
    BLUtils::FillAllValue(ioBuffer, onsetValue);
}

void
OnsetDetectProcess::SetThreshold(BL_FLOAT threshold)
{
    mOnsetDetector->SetThreshold(threshold);
}
