//
//  SASFrame.cpp
//  BL-SASViewer
//
//  Created by applematuer on 2/2/19.
//
//

#include <SASViewerProcess.h>

#include <PartialTWMEstimate.h>

#include <BLUtils.h>
#include <BLUtilsMath.h>

#include <BLDebug.h>

#include "SASFrame.h"


#define EPS 1e-15
#define INF 1e15

#define MIN_AMP_DB -120.0

// Synthesis
#define SYNTH_MAX_NUM_PARTIALS 10 //4 //10 // 20
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


#define PREV_FREQ_SMOOTH_COEFF 0.95

// Frequency computation
#define COMPUTE_FREQ_MAX_NUM_PARTIALS 4 //
#define COMPUTE_FREQ_MAX_FREQ         2000.0
// Avoid computing freq with very low amps
#define COMPUTE_FREQ_AMP_THRESHOLD    -60.0 //-40.0 // TODO: check and adjust this threshold

// Debug
#define DBG_FREQ     0 //1 // 0

// Compute samples directly from tracked partials
#define DBG_COMPUTE_SAMPLES 0 //0 //1

SASFrame::SASPartial::SASPartial()
{
    mFreq = 0.0;
    mAmpDB = MIN_AMP_DB;
    mPhase = 0.0;
}

SASFrame::SASPartial::SASPartial(const SASPartial &other)
{
    mFreq = other.mFreq;
    mAmpDB = other.mAmpDB;
    mPhase = other.mPhase;
}

SASFrame::SASPartial::~SASPartial() {}

bool
SASFrame::SASPartial::AmpLess(const SASPartial &p1, const SASPartial &p2)
{
    return (p1.mAmpDB < p2.mAmpDB);
}

SASFrame::SASFrame(int bufferSize, BL_FLOAT sampleRate, int overlapping)
{
    mBufferSize = bufferSize;
    
    mSampleRate = sampleRate;
    mOverlapping = overlapping;
    
    mAmplitudeDB = MIN_AMP_DB;
    mFrequency = 0.0;
    
    mPrevFrequency = 0.0;
    mPrevNumPartials = 0;
    
    mPitch = 1.0;
    
    mFreqEstimate = new PartialTWMEstimate(sampleRate);
}

SASFrame::~SASFrame()
{
    delete mFreqEstimate;
}

void
SASFrame::Reset()
{
    mPartials.clear();
    mPrevPartials.clear();
    
    mAmplitudeDB = MIN_AMP_DB;
    mFrequency = 0.0;
    
    mPrevFrequency = 0.0;
    mPrevNumPartials = 0;
    
    mSASPartials.clear();
    mPrevSASPartials.clear();
}

void
SASFrame::SetPartials(const vector<PartialTracker3::Partial> &partials)
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
SASFrame::GetAmplitudeDB()
{
    return mAmplitudeDB;
}

BL_FLOAT
SASFrame::GetFrequency()
{
    return mFrequency;
}

void
SASFrame::GetColor(WDL_TypedBuf<BL_FLOAT> *color)
{
    *color = mColor;
}

void
SASFrame::GetNormWarping(WDL_TypedBuf<BL_FLOAT> *warping)
{
    *warping = mNormWarping;
}

void
SASFrame::ComputeSamples(WDL_TypedBuf<BL_FLOAT> *samples)
{
#if !DBG_COMPUTE_SAMPLES
    ComputeSamplesSAS(samples);
#else
    DBG_ComputeSamples(samples);
#endif
}

void
SASFrame::ComputeSamplesSAS(WDL_TypedBuf<BL_FLOAT> *samples)
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
            
            SASPartial partial;
            for (int i = 0; i < samples->GetSize()/mOverlapping; i++)
            {
                // Get interpolated partial
                BL_FLOAT partialT = ((BL_FLOAT)i)/(samples->GetSize()/mOverlapping);
                GetSASPartial(&partial, partialIndex, partialT);
                //partial = mSASPartials[partialIndex]; // OLD
                
                BL_FLOAT freq = partial.mFreq;
                
                BL_FLOAT amp = DBToAmp(partial.mAmpDB);
                BL_FLOAT samp = amp*sin(phase); // cos
                
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


// Directly use partials provided
void
SASFrame::DBG_ComputeSamples(WDL_TypedBuf<BL_FLOAT> *samples)
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
            BL_FLOAT samp = amp*sin(phase); // cos
            
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
SASFrame::SetPitch(BL_FLOAT pitch)
{
    mPitch = pitch;
}

void
SASFrame::Compute()
{
    ComputeAmplitude();
    ComputeFrequency();
    ComputeColor();
    ComputeNormWarping();
}

void
SASFrame::ComputeAmplitude()
{
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

// NOTE: naive implementation
// TODO: implemented the method from the article referenced in the SAS paper
void
SASFrame::ComputeFrequency()
{
    mFrequency = 0.0;
    
    if (mPartials.empty())
    {
        mPrevFrequency = 0.0;
     
        return;
    }
    
#if 0 // Naive method
    // Find the first alive partial
    int idx = -1;
    for (int i = 0; i < mPartials.size(); i++)
    {
        const PartialTracker3::Partial &p = mPartials[i];
        
        if (p.mState == PartialTracker3::Partial::ALIVE)
        {
            idx = i;
            break;
        }
    }
    
    if (idx != -1)
    {
        const PartialTracker3::Partial &p = mPartials[idx];
        mFrequency = p.mFreq;
    }
    else
    {
        mFrequency = mPrevFrequency;
    }
    
    mPrevFrequency = mFrequency;
#endif
    
#if 1 // Estimate frequency from the set of partials
    
    // OPTIM: Keep only the first partials
    // (works well in case of more than 4 partials in input)
    vector<PartialTracker3::Partial> partials = mPartials;
    
#if 0 // TEST MONDAY
    SelectLowFreqPartials(&partials);
#endif
    
    // Works well, avoid freq jumps
#if 1
    // If amps are under a threshold, do not return partials
    // This will avoid computing bad freq later,
    // and put it in prevFreq
    SelectHighAmpPartials(&partials);
#endif
    
#if DBG_FREQ
    PartialTracker3::DBG_DumpPartials(partials, 10);
#endif
    
    // NOTE: Estimate and EstimateMultiRes worked well but now
    // only EstimateOptim works
    
    //BL_FLOAT freq = mFreqEstimate->Estimate(mPartials);
    //BL_FLOAT freq = mFreqEstimate->EstimateMultiRes(mPartials);
    
    // Works well
    BL_FLOAT freq = mFreqEstimate->EstimateOptim(partials);
    
#if DBG_FREQ
    BLDebug::AppendValue("freq0.txt", freq);
#endif
    
    BL_FLOAT minFreq = 0.0;
    
    // TEST MONDAY
#if 0 // GOOD !
    // Avoid octave jumps
    if (!partials.empty())
    {
        /*BL_FLOAT*/ minFreq = INF;
        for (int i = 0; i < partials.size(); i++)
        {
            const PartialTracker3::Partial &p = partials[i];
            if (p.mFreq < minFreq)
                minFreq = p.mFreq;
        }
        
        //freq = mFreqEstimate->GetNearestOctave(freq, mPartials[0].mFreq);
        
        freq = mFreqEstimate->GetNearestHarmonic(freq, minFreq); //mPartials[0].mFreq); /// ????
    }
#endif

#if DBG_FREQ
    BLDebug::AppendValue("prev-freq.txt", mPrevFrequency);
#endif

    // TEST MONDAY
    // Works better than min freq !
#if 1
    // Try to avoid octave jumps
    if (!partials.empty())
    {
        BL_FLOAT minFreqDiff = INF;
        for (int i = 0; i < partials.size(); i++)
        {
            const PartialTracker3::Partial &p = partials[i];
            BL_FLOAT diff = std::fabs(p.mFreq - mPrevFrequency);
            if (diff < minFreqDiff)
            {
                minFreqDiff = diff;
                minFreq = p.mFreq;
            }
        }
        
        //freq = mFreqEstimate->GetNearestOctave(freq, mPartials[0].mFreq);
        
        freq = mFreqEstimate->GetNearestHarmonic(freq, minFreq); //mPartials[0].mFreq); /// ????
    }
#endif
    
#if DBG_FREQ
    BLDebug::AppendValue("min-freq.txt", minFreq);
#endif

#if DBG_FREQ
    BLDebug::AppendValue("freq1.txt", freq);
#endif
    
    // Use this to avoid harmonic jumps of the fundamental frequency
    
#if 0 // Fix freq jumps
    
    // Allow freq jumps if we had only one partial before
    // (because we don't really now the real frequency if we have only
    // have one partial
    if (mPrevNumPartials > 1)
    {
        if (mPartials.size() > 1)
        {
            if (mPrevFrequency > EPS)
                freq = mFreqEstimate->FixFreqJumps(freq, mPrevFrequency);
        }
    }
#endif
    
#if 0 // Smooth frequency
    if (mPrevFrequency > EPS)
    {
        mFrequency = FREQ_SMOOTH_COEFF*mPrevFrequency + (1.0 - FREQ_SMOOTH_COEFF)*freq/*2*/;
    }
    else
    {
        mFrequency = freq;
    }
    
    mFrequency = mFreqEstimate->GetNearestHarmonic(mFrequency, mPrevFrequency);
    
#else
    mFrequency = freq;
#endif
    
    // If we have only 1 partial, the computed freq may not be the correct one
    // but the frequency of the harmonic instead.
    //
    // Wait for frequency estimation from several partials before considering the
    // computed frequency as correct.
    if ((mPartials.size() > 1) && (mPrevNumPartials > 1))
    {
#if 0 // No smooth
        mPrevFrequency = mFrequency;
#else // Smooth Prev frequency
        if (mPrevFrequency < EPS)
            mPrevFrequency = mFrequency;
        else
        {
            mPrevFrequency =
                PREV_FREQ_SMOOTH_COEFF*mPrevFrequency +
                (1.0 - PREV_FREQ_SMOOTH_COEFF)*mFrequency;
        }
#endif
    }
#endif
    
    mPrevNumPartials = partials.size();
}

void
SASFrame::SelectLowFreqPartials(vector<PartialTracker3::Partial> *partials)
{
    vector<PartialTracker3::Partial> partials0 = *partials;
    sort(partials0.begin(), partials0.end(), PartialTracker3::Partial::FreqLess);
    
    partials->clear();
    for (int i = 0; i < partials0.size(); i++)
    {
        // TEST SUNDAY
        //if (i >= COMPUTE_FREQ_MAX_NUM_PARTIALS)
        //    break;
        
        const PartialTracker3::Partial &p = partials0[i];
        partials->push_back(p);
        
        // TEST
        if (partials->size() >= COMPUTE_FREQ_MAX_NUM_PARTIALS)
            break;
    }
}

// TEST MONDAY
void
SASFrame::SelectHighAmpPartials(vector<PartialTracker3::Partial> *partials)
{
    vector<PartialTracker3::Partial> partials0 = *partials;
    sort(partials0.begin(), partials0.end(), PartialTracker3::Partial::AmpLess);
    reverse(partials0.begin(), partials0.end());
    
    partials->clear();
    for (int i = 0; i < partials0.size(); i++)
    {
        const PartialTracker3::Partial &p = partials0[i];
        if (p.mAmpDB > COMPUTE_FREQ_AMP_THRESHOLD)
            partials->push_back(p);
        
        if (partials->size() >= COMPUTE_FREQ_MAX_NUM_PARTIALS)
            break;
    }
    
    sort(partials->begin(), partials->end(), PartialTracker3::Partial::FreqLess);
}

void
SASFrame::ComputeColor()
{
    WDL_TypedBuf<BL_FLOAT> prevColor = mColor;
    
    ComputeColorAux();
    
    if (prevColor.GetSize() != mColor.GetSize())
        return;
    
    BLDebug::DumpData("color.txt", mColor);
    
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
SASFrame::ComputeColorAux()
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
SASFrame::ComputeNormWarping()
{
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
SASFrame::ComputeNormWarpingAux()
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
        BL_FLOAT freq1 = BLUtilsMath::FindNearestHarmonic(freq, freq0);
        
        BL_FLOAT normWarp = freq/freq1;
        
        if ((idx > 0) && (idx < mNormWarping.GetSize()))
            mNormWarping.Get()[(int)idx] = normWarp;
    }
    
    // Fill al the other value
    bool extendBounds = false;
    BLUtils::FillMissingValues(&mNormWarping, extendBounds, undefinedValue);
}

BL_FLOAT
SASFrame::ApplyNormWarping(BL_FLOAT freq)
{
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
SASFrame::ApplyColor(BL_FLOAT freq)
{
    BL_FLOAT hzPerBin = mSampleRate/mBufferSize;
    
    int idx = freq/hzPerBin;
    
    if (idx > mColor.GetSize())
        return 0.0;
    
    BL_FLOAT result = mColor.Get()[idx];
    
    return result;
}

bool
SASFrame::FindPartial(BL_FLOAT freq)
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
SASFrame::GetPartial(PartialTracker3::Partial *result, int index, BL_FLOAT t)
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
SASFrame::GetPartial(PartialTracker3::Partial *result, int index, BL_FLOAT t)
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
SASFrame::FindPrevPartialIdx(int currentPartialIdx)
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
SASFrame::GetSASPartial(SASPartial *result, int index, BL_FLOAT t)
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

