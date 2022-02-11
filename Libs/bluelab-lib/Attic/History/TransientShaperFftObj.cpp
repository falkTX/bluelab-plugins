#//
//  TransientShaperObj.cpp
//  BL-Shaper
//
//  Created by Pan on 16/04/18.
//
//

#include "BLUtils.h"
#include "TransientLib4.h"

#include "TransientShaperFftObj.h"

// Detection + correction
#define TRANSIENTNESS_COEFF 5.0

#define MAX_GAIN 50.0
#define MAX_GAIN_CLIP 6.0

// For transient
#define MAX_PRECISION 0.99

// GOOD : 1 is good !
// (so we have a good temporal definition)
#define VARIABLE_HANNING 1


// According to the author page, we must do windowing before fft !
// See: http://werner.yellowcouch.org/Papers/transients12/index.html
//
// With that, we can again separate white noise and sine
//
// But the amplitude of the curve changed when changing the quality...
//
// And with that, transients are focused on the center when using on
// white noise. (They should be equally spread !)

// When not using analysis windowing, transients are aqually spread for white noise

TransientShaperFftObj::TransientShaperFftObj(int bufferSize, int oversampling, int freqRes,
                                             bool useAnalysisWindowing,
                                             double precision,
                                             int maxNumPoints, double decimFactor,
                                             bool applyTransients)
: FftProcessObj12(bufferSize, oversampling, freqRes,
                  
                  useAnalysisWindowing ?
                    FftProcessObj12::AnalysisMethodWindow :
                  FftProcessObj12::AnalysisMethodNone,
                  
                  FftProcessObj12::SynthesisMethodWindow,
                  false, false, VARIABLE_HANNING),

#if 0 // This is bad, this homogeneize too much
// Keep synthesis energy
// This is because the processing in
// ShaperLib3::DetectShapersSmooth looses energy
// (see mix parameter, e.g setup at 50%)
: FftProcessObj11(bufferSize, oversampling, freqRes,
                  FftProcessObj11::AnalysisMethodWindow,
                  FftProcessObj11::SynthesisMethodWindow,
                  true),
#endif

mPrecision(precision),
mSoftHard(0.0),
mTransientness(maxNumPoints, decimFactor, false),
mInput(maxNumPoints, decimFactor, true),
mOutput(maxNumPoints, decimFactor, true)
{
    //mFreqsToTrans = false;
    //mAmpsToTrans = false;
    
    mFreqsToTrans = true;
    mAmpsToTrans = true;
    
    mDoApplyTransients = applyTransients;
    
    // Used to display the waveform without problem with overlap
    // (otherwise, it would made "packets" at each step)
    mBufferCount = 0;
}

TransientShaperFftObj::~TransientShaperFftObj() {}

void
TransientShaperFftObj::Reset(int oversampling, int freqRes)
{
    FftProcessObj12::Reset(oversampling, freqRes);
    
    mTransientness.Reset();
    mInput.Reset();
    mOutput.Reset();
    
    mBufferCount = 0;
}

void
TransientShaperFftObj::SetPrecision(double precision)
{
    mPrecision = precision;
}

void
TransientShaperFftObj::SetSoftHard(double softHard)
{
    mSoftHard = softHard;
}

void
TransientShaperFftObj::SetFreqsToTrans(bool flag)
{
    mFreqsToTrans = flag;
}

void
TransientShaperFftObj::SetAmpsToTrans(bool flag)
{
    mAmpsToTrans = flag;
}

void
TransientShaperFftObj::PreProcessSamplesBuffer(WDL_TypedBuf<double> *ioBuffer)
{
    WDL_TypedBuf<double> input = *ioBuffer;
    
    // No need !
    // When PreProcessSamplesBuffer() is called,  the window is not yet applied
    //
    //FftProcessObj12::UnapplyAnalysisWindowSamples(&input);
    
    if (mBufferCount % mOverlapping == 0)
        mInput.AddValues(input);
}

void
TransientShaperFftObj::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                                        const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer)
{
    // TODO: take half again, but modify TransientLib4::ComputeTransientness
    // to manage twice less phases with full transitness result size
    
    //BLUtils::TakeHalf(ioBuffer);
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> fftBuffer = *ioBuffer;
    
    WDL_TypedBuf<double> magns;
    WDL_TypedBuf<double> phases;
    BLUtils::ComplexToMagnPhase(&magns, &phases, fftBuffer);
    
    // Doesn't work...
    //FftProcessObj12::UnapplyAnalysisWindowFft(&magns, phases);
    
    // Compute the transientness
    WDL_TypedBuf<double> transientness;
    TransientLib4::ComputeTransientness(magns, phases,
                                        &mPrevPhases,
                                        mFreqsToTrans,
                                        mAmpsToTrans,
                                        1.0 - mPrecision,
                                        &transientness);
    
    // Doesn't work...
    //BLUtils::UnapplyWindow(&transientness, mAnalysisWindow, 2);
    
    if (mBufferCount % mOverlapping == 0)
        mTransientness.AddValues(transientness);
    
    mCurrentTransientness = transientness;
    BLUtils::MultValues(&mCurrentTransientness, TRANSIENTNESS_COEFF);
    
    mPrevPhases = phases;
    
#if 0
    // Necessary... ?
    // Maybe if we don"t take half it changes
    //BLUtils::MultValues(&magns, 2.0);
    
    // Useless, the modification of the sound will be done on the samples
    //BLUtils::MagnPhaseToComplex(ioBuffer, magns, phases);
#endif
    
    //BLUtils::ResizeFillZeros(ioBuffer, ioBuffer->GetSize()*2);
    //BLUtils::FillSecondFftHalf(ioBuffer);
}

void
TransientShaperFftObj::ProcessSamplesBuffer(WDL_TypedBuf<double> *ioBuffer,
                                         WDL_TypedBuf<double> *scBuffer)
{
    if (mDoApplyTransients)
    {
        ApplyTransientness(ioBuffer, mCurrentTransientness);
    }
}

void
TransientShaperFftObj::PostProcessSamplesBuffer(WDL_TypedBuf<double> *ioBuffer)
{
    // UnWindow ?
    
    if (mBufferCount % mOverlapping == 0)
        mOutput.AddValues(*ioBuffer);
    
    mBufferCount++;
}

void
TransientShaperFftObj::GetTransientness(WDL_TypedBuf<double> *outTransientness)
{
    mTransientness.GetValues(outTransientness);
}

void
TransientShaperFftObj::GetCurrentTransientness(WDL_TypedBuf<double> *outTransientness)
{
    *outTransientness = mCurrentTransientness;
}

void
TransientShaperFftObj::GetInput(WDL_TypedBuf<double> *outInput)
{
    mInput.GetValues(outInput);
}

void
TransientShaperFftObj::GetOutput(WDL_TypedBuf<double> *outOutput)
{
    mOutput.GetValues(outOutput);
}

void
TransientShaperFftObj::ApplyTransientness(WDL_TypedBuf<double> *ioSamples,
                                          const WDL_TypedBuf<double> &currentTransientness)
{
    WDL_TypedBuf<double> trans = currentTransientness;
    
    //BLUtils::MultValues(&trans, TRANSIENTNESS_COEFF);
    
    // Avoid clipping (intelligently)
    double maxTransientness = ComputeMaxTransientness();
    BLUtils::AntiClipping(&trans, maxTransientness);
    
    double gainDB = MAX_GAIN*mSoftHard;
    
    WDL_TypedBuf<double> gainsDB = trans;
    BLUtils::MultValues(&gainsDB, gainDB);
    
    WDL_TypedBuf<double> gains = gainsDB;
    BLUtils::DBToAmp(&gains);
    
    BLUtils::MultValues(ioSamples, gains);
    
    // Do not use that, it will make saturate,
    // and the host must do it itself !
#if 0
    // Avoid clipping
    // Because this is risky otherwise
    BLUtils::SamplesAntiClipping(ioBuffer, 0.999);
#endif
}

double
TransientShaperFftObj::ComputeMaxTransientness()
{
    // Just to be sure to not reach exactly 1.0 in the samples
#define FACTOR 0.999
    
#define EPS 1e-15
    
  if (std::fabs(mSoftHard) < EPS)
        return 1.0*FACTOR;
    
    double maxTransDB = -MAX_GAIN_CLIP/mSoftHard;
    
    double maxTrans = ::DBToAmp(maxTransDB);
    
    return maxTrans*FACTOR;
}
