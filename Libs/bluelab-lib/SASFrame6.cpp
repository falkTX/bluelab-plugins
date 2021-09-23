//
//  SASFrame6.cpp
//  BL-SASViewer
//
//  Created by applematuer on 2/2/19.
//
//

#include <SASViewerProcess.h>

#include <PartialsToFreq7.h>

#include <FftProcessObj16.h>

// Do not speed up...
#include <SinLUT.h>

#include <BLUtils.h>
#include <BLUtilsMath.h>
#include <BLUtilsComp.h>
#include <BLUtilsFft.h>

#include <BLDebug.h>

#include <Scale.h>

#include <OnsetDetector.h>

#include "SASFrame6.h"


// Optim: optimizes a little (~5/15%)
SIN_LUT_CREATE(SAS_FRAME_SIN_LUT, 4096);

// 4096*64 (2MB)
// 2x slower than 4096
//SIN_LUT_CREATE(SAS_FRAME_SIN_LUT, 262144);

// Synthesis
#define SYNTH_MAX_NUM_PARTIALS 40 //20 //10 origin: 10, (40 for test bell)
#define SYNTH_AMP_COEFF 4.0
#define SYNTH_MIN_FREQ 50.0
#define SYNTH_DEAD_PARTIAL_DECREASE 0.25

// NOTE: if we don't smooth the color, this will make clips
// (because if a partial is missing, this will make a hole in the color)
//
// Avoids jumps in envelope
#define COLOR_SMOOTH_COEFF 0.5
// WARPING 0.9 improves "bowl"
#define WARPING_SMOOTH_COEFF 0.0 //0.9 //0.5
// NOTE: for the moment, smooting freq in only for debugging
#define FREQ_SMOOTH_COEFF 0.0

// If a partial is missing, fill with zeroes around it
//
// At 0: low single partial => interpolated until the last frequencies
// Real signal => more smooth
//
// At 1: low single partial => interpolated locally, far frequencies set to 0
// Real signal: make some strange peaks at mid/high freqs
// Sine wave: very good, render only the analyzed input sine
//
//#define COLOR_CUT_MISSING_PARTIALS 1 // ORIGIN
#define COLOR_CUT_MISSING_PARTIALS 0 //1

// Set to 0 for optimization
//
// Origin: interpolate everything in db or mel scale
//
// Optim: this looks to work well when set to 0
// Perfs: 26% => 17%CPU
#define INTERP_RESCALE 0 // Origin: 1

// Origin: 1
#define FILL_ZERO_FIRST_LAST_VALUES_WARPING 1
#define FILL_ZERO_FIRST_LAST_VALUES_COLOR 0 //1

// Use OnsetDetector to avoid generating garbage harmonics when a transient appears
#define ENABLE_ONSET_DETECTION 1
#define DETECT_TRANSIENTS_ONSET 0 //1
#define ONSET_THRESHOLD 0.94
//#define ONSET_VALUE_THRESHOLD 0.0012
//#define ONSET_VALUE_THRESHOLD 0.06
//#define ONSET_VALUE_THRESHOLD 0.001 // Works ("bowl": takes the two peaks)
#define ONSET_VALUE_THRESHOLD 0.0025 // Works ("bowl": only take the main peak)

// Shift, so the onset peaks are in synch with mAmplitude 
#define ONSET_HISTORY_HACK 1
#define ONSET_HISTORY_HACK_SIZE 3

// Limit the maximum values that the detected warping can take
#define LIMIT_WARPING_MAX 1

// Smooth interpolation of warping envelope
#define WARP_ENVELOPE_USE_LAGRANGE_INTERP 1

// NOTE: Lagrange interp seems not good for color
// => it makes holes in "bowl" example, one the signal is converted
// in dB for displaying, and if adding additional values before
// interpolation this gives the same result as simple
// interpolation (no Lagrange)

// Smooth interpolation of color envelope?
#define COLOR_ENVELOPE_USE_LAGRANGE_INTERP 0 //1
#define LAGRANGE_MIN_NUM_COLOR_VALUES 16

// TEST: nearest interpolation, to be more quick
#define NEAREST_INTERP 0 //1

SASFrame6::SASPartial::SASPartial()
{
    mFreq = 0.0;
    mAmp = 0.0;
    mPhase = 0.0;
}

SASFrame6::SASPartial::SASPartial(const SASPartial &other)
{
    mFreq = other.mFreq;
    mAmp = other.mAmp;
    mPhase = other.mPhase;
}

SASFrame6::SASPartial::~SASPartial() {}

bool
SASFrame6::SASPartial::AmpLess(const SASPartial &p1, const SASPartial &p2)
{
    return (p1.mAmp < p2.mAmp);
}

SASFrame6::SASFrame6(int bufferSize, BL_FLOAT sampleRate,
                     int overlapping, int freqRes)
{
    SIN_LUT_INIT(SAS_FRAME_SIN_LUT);
    
    mBufferSize = bufferSize;
    
    mSampleRate = sampleRate;
    mOverlapping = overlapping;

    mFreqRes = freqRes;
    
    mSynthMode = RAW_PARTIALS;
    
    mAmplitude = 0.0;
    mPrevAmplitude = 0.0;
    
    mFrequency = 0.0;
    mPrevFrequency = -1.0;
    
    mAmpFactor = 1.0;
    mFreqFactor = 1.0;
    mColorFactor = 1.0;
    mWarpingFactor = 1.0;

    mScale = new Scale();
    
    mPartialsToFreq = new PartialsToFreq7(bufferSize, overlapping,
                                          freqRes, sampleRate);
        
    mMinAmpDB = -120.0;

    mSynthEvenPartials = true;
    mSynthOddPartials = true;

    mOnsetDetector = NULL;
#if ENABLE_ONSET_DETECTION
    mOnsetDetector = new OnsetDetector();
    mOnsetDetector->SetThreshold(ONSET_THRESHOLD);
#endif
}

SASFrame6::~SASFrame6()
{
    delete mPartialsToFreq;
    
    delete mScale;

#if ENABLE_ONSET_DETECTION
    delete mOnsetDetector;
#endif
}

void
SASFrame6::Reset(BL_FLOAT sampleRate)
{
    mPartials.clear();
    mPrevPartials.clear();
    
    mAmplitude = 0.0;
    
    mFrequency = 0.0;
    mPrevFrequency = -1.0;
    
    mSASPartials.clear();
    mPrevSASPartials.clear();
    
    mNoiseEnvelope.Resize(0);

    mPartialsToFreq->Reset(mBufferSize, mOverlapping, mFreqRes, sampleRate);
}

void
SASFrame6::Reset(int bufferSize, int oversampling,
                 int freqRes, BL_FLOAT sampleRate)
{
    mBufferSize = bufferSize;
    mOverlapping = oversampling;

    mFreqRes = freqRes;
    
    Reset(sampleRate);

#if ONSET_HISTORY_HACK
    mInputMagnsHistory.clear();
#endif
}

void
SASFrame6::SetMinAmpDB(BL_FLOAT ampDB)
{
    mMinAmpDB = ampDB;
}

void
SASFrame6::SetSynthMode(enum SynthMode mode)
{
    mSynthMode = mode;
}

SASFrame6::SynthMode
SASFrame6::GetSynthMode() const
{
    return mSynthMode;
}

void
SASFrame6::SetSynthEvenPartials(bool flag)
{
    mSynthEvenPartials = flag;
}

void
SASFrame6::SetSynthOddPartials(bool flag)
{
    mSynthOddPartials = flag;
}

void
SASFrame6::SetPartials(const vector<Partial> &partials)
{
    mPrevPartials = mPartials;
    mPartials = partials;

    // Should not be necessary
    //sort(mPrevPartials.begin(), mPrevPartials.end(), Partial::IdLess);
    
    sort(mPartials.begin(), mPartials.end(), Partial::IdLess);

    LinkPartialsIdx(&mPrevPartials, &mPartials);
    
    mAmplitude = 0.0;
    
    mFrequency = 0.0;
    
    ComputeAna();
}

void
SASFrame6::SetNoiseEnvelope(const WDL_TypedBuf<BL_FLOAT> &noiseEnv)
{
    mNoiseEnvelope = noiseEnv;
}

void
SASFrame6::GetNoiseEnvelope(WDL_TypedBuf<BL_FLOAT> *noiseEnv) const
{
    *noiseEnv = mNoiseEnvelope;
}

void
SASFrame6::SetInputData(const WDL_TypedBuf<BL_FLOAT> &magns,
                        const WDL_TypedBuf<BL_FLOAT> &phases)
{
    mInputMagns = magns;
    mInputPhases = phases;

#if ONSET_HISTORY_HACK
    mInputMagnsHistory.push_back(mInputMagns);
    if (mInputMagnsHistory.size() > ONSET_HISTORY_HACK_SIZE)
        mInputMagnsHistory.pop_front();
#endif
}

BL_FLOAT
SASFrame6::GetAmplitude() const
{
    return mAmplitude*mAmpFactor;
}

BL_FLOAT
SASFrame6::GetFrequency() const
{
    return mFrequency*mFreqFactor;
}

void
SASFrame6::GetColor(WDL_TypedBuf<BL_FLOAT> *color) const
{
    *color = mColor;
}

void
SASFrame6::GetNormWarping(WDL_TypedBuf<BL_FLOAT> *warping) const
{
    *warping = mNormWarping;

    BLUtils::AddValues(warping, (BL_FLOAT)-1.0);
    BLUtils::MultValues(warping, mWarpingFactor);
    BLUtils::AddValues(warping, (BL_FLOAT)1.0);
}

void
SASFrame6::SetAmplitude(BL_FLOAT amp)
{
    mAmplitude = amp;
}

void
SASFrame6::SetFrequency(BL_FLOAT freq)
{
    mFrequency = freq;
}

void
SASFrame6::SetColor(const WDL_TypedBuf<BL_FLOAT> &color)
{
    mPrevColor = mColor;
    
    mColor = color;
}

void
SASFrame6::SetNormWarping(const WDL_TypedBuf<BL_FLOAT> &warping)
{
    mPrevNormWarping = mNormWarping;
    
    mNormWarping = warping;
}

void
SASFrame6::ComputeSamples(WDL_TypedBuf<BL_FLOAT> *samples)
{
    if (mSynthMode == RAW_PARTIALS)
        ComputeSamplesPartialsRAW(samples);
    else if (mSynthMode == SOURCE_PARTIALS)
        ComputeSamplesPartialsSource(samples);
    else if (mSynthMode == RESYNTH_PARTIALS)
        ComputeSamplesPartialsResynth(samples);
}

// Directly use partials provided
void
SASFrame6::ComputeSamplesPartialsRAW(WDL_TypedBuf<BL_FLOAT> *samples)
{
    samples->Resize(mBufferSize);
    
    BLUtils::FillAllZero(samples);
    
    if (mPartials.empty())
    {
        return;
    }
    
    for (int i = 0; i < mPartials.size(); i++)
    {
        Partial partial;
        
        BL_FLOAT phase = 0.0;

        int prevPartialIdx = mPartials[i].mLinkedId;
        
        if (prevPartialIdx != -1)
            phase = mPrevPartials[prevPartialIdx].mPhase;

        BL_FLOAT twoPiSR = 2.0*M_PI/mSampleRate;
        
        for (int j = 0; j < samples->GetSize()/mOverlapping; j++)
        {
            // Get interpolated partial
            BL_FLOAT partialT = ((BL_FLOAT)j)/(samples->GetSize()/mOverlapping);
            GetPartial(&partial, i, partialT);
            
            BL_FLOAT freq = partial.mFreq;
                
            BL_FLOAT amp = partial.mAmp;
            
            BL_FLOAT samp = amp*std::sin(phase); // cos
            
            samp *= SYNTH_AMP_COEFF;
            
            if (freq >= SYNTH_MIN_FREQ)
                samples->Get()[j] += samp;
            
            //phase += 2.0*M_PI*freq/mSampleRate;
            phase += twoPiSR*freq;
        }
        
        mPartials[i].mPhase = phase;
    }
}

// Directly use partials provided, but also apply SAS parameter to them
void
SASFrame6::ComputeSamplesPartialsSource(WDL_TypedBuf<BL_FLOAT> *samples)
{
    samples->Resize(mBufferSize);
    
    BLUtils::FillAllZero(samples);
    
    if (mPartials.empty())
    {
        return;
    }

    // Optim
    BL_FLOAT twoPiSR = 2.0*M_PI/mSampleRate;

    BL_FLOAT hzPerBin = mSampleRate/mBufferSize;
    BL_FLOAT hzPerBinInv = 1.0/hzPerBin;
    
    for (int i = 0; i < mPartials.size(); i++)
    {
        int prevPartialIdx = mPartials[i].mLinkedId;

        BL_FLOAT phase = 0.0;
        if (prevPartialIdx != -1)
            phase = mPrevPartials[prevPartialIdx].mPhase;
        
        // Generate samples
        int numSamples = samples->GetSize()/mOverlapping;
        for (int j = 0; j < numSamples; j++)
        {
            Partial partial;
                        
            // Get interpolated partial
            BL_FLOAT partialT = ((BL_FLOAT)j)/numSamples;
            GetPartial(&partial, i, partialT);
            
            BL_FLOAT binIdx = partial.mFreq*hzPerBinInv;
            
            // Apply SAS parameters to current partial
            //

            // Cancel color
            BL_FLOAT colorFactor = mColorFactor;
            mColorFactor = 1.0;
            BL_FLOAT col0 = GetColor(mColor, binIdx);
            mColorFactor = colorFactor;

            partial.mAmp /= col0;

            // Inverse warping
            // => the detected partials will then be aligned to harmonics after that
            //
            // (No need to revert additional warping after)            
            BL_FLOAT warpFactor = mWarpingFactor;
            mWarpingFactor = 1.0;
            BL_FLOAT w0 = GetWarping(mNormWarpingInv, binIdx);
            mWarpingFactor = warpFactor;

            partial.mFreq *= w0;

            // Recompute bin idx
            binIdx = partial.mFreq*hzPerBinInv;
            
            // Apply warping
            BL_FLOAT w = GetWarping(mNormWarping, binIdx);
            partial.mFreq *= w;
            
            // Recompute bin idx
            binIdx = partial.mFreq*hzPerBinInv;
            
            // Apply color
            BL_FLOAT col = GetColor(mColor, binIdx);
            
            partial.mAmp *= col;

            // Amplitude
            partial.mAmp *= mAmpFactor;
                        
            // Freq factor (post)
            partial.mFreq *= mFreqFactor;
                
            //
            BL_FLOAT freq = partial.mFreq;
                
            BL_FLOAT amp = partial.mAmp;
            
            BL_FLOAT samp = amp*std::sin(phase);
            
            samp *= SYNTH_AMP_COEFF;
            
            if (freq >= SYNTH_MIN_FREQ)
                samples->Get()[j] += samp;
            
            phase += twoPiSR*freq;
        }
        
        mPartials[i].mPhase = phase;
    }
}

void
SASFrame6::ComputeSamplesPartialsResynth(WDL_TypedBuf<BL_FLOAT> *samples)
{    
    bool transientDetected = false;
#if ENABLE_ONSET_DETECTION
#if !ONSET_HISTORY_HACK
    mOnsetDetector->Detect(mInputMagns); // Origin
#else
    mOnsetDetector->Detect(mInputMagnsHistory[0]);
#endif
    
    BL_FLOAT onsetValue = mOnsetDetector->GetCurrentOnsetValue();
    
#if DETECT_TRANSIENTS_ONSET
    transientDetected = (onsetValue > ONSET_VALUE_THRESHOLD);
#endif
    
#endif
    
    BLUtils::FillAllZero(samples);
    
    // First time: initialize the partials
    if (mSASPartials.empty())
    {
        mSASPartials.resize(SYNTH_MAX_NUM_PARTIALS);
    }
    
    if (mPrevSASPartials.empty())
        mPrevSASPartials = mSASPartials;
    
    if (mPrevColor.GetSize() != mColor.GetSize())
        mPrevColor = mColor;
    
    if (mPrevNormWarping.GetSize() != mNormWarping.GetSize())
        mPrevNormWarping = mNormWarping;
    
    // For applying amplitude the correct way
    // Keep a denominator for each sample.
    //WDL_TypedBuf<BL_FLOAT> ampDenoms;
    //ampDenoms.Resize(samples->GetSize());
    //BLUtils::FillAllZero(&ampDenoms);
    
    // Optim
    BL_FLOAT phaseCoeff = 2.0*M_PI/mSampleRate;
    BL_FLOAT hzPerBin = mSampleRate/mBufferSize;
    BL_FLOAT hzPerBinInv = 1.0/hzPerBin;
    
    // Sort partials by amplitude, in order to play the highest amplitudes ?
    BL_FLOAT partialFreq = mFrequency*mFreqFactor;
    int partialIndex = 0;
    while((partialFreq < mSampleRate/2.0) && (partialIndex < SYNTH_MAX_NUM_PARTIALS))
    {
        if (partialIndex > 0)
        {
            if ((!mSynthEvenPartials) && (partialIndex % 2 == 0))
            {
                partialIndex++;
                partialFreq = mFrequency*mFreqFactor*(partialIndex + 1);
                
                continue;
            }

            if ((!mSynthOddPartials) && (partialIndex % 2 == 1))
            {
                partialIndex++;
                partialFreq = mFrequency*mFreqFactor*(partialIndex + 1);
                
                continue;
            }
        }
        
        if (partialFreq > SYNTH_MIN_FREQ)
        {
            // Current and prev partials
            SASPartial &partial = mSASPartials[partialIndex];
            partial.mFreq = partialFreq;

            // Correct! (global amplitude will be applied later)
            partial.mAmp = 1.0;
            
            const SASPartial &prevPartial = mPrevSASPartials[partialIndex];
            
            // Current phase
            BL_FLOAT phase = prevPartial.mPhase;
            
            // Bin param
            BL_FLOAT binIdx = partialFreq*hzPerBinInv;
            
            // Warping
            BL_FLOAT w0 = GetWarping(mPrevNormWarping, binIdx);
            BL_FLOAT w1 = GetWarping(mNormWarping, binIdx);
            
            // Color
            BL_FLOAT prevBinIdxc = w0*prevPartial.mFreq*hzPerBinInv;
            BL_FLOAT binIdxc = w1*partial.mFreq*hzPerBinInv;

            //
            BL_FLOAT col0 = GetColor(mPrevColor, prevBinIdxc);
            BL_FLOAT col1 = GetColor(mColor, binIdxc);
            
            if ((col0 < BL_EPS) && (col1 < BL_EPS))
            // Color will be 0, no need to synthetize samples
            {
                partialIndex++;
                partialFreq = mFrequency*mFreqFactor*(partialIndex + 1);
                
                // TODO: update phase!!
                
                continue;
            }
            
            // Loop
            //
            BL_FLOAT t = 0.0;
            BL_FLOAT tStep = 1.0/(samples->GetSize() - 1);
            for (int i = 0; i < samples->GetSize(); i++)
            {
                // Compute norm warping
                BL_FLOAT w = 1.0;
                if (binIdx < mNormWarping.GetSize() - 1)
                {
                    w = (1.0 - t)*w0 + t*w1;
                }
                
                // Freq
                BL_FLOAT freq = GetFreq(prevPartial.mFreq, partial.mFreq, t);
                
                // Warping
                freq *= w;
                
                // Color
                BL_FLOAT col = GetCol(col0, col1, t);

                // Sample
                
                // Not 100% perfect (partials les neat)
                //BL_FLOAT samp;
                //SIN_LUT_GET(SAS_FRAME_SIN_LUT, samp, phase);
                
                // Better quality.
                // No "blurb" between frequencies
                BL_FLOAT samp = std::sin(phase);
                
                if (freq >= SYNTH_MIN_FREQ)
                {
                    // Generate samples only if not on an transient
                    if (!transientDetected)
                    {
                        samples->Get()[i] += samp*col;
                        //ampDenoms.Get()[i] += col;
                    }
                }
                
                t += tStep;
                phase += phaseCoeff*freq;
            }
            
            // Compute next phase
            mSASPartials[partialIndex].mPhase = phase;
        }
        
        partialIndex++;            
            
        partialFreq = mFrequency*mFreqFactor*(partialIndex + 1);
    }

    // At the end, apply amplitude
    //
    BL_FLOAT tStep = 1.0/(samples->GetSize() - 1);
    BL_FLOAT t = 0.0;
    for (int i = 0; i < samples->GetSize(); i++)
    {
        BL_FLOAT s = samples->Get()[i];
        //BL_FLOAT d = ampDenoms.Get()[i];
        
        //if (d < BL_EPS)
        //    continue;
        
        BL_FLOAT amp = GetAmp(mPrevAmplitude, mAmplitude, t);


        // Resynth the correct volume!
        BL_FLOAT a = amp*s;
        
        samples->Get()[i] = a;
        
        t += tStep;
    }
    
    mPrevAmplitude = mAmplitude;

    if (transientDetected)
    {
        // Set the volumes to 0, for next step, to avoid click
        mPrevAmplitude = 0.0;
    }
    
    mPrevSASPartials = mSASPartials;
}

BL_FLOAT
SASFrame6::GetColor(const WDL_TypedBuf<BL_FLOAT> &color, BL_FLOAT binIdx)
{
#if NEAREST_INTERP
    // Quick method
    binIdx = bl_round(ninIdx);
    if (binIdx < color.GetSize() - 1)
    {
        BL_FLOAT col = color.Get()[(int)binIdx];
        
        return col;
    }
    return 0.0;
#endif
    
    BL_FLOAT col = 0.0;
    if (binIdx < color.GetSize() - 1)
    {
        BL_FLOAT t = binIdx - (int)binIdx;
        
        BL_FLOAT col0 = color.Get()[(int)binIdx];
        BL_FLOAT col1 = color.Get()[((int)binIdx) + 1];
        
        // Method 2: interpolate in dB
#if INTERP_RESCALE
        col0 = mScale->ApplyScale(Scale::DB, col0, mMinAmpDB, (BL_FLOAT)0.0);
        col1 = mScale->ApplyScale(Scale::DB, col1, mMinAmpDB, (BL_FLOAT)0.0);
#endif
        
        col = (1.0 - t)*col0 + t*col1;

#if INTERP_RESCALE
        col = mScale->ApplyScale(Scale::DB_INV, col, mMinAmpDB, (BL_FLOAT)0.0);
#endif
    }

    //col *= mColorFactor;
    col = ApplyColorFactor(col, mColorFactor);
    
    return col;
}

BL_FLOAT
SASFrame6::GetWarping(const WDL_TypedBuf<BL_FLOAT> &warping,
                      BL_FLOAT binIdx)
{
#if NEAREST_INTERP
    // Quick method
    binIdx = bl_round(binIdx);
    if (binIdx < warping.GetSize() - 1)
    {
        BL_FLOAT w = warping.Get()[(int)binIdx];

        // Warping is center on 1
        w -= 1.0;
        w *= mWarpingFactor;
        w += 1.0;
        
        return w;
    }
    return 1.0;
#endif
    
    BL_FLOAT w = 0.0;
    if (binIdx < warping.GetSize() - 1)
    {
        BL_FLOAT t = binIdx - (int)binIdx;
        
        BL_FLOAT w0 = warping.Get()[(int)binIdx];
        BL_FLOAT w1 = warping.Get()[((int)binIdx) + 1];
        
        w = (1.0 - t)*w0 + t*w1;
    }

    // Warping is center on 1
    w -= 1.0;
    
    w *= mWarpingFactor;

    w += 1.0;
    
    return w;
}

void
SASFrame6::SetAmpFactor(BL_FLOAT factor)
{
    mAmpFactor = factor;
}

void
SASFrame6::SetFreqFactor(BL_FLOAT factor)
{
    mFreqFactor = factor;
}

void
SASFrame6::SetColorFactor(BL_FLOAT factor)
{
    mColorFactor = factor;
}

void
SASFrame6::SetWarpingFactor(BL_FLOAT factor)
{
    mWarpingFactor = factor;
}

void
SASFrame6::ComputeAna()
{
    ComputeAmplitude();
    ComputeFrequency();
    ComputeColor();
    ComputeNormWarping();
}

void
SASFrame6::ComputeAmplitude()
{
    // Amp must not be in dB, but direct!
    //mPrevAmplitude = mAmplitude;
    mAmplitude = 0.0;

    const vector<Partial> &partials = mPartials;

    BL_FLOAT amplitude = 0.0;
    for (int i = 0; i < partials.size(); i++)
    {
        const Partial &p = partials[i];
        
        BL_FLOAT amp = p.mAmp;
        amplitude += amp;
    }
    
    mAmplitude = amplitude;
}

void
SASFrame6::ComputeFrequency()
{
    BL_FLOAT freq =
        mPartialsToFreq->ComputeFrequency(mInputMagns, mInputPhases, mPartials);
    
    mFrequency = freq;
    
    // Smooth
    if (mPrevFrequency < 0.0)
        mPrevFrequency = freq;
    else
    {
        freq = (1.0 - FREQ_SMOOTH_COEFF)*freq + FREQ_SMOOTH_COEFF*mPrevFrequency;
        
        mPrevFrequency = freq;
        mFrequency = freq;
    }
}

void
SASFrame6::ComputeColor()
{
    mPrevColor = mColor;
    
    WDL_TypedBuf<BL_FLOAT> prevColor = mColor;
    
    ComputeColorAux();
    
    if (prevColor.GetSize() != mColor.GetSize())
        return;
    
    // Smooth
    for (int i = 0; i < mColor.GetSize(); i++)
    {
        BL_FLOAT col = mColor.Get()[i];
        BL_FLOAT prevCol = prevColor.Get()[i];
        
        BL_FLOAT result = COLOR_SMOOTH_COEFF*prevCol + (1.0 - COLOR_SMOOTH_COEFF)*col;
        
        mColor.Get()[i] = result;
    }
}

void
SASFrame6::ComputeColorAux()
{
    BL_FLOAT minColorValue = 0.0;
    BL_FLOAT undefinedValue = -1.0; // -300dB
    
    mColor.Resize(mBufferSize/2);
    
    if (mFrequency < BL_EPS)
    {
        BLUtils::FillAllValue(&mColor, minColorValue);
        
        return;
    }
    
    // Will interpolate values between the partials
    BLUtils::FillAllValue(&mColor, undefinedValue);
    
    // Fix bounds at 0
    mColor.Get()[0] = minColorValue;
    mColor.Get()[mBufferSize/2 - 1] = minColorValue;
    
    BL_FLOAT hzPerBin = mSampleRate/mBufferSize;
    
    // Put the values we have
    for (int i = 0; i < mPartials.size(); i++)
    {
        const Partial &p = mPartials[i];
 
        // Dead or zombie: do not use for color enveloppe
        // (this is important !)
        if (p.mState != Partial::ALIVE)
            continue;
        
        BL_FLOAT idx = p.mFreq/hzPerBin;
        
        // TODO: make an interpolation, it is not so good to align to bins
        idx = bl_round(idx);
        
        BL_FLOAT amp = p.mAmp;
        
        if (((int)idx >= 0) && ((int)idx < mColor.GetSize()))
            mColor.Get()[(int)idx] = amp;

        // TEST: with this and sines example, the first partial has the right gain
        // Without this, the first synth partial amp is lower 
        if (i == 0)
            mColor.Get()[0] = amp;
    }
    
#if COLOR_CUT_MISSING_PARTIALS
    // Put some zeros when partials are missing
    BL_FLOAT freq = mFrequency;
    while(freq < mSampleRate/2.0)
    {
        if (!FindPartial(freq))
        {
            BL_FLOAT idx = freq/hzPerBin;
            if (idx >= mColor.GetSize())
                break;
            
            mColor.Get()[(int)idx] = minColorValue;
        }
        
        freq += mFrequency;
    }
#endif

#if FILL_ZERO_FIRST_LAST_VALUES_COLOR
    // Avoid interpolating to the last partial value to 0
    // Would make color where there is no sound otherwise
    // (e.g example with just some sine waves is false)
    FillLastValues(&mColor, mPartials, minColorValue);
#endif
    
    // Fill al the other value
    bool extendBounds = false;
#if !COLOR_ENVELOPE_USE_LAGRANGE_INTERP
    BLUtils::FillMissingValues(&mColor, extendBounds, undefinedValue);
#else
    // Try to add intermediate values, to avoid too many oscillations
    // (not working, this leads to the same result as linear (no Lagrange)
    BLUtils::AddIntermediateValues(&mColor, LAGRANGE_MIN_NUM_COLOR_VALUES,
                                   undefinedValue);
    BLUtils::FillMissingValuesLagrange(&mColor, extendBounds, undefinedValue);
    // Lagrange oscillations can make the values to become negative sometimes
    BLUtils::ClipMin(&mColor, 0.0);
#endif

    // Fixed version => so the color max is 1
    // Normalize the color
    BL_FLOAT maxCol = BLUtils::ComputeMax(mColor);
    if (maxCol > BL_EPS)
        BLUtils::MultValues(&mColor, 1.0/maxCol);
}

void
SASFrame6::ComputeNormWarping()
{
    // Normal warping
    //
    mPrevNormWarping = mNormWarping;
    
    ComputeNormWarpingAux(&mNormWarping);
    
    if (mPrevNormWarping.GetSize() == mNormWarping.GetSize())
        BLUtils::Smooth(&mNormWarping, &mPrevNormWarping, WARPING_SMOOTH_COEFF);
    
    // Inverse warping
    //
    mPrevNormWarpingInv = mNormWarpingInv;
    
    ComputeNormWarpingAux(&mNormWarpingInv, true);
    
    if (mPrevNormWarpingInv.GetSize() == mNormWarpingInv.GetSize())
        BLUtils::Smooth(&mNormWarpingInv, &mPrevNormWarpingInv, WARPING_SMOOTH_COEFF);
}

// Problem: when incorrect partials are briefly detected, they affect warping a lot
// Solution: take each theorical synth partials, and find the closest detected
// partial to compute the norma warping (and ignore other partials)
// => a lot more robust for low thresholds, when we have many partials
//
// NOTE: this is not still perfect if we briefly loose the tracking
void
SASFrame6::ComputeNormWarpingAux(WDL_TypedBuf<BL_FLOAT> *warping,
                                 bool inverse)
{
    // Init

    int maxNumPartials = SYNTH_MAX_NUM_PARTIALS;
    //BL_FLOAT minFreq = 20.0;
    //int maxNumPartials = mSampleRate*0.5/minFreq;
    
    vector<PartialAux> theoricalPartials;
    theoricalPartials.resize(maxNumPartials);
    for (int i = 0; i < theoricalPartials.size(); i++)
    {
        theoricalPartials[i].mFreq = (i + 1)*mFrequency;
        theoricalPartials[i].mWarping = -1.0;
    }
    
    // Compute best match
    for (int i = 0; i < theoricalPartials.size(); i++)
    {
        PartialAux &pa = theoricalPartials[i];
        
        for (int j = 0; j < mPartials.size(); j++)
        {
            const Partial &p = mPartials[j];
            
            // Do no add to warping if dead or zombie
            if (p.mState != Partial::ALIVE)
                continue;

            // GOOD!
            // mFrequency is the step...
            // Take half the step! Over half the step,
            // this is another theorical partial which will be interesting
            if (p.mFreq < pa.mFreq - mFrequency*0.5)
                continue;
            if (p.mFreq > pa.mFreq + mFrequency*0.5)
                // Can not break, because now partials can be sorted by id
                // instead of by freq
                //break;
                continue;
            
            BL_FLOAT w = p.mFreq/pa.mFreq;
            
#if LIMIT_WARPING_MAX
            // Discard too high warping values,
            // that can come to short invalid partials
            // spread randomly
            if ((w < 0.8) || (w > 1.25))
                continue;
#endif

            // Take the smallest warping
            // (i.e the closest partial)
            if (pa.mWarping < 0.0)
                pa.mWarping = w;
            else
            {
                // Compute the ral "smallest" warping value
                BL_FLOAT wNorm = (w < 1.0) ? 1.0/w : w;
                BL_FLOAT paNorm = (pa.mWarping < 1.0) ? 1.0/pa.mWarping : pa.mWarping;
                
                // Keep smallest warping
                if (wNorm < paNorm)
                    pa.mWarping = w;
            }
        }
    }

    // Fill missing partials values
    // (in case no matching detected partial was found for a given theorical partial)
    for (int i = 0; i < theoricalPartials.size(); i++)
    {
        PartialAux &pa = theoricalPartials[i];

        if (pa.mWarping < 0.0)
            // Not defined
        {
            BL_FLOAT leftWarp = -1.0;
            int leftIdx = -1;
            for (int j = i - 1; j >= 0; j--)
            {
                const PartialAux &paL = theoricalPartials[j];
                if (paL.mWarping > 0.0)
                {
                    leftWarp = paL.mWarping;
                    leftIdx = j;
                    break;
                }
            }

            BL_FLOAT rightWarp = -1.0;
            int rightIdx = -1;
            for (int j = i + 1; j < theoricalPartials.size(); j++)
            {
                const PartialAux &paR = theoricalPartials[j];
                if (paR.mWarping > 0.0)
                {
                    rightWarp = paR.mWarping;
                    rightIdx = j;
                    break;
                }
            }

            if ((leftWarp > 0.0) && (rightWarp > 0.0))
            {
                BL_FLOAT t = (i - leftIdx)/(rightIdx - leftIdx);
                BL_FLOAT w = (1.0 - t)*leftWarp + t*rightWarp;

                pa.mWarping = w;
            }
        }
    }
    
    
    // Fill the warping envelope
    //
    
    warping->Resize(mBufferSize/2);
    
    if (mFrequency < BL_EPS)
    {
        BLUtils::FillAllValue(warping, (BL_FLOAT)1.0);
        
        return;
    }
    
    // Will interpolate values between the partials
    BL_FLOAT undefinedValue = -1.0;
    BLUtils::FillAllValue(warping, undefinedValue);
    
    // Fix bounds at 1
    warping->Get()[0] = 1.0;
    warping->Get()[mBufferSize/2 - 1] = 1.0;
    
    if (mPartials.size() < 2)
    {
        BLUtils::FillAllValue(warping, (BL_FLOAT)1.0);
        
        return;
    }
    
    // Fundamental frequency
    BL_FLOAT hzPerBin = mSampleRate/mBufferSize;

    for (int i = 0; i < theoricalPartials.size(); i++)
    {
        PartialAux &pa = theoricalPartials[i];
        
        BL_FLOAT idx = pa.mFreq/hzPerBin;
        BL_FLOAT w = pa.mWarping;

        // Fix missing values in the thorical partials here
        if (w < 0.0)
            w = 1.0;
        
        if (inverse)
        {
            if (w > BL_EPS)
            {
                idx *= w;
            
                w = 1.0/w;
            }
        }
        
        // TODO: make an interpolation ?
        idx = bl_round(idx);
        
        if ((idx > 0) && (idx < warping->GetSize()))
            warping->Get()[(int)idx] = w;
    }

    // Do not do this, no need because the undefined theorical partials
    // will now have a warping value of 1 assigned 
#if FILL_ZERO_FIRST_LAST_VALUES_WARPING
    // Keep the first partial warping of reference is chroma-compute freq
    // Avoid warping the first partial
    //FillFirstValues(warping, mPartials, 1.0);
    
    // NEW
    FillLastValues(warping, mPartials, 1.0);
#endif

    // Fill all the other value
    bool extendBounds = false;
#if !WARP_ENVELOPE_USE_LAGRANGE_INTERP
    BLUtils::FillMissingValues(warping, extendBounds, undefinedValue);
#else
    BLUtils::FillMissingValuesLagrange(warping, extendBounds, undefinedValue);
#endif
}

bool
SASFrame6::FindPartial(BL_FLOAT freq)
{
#define FIND_COEFF 0.25
    
    BL_FLOAT step = mFrequency*FIND_COEFF;
    
    for (int i = 0; i < mPartials.size(); i++)
    {
        const Partial &p = mPartials[i];
        
        if ((freq > p.mFreq - step) &&
            (freq < p.mFreq + step))
            // Found
            return true;
    }
    
    return false;
}

// Interpolate in amp
void
SASFrame6::GetPartial(Partial *result, int index, BL_FLOAT t)
{
    const Partial &currentPartial = mPartials[index];

    int prevPartialIdx = currentPartial.mLinkedId;
        
    *result = currentPartial;
    
    // Manage decrease of dead partials
    //
    if ((currentPartial.mState == Partial::DEAD) && currentPartial.mWasAlive)
    {
        // Decrease progressively the amplitude
        result->mAmp = 0.0;
        
        if (prevPartialIdx != -1)
            // Interpolate
        {
            BL_FLOAT t0 = t/SYNTH_DEAD_PARTIAL_DECREASE;
            if (t0 <= 1.0)
            {
                // Interpolate in amp
                const Partial &prevPartial = mPrevPartials[prevPartialIdx];
                
                BL_FLOAT amp = (1.0 - t0)*prevPartial.mAmp;
                result->mAmp = amp;
            }
        }
    }
    
    // Manage interpolation of freq and amp
    //
    if ((currentPartial.mState != Partial::DEAD) && currentPartial.mWasAlive)
    {
        if (prevPartialIdx != -1)
        {
            const Partial &prevPartial = mPrevPartials[prevPartialIdx];
                
            if (prevPartial.mState == Partial::ALIVE)
            {
                result->mFreq = (1.0 - t)*prevPartial.mFreq + t*currentPartial.mFreq;
                
                BL_FLOAT amp = (1.0 - t)*prevPartial.mAmp + t*currentPartial.mAmp;
                result->mAmp = amp;
            }
        }
        else
        // New partial, fade in
        {
            // NOTE: this avoids vertical bars in the spectrogram when
            // a partial starts
                
            // Increase progressively the amplitude
                
            // Interpolate
            BL_FLOAT t0 = t/SYNTH_DEAD_PARTIAL_DECREASE;
            if (t0 > 1.0)
                t0 = 1.0;
            
            BL_FLOAT amp = t0*currentPartial.mAmp;
            result->mAmp = amp;
        }
    }
}

void
SASFrame6::MixFrames(SASFrame6 *result,
                     const SASFrame6 &frame0,
                     const SASFrame6 &frame1,
                     BL_FLOAT t, bool mixFreq)
{
    // Amp
    BL_FLOAT amp0 = frame0.GetAmplitude();
    BL_FLOAT amp1 = frame1.GetAmplitude();
    BL_FLOAT resultAmp = (1.0 - t)*amp0 + t*amp1;
    result->SetAmplitude(resultAmp);
    
    // Freq
    if (mixFreq)
    {
        BL_FLOAT freq0 = frame0.GetFrequency();
        BL_FLOAT freq1 = frame1.GetFrequency();
        BL_FLOAT resultFreq = (1.0 - t)*freq0 + t*freq1;
        
        result->SetFrequency(resultFreq);
    }
    else
    {
        BL_FLOAT freq0 = frame0.GetFrequency();
        result->SetFrequency(freq0);
    }
    
    // Color
    WDL_TypedBuf<BL_FLOAT> color0;
    frame0.GetColor(&color0);
    
    WDL_TypedBuf<BL_FLOAT> color1;
    frame1.GetColor(&color1);
    
    WDL_TypedBuf<BL_FLOAT> resultColor;
    BLUtils::Interp(&resultColor, &color0, &color1, t);
    
    result->SetColor(resultColor);
    
    // Warping
    WDL_TypedBuf<BL_FLOAT> warp0;
    frame0.GetNormWarping(&warp0);
    
    WDL_TypedBuf<BL_FLOAT> warp1;
    frame1.GetNormWarping(&warp1);
    
    WDL_TypedBuf<BL_FLOAT> resultWarping;
    BLUtils::Interp(&resultWarping, &warp0, &warp1, t);
    
    result->SetNormWarping(resultWarping);
    
    // Noise envelope
    WDL_TypedBuf<BL_FLOAT> noise0;
    frame0.GetNoiseEnvelope(&noise0);
    
    WDL_TypedBuf<BL_FLOAT> noise1;
    frame1.GetNoiseEnvelope(&noise1);
    
    WDL_TypedBuf<BL_FLOAT> resultNoise;
    BLUtils::Interp(&resultNoise, &noise0, &noise1, t);
    
    result->SetNoiseEnvelope(resultNoise);
}

BL_FLOAT
SASFrame6::GetFreq(BL_FLOAT freq0, BL_FLOAT freq1, BL_FLOAT t)
{
#if INTERP_RESCALE
    // Mel scale
    BL_FLOAT maxFreq = mSampleRate*0.5;
    
    freq0 = mScale->ApplyScale(Scale::MEL, freq0/maxFreq, (BL_FLOAT)0.0, maxFreq);
    freq1 = mScale->ApplyScale(Scale::MEL, freq1/maxFreq, (BL_FLOAT)0.0, maxFreq);
#endif
    
    BL_FLOAT freq = (1.0 - t)*freq0 + t*freq1;

#if INTERP_RESCALE
    freq = mScale->ApplyScale(Scale::MEL_INV, freq, (BL_FLOAT)0.0, maxFreq);    
    freq *= maxFreq;
#endif

    //freq *= mFreqFactor;
    
    return freq;
}

BL_FLOAT
SASFrame6::GetAmp(BL_FLOAT amp0, BL_FLOAT amp1, BL_FLOAT t)
{
#if INTERP_RESCALE
    // If amp is not already in dB)
    
    amp0 = mScale->ApplyScale(Scale::DB, amp0, mMinAmpDB, (BL_FLOAT)0.0);
    amp1 = mScale->ApplyScale(Scale::DB, amp1, mMinAmpDB, (BL_FLOAT)0.0);
#endif
    
    BL_FLOAT amp = (1.0 - t)*amp0 + t*amp1;

#if INTERP_RESCALE
    amp = mScale->ApplyScale(Scale::DB_INV, amp, mMinAmpDB, (BL_FLOAT)0.0);
#endif

    amp *= mAmpFactor;
    
    return amp;
}

BL_FLOAT
SASFrame6::GetCol(BL_FLOAT col0, BL_FLOAT col1, BL_FLOAT t)
{
#if INTERP_RESCALE
    // Method 2: (if col is not already in dB)
    col0 = mScale->ApplyScale(Scale::DB, col0, mMinAmpDB, (BL_FLOAT)0.0);
    col1 = mScale->ApplyScale(Scale::DB, col1, mMinAmpDB, (BL_FLOAT)0.0);
#endif
    
    BL_FLOAT col = (1.0 - t)*col0 + t*col1;

#if INTERP_RESCALE
    col = mScale->ApplyScale(Scale::DB_INV, col, mMinAmpDB, (BL_FLOAT)0.0);
#endif
    
    return col;
}

void
SASFrame6::FillLastValues(WDL_TypedBuf<BL_FLOAT> *values,
                          const vector<Partial> &partials, BL_FLOAT val)
{    
    // First, find the last bin index
    int maxIdx = -1;
    for (int i = 0; i < partials.size(); i++)
    {
        const Partial &p = partials[i];
        
        // Dead or zombie: do not use for color enveloppe
        // (this is important !)
        if (p.mState != Partial::ALIVE)
            continue;
            
        //if (p.mRightIndex > maxIdx)
        //    maxIdx = p.mRightIndex;
        if (p.mPeakIndex > maxIdx)
            maxIdx = p.mPeakIndex;
    }
    
    // Then fill with zeros after this index
    if (maxIdx > 0)
    {
        for (int i = maxIdx + 1; i < values->GetSize(); i++)
        {
            values->Get()[i] = val;
        }
    }
}

void
SASFrame6::FillFirstValues(WDL_TypedBuf<BL_FLOAT> *values,
                           const vector<Partial> &partials, BL_FLOAT val)
{
    // First, find the last bin idex
    int minIdx = values->GetSize() - 1;
    for (int i = 0; i < partials.size(); i++)
    {
        const Partial &p = partials[i];

        // Dead or zombie: do not use for color enveloppe
        // (this is important !)
        if (p.mState != Partial::ALIVE)
            continue;
        
        //if (p.mLeftIndex < minIdx)
        //    minIdx = p.mLeftIndex;
        if (p.mRightIndex < minIdx)
            minIdx = p.mRightIndex;
    }
    
    // Then fill with zeros after this index
    if (minIdx < values->GetSize() - 1)
    {
        for (int i = 0; i <= minIdx; i++)
        {
            values->Get()[i] = val;
        }
    }            
}

BL_FLOAT
SASFrame6::ApplyColorFactor(BL_FLOAT color, BL_FLOAT factor)
{
#if 0
    // Naive
    BL_FLOAT res = color*factor;
#endif

#if 1
    // Sigmoid
    BL_FLOAT a = factor*0.5;
    if (a < BL_EPS)
        a = BL_EPS;
    if (a > 1.0 - BL_EPS)
        a = 1.0 - BL_EPS;

    BL_FLOAT res = BLUtilsMath::ApplySigmoid(color, a);
#endif
        
    return res;
}

void
SASFrame6::ApplyColorFactor(WDL_TypedBuf<BL_FLOAT> *color, BL_FLOAT factor)
{
    for (int i = 0; i < color->GetSize(); i++)
    {
        BL_FLOAT col = color->Get()[i];
        col = ApplyColorFactor(col, factor);
        color->Get()[i] = col;
    }
}

void
SASFrame6::LinkPartialsIdx(vector<Partial> *partials0,
                           vector<Partial> *partials1)
{
    // Init
    for (int i = 0; i < partials0->size(); i++)
        (*partials0)[i].mLinkedId = -1;
    for (int i = 0; i < partials1->size(); i++)
        (*partials1)[i].mLinkedId = -1;

    int i0 = 0;
    int i1 = 0;
    while(true)
    {
        if (i0 >= partials0->size())
            return;
        if (i1 >= partials1->size())
            return;
        
        Partial &p0 = (*partials0)[i0];
        Partial &p1 = (*partials1)[i1];

        if (p0.mId == p1.mId)
        {
            p0.mLinkedId = i1;
            p1.mLinkedId = i0;

            i0++;
            i1++;
            
            continue;
        }

        if (p0.mId > p1.mId)
            i1++;

        if (p0.mId < p1.mId)
            i0++;
    }
}
