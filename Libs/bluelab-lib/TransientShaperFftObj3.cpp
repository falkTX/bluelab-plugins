//
//  TransientShaperFftObj3.cpp
//  BL-Shaper
//
//  Created by Pan on 16/04/18.
//
//

#include <BLUtils.h>
#include <BLUtilsComp.h>

#include <BLDebug.h>

//#include "TransientLib4.h"
#include <TransientLib5.h>

#include <FifoDecimator3.h>

#include "TransientShaperFftObj3.h"


// GOOD : 1 is good !
// (so we have a good temporal definition)
//
// (For PostTransientFftObj, it doesn't make a real difference)
//#define VARIABLE_HANNING 1

// Must set to 0 to keep transients (i.e energy modification !)
//#define KEEP_SYNTHESIS_ENERGY 0


// Detection + correction
#define TRANSIENTNESS_COEFF 5.0


// Multiply transientness by 24 dB
#define TRANS_GAIN_FACTOR 24.0

// Clip if the resulting gain is greater than 6dB
#define TRANS_GAIN_CLIP 6.0

// The old version is better !
// (but be careful about saturation... !)
#define USE_OLD_VERSION 1

// NOTE: Transientness doe not depend on the magnitude...
// So impossible to normalize in a simple way...

// According to the author page, we must do windowing before fft !
// See: http://werner.yellowcouch.org/Papers/transients12/index.html
//
// With that, we can again separate white noise and sine
//

#define BYPASS_IF_NOCHANGE 1

// Validated!
#define DENOISER_OPTIM5 1

#if 0
SMALL PROBLEMs:
"Rumble" sound sometimes, when very short "p" transients are detected
#endif

TransientShaperFftObj3::TransientShaperFftObj3(int bufferSize, int oversampling,
                                               int freqRes, BL_FLOAT sampleRate,
                                               int decimNumPoints,
                                               BL_FLOAT decimFactor,
                                               bool doApplyTransients)
    : ProcessObj(bufferSize),
mInput(true),
mOutput(true)
{
    mTransLib = new TransientLib5();
    
    mPrecision = 0.0;
    mSoftHard = 0.0;
    
    mFreqAmpRatio = 0.5;
    
    mDoApplyTransients = doApplyTransients;
    
    mTransientness = NULL;
    
    ProcessObj::Reset(bufferSize, oversampling, freqRes, sampleRate);
    
    //
    mInput.SetParams(decimNumPoints, decimFactor, true);
    mOutput.SetParams(decimNumPoints, decimFactor, true);
    
#if FORCE_SAMPLE_RATE
    InitResamplers();
    
    mRemainingSamples = 0.0;
#endif

    mHasNewData = true;
}

TransientShaperFftObj3::~TransientShaperFftObj3()
{
    if (mTransientness != NULL)
        delete mTransientness;

    delete mTransLib;
}

void
TransientShaperFftObj3::Reset()
{
    //ProcessObj::Reset();
    
    if (mTransientness != NULL)
        mTransientness->Reset();
    
#if FORCE_SAMPLE_RATE
    ResetResamplers();
    
    mRemainingSamples = 0.0;
#endif
    
#if FORCE_SAMPLE_RATE_KEEP_QUALITY
    mSamplesIn.Resize(0);
    mTransientnessBuf.Resize(0);
#endif

    // 
    mInput.Reset();
    mOutput.Reset();

    mHasNewData = true;
}

void
TransientShaperFftObj3::Reset(int bufferSize, int oversampling,
                              int freqRes, BL_FLOAT sampleRate)
{
    ProcessObj::Reset(bufferSize, oversampling, freqRes, sampleRate);
    
    if (mTransientness != NULL)
        mTransientness->Reset();
    
#if FORCE_SAMPLE_RATE
    ResetResamplers();
    
    mRemainingSamples = 0.0;
#endif
    
#if FORCE_SAMPLE_RATE_KEEP_QUALITY
    mSamplesIn.Resize(0);
    mTransientnessBuf.Resize(0);
#endif

    mHasNewData = true;
}

#if FORCE_SAMPLE_RATE
// From Rebalance
void
TransientShaperFftObj3::ProcessInputSamplesPre(WDL_TypedBuf<BL_FLOAT> *ioBuffer,
                                               const WDL_TypedBuf<BL_FLOAT> *scBuffer)
{
    if (mSampleRate == SAMPLE_RATE)
        return;
    
#if FORCE_SAMPLE_RATE_KEEP_QUALITY
    // Keep the input samples with original quality
    mSamplesIn.Add(ioBuffer->Get(), ioBuffer->GetSize());
#endif
        
    WDL_ResampleSample *resampledAudio = NULL;
    int desiredSamples = ioBuffer->GetSize(); // Input driven
    int numSamples = mResamplerIn.ResamplePrepare(desiredSamples, 1, &resampledAudio);
        
    for (int j = 0; j < numSamples; j++)
    {
        if (j >= ioBuffer->GetSize())
            break;
        resampledAudio[j] = ioBuffer->Get()[j];
    }
        
    WDL_TypedBuf<BL_FLOAT> &outSamples = mTmpBuf0;
    outSamples.Resize(desiredSamples);
    int numResampled = mResamplerIn.ResampleOut(outSamples.Get(),
                                               ioBuffer->GetSize(),
                                               outSamples.GetSize(), 1);
        
    // GOOD !
    // Avoid clicks sometimes (for example with 88200Hz and buffer size 447)
    // The numResampled varies around a value, to keep consistency of the stream
    outSamples.Resize(numResampled);
        
    *ioBuffer = outSamples;
}
#endif

void
TransientShaperFftObj3::SetPrecision(BL_FLOAT precision)
{
    mPrecision = precision;
}

void
TransientShaperFftObj3::SetSoftHard(BL_FLOAT softHard)
{
    mSoftHard = softHard;
}

void
TransientShaperFftObj3::SetFreqAmpRatio(BL_FLOAT ratio)
{
    mFreqAmpRatio = ratio;
}

void
TransientShaperFftObj3::
ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                 const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer)
{
#if DENOISER_OPTIM5

#if BYPASS_IF_NOCHANGE
#define EPS 1e-15
    if (std::fabs(mSoftHard) < EPS)
    {
        // TODO: check this
        //mPrevPhases = phases;
        mPrevPhases.Resize(0);
        
        return;
    }
#endif
    
#endif
    
    // Seems hard to take half, since we work in sample space too...
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> &fftBuffer = mTmpBuf1;
    fftBuffer = *ioBuffer;
    
    WDL_TypedBuf<BL_FLOAT> &magns = mTmpBuf2;
    WDL_TypedBuf<BL_FLOAT> &phases = mTmpBuf3;
    BLUtilsComp::ComplexToMagnPhase(&magns, &phases, fftBuffer);

#if !DENOISER_OPTIM5
    
#if BYPASS_IF_NOCHANGE
#define EPS 1e-15
    if (std::fabs(mSoftHard) < EPS)
    {
        mPrevPhases = phases;
        
        return;
    }
#endif
    
#endif
    
    // Doesn't work...
    //FftProcessObj12::UnapplyAnalysisWindowFft(&magns, phases);
    
    // Compute the transientness
    WDL_TypedBuf<BL_FLOAT> &transientness = mTmpBuf4;
#if USE_TRANSIENT_DETECT_2
    // Old version: can not extract "s" only
    //TransientLib4::ComputeTransientness2(magns, phases,
    mTransLib->ComputeTransientness2(magns, phases,
                                    &mPrevPhases,
                                    //mFreqsToTrans,
                                    //mAmpsToTrans,
                                    mFreqAmpRatio,
                                    1.0 - mPrecision,
                                    &transientness);
#endif

#if USE_TRANSIENT_DETECT_3
    // New version(fixed): can extract "s" only
    // And the volume is not increased compared to bypass
    
    // This version works great with 44100Hz,
    // but not so good with 88200Hz
    // This is certainly due to transient smoothing
    // that shifts the maximum out of the correct value
    //TransientLib4::ComputeTransientness3(magns, phases,
    mTransLib->ComputeTransientness3(magns, phases,
                                    &mPrevPhases,
                                    //mFreqsToTrans,
                                    //mAmpsToTrans,
                                    mFreqAmpRatio,
                                    1.0 - mPrecision,
                                    &transientness);
#endif

#if USE_TRANSIENT_DETECT_4
    // Same as above, but with smoothing fixed and accurate
    // (smooth using LOD pyramid)
    //TransientLib4::ComputeTransientness4(magns, phases,
    mTransLib->ComputeTransientness4(magns, phases,
                                    &mPrevPhases,
                                    //mFreqsToTrans,
                                    //mAmpsToTrans,
                                    mFreqAmpRatio,
                                    1.0 - mPrecision,
                                    mSampleRate,
                                    &mTransSmoothWin,
                                    &transientness);
    
#endif

#if USE_TRANSIENT_DETECT_5
    // TEST
    //TransientLib4::ComputeTransientness5(magns, phases,
    mTransLib->ComputeTransientness5(magns, phases,
                                    &mPrevPhases,
                                    mFreqAmpRatio,
                                    1.0 - mPrecision,
                                    mSampleRate,
                                    &mTransSmoothWin,
                                    &transientness);
    
#endif

#if USE_TRANSIENT_DETECT_6
    // Final: good for 88200Hz + buffer size 4096
    //TransientLib4::ComputeTransientness6(magns, phases,
    mTransLib->ComputeTransientness6(magns, phases,
                                    &mPrevPhases,
                                    mFreqAmpRatio,
                                    1.0 - mPrecision,
                                    mSampleRate,
                                    &transientness);
    
#endif
    
    // Doesn't work...
    //BLUtils::UnapplyWindow(&transientness, mAnalysisWindow, 2);
    
    if (mTransientness != NULL)
        mTransientness->AddValues(transientness);
    
    mCurrentTransientness = transientness;
    BLUtils::MultValues(&mCurrentTransientness, (BL_FLOAT)TRANSIENTNESS_COEFF);
    
    mPrevPhases = phases;
    
#if FORCE_SAMPLE_RATE_KEEP_QUALITY
    if (mSampleRate != SAMPLE_RATE)
    {
        WDL_TypedBuf<BL_FLOAT> &currentTransientness = mTmpBuf5;
        currentTransientness = mCurrentTransientness;
        
        BL_FLOAT coeff = mSampleRate/SAMPLE_RATE;
        int newSize = currentTransientness.GetSize()*coeff;
        
        BLUtils::ResizeLinear2(&currentTransientness, newSize);
        
        mTransientnessBuf.Add(currentTransientness.Get(),
                              currentTransientness.GetSize());
    }
#endif

    mHasNewData = true;
}

#if !FORCE_SAMPLE_RATE_KEEP_QUALITY
void
TransientShaperFftObj3::ProcessSamplesBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer,
                                             WDL_TypedBuf<BL_FLOAT> *scBuffer)
{
    mInput.AddValues(*ioBuffer);
    
    if (mDoApplyTransients)
    {
#if BYPASS_IF_NOCHANGE
#define EPS 1e-15
        if (std::fabs(mSoftHard) < EPS)
        {
            mHasNewData = true;
                
            return;
        }
#endif
        
        ApplyTransientness(ioBuffer, mCurrentTransientness);
    }

    //mInput.AddValues(*ioBuffer);

    mHasNewData = true;
}
#else
void
TransientShaperFftObj3::ProcessSamplesBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer,
                                             WDL_TypedBuf<BL_FLOAT> *scBuffer)
{
    mInput.AddValues(*ioBuffer);

    mHasNewData = true;
}
#endif

#if FORCE_SAMPLE_RATE
#if !FORCE_SAMPLE_RATE_KEEP_QUALITY
// Re-upsample signal
void
TransientShaperFftObj3::ProcessSamplesPost(WDL_TypedBuf<BL_FLOAT> *ioBuffer)
{
    if (mSampleRate == SAMPLE_RATE)        
        return;
    
    BL_FLOAT sampleRate = mSampleRate;
    
    WDL_ResampleSample *resampledAudio = NULL;
    int desiredSamples = ioBuffer->GetSize(); // Input driven
    int numOutSamples = ioBuffer->GetSize()*sampleRate/SAMPLE_RATE; // Input driven
    int numSamples =
        mResamplerOut.ResamplePrepare(desiredSamples, 1, &resampledAudio);
    
    // Compute remaining "parts of sample", due to rounding
    // and re-add it to the number of requested samples
    // FIX: fixes blank frame with sample rate 48000 and buffer size 447
    //
    BL_FLOAT remaining =
        ((BL_FLOAT)ioBuffer->GetSize())*sampleRate/SAMPLE_RATE - numOutSamples;
    mRemainingSamples += remaining;
    if (mRemainingSamples >= 1.0)
    {
        int addSamples = floor(mRemainingSamples);
        mRemainingSamples -= addSamples;
        
        numOutSamples += addSamples;
    }
    
    for (int i = 0; i < numSamples; i++)
    {
        if (i >= ioBuffer->GetSize())
            break;
        resampledAudio[i] = ioBuffer->Get()[i];
    }
    
    WDL_TypedBuf<BL_FLOAT> &outSamples = mTmpBuf6;
    outSamples.Resize(numOutSamples); // Input driven
    
    int numResampled = mResamplerOut.ResampleOut(outSamples.Get(),
                                                 ioBuffer->GetSize(),
                                                 outSamples.GetSize(), 1);
    
    // GOOD!
    // Avoid a click with 48000Hz near the beginning
    // (when numResampled is just 1 sample less than numOutSamples)
    if (numResampled < numOutSamples)
        outSamples.Resize(numResampled);
    
    *ioBuffer = outSamples;
}
#endif
#endif

#if FORCE_SAMPLE_RATE_KEEP_QUALITY
// Output the registerd original input signal,
// modulated by transientness
void
TransientShaperFftObj3::ProcessSamplesPost(WDL_TypedBuf<BL_FLOAT> *ioBuffer)
{
    if (mSampleRate == SAMPLE_RATE)        
        return;
    
    BL_FLOAT coeff = mSampleRate/SAMPLE_RATE;
    int newSize = ioBuffer->GetSize()*coeff;
    
    // Extract a buffer of input samples
    //
    
    // Just in case
    if (mSamplesIn.GetSize() < newSize)        
        return;
    
    WDL_TypedBuf<BL_FLOAT> &result = mTmpBuf7;
    //result.Add(mSamplesIn.Get(), newSize);
    result.Resize(newSize);
    memcpy(result.Get(), mSamplesIn.Get(), newSize*sizeof(BL_FLOAT));
    
    BLUtils::ConsumeLeft(&mSamplesIn, newSize);
    
    // Extract a buffer of transientness
    //
    
    // Just in case
    if (mTransientnessBuf.GetSize() < newSize)
    {
        return;
    }
    
    WDL_TypedBuf<BL_FLOAT> &trans = mTmpBuf8;
    //trans.Add(mTransientnessBuf.Get(), newSize);
    trans.Resize(newSize);
    memcpy(trans.Get(), mTransientnessBuf.Get(), newSize*sizeof(BL_FLOAT));
    BLUtils::ConsumeLeft(&mTransientnessBuf, newSize);
    
    // Apply transientness
    ApplyTransientness(&result, trans);
    
    // Result
    *ioBuffer = result;

    mHasNewData = true;
}
#endif

void
TransientShaperFftObj3::
ProcessSamplesBufferEnergy(WDL_TypedBuf<BL_FLOAT> *ioBuffer,
                           const WDL_TypedBuf<BL_FLOAT> *scBuffer)
{
    mOutput.AddValues(*ioBuffer);

    mHasNewData = true;
}

void
TransientShaperFftObj3::SetTrackIO(int maxNumPoints, BL_FLOAT decimFactor,
                                   bool trackInput, bool trackOutput,
                                   bool trackTransientness)
{
    //ProcessObj::SetTrackIO(maxNumPoints, decimFactor,
    //                       trackInput, trackOutput);
    
    if (mTransientness != NULL)
        delete mTransientness;
    mTransientness = NULL;
    
    if (trackTransientness)
    {
        //mTransientness = new FifoDecimator(maxNumPoints, decimFactor, false);
        mTransientness = new FifoDecimator3(maxNumPoints, decimFactor, false);
    }

    // ??
    if (trackInput)
        mInput.SetParams(maxNumPoints, decimFactor);
    
    if (trackOutput)
        mOutput.SetParams(maxNumPoints, decimFactor);

    mHasNewData = true;
}

void
TransientShaperFftObj3::GetTransientness(WDL_TypedBuf<BL_FLOAT> *outTransientness)
{
    if (mTransientness != NULL)
        mTransientness->GetValues(outTransientness);
}

void
TransientShaperFftObj3::GetCurrentInput(WDL_TypedBuf<BL_FLOAT> *outInput)
{
    mInput.GetValues(outInput);
}
    
void
TransientShaperFftObj3::GetCurrentOutput(WDL_TypedBuf<BL_FLOAT> *outOutput)
{
    mOutput.GetValues(outOutput);
}

void
TransientShaperFftObj3::
GetCurrentTransientness(WDL_TypedBuf<BL_FLOAT> *outTransientness)
{
    *outTransientness = mCurrentTransientness;
}

bool
TransientShaperFftObj3::HasNewData()
{
    return mHasNewData;
}

void
TransientShaperFftObj3::TouchNewData()
{
    mHasNewData = false;
}

// The old version give strong change in amplitude
// which is good
// (but take care of saturation... !)
#if USE_OLD_VERSION
#define MAX_GAIN 50.0
#define MAX_GAIN_CLIP 6.0

BL_FLOAT
TransientShaperFftObj3::ComputeMaxTransientness()
{
    // Just to be sure to not reach exactly 1.0 in the samples
#define FACTOR 0.999
    
#define EPS 1e-15
    
  if (std::fabs(mSoftHard) < EPS)
        return 1.0*FACTOR;
    
    BL_FLOAT maxTransDB = -MAX_GAIN_CLIP/mSoftHard;
    
    BL_FLOAT maxTrans = BLUtils::DBToAmp(maxTransDB);
    
    return maxTrans*FACTOR;
}


void
TransientShaperFftObj3::ApplyTransientness(WDL_TypedBuf<BL_FLOAT> *ioSamples,
                                          const WDL_TypedBuf<BL_FLOAT> &currentTransientness)
{
    // Check, to avoid crash in case of empty transientness
    if (currentTransientness.GetSize() != ioSamples->GetSize())
        return;
    
    WDL_TypedBuf<BL_FLOAT> &trans = mTmpBuf9;
    trans = currentTransientness;
    
    //BLUtils::MultValues(&trans, TRANSIENTNESS_COEFF);
    
    // Avoid clipping (intelligently)
    BL_FLOAT maxTransientness = ComputeMaxTransientness();
    BLUtils::AntiClipping(&trans, maxTransientness);
    
    BL_FLOAT gainDB = MAX_GAIN*mSoftHard;
    
    WDL_TypedBuf<BL_FLOAT> &gainsDB = mTmpBuf10;
    gainsDB = trans;
    BLUtils::MultValues(&gainsDB, gainDB);
    
    WDL_TypedBuf<BL_FLOAT> &gains = mTmpBuf11;
    gains = gainsDB;
    BLUtils::DBToAmp(&gains);
    
    BLUtils::MultValues(ioSamples, gains);
    
    // Do not use that, it will make saturate,
    // and the host must do it itself !
#if 0
    // Avoid clipping
    // Because this is risky otherwise
    BLUtils::SamplesAntiClipping(ioBuffer, 0.999);
#endif
    
#if 0
    // DEBUG
    //
    // Uncomment to display the transientness as waveform
    //
    // WARNING: depending on the sample rate, the cuve will be different
    // (but when applied, the result is the same)
    //
    *ioSamples = currentTransientness;
#endif
}

#endif

// The new version is more secure,
// but the changes in amplitude are very low...
#if !USE_OLD_VERSION
// New version...
// Uses power to flatten peaks ("param shape")
void
TransientShaperFftObj3::ApplyTransientness(WDL_TypedBuf<BL_FLOAT> *ioSamples,
                                           const WDL_TypedBuf<BL_FLOAT> &currentTransientness)
{
#define EXP 0.75
#define FACTOR 0.5
    
    WDL_TypedBuf<BL_FLOAT> &trans = mTmpBuf12;
    trans = currentTransientness;
    
    // Flatten the peaks
    // The factor is used to keep almost the same scale
    // for the values that are not peaks
    BLUtils::ApplyPow(&trans, EXP);
    BLUtils::MultValues(&trans, FACTOR);
    
    BL_FLOAT gainDB = TRANS_GAIN_FACTOR*mSoftHard;
    
    WDL_TypedBuf<BL_FLOAT> &gainsDB = mTmpBuf13;
    gainsDB = trans;
    BLUtils::MultValues(&gainsDB, gainDB);
    
    // Clip the gain at 6dB
    // This avoid that very high transients make
    // a huge gain increase
    //
    // This is better to clip the transient gain instead of the signal:
    // => this way, we shouldn't have signal distortion !
    BLUtils::AntiClipping(&gainsDB, TRANS_GAIN_CLIP);
    
    WDL_TypedBuf<BL_FLOAT> &gains = mTmpBuf14;
    gains = gainsDB;
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
#endif

#if FORCE_SAMPLE_RATE
void
TransientShaperFftObj3::InitResamplers()
{
    // In
    //
    mResamplerIn.Reset(); //
        
    mResamplerIn.SetMode(true, 1, false, 0, 0);
    mResamplerIn.SetFilterParms();
        
    // GOOD !
    // Set input driven
    // (because output driven has a bug when downsampling:
    // the first samples are bad)
    mResamplerIn.SetFeedMode(true);
        
    // set input and output samplerates
    mResamplerIn.SetRates(mSampleRate, SAMPLE_RATE);
    
    // Out
    //
    mResamplerOut.Reset(); //
    
    // Out
    mResamplerOut.SetMode(true, 1, false, 0, 0);
    mResamplerOut.SetFilterParms();
    
    // GOOD !
    mResamplerOut.SetFeedMode(true); // Input driven
    
    // set input and output samplerates
    mResamplerOut.SetRates(SAMPLE_RATE, mSampleRate);
}
#endif


#if FORCE_SAMPLE_RATE
void
TransientShaperFftObj3::ResetResamplers()
{
    mResamplerIn.Reset();
    mResamplerIn.SetRates(mSampleRate, SAMPLE_RATE);
    
    mResamplerOut.Reset();
    mResamplerOut.SetRates(SAMPLE_RATE, mSampleRate);
}
#endif
