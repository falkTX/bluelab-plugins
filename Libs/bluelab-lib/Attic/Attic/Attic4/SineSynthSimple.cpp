//
//  SineSynthSimple.cpp
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
#include <SinLUT2.h>

#include <BLUtils.h>

#include "SineSynthSimple.h"


// Optim: optimizes a little (~5/15%)
SIN_LUT_CREATE(SAS_FRAME_SIN_LUT, 4096);

// 4096*64 (2MB)
// 2x slower than 4096
//SIN_LUT_CREATE(SAS_FRAME_SIN_LUT, 262144);


#define EPS 1e-15
#define INF 1e15

#define MIN_AMP_DB -120.0

#define FREQ_SMOOTH_FACTOR 0.999 //0.8


SineSynthSimple::Partial::Partial()
{
    mFreq = 0.0;
    mPhase = 0.0;
    mId = -1;
    
#if !INFRA_SYNTH_OPTIM3
    mAmpDB = MIN_AMP_DB;
#else
    mAmp = 0.0;
#endif
}

SineSynthSimple::Partial::Partial(const Partial &other)
{
    mFreq = other.mFreq;
    mPhase = other.mPhase;
    mId = other.mId;
    
#if !INFRA_SYNTH_OPTIM3
    mAmpDB = other.mAmpDB;
#else
    mAmp = other.mAmp;
#endif
}

SineSynthSimple::Partial::~Partial() {}


SineSynthSimple::SineSynthSimple(BL_FLOAT sampleRate)
{
    SIN_LUT_INIT(SAS_FRAME_SIN_LUT);
    
    mSampleRate = sampleRate;
    
    mSyncDirection = 1.0;
    
    mDebug = false;
}

SineSynthSimple::~SineSynthSimple() {}

void
SineSynthSimple::Reset(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
}

BL_FLOAT
SineSynthSimple::NextSample(vector<Partial> *partials)
{
    BL_FLOAT sample = 0.0;
    
    // Create samples from new partials
    for (int i = 0; i < partials->size(); i++)
    {
        const Partial &p = (*partials)[i];
        
#if !INFRA_SYNTH_OPTIM3
        BL_FLOAT amp = DBToAmp(p.mAmpDB);
#else
        BL_FLOAT amp = p.mAmp;
#endif
        BL_FLOAT freq = p.mFreq;
        
        BL_FLOAT phase = (*partials)[i].mPhase;
        
        // NOTE: do not manage prev partials here
        
        // Optim
        BL_FLOAT phaseCoeff = 2.0*M_PI/mSampleRate;
        
#if 0
        BL_FLOAT samp = std::sin(phase);
#else
        BL_FLOAT samp;
        SIN_LUT_GET(SAS_FRAME_SIN_LUT, samp, phase);
#endif
            
        samp *= amp;
        
        sample += samp;
        
        phase += phaseCoeff*freq*mSyncDirection;
        
        // Compute next phase
        (*partials)[i].mPhase = phase;
    }
    
    return sample;
}

void
SineSynthSimple::TriggerSync()
{
    // Soft sync
    // See: https://en.wikipedia.org/wiki/Oscillator_sync
    // For digital oscillators, Reversing Sync may less frequently generate aliasing.
    //
    mSyncDirection = -mSyncDirection;
}

void
SineSynthSimple::SetDebug(bool flag)
{
    mDebug = flag;
}
