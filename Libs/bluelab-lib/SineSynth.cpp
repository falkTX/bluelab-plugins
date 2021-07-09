//
//  SineSynth.cpp
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

#include "SineSynth.h"


// Optim: optimizes a little (~5/15%)
SIN_LUT_CREATE(SAS_FRAME_SIN_LUT, 4096);

// 4096*64 (2MB)
// 2x slower than 4096
//SIN_LUT_CREATE(SAS_FRAME_SIN_LUT, 262144);


#define EPS 1e-15
#define INF 1e15

#define MIN_AMP_DB -120.0

#if 0
// Synthesis
#define SYNTH_MAX_NUM_PARTIALS 40 //20 //10 origin: 10, (40 for test bell)
#define SYNTH_AMP_COEFF 4.0
#define SYNTH_MIN_FREQ 50.0
#define SYNTH_DEAD_PARTIAL_DECREASE 0.25
#endif


#define COMPUTE_SAS_SAMPLES_TABLE    0 // Perf gain ~x10 compared to (with nearest + block buffers)


SineSynth::Partial::Partial()
{
    mFreq = 0.0;
    mAmpDB = MIN_AMP_DB;
    mPhase = 0.0;
    mId = -1;
}

SineSynth::Partial::Partial(const Partial &other)
{
    mFreq = other.mFreq;
    mAmpDB = other.mAmpDB;
    mPhase = other.mPhase;
    mId = other.mId;
}

SineSynth::Partial::~Partial() {}


SineSynth::SineSynth(int bufferSize, BL_FLOAT sampleRate, int overlapping)
{
    SIN_LUT_INIT(SAS_FRAME_SIN_LUT);
    
    mBufferSize = bufferSize;
    mSampleRate = sampleRate;
    mOverlapping = overlapping;

    mTableSynth = NULL;
    
#if COMPUTE_SAS_SAMPLES_TABLE
    BL_FLOAT minFreq = 20.0;
    mTableSynth = new WavetableSynth(bufferSize, overlapping,
                                     sampleRate, 1, minFreq);
#endif
}

SineSynth::~SineSynth()
{
#if COMPUTE_SAS_SAMPLES_TABLE
    delete mTableSynth;
#endif
}

void
SineSynth::Reset(int bufferSize, BL_FLOAT sampleRate, int overlapping)
{
    mPartials.clear();
    mPrevPartials.clear();
    
    mBufferSize = bufferSize;
    mSampleRate = sampleRate;
    mOverlapping = overlapping;
    
#if COMPUTE_SAS_SAMPLES_TABLE
    mTableSynth->Reset(sampleRate);
#endif
}

void
SineSynth::SetPartials(const vector<PartialTracker3::Partial> &partials)
{
    mPrevPartials = mPartials;
    
    mPartials.clear();
    for (int i = 0; i < partials.size(); i++)
    {
        const PartialTracker3::Partial &pt = partials[i];
        
        Partial p;
        p.mAmpDB = pt.mAmpDB;
        p.mFreq = pt.mFreq;
        p.mPhase = pt.mPhase;
        p.mId = pt.mId;
        
        mPartials.push_back(p);
    }
}

// OPTIM PROF Infra
#if 0 // ORIGIN
void
SineSynth::ComputeSamples(WDL_TypedBuf<BL_FLOAT> *samples)
{
    samples->Resize(mBufferSize);
    BLUtils::FillAllZero(samples);
    
    // Create samples from new partials
    for (int i = 0; i < mPartials.size(); i++)
    {
        const Partial &p = mPartials[i];
        BL_FLOAT amp = DBToAmp(p.mAmpDB);
        BL_FLOAT freq = p.mFreq;
        
        int prevIndex = FindPrevPartialIdx(i);
        BL_FLOAT amp0 = (prevIndex >= 0) ? DBToAmp(mPrevPartials[prevIndex].mAmpDB) : 0.0;
        BL_FLOAT freq0 = (prevIndex >= 0) ? mPrevPartials[prevIndex].mFreq : freq;
        BL_FLOAT phase = (prevIndex >= 0) ? mPrevPartials[prevIndex].mPhase : p.mPhase; // 0.0
        
        // Optim
        BL_FLOAT phaseCoeff = 2.0*M_PI/mSampleRate;
        
        // Loop
        BL_FLOAT t = 0.0;
        BL_FLOAT tStep = 1.0/(samples->GetSize()/mOverlapping - 1); //-1
        
        for (int j = 0; j < samples->GetSize()/mOverlapping; j++)
        {
            // Compute freq and partial amp
            BL_FLOAT freq1 = (1.0 - t)*freq0 + t*freq;
            BL_FLOAT amp1 = (1.0 - t)*amp0 + t*amp;

#if 0
            BL_FLOAT samp = std::sin(phase);
#else
            BL_FLOAT samp;
            SIN_LUT_GET(SAS_FRAME_SIN_LUT, samp, phase);
#endif
            
            samp *= amp1;
            
          
            samples->Get()[j] += samp;
            
            t += tStep;
            phase += phaseCoeff*freq1;
        }
        
        // Compute next phase
        mPartials[i].mPhase = phase;
    }
    
    // Fade out prev partial that just have disappeared
    for (int i = 0; i < mPrevPartials.size(); i++)
    {
        const Partial &p = mPrevPartials[i];
        
        int index = FindPartialFromIdx(p.mId);
        if (index != -1)
            continue;
        
        BL_FLOAT amp0 = DBToAmp(p.mAmpDB);
        BL_FLOAT freq0 = p.mFreq;
        BL_FLOAT phase = p.mPhase;
        
        BL_FLOAT amp = 0.0;
        BL_FLOAT freq = p.mFreq;
        
        // Optim
        BL_FLOAT phaseCoeff = 2.0*M_PI/mSampleRate;
        
        // Loop
        BL_FLOAT t = 0.0;
        BL_FLOAT tStep = 1.0/(samples->GetSize()/mOverlapping - 1); //-1
        for (int j = 0; j < samples->GetSize()/mOverlapping; j++)
        {
            // Compute freq and partial amp
            BL_FLOAT freq1 = (1.0 - t)*freq0 + t*freq;
            BL_FLOAT amp1 = (1.0 - t)*amp0 + t*amp;
            
#if 0
            BL_FLOAT samp = std::sin(phase);
#else
            BL_FLOAT samp;
            SIN_LUT_GET(SAS_FRAME_SIN_LUT, samp, phase);
#endif
            
            samp *= amp1;
            
            
            samples->Get()[j] += samp;
            
            t += tStep;
            phase += phaseCoeff*freq1;
        }
    }
}
#else // OPTIMIZED
void
SineSynth::ComputeSamples(WDL_TypedBuf<BL_FLOAT> *samples)
{
    samples->Resize(mBufferSize);
    BLUtils::FillAllZero(samples);
    
    // Create samples from new partials
    for (int i = 0; i < mPartials.size(); i++)
    {
        const Partial &p = mPartials[i];
        BL_FLOAT amp = DBToAmp(p.mAmpDB);
        BL_FLOAT freq = p.mFreq;
        
        int prevIndex = FindPrevPartialIdx(i);
        BL_FLOAT amp0 = (prevIndex >= 0) ? DBToAmp(mPrevPartials[prevIndex].mAmpDB) : 0.0;
        BL_FLOAT freq0 = (prevIndex >= 0) ? mPrevPartials[prevIndex].mFreq : freq;
        BL_FLOAT phase = (prevIndex >= 0) ? mPrevPartials[prevIndex].mPhase : p.mPhase; // 0.0
        
        // Optim
        BL_FLOAT phaseCoeff = 2.0*M_PI/mSampleRate;
        
        // Loop
        BL_FLOAT t = 0.0;
        BL_FLOAT tStep = 1.0/(samples->GetSize()/mOverlapping - 1); //-1
        
        
        int numIter = samples->GetSize()/mOverlapping;
        
        BL_FLOAT freq1 = freq0;
        BL_FLOAT freq1Step = 0.0;
        if (numIter > 1)
            freq1Step = (freq - freq0)/(numIter - 1);
        
        BL_FLOAT amp1 = amp0;
        BL_FLOAT amp1Step = 0.0;
        if (numIter > 1)
            amp1Step = (amp - amp0)/(numIter - 1);
        
        for (int j = 0; j < numIter; j++)
        {
            // Compute freq and partial amp
            
#if 0
	  BL_FLOAT samp = std::sin(phase);
#else
            BL_FLOAT samp;
            SIN_LUT_GET(SAS_FRAME_SIN_LUT, samp, phase);
#endif
            
            samp *= amp1;
            
            
            samples->Get()[j] += samp;
            
            t += tStep;
            phase += phaseCoeff*freq1;
            
            freq1 += freq1Step;
            amp1 += amp1Step;
        }
        
        // Compute next phase
        mPartials[i].mPhase = phase;
    }
    
    // Fade out prev partial that just have disappeared
    for (int i = 0; i < mPrevPartials.size(); i++)
    {
        const Partial &p = mPrevPartials[i];
        
        int index = FindPartialFromIdx(p.mId);
        if (index != -1)
            continue;
        
        BL_FLOAT amp0 = DBToAmp(p.mAmpDB);
        BL_FLOAT freq0 = p.mFreq;
        BL_FLOAT phase = p.mPhase;
        
        BL_FLOAT amp = 0.0;
        BL_FLOAT freq = p.mFreq;
        
        // Optim
        BL_FLOAT phaseCoeff = 2.0*M_PI/mSampleRate;
        
        // Loop
        BL_FLOAT t = 0.0;
        BL_FLOAT tStep = 1.0/(samples->GetSize()/mOverlapping - 1); //-1
        for (int j = 0; j < samples->GetSize()/mOverlapping; j++)
        {
            // Compute freq and partial amp
            BL_FLOAT freq1 = (1.0 - t)*freq0 + t*freq;
            BL_FLOAT amp1 = (1.0 - t)*amp0 + t*amp;
            
#if 0
            BL_FLOAT samp = std::sin(phase);
#else
            BL_FLOAT samp;
            SIN_LUT_GET(SAS_FRAME_SIN_LUT, samp, phase);
#endif
            
            samp *= amp1;
            
            
            samples->Get()[j] += samp;
            
            t += tStep;
            phase += phaseCoeff*freq1;
        }
    }
}
#endif

#if 0
void
SineSynth::ComputeSamplesWin(WDL_TypedBuf<BL_FLOAT> *samples)
{
#if !COMPUTE_SAS_SAMPLES_TABLE
    ComputeSamplesSAS5(samples);
#else
    // TEST: was tested to put it in ComputeSamples()
    // benefit from overlapping (result: no clicks),
    // and to take full nearest buffers
    // => but the sound was worse than ifft + freqObj
    ComputeSamplesSASTable(samples);
    //ComputeSamplesSASTable2(samples);
#endif
}
#endif

#if 0
// Directly use partials provided
void
SineSynth::ComputeSamplesPartials(WDL_TypedBuf<BL_FLOAT> *samples)
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
            BL_FLOAT samp = amp*std::in(phase); // cos
            
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
#endif

#if 0
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
SineSynth::ComputeSamplesSAS5(WDL_TypedBuf<BL_FLOAT> *samples)
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
#endif

#if 0
void
SineSynth::ComputeSamplesSASTable(WDL_TypedBuf<BL_FLOAT> *samples)
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
#endif

#if 0
void
SineSynth::ComputeSamplesSASTable2(WDL_TypedBuf<BL_FLOAT> *samples)
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
                GetPartial(&partial, partialIndex, partialT);
                
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
#endif

#if 0
bool
SineSynth::FindPartial(BL_FLOAT freq)
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
#endif

// Interpolate in db
#if 0
void
SineSynth::GetPartial(PartialTracker3::Partial *result, int index, BL_FLOAT t)
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

#if 0
// Interpolate in amp
void
SineSynth::GetPartial(PartialTracker3::Partial *result, int index, BL_FLOAT t)
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
#endif

#if 0
int
SineSynth::FindPrevPartialIdx(int currentPartialIdx)
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
#endif

int
SineSynth::FindPrevPartialIdx(int currentPartialIdx)
{
    if (currentPartialIdx >= mPartials.size())
        return -1;
    
    const Partial &currentPartial = mPartials[currentPartialIdx];
    
    // Find the corresponding prev partial
    int prevPartialIdx = -1;
    for (int i = 0; i < mPrevPartials.size(); i++)
    {
        const Partial &prevPartial = mPrevPartials[i];
        if (prevPartial.mId == currentPartial.mId)
        {
            prevPartialIdx = i;
            
            // Found
            break;
        }
    }
    
    return prevPartialIdx;
}

int
SineSynth::FindPartialFromIdx(int partialIdx)
{
    // Find the corresponding partial
    int result = -1;
    for (int i = 0; i < mPartials.size(); i++)
    {
        const Partial &partial = mPartials[i];
        if (partial.mId == partialIdx)
        {
            result = i;
            
            // Found
            break;
        }
    }
    
    return result;
}

#if 0
void
SineSynth::GetPartial(Partial *result, int index, BL_FLOAT t)
{
    const Partial &currentPartial = mPartials[index];
    
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
#endif
