//
//  TransientShaperFftObj2.cpp
//  BL-Shaper
//
//  Created by Pan on 16/04/18.
//
//

#include "BLUtils.h"
#include "TransientLib4.h"

#include "TransientShaperFftObj2.h"


// GOOD : 1 is good !
// (so we have a good temporal definition)
//
// (For PostTransientFftObj, it doesn't make a real difference)
#define VARIABLE_HANNING 1

// Must set to 0 to keep transients (i.e energy modification !)
#define KEEP_SYNTHESIS_ENERGY 0


// Detection + correction
#define TRANSIENTNESS_COEFF 5.0


// Multiply transientness by 24 dB
#define TRANS_GAIN_FACTOR 24.0

// Clip if the resulting gain is greater than 6dB
#define TRANS_GAIN_CLIP 6.0


// NOTE: Transientness doe not depend on the magnitude...
// So impossible to normalize in a simple way...

// According to the author page, we must do windowing before fft !
// See: http://werner.yellowcouch.org/Papers/transients12/index.html
//
// With that, we can again separate white noise and sine
//

TransientShaperFftObj2::TransientShaperFftObj2(int bufferSize, int oversampling, int freqRes,
                                               bool doApplyTransients, int maxNumPoints, BL_FLOAT decimFactor)
: FftObj(bufferSize, oversampling, freqRes,
         AnalysisMethodWindow, SynthesisMethodWindow,
         KEEP_SYNTHESIS_ENERGY,
         VARIABLE_HANNING,
         false),
   mTransientness(maxNumPoints, decimFactor, false)
{
    mPrecision = 0.0;
    mSoftHard = 0.0;
    
    mFreqsToTrans = true;
    mAmpsToTrans = true;
    
    mDoApplyTransients = doApplyTransients;
}

TransientShaperFftObj2::~TransientShaperFftObj2() {}

void
TransientShaperFftObj2::Reset(int oversampling, int freqRes)
{
    FftObj::Reset(oversampling, freqRes);
    
    mTransientness.Reset();
}

void
TransientShaperFftObj2::SetPrecision(BL_FLOAT precision)
{
    mPrecision = precision;
}

void
TransientShaperFftObj2::SetSoftHard(BL_FLOAT softHard)
{
    mSoftHard = softHard;
}

void
TransientShaperFftObj2::SetFreqsToTrans(bool flag)
{
    mFreqsToTrans = flag;
}

void
TransientShaperFftObj2::SetAmpsToTrans(bool flag)
{
    mAmpsToTrans = flag;
}

void
TransientShaperFftObj2::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                                         const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer)
{
    // Seems hard to take half, since we work in sample space too...
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> fftBuffer = *ioBuffer;
    
    WDL_TypedBuf<BL_FLOAT> magns;
    WDL_TypedBuf<BL_FLOAT> phases;
    BLUtils::ComplexToMagnPhase(&magns, &phases, fftBuffer);
    
    // Doesn't work...
    //FftProcessObj12::UnapplyAnalysisWindowFft(&magns, phases);
    
    // Compute the transientness
    WDL_TypedBuf<BL_FLOAT> transientness;
    TransientLib4::ComputeTransientness(magns, phases,
                                        &mPrevPhases,
                                        mFreqsToTrans,
                                        mAmpsToTrans,
                                        1.0 - mPrecision,
                                        &transientness);
    
    // Doesn't work...
    //BLUtils::UnapplyWindow(&transientness, mAnalysisWindow, 2);
    
    if (mBufferNum % mOverlapping == 0)
        mTransientness.AddValues(transientness);
    
    mCurrentTransientness = transientness;
    BLUtils::MultValues(&mCurrentTransientness, TRANSIENTNESS_COEFF);
    
    mPrevPhases = phases;
}

void
TransientShaperFftObj2::ProcessSamplesBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer,
                                         WDL_TypedBuf<BL_FLOAT> *scBuffer)
{
    if (mDoApplyTransients)
    {
        ApplyTransientness(ioBuffer, mCurrentTransientness);
    }
}

void
TransientShaperFftObj2::GetTransientness(WDL_TypedBuf<BL_FLOAT> *outTransientness)
{
    mTransientness.GetValues(outTransientness);
}

void
TransientShaperFftObj2::GetCurrentTransientness(WDL_TypedBuf<BL_FLOAT> *outTransientness)
{
    *outTransientness = mCurrentTransientness;
}

#if 0 // Old version, makes saturate very briefly
void
TransientShaperFftObj2::ApplyTransientness(WDL_TypedBuf<BL_FLOAT> *ioSamples,
                                          const WDL_TypedBuf<BL_FLOAT> &currentTransientness)
{
    BL_FLOAT maxTrans = BLUtils::ComputeMax(currentTransientness);
    BLDebug::AppendValue("trans.txt", maxTrans);
    
    WDL_TypedBuf<BL_FLOAT> trans = currentTransientness;
    
    // TODO: modify this with a "param shape"
    
    // Avoid clipping (intelligently)
    BL_FLOAT maxTransientness = ComputeMaxTransientness();
    BLUtils::AntiClipping(&trans, maxTransientness);
    
    BL_FLOAT gainDB = MAX_GAIN*mSoftHard;
    
    WDL_TypedBuf<BL_FLOAT> gainsDB = trans;
    BLUtils::MultValues(&gainsDB, gainDB);
    
    WDL_TypedBuf<BL_FLOAT> gains = gainsDB;
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
#endif

// New version
// Uses power to flatten peaks ("param shape")
void
TransientShaperFftObj2::ApplyTransientness(WDL_TypedBuf<BL_FLOAT> *ioSamples,
                                           const WDL_TypedBuf<BL_FLOAT> &currentTransientness)
{
#define EXP 0.75
#define FACTOR 0.5
    
    WDL_TypedBuf<BL_FLOAT> trans = currentTransientness;
    
    // Flatten the peaks
    // The factor is used to keep almost the same scale
    // for the values that are not peaks
    BLUtils::ApplyPow(&trans, EXP);
    BLUtils::MultValues(&trans, FACTOR);
    
    BL_FLOAT gainDB = TRANS_GAIN_FACTOR*mSoftHard;
    
    WDL_TypedBuf<BL_FLOAT> gainsDB = trans;
    BLUtils::MultValues(&gainsDB, gainDB);
    
    // Clip the gain at 6dB
    // This avoid that very high transients make
    // a huge gain increase
    //
    // This is better to clip the transient gain instead of the signal:
    // => this way, we shouldn't have signal distortion !
    BLUtils::AntiClipping(&gainsDB, TRANS_GAIN_CLIP);
    
    WDL_TypedBuf<BL_FLOAT> gains = gainsDB;
    BLUtils::DBToAmp(&gains);
    
    // Apply the gain to the signal
    BLUtils::MultValues(ioSamples, gains);
    
    // Do not use that, it will make saturate,
    // and the host must do it itself !
#if 0
    // Avoid clipping
    // Because this is risky otherwise
    BLUtils::SamplesAntiClipping(ioBuffer, 0.999);
#endif
}

#if 0
BL_FLOAT
TransientShaperFftObj2::ComputeMaxTransientness()
{
    // Just to be sure to not reach exactly 1.0 in the samples
#define FACTOR 0.999
    
#define EPS 1e-15
    
  if (std::fabs(mSoftHard) < EPS)
        return 1.0*FACTOR;
    
    BL_FLOAT maxTransDB = -MAX_GAIN_CLIP/mSoftHard;
    
    BL_FLOAT maxTrans = ::DBToAmp(maxTransDB);
    
    return maxTrans*FACTOR;
}
#endif
