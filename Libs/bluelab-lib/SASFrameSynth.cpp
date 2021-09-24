#include <BLUtils.h>
#include <BLUtilsMath.h>

// Do not speed up...
#include <SinLUT.h>

#include <BLDebug.h>

#include "SASFrameSynth.h"

// Synthesis
#define SYNTH_AMP_COEFF 4.0
#define SYNTH_MIN_FREQ 50.0
#define SYNTH_DEAD_PARTIAL_DECREASE 0.25

// Set to 0 for optimization
//
// Origin: interpolate everything in db or mel scale
//
// Optim: this looks to work well when set to 0
// Perfs: 26% => 17%CPU
#define INTERP_RESCALE 0 // Origin: 1

// TEST: nearest interpolation, to be more quick
#define NEAREST_INTERP 0 //1

// Optim: optimizes a little (~5/15%)
SIN_LUT_CREATE(SAS_FRAME_SIN_LUT, 4096);

// 4096*64 (2MB)
// 2x slower than 4096
//SIN_LUT_CREATE(SAS_FRAME_SIN_LUT, 262144);

//
SASFrameSynth::SASFrameSynth(int bufferSize, int oversampling,
                             int freqRes, BL_FLOAT sampleRate)
{
    SIN_LUT_INIT(SAS_FRAME_SIN_LUT);

    mBufferSize = bufferSize;
    mOverlapping = oversampling;
    mFreqRes = freqRes;
    mSampleRate = sampleRate;
    
    mHarmoNoiseMix = 1.0;
    
    mSynthMode = RAW_PARTIALS;

    mMinAmpDB = -120.0;

    mSynthEvenPartials = true;
    mSynthOddPartials = true;

    mAmplitude = 0.0;
    mPrevAmplitude = 0.0;
    
    mFrequency = 0.0;
    mPrevFrequency = -1.0;
    
    mAmpFactor = 1.0;
    mFreqFactor = 1.0;
    mColorFactor = 1.0;
    mWarpingFactor = 1.0;
}

SASFrameSynth::~SASFrameSynth() {}

void
SASFrameSynth::Reset(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    
    mAmplitude = 0.0;
    mPrevAmplitude = 0.0;
    
    mFrequency = 0.0;
    mPrevFrequency = -1.0;
}

void
SASFrameSynth::Reset(int bufferSize, int oversampling,
                     int freqRes, BL_FLOAT sampleRate)
{
    mBufferSize = bufferSize;
    mOverlapping = oversampling;
    mFreqRes = freqRes;
    mSampleRate = sampleRate;
    
    Reset(sampleRate);
}

void
SASFrameSynth::SetMinAmpDB(BL_FLOAT ampDB)
{
    mMinAmpDB = ampDB;
}

void
SASFrameSynth::SetSynthMode(enum SynthMode mode)
{
    mSynthMode = mode;
}

SASFrameSynth::SynthMode
SASFrameSynth::GetSynthMode() const
{
    return mSynthMode;
}

void
SASFrameSynth::SetSynthEvenPartials(bool flag)
{
    mSynthEvenPartials = flag;
}

void
SASFrameSynth::SetSynthOddPartials(bool flag)
{
    mSynthOddPartials = flag;
}

void
SASFrameSynth::SetHarmoNoiseMix(BL_FLOAT mix)
{
    mHarmoNoiseMix = mix;
}

BL_FLOAT
SASFrameSynth::GetHarmoNoiseMix()
{
    return mHarmoNoiseMix;
}

void
SASFrameSynth::SetAmpFactor(BL_FLOAT factor)
{
    mAmpFactor = factor;
}

void
SASFrameSynth::SetFreqFactor(BL_FLOAT factor)
{
    mFreqFactor = factor;
}

void
SASFrameSynth::SetColorFactor(BL_FLOAT factor)
{
    mColorFactor = factor;
}

void
SASFrameSynth::SetWarpingFactor(BL_FLOAT factor)
{
    mWarpingFactor = factor;
}

void
SASFrameSynth::AddSASFrame(const SASFrame6 &frame)
{
    mPrevSASFrame = mSASFrame;
    mSASFrame = frame;

    SetSASFactors();

    mPrevAmplitude = mAmplitude;
    mAmplitude = mSASFrame.GetAmplitude();

    mPrevFrequency = mFrequency;
    mFrequency = mSASFrame.GetFrequency();
}

void
SASFrameSynth::GetSASFrame(SASFrame6 *frame)
{
    *frame = mSASFrame;
}

void
SASFrameSynth::ComputeSamples(WDL_TypedBuf<BL_FLOAT> *samples)
{
    UpdateSASData();

    bool onsetDetected = mSASFrame.GetOnsetDetected();
    
    if (!onsetDetected)
        // Generate samples only if not on an onset (transient)
    {
        if (mSynthMode == RAW_PARTIALS)
            ComputeSamplesPartialsRaw(samples);
        else if (mSynthMode == SOURCE_PARTIALS)
            ComputeSamplesPartialsSourceNorm(samples);
            //ComputeSamplesPartialsSource(samples);
        else if (mSynthMode == RESYNTH_PARTIALS)
            ComputeSamplesPartialsResynth(samples);
    }
    else
    {
        // Set the volumes to 0, for next step, to avoid click
        mPrevAmplitude = 0.0;
    }
}

// Directly use partials provided
void
SASFrameSynth::ComputeSamplesPartialsRaw(WDL_TypedBuf<BL_FLOAT> *samples)
{
    samples->Resize(mBufferSize);
    
    BLUtils::FillAllZero(samples);
    
    if (mPartials.empty())
    {
        return;
    }

    SASFrame6::SASPartial partial;
    for (int i = 0; i < mPartials.size(); i++)
    {
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
SASFrameSynth::ComputeSamplesPartialsSourceNorm(WDL_TypedBuf<BL_FLOAT> *samples)
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
        SASFrame6::SASPartial partial;
        for (int j = 0; j < numSamples; j++)
        {            
            // Get interpolated partial
            BL_FLOAT partialT = ((BL_FLOAT)j)/numSamples;
            GetPartial(&partial, i, partialT);
            
            BL_FLOAT binIdx = partial.mFreq*hzPerBinInv;
            
            // Apply SAS parameters to current partial
            //

            // Cancel color(1)
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
            BL_FLOAT w0 = GetWarping(mWarpingInv, binIdx);
            mWarpingFactor = warpFactor;

            partial.mFreq *= w0;

            // Recompute bin idx
            binIdx = partial.mFreq*hzPerBinInv;
            
            // Apply warping
            BL_FLOAT w = GetWarping(mWarping, binIdx);
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

// Directly use partials provided, but also apply SAS parameter to them
void
SASFrameSynth::ComputeSamplesPartialsSource(WDL_TypedBuf<BL_FLOAT> *samples)
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
        SASFrame6::SASPartial partial;
        int numSamples = samples->GetSize()/mOverlapping;
        for (int j = 0; j < numSamples; j++)
        {            
            // Get interpolated partial
            BL_FLOAT partialT = ((BL_FLOAT)j)/numSamples;
            GetPartial(&partial, i, partialT);
            
            BL_FLOAT binIdx = partial.mFreq*hzPerBinInv;
            
            // Apply SAS parameters to current partial
            //
            
            // Apply warping
            BL_FLOAT w = GetWarping(mWarping, binIdx);
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
SASFrameSynth::ComputeSamplesPartialsResynth(WDL_TypedBuf<BL_FLOAT> *samples)
{        
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
    
    if (mPrevWarping.GetSize() != mWarping.GetSize())
        mPrevWarping = mWarping;
    
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
            SASFrame6::SASPartial &partial = mSASPartials[partialIndex];
            partial.mFreq = partialFreq;

            // Correct! (global amplitude will be applied later)
            partial.mAmp = 1.0;
            
            const SASFrame6::SASPartial &prevPartial = mPrevSASPartials[partialIndex];
            
            // Current phase
            BL_FLOAT phase = prevPartial.mPhase;
            
            // Bin param
            BL_FLOAT binIdx = partialFreq*hzPerBinInv;
            
            // Warping
            BL_FLOAT w0 = GetWarping(mPrevWarping, binIdx);
            BL_FLOAT w1 = GetWarping(mWarping, binIdx);
            
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
                if (binIdx < mWarping.GetSize() - 1)
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
                    samples->Get()[i] += samp*col;
                    //ampDenoms.Get()[i] += col;
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
    
    mPrevSASPartials = mSASPartials;
}

BL_FLOAT
SASFrameSynth::GetColor(const WDL_TypedBuf<BL_FLOAT> &color, BL_FLOAT binIdx)
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

#if 0 // No need anymore
    //col *= mColorFactor;
    col = ApplyColorFactor(col, mColorFactor);
#endif
    
    return col;
}

BL_FLOAT
SASFrameSynth::GetWarping(const WDL_TypedBuf<BL_FLOAT> &warping,
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

bool
SASFrameSynth::FindPartial(BL_FLOAT freq)
{
#define FIND_COEFF 0.25
    
    BL_FLOAT step = mFrequency*FIND_COEFF;
    
    for (int i = 0; i < mPartials.size(); i++)
    {
        const SASFrame6::SASPartial &p = mPartials[i];
        
        if ((freq > p.mFreq - step) &&
            (freq < p.mFreq + step))
            // Found
            return true;
    }
    
    return false;
}

// Interpolate in amp
void
SASFrameSynth::GetPartial(SASFrame6::SASPartial *result, int index, BL_FLOAT t)
{
    const SASFrame6::SASPartial &currentPartial = mPartials[index];

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
                const SASFrame6::SASPartial &prevPartial =
                    mPrevPartials[prevPartialIdx];
                
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
            const SASFrame6::SASPartial &prevPartial = mPrevPartials[prevPartialIdx];
                
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

BL_FLOAT
SASFrameSynth::GetFreq(BL_FLOAT freq0, BL_FLOAT freq1, BL_FLOAT t)
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
SASFrameSynth::GetAmp(BL_FLOAT amp0, BL_FLOAT amp1, BL_FLOAT t)
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
SASFrameSynth::GetCol(BL_FLOAT col0, BL_FLOAT col1, BL_FLOAT t)
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
SASFrameSynth::SetSASFactors()
{
    mSASFrame.SetAmpFactor(mAmpFactor);
    mSASFrame.SetFreqFactor(mFreqFactor);
    mSASFrame.SetColorFactor(mColorFactor);
    mSASFrame.SetWarpingFactor(mWarpingFactor);
}

void
SASFrameSynth::UpdateSASData()
{
    // Partials
    mPrevPartials = mPartials;
    mSASFrame.GetPartials(&mPartials);

    //
    mPrevColor = mColor;
    mSASFrame.GetColor(&mColor);

    mPrevWarping = mWarping;
    mSASFrame.GetWarping(&mWarping);

    mSASFrame.GetWarpingInv(&mWarpingInv);
}
