//
//  SASFrame2.cpp
//  BL-SASViewer
//
//  Created by applematuer on 2/2/19.
//
//

#include <SASViewerProcess.h>

//#include <PartialsToFreqCepstrum.h>
#include <PartialsToFreq4.h>

#include <FreqAdjustObj3.h>

#include <WavetableSynth.h>

// Do not speed up...
#include <SinLUT.h>

#include <BLUtils.h>

#include "SASFrame2.h"


// Optim: optimizes a little (~5/15%)
SIN_LUT_CREATE(SAS_FRAME_SIN_LUT, 4096);

// 4096*64 (2MB)
// 2x slower than 4096
//SIN_LUT_CREATE(SAS_FRAME_SIN_LUT, 262144);


#define EPS 1e-15
#define INF 1e15

#define MIN_AMP_DB -120.0

// Synthesis
#define SYNTH_MAX_NUM_PARTIALS 40 //20 //10 origin: 10, (40 for test bell)
#define SYNTH_AMP_COEFF 4.0
#define SYNTH_MIN_FREQ 50.0
#define SYNTH_DEAD_PARTIAL_DECREASE 0.25

#if 0 // TEST MONDAY
// Avoids jumps in envelope
#define COLOR_SMOOTH_COEFF 0.95 //0.9
#define WARPING_SMOOTH_COEFF 0.95 //0.9
#define FREQ_SMOOTH_COEFF 0.9 //0.95
#endif

// TEST MONDAY
// NOTE: if we don't smooth the color, this will make clips
// (because if a partial is missing, this will make a hole in the color)
//
// Avoids jumps in envelope
#define COLOR_SMOOTH_COEFF 0.5 //0.95
#define WARPING_SMOOTH_COEFF 0.0 //0.95
#define FREQ_SMOOTH_COEFF 0.0 //0.9

// Compute samples directly from tracked partials
#define COMPUTE_PARTIALS_SAMPLES     0
#define COMPUTE_SAS_SAMPLES          1 // GOOD (best quality, but slow)
#define COMPUTE_SAS_FFT              0 // Perf gain x10 compared to COMPUTE_SAS_SAMPLES
#define COMPUTE_SAS_FFT_FREQ_ADJUST  1 // GOOD: Avoids a "reverb" phase effet as with COMPUTE_SAS_FFT
#define COMPUTE_SAS_SAMPLES_TABLE    0 // Perf gain ~x10 compared to COMPUTE_SAS_SAMPLES (with nearest + block buffers)
#define COMPUTE_SAS_SAMPLES_OVERLAP  0 // not very efficient (and sound is not good

#define DBG_DISABLE_WARPING 1

// Get nearest color value, or interpolate ?
//
// If set to 1, avoid many small visual "clicks" on the spectrogram
// (but maybe these leaks are not audible)
#define INTERP_COLOR_WARPING 1

// Optim: 35 => 15/32ms (~25%)
#define OPTIM_COLOR_INTERP 1


SASFrame2::SASPartial::SASPartial()
{
    mFreq = 0.0;
    mAmpDB = MIN_AMP_DB;
    mPhase = 0.0;
}

SASFrame2::SASPartial::SASPartial(const SASPartial &other)
{
    mFreq = other.mFreq;
    mAmpDB = other.mAmpDB;
    mPhase = other.mPhase;
}

SASFrame2::SASPartial::~SASPartial() {}

bool
SASFrame2::SASPartial::AmpLess(const SASPartial &p1, const SASPartial &p2)
{
    return (p1.mAmpDB < p2.mAmpDB);
}

SASFrame2::SASFrame2(int bufferSize, BL_FLOAT sampleRate, int overlapping)
{
    SIN_LUT_INIT(SAS_FRAME_SIN_LUT);
    
    mBufferSize = bufferSize;
    
    mSampleRate = sampleRate;
    mOverlapping = overlapping;
    
    mSynthMode = OSC;
    
    mAmplitudeDB = MIN_AMP_DB;
    mPrevAmplitudeDB = MIN_AMP_DB;
    
    mFrequency = 0.0;
    
    mPitch = 1.0;
    
    //mPartialsToFreq = new PartialsToFreqCepstrum(bufferSize, sampleRate);
    mPartialsToFreq = new PartialsToFreq4(bufferSize, sampleRate);
    
#if COMPUTE_SAS_FFT_FREQ_ADJUST
    int oversampling = 1;
    mFreqObj = new FreqAdjustObj3(bufferSize,
                                  overlapping, oversampling,
                                  sampleRate);
#endif
    
#if COMPUTE_SAS_SAMPLES_TABLE
    BL_FLOAT minFreq = 20.0;
    mTableSynth = new WavetableSynth(bufferSize, overlapping,
                                     sampleRate, 1, minFreq);
#endif
}

SASFrame2::~SASFrame2()
{
    delete mPartialsToFreq;
    
#if COMPUTE_SAS_FFT_FREQ_ADJUST
    delete mFreqObj;
#endif
    
#if COMPUTE_SAS_SAMPLES_TABLE
    delete mTableSynth;
#endif
}

void
SASFrame2::Reset(BL_FLOAT sampleRate)
{
    mPartials.clear();
    mPrevPartials.clear();
    
    mAmplitudeDB = MIN_AMP_DB;
    mFrequency = 0.0;
    
    mSASPartials.clear();
    mPrevSASPartials.clear();
    
#if COMPUTE_SAS_FFT_FREQ_ADJUST
    mFreqObj->Reset(mBufferSize, mOverlapping, 1, sampleRate);
#endif
    
#if COMPUTE_SAS_SAMPLES_TABLE
    mTableSynth->Reset(sampleRate);
#endif
}

void
SASFrame2::SetSynthMode(enum SynthMode mode)
{
    mSynthMode = mode;
}

void
SASFrame2::SetPartials(const vector<PartialTracker3::Partial> &partials)
{
    mPrevPartials = mPartials;
    mPartials = partials;
    
    // FIX: sorting by freq avoids big jumps in computed frequency when
    // id of a given partial changes.
    // (at least when the id of the first partial).
    sort(mPartials.begin(), mPartials.end(), PartialTracker3::Partial::FreqLess);
    
    mAmplitudeDB = MIN_AMP_DB;
    mFrequency = 0.0;
    
    Compute();
}

BL_FLOAT
SASFrame2::GetAmplitudeDB() const
{
    return mAmplitudeDB;
}

BL_FLOAT
SASFrame2::GetFrequency() const
{
    return mFrequency;
}

void
SASFrame2::GetColor(WDL_TypedBuf<BL_FLOAT> *color) const
{
    *color = mColor;
}

void
SASFrame2::GetNormWarping(WDL_TypedBuf<BL_FLOAT> *warping) const
{
    *warping = mNormWarping;
}

void
SASFrame2::SetAmplitudeDB(BL_FLOAT amp)
{
    mAmplitudeDB = amp;
}

void
SASFrame2::SetFrequency(BL_FLOAT freq)
{
    mFrequency = freq;
}

void
SASFrame2::SetColor(const WDL_TypedBuf<BL_FLOAT> &color)
{
    mPrevColor = mColor;
    
    mColor = color;
}

void
SASFrame2::SetNormWarping(const WDL_TypedBuf<BL_FLOAT> &warping)
{
    mPrevNormWarping = mNormWarping;
    
    mNormWarping = warping;
}

void
SASFrame2::ComputeSamplesWin(WDL_TypedBuf<BL_FLOAT> *samples)
{
#if COMPUTE_PARTIALS_SAMPLES
    ComputeSamplesPartials(samples);
#endif
    
#if COMPUTE_SAS_SAMPLES
    if (mSynthMode == OSC)
    {
        //ComputeSamplesSAS(samples);
        //ComputeSamplesSAS2(samples);
        //ComputeSamplesSAS3(samples);
        //ComputeSamplesSAS4(samples);
        ComputeSamplesSAS5(samples);
    }
#endif
    
    // TEST: was tested to put it in ComputeSamples()
    // benefit from overlapping (result: no clicks),
    // and to take full nearest buffers
    // => but the sound was worse than ifft + freqObj
#if COMPUTE_SAS_SAMPLES_TABLE
    ComputeSamplesSASTable(samples);
    //ComputeSamplesSASTable2(samples);
#endif
}

void
SASFrame2::ComputeSamples(WDL_TypedBuf<BL_FLOAT> *samples)
{
#if COMPUTE_SAS_FFT
    ComputeFftSAS(samples);
#endif
    
#if COMPUTE_SAS_FFT_FREQ_ADJUST
    if (mSynthMode == FFT)
    {
        ComputeFftSASFreqAdjust(samples);
    }
#endif
    
#if COMPUTE_SAS_SAMPLES_OVERLAP
    ComputeSamplesSASOverlap(samples);
#endif

}

// Directly use partials provided
void
SASFrame2::ComputeSamplesPartials(WDL_TypedBuf<BL_FLOAT> *samples)
{
    samples->Resize(mBufferSize);
    
    BLUtils::FillAllZero(samples);
    
    if (mPartials.empty())
    {
        return;
    }
    
    for (int i = 0; i < mPartials.size(); i++)
    {
        PartialTracker3::Partial partial;
        
        BL_FLOAT phase = 0.0;
        int prevPartialIdx = FindPrevPartialIdx(i);
        if (prevPartialIdx != -1)
            phase = mPrevPartials[prevPartialIdx].mPhase;
        
        //const PartialTracker3::Partial &partial = mPartials[i]; // OLD
        for (int j = 0; j < samples->GetSize()/mOverlapping; j++)
        {
            // Get interpolated partial
            BL_FLOAT partialT = ((BL_FLOAT)j)/(samples->GetSize()/mOverlapping);
            GetPartial(&partial, i, partialT);
            
            BL_FLOAT freq = partial.mFreq;
            
            BL_FLOAT amp = DBToAmp(partial.mAmpDB);
            BL_FLOAT samp = amp*std::sin(phase); // cos
            
            samp *= SYNTH_AMP_COEFF;
            
            if (freq >= SYNTH_MIN_FREQ)
                samples->Get()[j] += samp;
            
            phase += 2.0*M_PI*freq/mSampleRate;
        }
        
        //phase = BLUtils::MapToPi(phase);
        
        //mPartials[i].mPhase += phase; //?????? STRANGE
        mPartials[i].mPhase = phase;
    }
}

void
SASFrame2::ComputeSamplesSAS(WDL_TypedBuf<BL_FLOAT> *samples)
{
    samples->Resize(mBufferSize);
    
    BLUtils::FillAllZero(samples);
    
    if (mPartials.empty())
        return;
    
    BL_FLOAT amp0DB = GetAmplitudeDB();
    BL_FLOAT amp0 = DBToAmp(amp0DB);
    
    BL_FLOAT freq0 = GetFrequency();
    
    freq0 *= mPitch;
    
    // Sort partials by amplitude, in order to play the highest amplitudes ?
    
    BL_FLOAT freq = freq0;
    int partialIndex = 0;
    while((freq < mSampleRate/2.0) && (partialIndex < SYNTH_MAX_NUM_PARTIALS))
    {
        if (freq > SYNTH_MIN_FREQ)
        {
            //freq = ApplyNormWarping(freq);
            //BL_FLOAT col = ApplyColor(freq);
            
            if (mSASPartials.size() <= partialIndex)
            {
                SASPartial newPartial;
                newPartial.mFreq = freq;
                newPartial.mAmpDB = AmpToDB(amp0/**col*/);
                
                mSASPartials.push_back(newPartial);
            }
            else
            {
                SASPartial &partial = mSASPartials[partialIndex];
                partial.mFreq = freq;
                partial.mAmpDB = AmpToDB(amp0/**col*/);
            }
            
            BL_FLOAT phase = 0.0;
            if (partialIndex < mPrevSASPartials.size())
                phase = mPrevSASPartials[partialIndex].mPhase;
            
            SASPartial partial;
            for (int i = 0; i < samples->GetSize()/mOverlapping; i++)
            {
                // Get interpolated partial
                BL_FLOAT partialT = ((BL_FLOAT)i)/(samples->GetSize()/mOverlapping);
                GetSASPartial(&partial, partialIndex, partialT);
                //partial = mSASPartials[partialIndex]; // OLD
                
                BL_FLOAT freq = partial.mFreq;
                
                // Apply warping and col here avoids many clips
                //freq = ApplyNormWarping(freq);
                //BL_FLOAT col = ApplyColor(freq);
                
                // Better: interpolate over time
                // (with this, it avoids vertical spectrogram leaks)
                // (the spectrogram is very neat
                freq = ApplyNormWarping(freq, partialT);
                BL_FLOAT col = ApplyColor(freq, partialT);
                
                BL_FLOAT amp = DBToAmp(partial.mAmpDB)*col;
                
                BL_FLOAT samp = std::sin(phase); // cos
                
                //BL_FLOAT samp;
                //SIN_LUT_GET(SAS_FRAME_SIN_LUT, samp, phase);
                
                samp *= amp;
                
                samp *= SYNTH_AMP_COEFF;
                
                if (freq >= SYNTH_MIN_FREQ)
                    samples->Get()[i] += samp;
                
                phase += 2.0*M_PI*freq/mSampleRate;
            }
            
            mSASPartials[partialIndex].mPhase =/*+=*/ phase;
        }
        
        partialIndex++;
        freq = freq0*(partialIndex + 1);
    }
    
    mPrevSASPartials = mSASPartials;
}

// ComputeSamplesSAS2
// Optim
// Gain (by suppressing loops): 74 => 57ms (~20%)
void
SASFrame2::ComputeSamplesSAS2(WDL_TypedBuf<BL_FLOAT> *samples)
{
    samples->Resize(mBufferSize);
    
    BLUtils::FillAllZero(samples);
    
    if (mPartials.empty())
        return;
    
    // Will be automatically interpolated
    BL_FLOAT amp0DB = GetAmplitudeDB();
    //BL_FLOAT amp0 = DBToAmp(amp0DB);
    
    BL_FLOAT freq0 = GetFrequency();
    
    freq0 *= mPitch;
    
    // Optim
    //BL_FLOAT phaseCoeff = (samples->GetSize()/mOverlapping)*2.0*M_PI/mSampleRate;
    BL_FLOAT phaseCoeff = 2.0*M_PI/mSampleRate;
    
    BL_FLOAT hzPerBin = mSampleRate/mBufferSize;
    BL_FLOAT hzPerBinInv = 1.0/hzPerBin;
    
    // Sort partials by amplitude, in order to play the highest amplitudes ?
    BL_FLOAT freq = freq0;
    int partialIndex = 0;
    while((freq < mSampleRate/2.0) && (partialIndex < SYNTH_MAX_NUM_PARTIALS))
    {
        if (freq > SYNTH_MIN_FREQ)
        {
            // !!!
            // TODO: check well that we match the partials correctly
            // !!!
            
            if (mSASPartials.size() <= partialIndex)
            {
                SASPartial newPartial;
                newPartial.mFreq = freq;
                newPartial.mAmpDB = amp0DB;
                
                mSASPartials.push_back(newPartial);
            }
            else
            {
                SASPartial &partial = mSASPartials[partialIndex];
                partial.mFreq = freq;
                partial.mAmpDB = amp0DB;
            }
            
            // Current phase
            BL_FLOAT phase = 0.0;
            if (partialIndex < mPrevSASPartials.size())
                phase = mPrevSASPartials[partialIndex].mPhase;
            
            const SASPartial &partial = mSASPartials[partialIndex];
            SASPartial *prevPartial = NULL;
            if (partialIndex < mPrevSASPartials.size())
                prevPartial = &mPrevSASPartials[partialIndex];
            
            BL_FLOAT prevPartialAmp = 0.0;
            if (prevPartial != NULL)
                prevPartialAmp = DBToAmp(prevPartial->mAmpDB);
            BL_FLOAT partialAmp = DBToAmp(partial.mAmpDB);
            
            BL_FLOAT tCoeff = 1.0/(samples->GetSize()/mOverlapping);
            for (int i = 0; i < samples->GetSize()/mOverlapping; i++)
            {
                BL_FLOAT t = i*tCoeff;
                
                // Compute color
                //
                BL_FLOAT idx = freq*hzPerBinInv;
                idx = bl_round(idx);
                
                BL_FLOAT col = 0.0;
                if (idx < mColor.GetSize())
                {
                    BL_FLOAT col0 = mPrevColor.Get()[(int)idx];
                    BL_FLOAT col1 = mColor.Get()[(int)idx];
                    
                    // Optim: if color is 0, do not compute the rest
                    // Gain: 54 => 35ms (30%)
                    if ((col0 < EPS) && (col1 < EPS))
                    {
                        phase += phaseCoeff*freq;
                        
                        continue;
                    }
                    
                    col = (1.0 - t)*col0 + t*col1;
                }
                
                // Compute norm warping
                //
                BL_FLOAT w = 1.0;
                if (idx < mNormWarping.GetSize())
                {
                    BL_FLOAT w0 = mPrevNormWarping.Get()[(int)idx];
                    BL_FLOAT w1 = mNormWarping.Get()[(int)idx];
                    
                    w = (1.0 - t)*w0 + t*w1;
                }
                
                // Compute freq and partial amp
                //
                BL_FLOAT freq;
                //BL_FLOAT ampDB;
                BL_FLOAT amp;
                if (prevPartial != NULL)
                {
                    freq = (1.0 - t)*prevPartial->mFreq + t*partial.mFreq;
                    //ampDB = (1.0 - t)*prevPartial->mAmpDB + t*partial.mAmpDB;
                    amp = (1.0 - t)*prevPartialAmp + t*partialAmp;
                }
                else
                {
                    freq = partial.mFreq;
                    
                    // New partial, fade in amp
                    
                    // Interpolate
                    BL_FLOAT t0 = t/SYNTH_DEAD_PARTIAL_DECREASE;
                    if (t0 > 1.0)
                        t0 = 1.0;
                    
                    //ampDB = t0*partial.mAmpDB;
                    amp = t0*partialAmp;
                }
                
                //BL_FLOAT amp = DBToAmp(partial.mAmpDB)*col;
                
                freq *= w;
                
#if 0
                BL_FLOAT samp = std::sin(phase);
#else
                BL_FLOAT samp;
                SIN_LUT_GET(SAS_FRAME_SIN_LUT, samp, phase);
#endif
                
                //samp *= amp;
                samp *= amp*col;
                
                samp *= SYNTH_AMP_COEFF;
                
                if (freq >= SYNTH_MIN_FREQ)
                    samples->Get()[i] += samp;
                
                //phase += 2.0*M_PI*freq/mSampleRate;
                phase += phaseCoeff*freq;
            }
            
            // Compute next phase
            //phase += phaseCoeff*freq;
            mSASPartials[partialIndex].mPhase = phase;
        }
        
        partialIndex++;
        freq = freq0*(partialIndex + 1);
    }
    
    mPrevSASPartials = mSASPartials;
}

// ComputeSamplesSAS2
// Optim
// Gain (by suppressing loops): 74 => 57ms (~20%)
//
// ComputeSamplesSAS3: avoid tiny clicks (not audible)
//
void
SASFrame2::ComputeSamplesSAS3(WDL_TypedBuf<BL_FLOAT> *samples)
{
    samples->Resize(mBufferSize);
    
    BLUtils::FillAllZero(samples);
    
    if (mPartials.empty())
        return;
    
    // First time: initialize the partials
    if (mSASPartials.empty())
    {
        mSASPartials.resize(SYNTH_MAX_NUM_PARTIALS);
    }
    if (mPrevSASPartials.empty())
        mPrevSASPartials = mSASPartials;
    
    // Optim
    BL_FLOAT phaseCoeff = 2.0*M_PI/mSampleRate;
    BL_FLOAT hzPerBin = mSampleRate/mBufferSize;
    BL_FLOAT hzPerBinInv = 1.0/hzPerBin;
    
    // Sort partials by amplitude, in order to play the highest amplitudes ?
    BL_FLOAT partialFreq = mFrequency*mPitch;
    int partialIndex = 0;
    while((partialFreq < mSampleRate/2.0) && (partialIndex < SYNTH_MAX_NUM_PARTIALS))
    {
        if (partialFreq > SYNTH_MIN_FREQ)
        {
            // Current and prev partials
            SASPartial &partial = mSASPartials[partialIndex];
            partial.mFreq = partialFreq;
            partial.mAmpDB = mAmplitudeDB;
            
            const SASPartial &prevPartial = mPrevSASPartials[partialIndex];
            
            // Current phase
            BL_FLOAT phase = prevPartial.mPhase;
            
            // Amp
            BL_FLOAT partialAmp = DBToAmp(partial.mAmpDB);
            BL_FLOAT prevPartialAmp = DBToAmp(prevPartial.mAmpDB);
            
            // Optim
            // For color
            BL_FLOAT binIdx = partialFreq*hzPerBinInv;
#if !INTERP_COLOR_WARPING
            binIdx = bl_round(binIdx);
#endif
            
            // Loop
            BL_FLOAT tCoeff = 1.0/(samples->GetSize()/mOverlapping);
            for (int i = 0; i < samples->GetSize()/mOverlapping; i++)
            {
                BL_FLOAT t = i*tCoeff; // TODO: make a step instead, + addition
                
                // OLD color
                
                // Compute norm warping
                //
                BL_FLOAT w = 1.0;
                if (binIdx < mNormWarping.GetSize() - 1)
                {
#if INTERP_COLOR_WARPING
                    BL_FLOAT t0 = binIdx - (int)binIdx;
                    
                    BL_FLOAT w00 = mPrevNormWarping.Get()[(int)binIdx];
                    BL_FLOAT w01 = mPrevNormWarping.Get()[((int)binIdx) + 1];
                    BL_FLOAT w0 = (1.0 - t0)*w00 + t0*w01;
                    
                    BL_FLOAT w10 = mNormWarping.Get()[(int)binIdx];
                    BL_FLOAT w11 = mNormWarping.Get()[((int)binIdx) + 1];
                    BL_FLOAT w1 = (1.0 - t0)*w10 + t0*w11;
#else
                    BL_FLOAT w0 = mPrevNormWarping.Get()[(int)binIdx];
                    BL_FLOAT w1 = mNormWarping.Get()[(int)binIdx];
#endif
                    w = (1.0 - t)*w0 + t*w1;
                }
                
                // Compute freq and partial amp
                //
                BL_FLOAT freq = (1.0 - t)*prevPartial.mFreq + t*partial.mFreq;
                BL_FLOAT amp = (1.0 - t)*prevPartialAmp + t*partialAmp;
                
                freq *= w;
                
                // Compute color
                //
                BL_FLOAT binIdxc = freq*hzPerBinInv;
#if !INTERP_COLOR_WARPING
                binIdxc = bl_round(binIdxc);
#endif
                
                BL_FLOAT col = 0.0;
                if (binIdxc < mColor.GetSize() - 1)
                {
#if INTERP_COLOR_WARPING
                    BL_FLOAT t0 = binIdxc - (int)binIdxc;

                    BL_FLOAT col00 = mPrevColor.Get()[(int)binIdxc];
                    BL_FLOAT col01 = mPrevColor.Get()[((int)binIdxc) + 1];
                    BL_FLOAT col0 = (1.0 - t0)*col00 + t0*col01;
                    
                    BL_FLOAT col10 = mColor.Get()[(int)binIdxc];
                    BL_FLOAT col11 = mColor.Get()[((int)binIdxc) + 1];
                    BL_FLOAT col1 = (1.0 - t0)*col10 + t0*col11;
#else
                    BL_FLOAT col0 = mPrevColor.Get()[(int)binIdxc];
                    BL_FLOAT col1 = mColor.Get()[(int)binIdxc];
#endif
                    // Optim: if color is 0, do not compute the rest
                    // Gain: 54 => 35ms (30%)
                    if ((col0 < EPS) && (col1 < EPS))
                    {
                        phase += phaseCoeff*partialFreq;
                        
                        continue;
                    }
                    
                    col = (1.0 - t)*col0 + t*col1;
                }
                
#if 0
                BL_FLOAT samp = std::sin(phase);
#else
                BL_FLOAT samp;
                SIN_LUT_GET(SAS_FRAME_SIN_LUT, samp, phase);
#endif
                
                //samp *= amp;
                samp *= amp*col;
                
                samp *= SYNTH_AMP_COEFF;
                
                if (freq >= SYNTH_MIN_FREQ)
                    samples->Get()[i] += samp;
                
                phase += phaseCoeff*freq;
            }
            
            // Compute next phase
            mSASPartials[partialIndex].mPhase = phase;
        }
        
        partialIndex++;
        partialFreq = mFrequency*mPitch*(partialIndex + 1);
    }
    
    mPrevSASPartials = mSASPartials;
}

// ComputeSamplesSAS2
// Optim
// Gain (by suppressing loops): 74 => 57ms (~20%)
//
// ComputeSamplesSAS3: avoid tiny clicks (not audible)
//
// ComputeSamplesSAS4: optimize more
void
SASFrame2::ComputeSamplesSAS4(WDL_TypedBuf<BL_FLOAT> *samples)
{
    samples->Resize(mBufferSize);
    
    BLUtils::FillAllZero(samples);
    
    if (mPartials.empty())
        return;
    
    // First time: initialize the partials
    if (mSASPartials.empty())
    {
        mSASPartials.resize(SYNTH_MAX_NUM_PARTIALS);
    }
    if (mPrevSASPartials.empty())
        mPrevSASPartials = mSASPartials;
    
    // Optim
    BL_FLOAT phaseCoeff = 2.0*M_PI/mSampleRate;
    BL_FLOAT hzPerBin = mSampleRate/mBufferSize;
    BL_FLOAT hzPerBinInv = 1.0/hzPerBin;
    
    // Sort partials by amplitude, in order to play the highest amplitudes ?
    BL_FLOAT partialFreq = mFrequency*mPitch;
    int partialIndex = 0;
    while((partialFreq < mSampleRate/2.0) && (partialIndex < SYNTH_MAX_NUM_PARTIALS))
    {
        if (partialFreq > SYNTH_MIN_FREQ)
        {
            // Current and prev partials
            SASPartial &partial = mSASPartials[partialIndex];
            partial.mFreq = partialFreq;
            partial.mAmpDB = mAmplitudeDB;
            
            const SASPartial &prevPartial = mPrevSASPartials[partialIndex];
            
            // Current phase
            BL_FLOAT phase = prevPartial.mPhase;
            
            // Amp
            BL_FLOAT partialAmp = DBToAmp(partial.mAmpDB);
            BL_FLOAT prevPartialAmp = DBToAmp(prevPartial.mAmpDB);
            
            // Optim
            // For color
            BL_FLOAT binIdx = partialFreq*hzPerBinInv;
            
#if INTERP_COLOR_WARPING
            BL_FLOAT t0 = binIdx - (int)binIdx;
            
            BL_FLOAT w00 = mPrevNormWarping.Get()[(int)binIdx];
            BL_FLOAT w01 = mPrevNormWarping.Get()[((int)binIdx) + 1];
            BL_FLOAT w0 = (1.0 - t0)*w00 + t0*w01;
            
            BL_FLOAT w10 = mNormWarping.Get()[(int)binIdx];
            BL_FLOAT w11 = mNormWarping.Get()[((int)binIdx) + 1];
            BL_FLOAT w1 = (1.0 - t0)*w10 + t0*w11;
#else
            binIdx = bl_round(binIdx);
            BL_FLOAT w0 = mPrevNormWarping.Get()[(int)binIdx];
            BL_FLOAT w1 = mNormWarping.Get()[(int)binIdx];
#endif
            
            // Loop
            BL_FLOAT t = 0.0;
            BL_FLOAT tStep = 1.0/(samples->GetSize()/mOverlapping - 1); //-1
            for (int i = 0; i < samples->GetSize()/mOverlapping; i++)
            {
                // OLD color
                
                // Compute norm warping
                //
                BL_FLOAT w = 1.0;
                if (binIdx < mNormWarping.GetSize() - 1)
                {
                    w = (1.0 - t)*w0 + t*w1;
                }
                
                // Compute freq and partial amp
                //
                BL_FLOAT freq = (1.0 - t)*prevPartial.mFreq + t*partial.mFreq;
                BL_FLOAT amp = (1.0 - t)*prevPartialAmp + t*partialAmp;
                
                freq *= w;
                
                // Compute color
                //
                BL_FLOAT binIdxc = freq*hzPerBinInv;                
                BL_FLOAT col = 0.0;
                if (binIdxc < mColor.GetSize() - 1)
                {
#if INTERP_COLOR_WARPING
                    BL_FLOAT t0 = binIdxc - (int)binIdxc;
                    
                    BL_FLOAT col00 = mPrevColor.Get()[(int)binIdxc];
                    BL_FLOAT col01 = mPrevColor.Get()[((int)binIdxc) + 1];
                    BL_FLOAT col0 = (1.0 - t0)*col00 + t0*col01;
                    
                    BL_FLOAT col10 = mColor.Get()[(int)binIdxc];
                    BL_FLOAT col11 = mColor.Get()[((int)binIdxc) + 1];
                    BL_FLOAT col1 = (1.0 - t0)*col10 + t0*col11;
#else
                    binIdxc = bl_round(binIdxc);
                    
                    BL_FLOAT col0 = mPrevColor.Get()[(int)binIdxc];
                    BL_FLOAT col1 = mColor.Get()[(int)binIdxc];
#endif
                    // Optim: if color is 0, do not compute the rest
                    // Gain: 54 => 35ms (30%)
                    if ((col0 < EPS) && (col1 < EPS))
                    {
                        t += tStep;
                        phase += phaseCoeff*partialFreq;
                        
                        continue;
                    }
                    
                    col = (1.0 - t)*col0 + t*col1;
                }
                
#if 0
                BL_FLOAT samp = std::sin(phase);
#else
                BL_FLOAT samp;
                SIN_LUT_GET(SAS_FRAME_SIN_LUT, samp, phase);
#endif
                
                samp *= amp*col;
                
                samp *= SYNTH_AMP_COEFF;
                
                if (freq >= SYNTH_MIN_FREQ)
                    samples->Get()[i] += samp;
                
                t += tStep;
                phase += phaseCoeff*freq;
            }
            
            // Compute next phase
            mSASPartials[partialIndex].mPhase = phase;
        }
        
        partialIndex++;
        partialFreq = mFrequency*mPitch*(partialIndex + 1);
    }
    
    mPrevSASPartials = mSASPartials;
}

// ComputeSamplesSAS2
// Optim
// Gain (by suppressing loops): 74 => 57ms (~20%)
//
// ComputeSamplesSAS3: avoid tiny clicks (not audible)
//
// ComputeSamplesSAS4: optimize more
//
// ComputeSamplesSAS5: optimize more
//
void
SASFrame2::ComputeSamplesSAS5(WDL_TypedBuf<BL_FLOAT> *samples)
{
    samples->Resize(mBufferSize);
    
    BLUtils::FillAllZero(samples);
    
#if 0 // TEST
    if (mPartials.empty())
        return;
#endif
    
    // First time: initialize the partials
    if (mSASPartials.empty())
    {
        mSASPartials.resize(SYNTH_MAX_NUM_PARTIALS);
    }
    if (mPrevSASPartials.empty())
        mPrevSASPartials = mSASPartials;
    
    // TEST
    if (mPrevColor.GetSize() != mColor.GetSize())
        mPrevColor = mColor;
    
    if (mPrevNormWarping.GetSize() != mNormWarping.GetSize())
        mPrevNormWarping = mNormWarping;
    
    // Optim
    BL_FLOAT phaseCoeff = 2.0*M_PI/mSampleRate;
    BL_FLOAT hzPerBin = mSampleRate/mBufferSize;
    BL_FLOAT hzPerBinInv = 1.0/hzPerBin;
    
    // Sort partials by amplitude, in order to play the highest amplitudes ?
    BL_FLOAT partialFreq = mFrequency*mPitch;
    int partialIndex = 0;
    while((partialFreq < mSampleRate/2.0) && (partialIndex < SYNTH_MAX_NUM_PARTIALS))
    {
        if (partialFreq > SYNTH_MIN_FREQ)
        {
            // Current and prev partials
            SASPartial &partial = mSASPartials[partialIndex];
            partial.mFreq = partialFreq;
            partial.mAmpDB = mAmplitudeDB;
            
            const SASPartial &prevPartial = mPrevSASPartials[partialIndex];
            
            // Current phase
            BL_FLOAT phase = prevPartial.mPhase;
            
            // Amp
            BL_FLOAT partialAmp = DBToAmp(partial.mAmpDB);
            BL_FLOAT prevPartialAmp = DBToAmp(prevPartial.mAmpDB);
            
            // Optim
            // For color
            BL_FLOAT binIdx = partialFreq*hzPerBinInv;
            
            // Warping
#if INTERP_COLOR_WARPING
            BL_FLOAT t0 = binIdx - (int)binIdx;
            
            BL_FLOAT w00 = mPrevNormWarping.Get()[(int)binIdx];
            BL_FLOAT w01 = mPrevNormWarping.Get()[((int)binIdx) + 1];
            BL_FLOAT w0 = (1.0 - t0)*w00 + t0*w01;
            
            BL_FLOAT w10 = mNormWarping.Get()[(int)binIdx];
            BL_FLOAT w11 = mNormWarping.Get()[((int)binIdx) + 1];
            BL_FLOAT w1 = (1.0 - t0)*w10 + t0*w11;
#else
            binIdx = bl_round(binIdx);
            BL_FLOAT w0 = mPrevNormWarping.Get()[(int)binIdx];
            BL_FLOAT w1 = mNormWarping.Get()[(int)binIdx];
#endif
            
            // Color
#if OPTIM_COLOR_INTERP
            BL_FLOAT prevBinIdxc = w0*prevPartial.mFreq*hzPerBinInv;
            BL_FLOAT binIdxc = w1*partial.mFreq*hzPerBinInv;
            
            BL_FLOAT col0 = GetColor(mPrevColor, prevBinIdxc);
            BL_FLOAT col1 = GetColor(mColor, binIdxc);
            
            if ((col0 < EPS) && (col1 < EPS))
                // Color will be 0, no need to synthetize samples
            {
                partialIndex++;
                partialFreq = mFrequency*mPitch*(partialIndex + 1);
                
                continue;
            }
#endif
            
            // Loop
            BL_FLOAT t = 0.0;
            BL_FLOAT tStep = 1.0/(samples->GetSize()/mOverlapping - 1); //-1
            for (int i = 0; i < samples->GetSize()/mOverlapping; i++)
            {
                // Compute norm warping
                //
                BL_FLOAT w = 1.0;
                if (binIdx < mNormWarping.GetSize() - 1)
                {
                    w = (1.0 - t)*w0 + t*w1;
                }
                
                // Compute freq and partial amp
                //
                BL_FLOAT freq = (1.0 - t)*prevPartial.mFreq + t*partial.mFreq;
                BL_FLOAT amp = (1.0 - t)*prevPartialAmp + t*partialAmp;
                
                freq *= w;
                
#if !OPTIM_COLOR_INTERP
                // Compute color
                //
                BL_FLOAT binIdxc = freq*hzPerBinInv;
                BL_FLOAT col = 0.0;
                if (binIdxc < mColor.GetSize() - 1)
                {
#if INTERP_COLOR_WARPING
                    BL_FLOAT t0 = binIdxc - (int)binIdxc;
                    
                    BL_FLOAT col00 = mPrevColor.Get()[(int)binIdxc];
                    BL_FLOAT col01 = mPrevColor.Get()[((int)binIdxc) + 1];
                    BL_FLOAT col0 = (1.0 - t0)*col00 + t0*col01;
                    
                    BL_FLOAT col10 = mColor.Get()[(int)binIdxc];
                    BL_FLOAT col11 = mColor.Get()[((int)binIdxc) + 1];
                    BL_FLOAT col1 = (1.0 - t0)*col10 + t0*col11;
#else
                    binIdxc = bl_round(binIdxc);
                    
                    BL_FLOAT col0 = mPrevColor.Get()[(int)binIdxc];
                    BL_FLOAT col1 = mColor.Get()[(int)binIdxc];
#endif
                    // Optim: if color is 0, do not compute the rest
                    // Gain: 54 => 35ms (30%)
                    if ((col0 < EPS) && (col1 < EPS))
                    {
                        t += tStep;
                        phase += phaseCoeff*partialFreq;
                        
                        continue;
                    }
                    
                    col = (1.0 - t)*col0 + t*col1;
                }
#else
                BL_FLOAT col = (1.0 - t)*col0 + t*col1;
#endif
                
                
#if 0
                BL_FLOAT samp = std::sin(phase);
#else
                BL_FLOAT samp;
                SIN_LUT_GET(SAS_FRAME_SIN_LUT, samp, phase);
#endif
                
                samp *= amp*col;
                
                samp *= SYNTH_AMP_COEFF;
                
                if (freq >= SYNTH_MIN_FREQ)
                    samples->Get()[i] += samp;
                
                t += tStep;
                phase += phaseCoeff*freq;
            }
            
            // Compute next phase
            mSASPartials[partialIndex].mPhase = phase;
        }
        
        partialIndex++;
        partialFreq = mFrequency*mPitch*(partialIndex + 1);
    }
    
    mPrevSASPartials = mSASPartials;
}

BL_FLOAT
SASFrame2::GetColor(const WDL_TypedBuf<BL_FLOAT> &color,
                    BL_FLOAT binIdx)
{
    BL_FLOAT col = 0.0;
    if (binIdx < color.GetSize() - 1)
    {
#if INTERP_COLOR_WARPING
        BL_FLOAT t0 = binIdx - (int)binIdx;
        
        BL_FLOAT col0 = color.Get()[(int)binIdx];
        BL_FLOAT col1 = color.Get()[((int)binIdx) + 1];

        col = (1.0 - t0)*col0 + t0*col1;
#else
        binIdx = bl_round(binIdx);
        
        col = color.Get()[(int)binIdx];
#endif
    }
    
    return col;
}

void
SASFrame2::ComputeSamplesSASOverlap(WDL_TypedBuf<BL_FLOAT> *samples)
{
    samples->Resize(mBufferSize);
    
    BLUtils::FillAllZero(samples);
    
    if (mPartials.empty())
        return;
    
    BL_FLOAT amp0DB = GetAmplitudeDB();
    BL_FLOAT amp0 = DBToAmp(amp0DB);
    
    BL_FLOAT freq0 = GetFrequency();
    
    freq0 *= mPitch;
    
    // Sort partials by amplitude, in order to play the highest amplitudes ?
    
    BL_FLOAT freq = freq0;
    int partialIndex = 0;
    while((freq < mSampleRate/2.0) && (partialIndex < SYNTH_MAX_NUM_PARTIALS))
    {
        if (freq > SYNTH_MIN_FREQ)
        {
            freq = ApplyNormWarping(freq);
            BL_FLOAT col = ApplyColor(freq);
            
            if (mSASPartials.size() <= partialIndex)
            {
                SASPartial newPartial;
                newPartial.mFreq = freq;
                newPartial.mAmpDB = AmpToDB(amp0*col);
                
                mSASPartials.push_back(newPartial);
            }
            else
            {
                SASPartial &partial = mSASPartials[partialIndex];
                partial.mFreq = freq;
                partial.mAmpDB = AmpToDB(amp0*col);
            }
            
            BL_FLOAT phase = 0.0;
            if (partialIndex < mPrevSASPartials.size())
                phase = mPrevSASPartials[partialIndex].mPhase;
            
            SASPartial partial = mSASPartials[partialIndex];
            for (int i = 0; i < samples->GetSize(); i++)
            {
                BL_FLOAT freq = partial.mFreq;
                
                BL_FLOAT amp = DBToAmp(partial.mAmpDB);
                
                BL_FLOAT samp = std::sin(phase);
                
                // WORKS !
                //BL_FLOAT samp;
                //SIN_LUT_GET(SAS_FRAME_SIN_LUT, samp, phase);
                
                samp *= amp;
                
                samp *= SYNTH_AMP_COEFF;
                
                if (freq >= SYNTH_MIN_FREQ)
                    samples->Get()[i] += samp;
                
                phase += 2.0*M_PI*freq/mSampleRate;
            }
            
            mSASPartials[partialIndex].mPhase =/*+=*/ phase;
        }
        
        partialIndex++;
        freq = freq0*(partialIndex + 1);
    }
    
    mPrevSASPartials = mSASPartials;
}

void
SASFrame2::ComputeFftSAS(WDL_TypedBuf<BL_FLOAT> *samples)
{
    samples->Resize(mBufferSize);
    
    BLUtils::FillAllZero(samples);
    
    if (mPartials.empty())
        return;
    
    BL_FLOAT amp0DB = GetAmplitudeDB();
    BL_FLOAT amp0 = DBToAmp(amp0DB);
    
    BL_FLOAT freq0 = GetFrequency();
    
    freq0 *= mPitch;
    
    // Fft
    WDL_TypedBuf<BL_FLOAT> magns;
    BLUtils::ResizeFillZeros(&magns, mBufferSize/2);
    
    WDL_TypedBuf<BL_FLOAT> phases;
    BLUtils::ResizeFillZeros(&phases, mBufferSize/2);
    
    BL_FLOAT hzPerBin = mSampleRate/mBufferSize;
    
    // Sort partials by amplitude, in order to play the highest amplitudes ?
    BL_FLOAT freq = freq0;
    int partialIndex = 0;
    while((freq < mSampleRate/2.0) && (partialIndex < SYNTH_MAX_NUM_PARTIALS))
    {
        if (freq > SYNTH_MIN_FREQ)
        {
            //freq = ApplyNormWarping(freq);
            //BL_FLOAT col = ApplyColor(freq);
            
            if (mSASPartials.size() <= partialIndex)
            {
                SASPartial newPartial;
                newPartial.mFreq = freq;
                newPartial.mAmpDB = AmpToDB(amp0/**col*/);
                
                mSASPartials.push_back(newPartial);
            }
            else
            {
                SASPartial &partial = mSASPartials[partialIndex];
                partial.mFreq = freq;
                partial.mAmpDB = AmpToDB(amp0/**col*/);
            }
            
            // Freq
            BL_FLOAT freq = 0.0;
            if (partialIndex < mPrevSASPartials.size())
                freq = mPrevSASPartials[partialIndex].mFreq;
            freq = ApplyNormWarping(freq);
                
            // Magn
            BL_FLOAT magn = 0.0;
            if (partialIndex < mPrevSASPartials.size())
            {
                magn = mPrevSASPartials[partialIndex].mAmpDB;
                magn = DBToAmp(magn);
            }
            
            // Color
            BL_FLOAT col = ApplyColor(freq);
            magn *= col;
            
            // ??
            //magn *= SYNTH_AMP_COEFF;
            
            // Phase
            BL_FLOAT phase = 0.0;
            if (partialIndex < mPrevSASPartials.size())
                phase = mPrevSASPartials[partialIndex].mPhase;
            
            // Fill the fft
            if (freq >= SYNTH_MIN_FREQ)
            {
                BL_FLOAT binNum = freq/hzPerBin;
                binNum = bl_round(binNum);
                if (binNum < 0)
                    binNum = 0;
                if (binNum > magns.GetSize() - 1)
                    binNum = magns.GetSize() - 1;
                
                magns.Get()[(int)binNum] = magn;
                phases.Get()[(int)binNum] = phase;
            }
            
            // Update the phases
            int numSamples = samples->GetSize()/mOverlapping;
            phase += numSamples*2.0*M_PI*freq/mSampleRate;
            mSASPartials[partialIndex].mPhase = phase;
        }
                    
        partialIndex++;
        freq = freq0*(partialIndex + 1);
    }
    
    // Apply Ifft
    WDL_TypedBuf<WDL_FFT_COMPLEX> complexBuf;
    BLUtils::MagnPhaseToComplex(&complexBuf, magns, phases);
    complexBuf.Resize(complexBuf.GetSize()*2);
    BLUtils::FillSecondFftHalf(&complexBuf);
    
    // Ifft
    FftProcessObj16::FftToSamples(complexBuf, samples);
    
    mPrevSASPartials = mSASPartials;
}

void
SASFrame2::ComputeFftSASFreqAdjust(WDL_TypedBuf<BL_FLOAT> *samples)
{
    samples->Resize(mBufferSize);
    
    BLUtils::FillAllZero(samples);
    
#if 0 // TEST
    if (mPartials.empty())
        return;
#endif
    
    BL_FLOAT amp0DB = GetAmplitudeDB();
    BL_FLOAT amp0 = DBToAmp(amp0DB);
    
    BL_FLOAT freq0 = GetFrequency();
    
    freq0 *= mPitch;
    
    // Fft
    WDL_TypedBuf<BL_FLOAT> magns;
    BLUtils::ResizeFillZeros(&magns, mBufferSize/2);
    
    WDL_TypedBuf<BL_FLOAT> phases;
    BLUtils::ResizeFillZeros(&phases, mBufferSize/2);
    
    BL_FLOAT hzPerBin = mSampleRate/mBufferSize;
    
    WDL_TypedBuf<BL_FLOAT> realFreqs;
    BLUtils::ResizeFillZeros(&realFreqs, mBufferSize/2);
    
    // Sort partials by amplitude, in order to play the highest amplitudes ?
    BL_FLOAT freq = freq0;
    int partialIndex = 0;
    while((freq < mSampleRate/2.0) && (partialIndex < SYNTH_MAX_NUM_PARTIALS))
    {
        if (freq > SYNTH_MIN_FREQ)
        {
            //freq = ApplyNormWarping(freq);
            //BL_FLOAT col = ApplyColor(freq);
            
            if (mSASPartials.size() <= partialIndex)
            {
                SASPartial newPartial;
                newPartial.mFreq = freq;
                newPartial.mAmpDB = AmpToDB(amp0/**col*/);
                
                mSASPartials.push_back(newPartial);
            }
            else
            {
                SASPartial &partial = mSASPartials[partialIndex];
                partial.mFreq = freq;
                partial.mAmpDB = AmpToDB(amp0/**col*/);
            }
            
            // Freq
            BL_FLOAT freq = 0.0;
            if (partialIndex < mPrevSASPartials.size())
                freq = mPrevSASPartials[partialIndex].mFreq;
            freq = ApplyNormWarping(freq);
            
            // Magn
            BL_FLOAT magn = 0.0;
            if (partialIndex < mPrevSASPartials.size())
            {
                magn = mPrevSASPartials[partialIndex].mAmpDB;
                magn = DBToAmp(magn);
            }
            
            // Color
            BL_FLOAT col = ApplyColor(freq);
            magn *= col;
            
            // Phase
            BL_FLOAT phase = 0.0;
            if (partialIndex < mPrevSASPartials.size())
                phase = mPrevSASPartials[partialIndex].mPhase;
            
            // Fill the fft
            if (freq >= SYNTH_MIN_FREQ)
            {
                BL_FLOAT binNum = freq/hzPerBin;
                binNum = bl_round(binNum);
                if (binNum < 0)
                    binNum = 0;
                if (binNum > magns.GetSize() - 1)
                    binNum = magns.GetSize() - 1;
                
                magns.Get()[(int)binNum] = magn;
                phases.Get()[(int)binNum] = phase;
                
                // Set real freq
                realFreqs.Get()[(int)binNum] = freq;
            }
            
            // Update the phases
            int numSamples = samples->GetSize()/mOverlapping;
            phase += numSamples*2.0*M_PI*freq/mSampleRate;
            mSASPartials[partialIndex].mPhase = phase;
        }
        
        partialIndex++;
        freq = freq0*(partialIndex + 1);
    }
    
    // Set adjusted phases
    WDL_TypedBuf<BL_FLOAT> phasesAdjust;
    mFreqObj->ComputePhases(&phasesAdjust, realFreqs);
    for (int i = 0; i < realFreqs.GetSize(); i++)
    {
        phases.Get()[i] = phasesAdjust.Get()[i];
    }
        
    // Apply Ifft
    WDL_TypedBuf<WDL_FFT_COMPLEX> complexBuf;
    BLUtils::MagnPhaseToComplex(&complexBuf, magns, phases);
    complexBuf.Resize(complexBuf.GetSize()*2);
    BLUtils::FillSecondFftHalf(&complexBuf);
    
    // Ifft
    FftProcessObj16::FftToSamples(complexBuf, samples);
    
    mPrevSASPartials = mSASPartials;
}

void
SASFrame2::ComputeSamplesSASTable(WDL_TypedBuf<BL_FLOAT> *samples)
{
    samples->Resize(mBufferSize);
    BLUtils::FillAllZero(samples);
    
    if (mPartials.empty())
        return;
    
    BL_FLOAT amp0DB = GetAmplitudeDB();
    BL_FLOAT amp0 = DBToAmp(amp0DB);
    
    BL_FLOAT freq0 = GetFrequency();
    
    freq0 *= mPitch;
    
    // Sort partials by amplitude, in order to play the highest amplitudes ?
    BL_FLOAT freq = freq0;
    int partialIndex = 0;
    while((freq < mSampleRate/2.0) && (partialIndex < SYNTH_MAX_NUM_PARTIALS))
    {
        if (freq > SYNTH_MIN_FREQ)
        {
            //freq = ApplyNormWarping(freq);
            //BL_FLOAT col = ApplyColor(freq);
            
            if (mSASPartials.size() <= partialIndex)
            {
                SASPartial newPartial;
                newPartial.mFreq = freq;
                newPartial.mAmpDB = AmpToDB(amp0/**col*/);
                
                mSASPartials.push_back(newPartial);
            }
            else
            {
                SASPartial &partial = mSASPartials[partialIndex];
                partial.mFreq = freq;
                partial.mAmpDB = AmpToDB(amp0/**col*/);
            }
            
            // Freq
            BL_FLOAT freq = 0.0;
            if (partialIndex < mPrevSASPartials.size())
                freq = mPrevSASPartials[partialIndex].mFreq;
            freq = ApplyNormWarping(freq);
            
            // Magn
            BL_FLOAT amp = 0.0;
            if (partialIndex < mPrevSASPartials.size())
            {
                amp = mPrevSASPartials[partialIndex].mAmpDB;
                amp = DBToAmp(amp);
            }
            
            // Color
            BL_FLOAT col = ApplyColor(freq);
            amp *= col;
            
            // !
            amp *= SYNTH_AMP_COEFF;
            
            // Phase
            BL_FLOAT phase = 0.0;
            if (partialIndex < mPrevSASPartials.size())
                phase = mPrevSASPartials[partialIndex].mPhase;
            
            // Fill the fft
            if (freq >= SYNTH_MIN_FREQ)
            {
                WDL_TypedBuf<BL_FLOAT> freqBuffer;
                freqBuffer.Resize(mBufferSize/mOverlapping);
                
                //mTableSynth->GetSamplesNearest(&freqBuffer, freq, amp);
                mTableSynth->GetSamplesLinear(&freqBuffer, freq, amp);
                
                BLUtils::AddValues(samples, freqBuffer);
            }
            
            // Update the phases
            int numSamples = samples->GetSize()/mOverlapping;
            phase += numSamples*2.0*M_PI*freq/mSampleRate;
            mSASPartials[partialIndex].mPhase = phase;
        }
        
        partialIndex++;
        freq = freq0*(partialIndex + 1);
    }
    
    mPrevSASPartials = mSASPartials;
    
    mTableSynth->NextBuffer();
}

void
SASFrame2::ComputeSamplesSASTable2(WDL_TypedBuf<BL_FLOAT> *samples)
{
    samples->Resize(mBufferSize);
    BLUtils::FillAllZero(samples);
    
    if (mPartials.empty())
        return;
    
    BL_FLOAT amp0DB = GetAmplitudeDB();
    BL_FLOAT amp0 = DBToAmp(amp0DB);
    
    BL_FLOAT freq0 = GetFrequency();
    
    freq0 *= mPitch;
    
    // Sort partials by amplitude, in order to play the highest amplitudes ?
    BL_FLOAT freq = freq0;
    int partialIndex = 0;
    while((freq < mSampleRate/2.0) && (partialIndex < SYNTH_MAX_NUM_PARTIALS))
    {
        if (freq > SYNTH_MIN_FREQ)
        {
            if (mSASPartials.size() <= partialIndex)
            {
                SASPartial newPartial;
                newPartial.mFreq = freq;
                newPartial.mAmpDB = AmpToDB(amp0/**col*/);
                
                mSASPartials.push_back(newPartial);
            }
            else
            {
                SASPartial &partial = mSASPartials[partialIndex];
                partial.mFreq = freq;
                partial.mAmpDB = AmpToDB(amp0/**col*/);
            }
            
            SASPartial partial;
            for (int i = 0; i < samples->GetSize()/mOverlapping; i++)
            {
                // Get interpolated partial
                BL_FLOAT partialT = ((BL_FLOAT)i)/(samples->GetSize()/mOverlapping);
                GetSASPartial(&partial, partialIndex, partialT);
                
                BL_FLOAT freq = partial.mFreq;
                
                freq = ApplyNormWarping(freq, partialT);
                BL_FLOAT col = ApplyColor(freq, partialT);
                
                BL_FLOAT amp = DBToAmp(partial.mAmpDB);
                amp  *= col;
                
                amp *= SYNTH_AMP_COEFF;
                
                // Get from the wavetable
                if (freq >= SYNTH_MIN_FREQ)
                {
                    BL_FLOAT samp = mTableSynth->GetSampleNearest(i, freq, amp);
                    
                    // Makes wobbles with pure sine
                    //BL_FLOAT samp = mTableSynth->GetSampleLinear(i, freq, amp);
                    
                    samples->Get()[i] += samp;
                }
            }
            
            // Phase
            BL_FLOAT phase = 0.0;
            if (partialIndex < mPrevSASPartials.size())
                phase = mPrevSASPartials[partialIndex].mPhase;
            
            // Update the phases
            int numSamples = samples->GetSize()/mOverlapping;
            phase += numSamples*2.0*M_PI*freq/mSampleRate;
            mSASPartials[partialIndex].mPhase = phase;
        }
        
        partialIndex++;
        freq = freq0*(partialIndex + 1);
    }
    
    mPrevSASPartials = mSASPartials;
    
    mTableSynth->NextBuffer();
}

void
SASFrame2::SetPitch(BL_FLOAT pitch)
{
    mPitch = pitch;
}

void
SASFrame2::SetHarmonicSoundFlag(bool flag)
{
    mPartialsToFreq->SetHarmonicSoundFlag(flag);
}

bool
SASFrame2::ComputeSamplesFlag()
{
#if COMPUTE_PARTIALS_SAMPLES
    return false;
#endif

#if COMPUTE_SAS_SAMPLES
    if (mSynthMode == OSC)
        return false;
#endif

#if COMPUTE_SAS_FFT
    return true;
#endif
    
#if COMPUTE_SAS_FFT_FREQ_ADJUST
    if (mSynthMode == FFT)
        return true;
#endif
    
#if COMPUTE_SAS_SAMPLES_TABLE
    return false;
#endif
    
#if COMPUTE_SAS_SAMPLES_OVERLAP
    return true;
#endif
}

bool
SASFrame2::ComputeSamplesWinFlag()
{
#if COMPUTE_PARTIALS_SAMPLES
    return true;
#endif
    
#if COMPUTE_SAS_SAMPLES
    if (mSynthMode == OSC)
        return true;
#endif
    
#if COMPUTE_SAS_FFT
    return false;
#endif
    
#if COMPUTE_SAS_FFT_FREQ_ADJUST
    if (mSynthMode == FFT)
        return false;
#endif
    
#if COMPUTE_SAS_SAMPLES_TABLE
    return true;
#endif

#if COMPUTE_SAS_SAMPLES_OVERLAP
    return false;
#endif
}

void
SASFrame2::Compute()
{
    ComputeAmplitude();
    ComputeFrequency();
    ComputeColor();
    ComputeNormWarping();
}

void
SASFrame2::ComputeAmplitude()
{
    mPrevAmplitudeDB = mAmplitudeDB;
    mAmplitudeDB = MIN_AMP_DB; //0.0;
    
    BL_FLOAT amplitude = 0.0;
    for (int i = 0; i < mPartials.size(); i++)
    {
        const PartialTracker3::Partial &p = mPartials[i];
        
        BL_FLOAT ampDB = p.mAmpDB;
        BL_FLOAT amp = DBToAmp(ampDB); //
        //amp = SASViewerProcess::DbToAmpNorm(amp);
        
        //mAmplitudeDB += (ampDB - MIN_AMP_DB);
        amplitude += amp;
    }
    
    mAmplitudeDB = AmpToDB(amplitude);
    
    //mAmplitude = AmpToDB(mAmplitude);
    //mAmplitude = SASViewerProcess::AmpToDBNorm(mAmplitude);
}

void
SASFrame2::ComputeFrequency()
{
    BL_FLOAT freq = mPartialsToFreq->ComputeFrequency(mPartials);
    
    mFrequency = freq;
}

void
SASFrame2::ComputeColor()
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
SASFrame2::ComputeColorAux()
{
//#define EPS 1e-8
    
    mColor.Resize(mBufferSize/2);
    
    if (mFrequency < EPS)
    {
        BLUtils::FillAllZero(&mColor);
        
        return;
    }
    
    // Will interpolate values between the partials
    
    BL_FLOAT undefinedValue = -1.0;
    BLUtils::FillAllValue(&mColor, undefinedValue);
    
    // Fix bounds at 0
    mColor.Get()[0] = 0.0;
    mColor.Get()[mBufferSize/2 - 1] = 0.0;
    
    BL_FLOAT hzPerBin = mSampleRate/mBufferSize;
    
    // Put the values we have
    for (int i = 0; i < mPartials.size(); i++)
    {
        const PartialTracker3::Partial &p = mPartials[i];
 
        // Dead or zombie: do not use for color enveloppe
        // (this is important !)
        if (p.mState != PartialTracker3::Partial::ALIVE)
            continue;
        
        BL_FLOAT idx = p.mFreq/hzPerBin;
        
        // TODO: make an interpolation, it is not so good to align to bins
        idx = bl_round(idx);
        
        BL_FLOAT ampDB = p.mAmpDB;
        BL_FLOAT amp = DBToAmp(ampDB);
        
        if (((int)idx >= 0) && ((int)idx < mColor.GetSize()))
            mColor.Get()[(int)idx] = amp;
    }
    
    // Put some zeros when partials are missing
    BL_FLOAT freq = mFrequency;
    while(freq < mSampleRate/2.0)
    {
        if (!FindPartial(freq))
        {
            BL_FLOAT idx = freq/hzPerBin;
            if (idx >= mColor.GetSize())
                break;
            
            mColor.Get()[(int)idx] = 0.0;
        }
        
        freq += mFrequency;
    }
    
    // Fill al the other value
    bool extendBounds = false;
    BLUtils::FillMissingValues(&mColor, extendBounds, undefinedValue);
    
    // Normalize the color
    BL_FLOAT amplitude = DBToAmp(mAmplitudeDB);
    if (amplitude > 0.0)
    {
        BL_FLOAT coeff = 1.0/amplitude;
        
        BLUtils::MultValues(&mColor, coeff);
    }
}

void
SASFrame2::ComputeNormWarping()
{
    mPrevNormWarping = mNormWarping;
    
    WDL_TypedBuf<BL_FLOAT> prevWarping = mNormWarping;
    
    ComputeNormWarpingAux();
    
    if (prevWarping.GetSize() != mNormWarping.GetSize())
        return;
    
#if 1
    // Smooth
    for (int i = 0; i < mNormWarping.GetSize(); i++)
    {
        BL_FLOAT warp = mNormWarping.Get()[i];
        BL_FLOAT prevWarp = prevWarping.Get()[i];
        
        BL_FLOAT result = WARPING_SMOOTH_COEFF*prevWarp + (1.0 - WARPING_SMOOTH_COEFF)*warp;
        
        mNormWarping.Get()[i] = result;
    }
#endif
}

void
SASFrame2::ComputeNormWarpingAux()
{
    mNormWarping.Resize(mBufferSize/2);
    
    if (mFrequency < EPS)
    {
        BLUtils::FillAllValue(&mNormWarping, (BL_FLOAT)1.0);
        
        return;
    }
    
    // Will interpolate values between the partials
    
    BL_FLOAT undefinedValue = -1.0;
    BLUtils::FillAllValue(&mNormWarping, undefinedValue);
    
    // Fix bounds at 1
    mNormWarping.Get()[0] = 1.0;
    mNormWarping.Get()[mBufferSize/2 - 1] = 1.0;
    
    if (mPartials.size() < 2)
    {
        BLUtils::FillAllValue(&mNormWarping, (BL_FLOAT)1.0);
        
        return;
    }
    
    // Fundamental frequency
    
    BL_FLOAT hzPerBin = mSampleRate/mBufferSize;
    //BL_FLOAT freq0 = mPartials[0].mFreq; // OLD
    BL_FLOAT freq0 = mFrequency;
    
    // Put the values we have
    for (int i = /*1*/0; i < mPartials.size(); i++)
    {
        const PartialTracker3::Partial &p = mPartials[i];
    
        // Do no add to warping if dead or zombie
        if (p.mState != PartialTracker3::Partial::ALIVE)
            continue;
        
        BL_FLOAT idx = p.mFreq/hzPerBin;
        
        // TODO: make an interpolation, it is not so good to align to bins
        idx = bl_round(idx);
        
        BL_FLOAT freq = mPartials[i].mFreq;
        BL_FLOAT freq1 = BLUtils::FindNearestHarmonic(freq, freq0);
        
        BL_FLOAT normWarp = freq/freq1;
        
        if ((idx > 0) && (idx < mNormWarping.GetSize()))
            mNormWarping.Get()[(int)idx] = normWarp;
    }
    
    // Fill al the other value
    bool extendBounds = false;
    BLUtils::FillMissingValues(&mNormWarping, extendBounds, undefinedValue);
}

BL_FLOAT
SASFrame2::ApplyNormWarping(BL_FLOAT freq)
{
#if DBG_DISABLE_WARPING
    return freq;
#endif
    
    BL_FLOAT hzPerBin = mSampleRate/mBufferSize;
    
    //int idx = freq/hzPerBin;
    BL_FLOAT idx = freq/hzPerBin;
    idx = bl_round(idx);
    
    if (idx > mNormWarping.GetSize())
        return freq;
    
    BL_FLOAT w = mNormWarping.Get()[(int)idx];
    
    BL_FLOAT result = freq*w; // freq/W ??
    
    return result;
}

BL_FLOAT
SASFrame2::ApplyColor(BL_FLOAT freq)
{
    BL_FLOAT hzPerBin = mSampleRate/mBufferSize;
    
    int idx = freq/hzPerBin;
    
    if (idx > mColor.GetSize())
        return 0.0;
    
    BL_FLOAT result = mColor.Get()[idx];
    
    return result;
}

BL_FLOAT
SASFrame2::ApplyNormWarping(BL_FLOAT freq, BL_FLOAT t)
{
#if DBG_DISABLE_WARPING
    return freq;
#endif
    
    BL_FLOAT hzPerBin = mSampleRate/mBufferSize;
    
    //int idx = freq/hzPerBin;
    BL_FLOAT idx = freq/hzPerBin;
    idx = bl_round(idx);
    
    if (idx > mNormWarping.GetSize())
        return freq;
    
    BL_FLOAT w0 = mPrevNormWarping.Get()[(int)idx];
    BL_FLOAT w1 = mNormWarping.Get()[(int)idx];
    
    BL_FLOAT w = (1.0 - t)*w0 + t*w1;
    
    BL_FLOAT result = freq*w; // freq/W ??
    
    return result;
}

BL_FLOAT
SASFrame2::ApplyColor(BL_FLOAT freq, BL_FLOAT t)
{
    BL_FLOAT hzPerBin = mSampleRate/mBufferSize;
    
    int idx = freq/hzPerBin;
    
    if (idx > mColor.GetSize())
        return 0.0;
    
    BL_FLOAT col0 = mPrevColor.Get()[idx];
    BL_FLOAT col1 = mColor.Get()[idx];
    
    BL_FLOAT col = (1.0 - t)*col0 + t*col1;
    
    return col;
}

bool
SASFrame2::FindPartial(BL_FLOAT freq)
{
#define FIND_COEFF 0.25
    
    BL_FLOAT step = mFrequency*FIND_COEFF;
    
    for (int i = 0; i < mPartials.size(); i++)
    {
        const PartialTracker3::Partial &p = mPartials[i];
        
        if ((freq > p.mFreq - step) &&
            (freq < p.mFreq + step))
            // Found
            return true;
    }
    
    return false;
}

// Interpolate in db
#if 0
void
SASFrame2::GetPartial(PartialTracker3::Partial *result, int index, BL_FLOAT t)
{
    const PartialTracker3::Partial &currentPartial = mPartials[index];
    
    int prevPartialIdx = FindPrevPartialIdx(index);
    
    *result = currentPartial;

    // Manage decrease of dead partials
    //
    if ((currentPartial.mState == PartialTracker3::Partial::DEAD) &&
         currentPartial.mWasAlive)
    {
        // Decrease progressively the amplitude
        result->mAmpDB = MIN_AMP_DB;
      
        if (prevPartialIdx != -1)
            // Interpolate
        {
            const PartialTracker3::Partial &prevPartial = mPrevPartials[prevPartialIdx];
            
            BL_FLOAT t0 = t/SYNTH_DEAD_PARTIAL_DECREASE;
            if (t0 <= 1.0)
            {
                result->mAmpDB = (1.0 - t0)*prevPartial.mAmpDB + t0*MIN_AMP_DB; // + t0*currentPartial.mAmp;
            }
        }
    }
    
    // Manage interpolation of freq and amp
    //
#if 0 // Origin
    if (currentPartial.mState == PartialTracker3::Partial::ALIVE)
#else // More continuous
    if ((currentPartial.mState != PartialTracker3::Partial::DEAD) &&
         currentPartial.mWasAlive)
#endif
    {
        if (prevPartialIdx != -1)
        {
            const PartialTracker3::Partial &prevPartial = mPrevPartials[prevPartialIdx];
            
            if (prevPartial.mState == PartialTracker3::Partial::ALIVE)
            {
                result->mFreq = (1.0 - t)*prevPartial.mFreq + t*currentPartial.mFreq;
                result->mAmpDB = (1.0 - t)*prevPartial.mAmpDB + t*currentPartial.mAmpDB;
            }
        }
        else
            // New partial, fade in
        {
            // NOTE: this avoids vertical bars in the spectrogram when a partial starts
            
            // Increase progressively the amplitude
        
            // Interpolate
            BL_FLOAT t0 = t/SYNTH_DEAD_PARTIAL_DECREASE;
            if (t0 > 1.0)
                t0 = 1.0;
            
            result->mAmpDB = (1.0 - t0)*MIN_AMP_DB + t0*currentPartial.mAmpDB;
        }
    }
}
#endif

// Interpolate in amp
void
SASFrame2::GetPartial(PartialTracker3::Partial *result, int index, BL_FLOAT t)
{
    const PartialTracker3::Partial &currentPartial = mPartials[index];
    
    int prevPartialIdx = FindPrevPartialIdx(index);
    
    *result = currentPartial;
    
    // Manage decrease of dead partials
    //
    if ((currentPartial.mState == PartialTracker3::Partial::DEAD) &&
        currentPartial.mWasAlive)
    {
        // Decrease progressively the amplitude
        result->mAmpDB = MIN_AMP_DB;
        
        if (prevPartialIdx != -1)
            // Interpolate
        {
            const PartialTracker3::Partial &prevPartial = mPrevPartials[prevPartialIdx];
            
            BL_FLOAT t0 = t/SYNTH_DEAD_PARTIAL_DECREASE;
            if (t0 <= 1.0)
            {
                // Interpolate in amp
                BL_FLOAT amp = (1.0 - t0)*DBToAmp(prevPartial.mAmpDB); // + 0.0
                BL_FLOAT ampDB = AmpToDB(amp);
                result->mAmpDB = ampDB;
                
                //result->mAmpDB = (1.0 - t0)*prevPartial.mAmpDB + t0*MIN_AMP_DB; // + t0*currentPartial.mAmp;
            }
        }
    }
    
    // Manage interpolation of freq and amp
    //
#if 0 // Origin
    if (currentPartial.mState == PartialTracker3::Partial::ALIVE)
#else // More continuous
        if ((currentPartial.mState != PartialTracker3::Partial::DEAD) &&
            currentPartial.mWasAlive)
#endif
        {
            if (prevPartialIdx != -1)
            {
                const PartialTracker3::Partial &prevPartial = mPrevPartials[prevPartialIdx];
                
                if (prevPartial.mState == PartialTracker3::Partial::ALIVE)
                {
                    result->mFreq = (1.0 - t)*prevPartial.mFreq + t*currentPartial.mFreq;
                    
                    BL_FLOAT amp = (1.0 - t)*DBToAmp(prevPartial.mAmpDB) + t*DBToAmp(currentPartial.mAmpDB);
                    result->mAmpDB = AmpToDB(amp);
                    
                    //result->mAmpDB = (1.0 - t)*prevPartial.mAmpDB + t*currentPartial.mAmpDB;
                }
            }
            else
                // New partial, fade in
            {
                // NOTE: this avoids vertical bars in the spectrogram when a partial starts
                
                // Increase progressively the amplitude
                
                // Interpolate
                BL_FLOAT t0 = t/SYNTH_DEAD_PARTIAL_DECREASE;
                if (t0 > 1.0)
                    t0 = 1.0;
                
                BL_FLOAT amp = t0*DBToAmp(currentPartial.mAmpDB);
                result->mAmpDB = AmpToDB(amp);
                
                //result->mAmpDB = (1.0 - t0)*MIN_AMP_DB + t0*currentPartial.mAmpDB;
            }
        }
}

int
SASFrame2::FindPrevPartialIdx(int currentPartialIdx)
{
    if (currentPartialIdx >= mPartials.size())
        return -1;
    
    const PartialTracker3::Partial &currentPartial = mPartials[currentPartialIdx];
    
    // Find the corresponding prev partial
    int prevPartialIdx = -1;
    for (int i = 0; i < mPrevPartials.size(); i++)
    {
        const PartialTracker3::Partial &prevPartial = mPrevPartials[i];
        if (prevPartial.mId == currentPartial.mId)
        {
            prevPartialIdx = i;
        
            // Found
            break;
        }
    }
    
    return prevPartialIdx;
}

void
SASFrame2::GetSASPartial(SASPartial *result, int index, BL_FLOAT t)
{
    const SASPartial &currentPartial = mSASPartials[index];
    
    *result = currentPartial;
    
#if 0 // TEST
    // Manage decrease of dead partials
    //
    if ((currentPartial.mState == PartialTracker3::Partial::DEAD) &&
         currentPartial.mWasAlive)
    {
        // Decrease progressively the amplitude
        result->mAmpDB = MIN_AMP_DB;
        
        if (prevPartialIdx != -1)
            // Interpolate
        {
            const PartialTracker3::Partial &prevPartial = mSASPrevPartials[prevPartialIdx];
            
            BL_FLOAT t0 = t/DEAD_PARTIAL_DECREASE;
            if (t0 <= 1.0)
            {
                result->mAmp = (1.0 - t0)*prevPartial.mAmp; // + 0.0;
            }
        }
    }
#endif
    
    // Manage interpolation of freq and amp
    //
    if (index < mPrevSASPartials.size())
    {
        const SASPartial &prevPartial = mPrevSASPartials[index];
        
        result->mFreq = (1.0 - t)*prevPartial.mFreq + t*currentPartial.mFreq;
        result->mAmpDB = (1.0 - t)*prevPartial.mAmpDB + t*currentPartial.mAmpDB;
    }
    else
        // New partial, fade in
    {
        // NOTE: this avoids vertical bars in the spectrogram when a partial starts
            
        // Increase progressively the amplitude
            
        // Interpolate
        BL_FLOAT t0 = t/SYNTH_DEAD_PARTIAL_DECREASE;
        if (t0 > 1.0)
            t0 = 1.0;
            
        result->mAmpDB = t0*currentPartial.mAmpDB;
    }
}

