//
//  SineSynth3.cpp
//  BL-SASViewer
//
//  Created by applematuer on 2/2/19.
//
//

#include <SASViewerProcess.h>

#include <PartialsToFreq4.h>

#include <FreqAdjustObj3.h>

#include <WavetableSynth.h>

// Do not speed up...
#include <SinLUT2.h>

#include <BLUtils.h>

#include "SineSynth3.h"


// Optim: optimizes a little (~5/15%)
SIN_LUT_CREATE(SAS_FRAME_SIN_LUT, 4096);

// 4096*64 (2MB)
// 2x slower than 4096
//SIN_LUT_CREATE(SAS_FRAME_SIN_LUT, 262144);


#define MIN_AMP_DB -120.0

// Perf gain ~x10 compared to (with nearest + block buffers)
#define COMPUTE_SAS_SAMPLES_TABLE 0 

#define FREQ_SMOOTH_FACTOR 0.999 //0.8


SineSynth3::Partial::Partial()
{
    mFreq = 0.0;
    mAmpDB = MIN_AMP_DB;
    mPhase = 0.0;
    mId = -1;
}

SineSynth3::Partial::Partial(const Partial &other)
{
    mFreq = other.mFreq;
    mAmpDB = other.mAmpDB;
    mPhase = other.mPhase;
    mId = other.mId;
}

SineSynth3::Partial::~Partial() {}


SineSynth3::SineSynth3(int bufferSize, BL_FLOAT sampleRate, int overlapping)
{
    SIN_LUT_INIT(SAS_FRAME_SIN_LUT);
    
    mBufferSize = bufferSize;
    mSampleRate = sampleRate;
    mOverlapping = overlapping;
    
#if COMPUTE_SAS_SAMPLES_TABLE
    BL_FLOAT minFreq = 20.0;
    mTableSynth = new WavetableSynth(bufferSize, overlapping,
                                     sampleRate, 1, minFreq);
#endif
    
    mDebug = false;
}

SineSynth3::~SineSynth3()
{
#if COMPUTE_SAS_SAMPLES_TABLE
    delete mTableSynth;
#endif
}

void
SineSynth3::Reset(int bufferSize, BL_FLOAT sampleRate, int overlapping)
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
SineSynth3::SetPartials(const vector<PartialTracker5::Partial> &partials)
{
    mPrevPartials = mPartials;
    
    mPartials.clear();
    for (int i = 0; i < partials.size(); i++)
    {
        const PartialTracker5::Partial &pt = partials[i];
        
        Partial p;
        p.mAmpDB = pt.mAmpDB;
        p.mFreq = pt.mFreq;
        p.mPhase = pt.mPhase;
        p.mId = pt.mId;
        
        mPartials.push_back(p);
    }
}

void
SineSynth3::ComputeSamples(WDL_TypedBuf<BL_FLOAT> *samples)
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
        BL_FLOAT amp0 =
            (prevIndex >= 0) ? DBToAmp(mPrevPartials[prevIndex].mAmpDB) : 0.0;
        BL_FLOAT freq0 = (prevIndex >= 0) ? mPrevPartials[prevIndex].mFreq : freq;
        BL_FLOAT phase =
            (prevIndex >= 0) ? mPrevPartials[prevIndex].mPhase : p.mPhase;
        
        // Optim
        BL_FLOAT phaseCoeff = 2.0*M_PI/mSampleRate;
        
        // Loop
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
            
            BL_FLOAT samp;
            SIN_LUT_GET(SAS_FRAME_SIN_LUT, samp, phase);
            
            samp *= amp1;
            
            samples->Get()[j] += samp;
            
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
        BL_FLOAT tStep = 1.0/(samples->GetSize()/mOverlapping - 1);
        for (int j = 0; j < samples->GetSize()/mOverlapping; j++)
        {
            // Compute freq and partial amp
            BL_FLOAT freq1 = (1.0 - t)*freq0 + t*freq;
            BL_FLOAT amp1 = (1.0 - t)*amp0 + t*amp;
            
#if 0
            BL_FLOAT samp = std::sin(phase);
#else // Make a little spectro noise when generating low freqs
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

void
SineSynth3::SetDebug(bool flag)
{
    mDebug = flag;
}

int
SineSynth3::FindPrevPartialIdx(int currentPartialIdx)
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
SineSynth3::FindPartialFromIdx(int partialIdx)
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
