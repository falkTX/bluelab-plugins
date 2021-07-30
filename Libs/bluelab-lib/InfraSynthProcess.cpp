//
//  InfraSynthProcess.cpp
//  BL-Infra
//
//  Created by Pan on 20/04/18.
//
//

#include <algorithm>
using namespace std;

#include <SineSynthSimple.h>

#include <FilterIIRLow12dB.h>

#include <BLUtils.h>
#include <BLUtilsMath.h>

#include "InfraSynthProcess.h"

#define EPS 1e-15

#define FIX_RESET 1

// Phantom
#define PHANTOM_NUM_PARTIALS 10
#define DECREASE_DB 24.0 //12.0
#define PHANTOM_PARTIAL_ID_OFFSET 10000

#define PHANTOM_INIT_DB 10.0 // NEW

// PROBLEM: we can't compute the initial freq very exactly
// with partial tracker (error 0.2%)
// => so when we add it to the original sound, there is
// and oscillation in amp (due to beating frequency)
// oscillation: around 0.5-1Hz
#define ADD_INITIAL_FREQ_PARTIAL 0

// PROBLEM
// Makes a small EQ effect (seems to increase some medium freqs)
#define INCREASE_INITIAL_FREQ 0

// Increase all original signal
// PROBLEM: also increase the high frequencies
#define INCREASE_ALL_FREQS 0

// GOOD
// Low pass filter then increase
#define INCREASE_LOW_FREQS 1
// See: https://www.teachmeaudio.com/mixing/techniques/audio-spectrum/
#define LOW_FREQ_CUTOFF 300.0 //250.0

#define INITIAL_INCREASE_DB 6.0 //24.0 //6.0 //12.0 //6.0 //24.0 //6.0

// Sub
#define SUB_PARTIAL_ID_OFFSET 20000

// Do not process phantom fundamental if the first partial frequency is too high
#define MIN_PHANTOM_FREQ_PARTIAL 300.0 //250.0

// Do not create sub freq if partial freq is greater than this value
#define MIN_SUB_FREQ_PARTIAL 300.0 //250.0

// With that, we really hear the phantom harmonic effect !
#define PHANTOM_BOOST 1

// Filter high frequencies that appear when generating the sub octave frequency
#define FIX_HIGH_FREQS_SUB 1
#define SUB_CUTOFF 2000.0

// OPTIM PROF Infra
#define MAX_DETECT_FREQ 500.0

// OPTIM PROF Infra
// Do not compute the fft phases to avoid atan2
// (in the current implementation, we don't need fft phases
// => be careful if we change flags here or in PartialTracker3 )
#define SKIP_FFT_PHASES 1

// Sub freq boost
// With that, the result is more like the one of Locic SubBass plugin
#define SUB_FREQ_BOOST 1
#define SUB_FREQ_BOOST_FACTOR 2.0

//#define SUB_FREQ_BOOST_FACTOR_DB 12.0 // NEW
#define SUB_FREQ_DB_OFFSET -12.0 // NEW

// Totally fixes phant mix knob crackles
// (there remained very light crackles
#define FIX_PHANT_MIX_CRACKLE 1

#define KEEP_ONLY_FIRST_DETECTED_PARTIAL 0 //1

//#define INITIAL_INCREASE_DB2 20.0

#define INFRA_SYNTH_OPTIM3 1

#define RANGE_500_HZ 1

#if RANGE_500_HZ
#undef MIN_PHANTOM_FREQ_PARTIAL
#undef MIN_SUB_FREQ_PARTIAL
#undef LOW_FREQ_CUTOFF
//#define MAX_DETECT_FREQ 600.0 //500.0
#define MIN_PHANTOM_FREQ_PARTIAL 500.0
#define MIN_SUB_FREQ_PARTIAL 500.0
#define LOW_FREQ_CUTOFF 600.0 //500.0
#endif

// BAD: doesn't remove rumble with low frequencies (octave down) and sub octave
#define FIX_RUMBLE_SOUND 0

// NOT SO GOOD: avoid rumble when playing low freq notes
// (due to sub octave frequency)
#define FIX_RUMBLE_LOW_SUB 0 //1

#define OPTIM_APPLY_PARAM_SHAPE 1


InfraSynthProcess::InfraSynthProcess(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    
    mPhantomSynth = new SineSynthSimple(sampleRate);
    mSubSynth = new SineSynthSimple(sampleRate);
    
    //
    mPhantomFreq = 20.0;
    mPhantomMix = 0.5;
    mSubOrder = 1;
    mSubMix = 0.0;
    
    // For ramps
    mPrevPhantomMix = mPhantomMix;
    mPrevSubMix = mSubMix;
    
    //
    mLowFilter = NULL;
    mSubLowFilter = NULL;
    
#if INCREASE_LOW_FREQS
    mLowFilter = new FilterIIRLow12dB();
    mLowFilter->Init(LOW_FREQ_CUTOFF, sampleRate);
#endif
    
#if FIX_HIGH_FREQS_SUB
    mSubLowFilter = new FilterIIRLow12dB();
    mSubLowFilter->Init(SUB_CUTOFF, sampleRate);
#endif
    
#if INFRA_PROCESS_PROFILE
    BlaTimer::Reset(&mTimer, &mCount);
#endif
    
    mEnableStartSync = false;
    
    mDebug = false;
}

InfraSynthProcess::~InfraSynthProcess()
{
    delete mPhantomSynth;
    delete mSubSynth;
    
#if INCREASE_LOW_FREQS
    delete mLowFilter;
#endif
    
#if FIX_HIGH_FREQS_SUB
    delete mSubLowFilter;
#endif
}

void
InfraSynthProcess::Reset()
{
    Reset(mSampleRate);
}

void
InfraSynthProcess::Reset(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    
    mPhantomSynth->Reset(sampleRate);
    mSubSynth->Reset(sampleRate);
    
#if INCREASE_LOW_FREQS
    mLowFilter->Init(LOW_FREQ_CUTOFF, sampleRate);
#endif
    
#if FIX_HIGH_FREQS_SUB
    mSubLowFilter->Init(SUB_CUTOFF, sampleRate);
#endif
}

BL_FLOAT
InfraSynthProcess::NextSample(vector<SineSynthSimple::Partial> *phantomPartials,
                              vector<SineSynthSimple::Partial> *subPartials,
                              BL_FLOAT *phantomSamp, BL_FLOAT *subSamp)
{
#if KEEP_ONLY_FIRST_DETECTED_PARTIAL
    sort(partials.begin(), partials.end(), SineSynthSimple::Partial::FreqLess);
#endif
    
    // Phantom
    //vector<SineSynthSimple::Partial> phantomPartials;
    //GeneratePhantomPartials(*partials, &phantomPartials);
    
    BL_FLOAT phantomSynthSample = mPhantomSynth->NextSample(phantomPartials);

#if !OPTIM_APPLY_PARAM_SHAPE
    BL_FLOAT phantomMix = BLUtils::ApplyParamShape(mPhantomMix, 0.5);
#else
    BL_FLOAT phantomMix = mPhantomMix;
#endif
    
    phantomSynthSample *= phantomMix;
    
    // TODO: fix knob crackles
    
    // Sub
    //vector<SineSynthSimple::Partial> subPartials;
    //GenerateSubPartials(*partials, &subPartials);
    BL_FLOAT subSynthSample = mSubSynth->NextSample(subPartials);
    
#if SUB_FREQ_BOOST
    subSynthSample *= SUB_FREQ_BOOST_FACTOR;
#endif
    
#if FIX_HIGH_FREQS_SUB
    subSynthSample = mSubLowFilter->Process(subSynthSample);
    
#if !OPTIM_APPLY_PARAM_SHAPE
    BL_FLOAT subMix = BLUtils::ApplyParamShape(mSubMix, 0.5);
#else
    BL_FLOAT subMix = mSubMix;
#endif
    
    subSynthSample *= subMix;
#endif
    
    // TODO: avoid knob crackles
    
    // Global result
    BL_FLOAT result = phantomSynthSample + subSynthSample;
    
    // Separated result
    if (phantomSamp != NULL)
        *phantomSamp = phantomSynthSample;
    
    if (subSamp != NULL)
        *subSamp = subSynthSample;
    
    return result;
}

BL_FLOAT
InfraSynthProcess::AmplifyOriginSample(BL_FLOAT sample)
{
    // Phantom initial freqs
#if INCREASE_LOW_FREQS
    BL_FLOAT lowSample = mLowFilter->Process(sample);
    
    BL_FLOAT dbCoeff = mPhantomMix*BLUtils::DBToAmp(INITIAL_INCREASE_DB);
    
#if 0 // BAD
    // Increase more, to be similar to Infra plug
    // (because in Infra, we managed coeffs based on left an right partial indices
    //BL_FLOAT dbCoeff2 = DBToAmp(INITIAL_INCREASE_DB2);
    //dbCoeff *= dbCoeff2;
#endif
    
    lowSample *= dbCoeff;
#endif
    
    return lowSample;
}

void
InfraSynthProcess::SetPhantomFreq(BL_FLOAT phantomFreq)
{
    mPhantomFreq = phantomFreq;
}

void
InfraSynthProcess::SetPhantomMix(BL_FLOAT phantomMix)
{
    mPhantomMix = phantomMix;
}

void
InfraSynthProcess::SetSubOrder(int subOrder)
{
    mSubOrder = subOrder;
}

void
InfraSynthProcess::SetSubMix(BL_FLOAT subMix)
{
    mSubMix = subMix;
}

void
InfraSynthProcess::SetDebug(bool flag)
{
    mDebug = flag;
    
    mPhantomSynth->SetDebug(flag);
}

// Generate higher partials
void
InfraSynthProcess::GeneratePhantomPartials(const SineSynthSimple::Partial &partial,
                                           vector<SineSynthSimple::Partial> *newPartials,
                                           bool fixedPhantomFreq)
{
    newPartials->clear();
    
    if (partial.mFreq > MIN_PHANTOM_FREQ_PARTIAL)
        return;
        
    // Compute phantom freq
    BL_FLOAT phantomFreq = partial.mFreq;
    if (fixedPhantomFreq)
    {
        while(phantomFreq > mPhantomFreq)
        {
                phantomFreq *= 0.5;
        }
        phantomFreq *= 2.0;
    }
    
#if ADD_INITIAL_FREQ_PARTIAL
    SineSynthSimple::Partial initP = partial;
    initP.mAmpDB = partial.mAmpDB + INITIAL_INCREASE_DB;
    initP.mFreq = partial.mFreq;
    initP.mId = i + PHANTOM_PARTIAL_ID_OFFSET;
        
    // Just in case
    initP.mPhase = partial.mPhase;
        
    newPartials->push_back(initP);
#endif
        
    int partialNum = 0;
#if !PHANTOM_BOOST
    BL_FLOAT ampDB = partial.mAmpDB - DECREASE_DB;
#else
    // If phantom boost, do not decrease the first partial
    // We can really hear the phantom fundamental effect here !
    
#if !INFRA_SYNTH_OPTIM3
    BL_FLOAT ampDB = partial.mAmpDB;
    
    // NEW
    // For InfraSynth, must substract 10dB to the first partial,
    // to be similar to the infra plug
    ampDB -= PHANTOM_INIT_DB;
#else
    BL_FLOAT amp = partial.mAmp;
    
    BL_FLOAT phantomInitAmp = BLUtils::DBToAmp(-PHANTOM_INIT_DB);
    amp *= phantomInitAmp;
#endif
    
#endif
    
    BL_FLOAT freq = partial.mFreq + phantomFreq;
    
    while(partialNum < PHANTOM_NUM_PARTIALS)
    {
        SineSynthSimple::Partial p0 = partial;
        p0.mFreq = freq;
        // Prefer choosing "i" to avoid problems when ids are blinking
        p0.mId = /*i +*/ (partialNum + 2)*PHANTOM_PARTIAL_ID_OFFSET;
        
#if !INFRA_SYNTH_OPTIM3
        p0.mAmpDB = ampDB;
#else
        p0.mAmp = amp;
#endif
        
        // Just in case
        p0.mPhase = partial.mPhase;
        
        if (mEnableStartSync)
        {
            p0.mPhase = 0.5*M_PI;
        }
        
        newPartials->push_back(p0);
        
        // DB per octave
        BL_FLOAT numOctave = std::log((freq + phantomFreq)/freq)/std::log(2.0);
        
#if !INFRA_SYNTH_OPTIM3
        ampDB -= numOctave*DECREASE_DB;
#else
        BL_FLOAT decreaseAmp = BLUtils::DBToAmp(-numOctave*DECREASE_DB);
        amp *= decreaseAmp;
#endif
        
        freq += phantomFreq;
        partialNum++;
    }
}

// Generate higher partials
void
InfraSynthProcess::GenerateSubPartials(const SineSynthSimple::Partial &partial,
                                       vector<SineSynthSimple::Partial> *newPartials)
{
    newPartials->clear();
        
    BL_FLOAT subFreq = partial.mFreq;
        
    if (subFreq > MIN_SUB_FREQ_PARTIAL)
        return;
    
    for (int j = 0; j < mSubOrder; j++)
        subFreq *= 0.5;
    
#if FIX_RUMBLE_LOW_SUB
    if (subFreq < /*30.0*/20.0)
    {
        // Still generate something !
        // => The volume of the original freq needs to be increased when sub mix is increased
        subFreq = partial.mFreq;
    }
#endif
    
#if 0 // ORIGIN
    BL_FLOAT ampDB = partial.mAmpDB + mSubOrder*DECREASE_DB;
    
    if (ampDB > 0.0)
        ampDB = 0.0;
#endif
    
#if 1 // NEW
    
#if !INFRA_SYNTH_OPTIM3
    BL_FLOAT ampDB = partial.mAmpDB + DECREASE_DB;
    
    // Add more db, to be like in Infra plug
    ampDB += SUB_FREQ_DB_OFFSET;
#else
    BL_FLOAT decreaseAmp = BLUtils::DBToAmp(DECREASE_DB);
    BL_FLOAT amp = partial.mAmp*decreaseAmp;

    // Add more db, to be like in Infra plug
    BL_FLOAT subOffsetAmp = BLUtils::DBToAmp(SUB_FREQ_DB_OFFSET);
    
    amp *= subOffsetAmp;
#endif
    
#endif
        
    SineSynthSimple::Partial p0 = partial;
    p0.mFreq = subFreq;
    // Prefer choosing "i" to avoid problems when ids are blinking
    p0.mId = /*i +*/ SUB_PARTIAL_ID_OFFSET;
    
#if !INFRA_SYNTH_OPTIM3
    p0.mAmpDB = ampDB;
#else
    p0.mAmp = amp;
#endif
    
#if !FIX_RUMBLE_SOUND
    // Just in case
    p0.mPhase = partial.mPhase;
    
    if (mEnableStartSync)
    {
        p0.mPhase = 0.5*M_PI;
    }
    
#else
    p0.mPhase = partial.mPhase + (((BL_FLOAT)rand()/RAND_MAX))*2.0*M_PI;
#endif
    
    newPartials->push_back(p0);
}

void
InfraSynthProcess::TriggerSync()
{
    mPhantomSynth->TriggerSync();
}

void
InfraSynthProcess::EnableStartSync(bool flag)
{
    mEnableStartSync = flag;
}

