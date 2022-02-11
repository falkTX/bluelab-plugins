//
//  FftObj.cpp
//  BL-PitchShift
//
//  Created by Pan on 17/04/18.
//
//

#include "FftObj.h"

#define TRACK_ALL_BUFFERS 1

FftObj::FftObj(int bufferSize, int oversampling, int freqRes,
               enum AnalysisMethod aMethod,
               enum SynthesisMethod sMethod,
               bool keepSynthesisEnergy,
               bool variableHanning,
               bool skipFFT)
: FftProcessObj13(bufferSize, oversampling, freqRes,
                  aMethod, sMethod,
                  keepSynthesisEnergy, variableHanning, skipFFT),
    mInput(true),
    mOutput(true)
{
    mTrackInput = false;
    mTrackOutput = false;
}

FftObj::~FftObj() {}

void
FftObj::Reset(int oversampling, int freqRes)
{
    FftProcessObj13::Reset(oversampling, freqRes);
    
    mInput.Reset();
    mOutput.Reset();
    
    // Used to display the waveform without problem with overlap
    // (otherwise, it would made "packets" at each step)
    mBufferNum = 0;
}

void
FftObj::PreProcessSamplesBuffer(WDL_TypedBuf<double> *ioBuffer)
{
    WDL_TypedBuf<double> input = *ioBuffer;
    
#if !TRACK_ALL_BUFFERS
    if (mBufferNum % mOverlapping == 0) // TODO: check this
#endif
    {
        mInput.AddValues(input);
    }
}

void
FftObj::PostProcessSamplesBuffer(WDL_TypedBuf<double> *ioBuffer)
{
    // UnWindow ?
    
#if !TRACK_ALL_BUFFERS
    if (mBufferNum % mOverlapping == 0) // TODO: check this
#endif
    {
        mOutput.AddValues(*ioBuffer);
    }
    
    mBufferNum++;
}

void
FftObj::SetTrackIO(int maxNumPoints, double decimFactor,
                   bool trackInput, bool trackOutput)
{
    mTrackInput = trackInput;
    mTrackOutput = trackOutput;
    
    if (mTrackInput)
        mInput.SetParams(maxNumPoints, decimFactor);
    
    if (mTrackOutput)
        mOutput.SetParams(maxNumPoints, decimFactor);
}

void
FftObj::GetCurrentInput(WDL_TypedBuf<double> *outInput)
{
    mInput.GetValues(outInput);
}

void
FftObj::GetCurrentOutput(WDL_TypedBuf<double> *outOutput)
{
    mOutput.GetValues(outOutput);
}
