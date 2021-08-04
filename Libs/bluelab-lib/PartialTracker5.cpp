//
//  PartialTracker5.cpp
//  BL-SASViewer
//
//  Created by applematuer on 2/2/19.
//
//

#include <algorithm>
using namespace std;

#include <SASViewerProcess.h>
#include <FreqAdjustObj3.h>

#include <Window.h>

#include <AWeighting.h>

#include <BLUtils.h>
#include <BLUtilsPhases.h>
#include <BLUtilsMath.h>

#include <BLDebug.h>

#include "PartialTracker5.h"

#define MIN_AMP_DB -120.0
#define MIN_NORM_AMP 1e-15

#define PARTIALS_HISTORY_SIZE 2

// As described in: https://www.dsprelated.com/freebooks/sasp/PARSHL_Program.html#app:parshlapp
#define SQUARE_MAGNS 0 //1


// Detect partials
//
#define DETECT_PARTIALS_START_INDEX 2

// Compute simple, avg, or parabola ?
// (Avg seems a little more smooth than parabola)
// Not perfect: gives hacked SASFrame amplitude
#define COMPUTE_PEAKS_AVG      0
// Good for narrow partials, but process flat partial tops badly
#define COMPUTE_PEAKS_PARABOLA 0
#define COMPUTE_PEAKS_SIMPLE   0
// Advance method in 2 steps.
// Good at least for mel and value stability, but inaccurate
#define COMPUTE_PEAKS_HALF_PROMINENCE_AVG 1

// With 1, that made more defined partials
// With 0, avoids partial leaking in noise ("oohoo")
#define NARROW_PARTIAL_FOOT 0 //1 //0
#define NARROW_PARTIAL_FOOT_COEFF 20.0 //40.0

#define DISCARD_FLAT_PARTIAL 1
#define DISCARD_FLAT_PARTIAL_COEFF 25000.0 //30000.0

#define DISCARD_INVALID_PEAKS 1

//
#define GLUE_BARBS              1 //0
#define GLUE_BARBS_AMP_RATIO    10.0 //4.0

// Filter
//

// Do we filter ?
#define FILTER_PARTIALS 1

#define MAX_ZOMBIE_AGE 2

// Seems better with 200Hz (tested on "oohoo")
#define DELTA_FREQ_ASSOC 0.01 // For normalized freqs. Around 100Hz

// Kalman
// "How much do we expect to our measurement vary"
//
// 200Hz (previously tested with 50Hz)
#define PT5_KF_E_MEA 0.01 // 200.0Hz
#define PT5_KF_E_EST PT5_KF_E_MEA

// "usually a small number between 0.001 and 1"
//
// If too low: predicted values move too slowly
// If too high: predicted values go straight
//
// 0.01: "oohoo" => fails when "EEAAooaa"
#define PT5_KF_Q 5.0 //2.0 // Was 1.0

// Seems better without, not sure...
#define USE_KALMAN_FOR_ASSOC 0 //1 //0
// GOOD: avoid many small zig-zags on the result lines
#define USE_KALMAN_FOR_RESULT 1

// Extract noise envelope
//
#define EXTRACT_NOISE_ENVELOPE_MAX    0
#define EXTRACT_NOISE_ENVELOPE_TRACK  0 // Prev
#define EXTRACT_NOISE_ENVELOPE_SIMPLE 1

#define DEBUG_DONT_FILL_NOISE_ENV     0
//#define DEBUG_DONT_FILL_NOISE_ENV   1 // TEST for Air

// Expe
// 1 gives good results for "Ooohoo" (method "Min")
// 2 gives good results for "Ti Tsu Koi" (method "Min")
#define NUM_ITER_EXTRACT_NOISE 4 //1

#define PROCESS_MUS_NOISE      1 // Working (but dos not remove all mus noise)
// 4 seems better than 2
#define HISTORY_SIZE_MUS_NOISE 4 //2 //4

// Take isles in the noise envelope, and suppress them if they are too big
// Interesting, but will need some sort of adaptive threshold
// Example: "oohoo"
#define THRESHOLD_NOISE_ISLES 0

// NOTE: See: https://www.dsprelated.com/freebooks/sasp/Spectral_Modeling_Synthesis.html
//
// and: https://www.dsprelated.com/freebooks/sasp/PARSHL_Program.html#app:parshlapp
//
// and: https://ccrma.stanford.edu/~jos/parshl/

// Get the precision when interpolating peak magns, but also for phases
#define INTERPOLATE_PHASES 1

// Better mel filtering of phase if they ar eunwrapped!
#define MEL_UNWRAP_PHASES 1

#define USE_FILTER_BANKS 1

// NEW
#define DENORM_PARTIAL_INDICES 1

unsigned long PartialTracker5::Partial::mCurrentId = 0;


PartialTracker5::Partial::Partial()
: mKf(PT5_KF_E_MEA, PT5_KF_E_EST, PT5_KF_Q)
{
    mPeakIndex = 0;
    mLeftIndex = 0;
    mRightIndex = 0;
    
    mFreq = 0.0;
    mAmp = 0.0;
    
    mPhase = 0.0;
    
    mState = ALIVE;
    
    mId = -1;
    
    mWasAlive = false;
    mZombieAge = 0;
    
    mAge = 0;
    
    mCookie = 0.0;
    
    // Kalman
    mPredictedFreq = 0.0;
}

    
PartialTracker5::Partial::Partial(const Partial &other)
: mKf(other.mKf)
{
    mPeakIndex = other.mPeakIndex;
    mLeftIndex = other.mLeftIndex;
    mRightIndex = other.mRightIndex;
    
    mFreq = other.mFreq;
    mAmp = other.mAmp;
    
    mPhase = other.mPhase;
        
    mState = other.mState;;
        
    mId = other.mId;
    
    mWasAlive = other.mWasAlive;
    mZombieAge = other.mZombieAge;
    
    mAge = other.mAge;
    
    mCookie = other.mCookie;
    
    // Kalman
    mPredictedFreq = other.mPredictedFreq;
}

PartialTracker5::Partial::~Partial() {}

void
PartialTracker5::Partial::GenNewId()
{
    mId = mCurrentId++;
}
    
bool
PartialTracker5::Partial::FreqLess(const Partial &p1, const Partial &p2)
{
    return (p1.mFreq < p2.mFreq);
}

bool
PartialTracker5::Partial::AmpLess(const Partial &p1, const Partial &p2)
{
    return (p1.mAmp < p2.mAmp);
}

bool
PartialTracker5::Partial::IdLess(const Partial &p1, const Partial &p2)
{
    return (p1.mId < p2.mId);
}

bool
PartialTracker5::Partial::CookieLess(const Partial &p1, const Partial &p2)
{
    return (p1.mCookie < p2.mCookie);
}

PartialTracker5::PartialTracker5(int bufferSize, BL_FLOAT sampleRate,
                                 BL_FLOAT overlapping)
{
    mBufferSize = bufferSize;
    mSampleRate = sampleRate;
    mOverlapping = overlapping;
    
    mThreshold = -60.0;
    
    mMaxDetectFreq = -1.0;
    
    // Scale
    mScale = new Scale();
    
    //
    
    //mXScale = Scale::LINEAR;
    //mXScale = Scale::MEL;
    mXScale = Scale::MEL_FILTER;
    
    //mYScale = Scale::LINEAR;
    mYScale = Scale::DB;
    
    //
    //mXScaleInv = Scale::LINEAR;
    //mXScaleInv = Scale::MEL_INV;
    mXScaleInv = Scale::MEL_FILTER_INV;
    mYScaleInv = Scale::DB_INV;
    
    mTimeSmoothCoeff = 0.5;
    mTimeSmoothNoiseCoeff = 0.5;
    
    mDbgParam = 1.0;

    // Default behavior, computed frequencies are not very accurate
    // (e.g ~6/8Hz accuracy)
    mComputeAccurateFreqs = false;
    
    // Optim
    ComputeAWeights(bufferSize/2, sampleRate);
    
    ReserveTmpBufs();
}

PartialTracker5::~PartialTracker5()
{
    delete mScale;
}

void
PartialTracker5::Reset()
{
    mPartials.clear();
    mResult.clear();
    
    mNoiseEnvelope.Resize(0);
    mHarmonicEnvelope.Resize(0);;

    mCurrentMagns.Resize(0);
    mCurrentPhases.Resize(0);
    
    mSmoothWinNoise.Resize(0);

    mPrevNoiseEnvelope.Resize(0);
    // For ComputeMusicalNoise()
    mPrevNoiseMasks.unfreeze();
    mPrevNoiseMasks.clear();
    
    mTimeSmoothPrevMagns.Resize(0);
    mTimeSmoothPrevNoise.Resize(0);
}

void
PartialTracker5::Reset(int bufferSize, BL_FLOAT sampleRate)
{
    mBufferSize = bufferSize;
    mSampleRate = sampleRate;
    
    Reset();

    // Optim
    ComputeAWeights(bufferSize/2, sampleRate);
    
    ReserveTmpBufs();
}

void
PartialTracker5::SetComputeAccurateFreqs(bool flag)
{
    mComputeAccurateFreqs = flag;
}

BL_FLOAT
PartialTracker5::GetMinAmpDB()
{
    return MIN_AMP_DB;
}

void
PartialTracker5::SetThreshold(BL_FLOAT threshold)
{
    mThreshold = threshold;
}

void
PartialTracker5::SetData(const WDL_TypedBuf<BL_FLOAT> &magns,
                         const WDL_TypedBuf<BL_FLOAT> &phases)
{
    mCurrentMagns = magns;
    mCurrentPhases = phases;

    // Not smoothed (will be overwritten later)
    //mLinearMagns = magns;
    
    PreProcess(&mCurrentMagns, &mCurrentPhases);
}

void
PartialTracker5::GetPreProcessedMagns(WDL_TypedBuf<BL_FLOAT> *magns)
{
    *magns = mCurrentMagns;
}

void
PartialTracker5::DetectPartials()
{
    WDL_TypedBuf<BL_FLOAT> &magns0 = mTmpBuf0;
    magns0 = mCurrentMagns;
    
    vector<Partial> &partials = mTmpPartials0;
    partials.resize(0);
    DetectPartials(magns0, mCurrentPhases, &partials);

    if (mComputeAccurateFreqs)
        ComputeAccurateFreqs(&partials);
    
    SuppressZeroFreqPartials(&partials);
    
    // Some operations
#if GLUE_BARBS
    vector<Partial> &prev = mTmpPartials1;
    prev = partials;
    
    GluePartialBarbs(magns0, &partials);
#endif
    
#if NARROW_PARTIAL_FOOT
    NarrowPartialFoot(magns0, &partials);
#endif
    
#if DISCARD_FLAT_PARTIAL
    DiscardFlatPartials(magns0, &partials);
#endif
    
    ComputePeaksHeights(mCurrentMagns, &partials);
    
    // Threshold
    ThresholdPartialsPeakHeight(&partials);
    
    mPartials.push_front(partials);
    
    while(mPartials.size() > PARTIALS_HISTORY_SIZE)
        mPartials.pop_back();
    
    mResult = partials;
}

void
PartialTracker5::ExtractNoiseEnvelope()
{
#if EXTRACT_NOISE_ENVELOPE_MAX
    ExtractNoiseEnvelopeMax();
#endif

#if EXTRACT_NOISE_ENVELOPE_TRACK
    ExtractNoiseEnvelopeTrack();
#endif
    
#if EXTRACT_NOISE_ENVELOPE_SIMPLE
    ExtractNoiseEnvelopeSimple();
#endif
    
    TimeSmoothNoise(&mNoiseEnvelope);
}

void
PartialTracker5::ExtractNoiseEnvelopeMax()
{
    // If not partial detected, noise envelope is simply the input magns
    mNoiseEnvelope = mCurrentMagns;
    
    if (mPartials.empty())
        return;
    
    // NOTE: may be better in the loop
    // (to avoid special case for first iteration)
    CutPartials(mPartials[0], &mNoiseEnvelope);
    
    // Iterate
    WDL_TypedBuf<BL_FLOAT> &dummyPhases = mTmpBuf1;
    ///BLUtils::ResizeFillZeros(&dummyPhases, mNoiseEnvelope.GetSize());
    dummyPhases.Resize(mNoiseEnvelope.GetSize());
    BLUtils::FillAllZero(&dummyPhases);
    for (int i = 0; i < NUM_ITER_EXTRACT_NOISE - 1; i++)
    {
        vector<Partial> &partials = mTmpPartials2;
        partials.resize(0);
        DetectPartials(mNoiseEnvelope, dummyPhases, &partials);
        
        CutPartials(partials, &mNoiseEnvelope);
    }
}

void
PartialTracker5::ExtractNoiseEnvelopeTrack()
{
    // If not partial detected, noise envelope is simply the input magns
    mNoiseEnvelope = mCurrentMagns;
    
    vector<Partial> &partials = mTmpPartials3;
    partials.resize(0);
    
    // Get all partials, or only alive ?
    //partials = mResult; // Not so good, makes noise peaks
    
    // NOTE: Must get the alive partials only,
    // otherwise we would get additional "garbage" partials,
    // that would corrupt the partial rectangle
    // and then compute incorrect noise peaks
    // (the state must be ALIVE, and not mWasAlive !)
    GetAlivePartials(&partials);
    
    // "tibetan bell": avoids keeping a partial at 2KHz in the noise envelope
    // => makes a better noise envelope !
#if GLUE_TWIN_PARTIALS
    GlueTwinPartials(mCurrentMagns, &partials);
#endif
    
    CutPartials(partials, &mNoiseEnvelope);
    // CutPartialsMinEnv(&mNoiseEnvelope);
    
    // Compute harmonic envelope
    // (origin signal less noise)
    mHarmonicEnvelope = mCurrentMagns;
    BLUtils::SubstractValues(&mHarmonicEnvelope, mNoiseEnvelope);
    
    BLUtils::ClipMin(&mHarmonicEnvelope, (BL_FLOAT)0.0);
}

void
PartialTracker5::ExtractNoiseEnvelopeSimple()
{
    vector<Partial> &partials = mTmpPartials4;
    partials.resize(0);
    
    // Get all partials, or only alive ?
    //partials = mResult; // Not so good, makes noise peaks
    
    // NOTE: Must get the alive partials only,
    // otherwise we would get additional "garbage" partials,
    // that would corrupt the partial rectangle
    // and then compute incorrect noise peaks
    // (the state must be ALIVE, and not mWasAlive !)
    GetAlivePartials(&partials);
    
    mHarmonicEnvelope = mCurrentMagns;
    
    // Just in case
    for (int i = 0; i < DETECT_PARTIALS_START_INDEX; i++)
        mHarmonicEnvelope.Get()[i] = MIN_NORM_AMP;
    
    KeepOnlyPartials(partials, &mHarmonicEnvelope);
    
    // Compute harmonic envelope
    // (origin signal less noise)
    mNoiseEnvelope = mCurrentMagns;
    
    BLUtils::SubstractValues(&mNoiseEnvelope, mHarmonicEnvelope);
    
    // Because it is in dB
    BLUtils::AddValues(&mNoiseEnvelope, (BL_FLOAT)0.0);
    
    BLUtils::ClipMin(&mNoiseEnvelope, (BL_FLOAT)0.0);
    
    // Avoids interpolation from 0 to the first valid index
    // (could have made an artificial increasing slope in the low freqs)
    for (int i = 0; i < mNoiseEnvelope.GetSize(); i++)
    {
        BL_FLOAT val = mNoiseEnvelope.Get()[i];
        if (val > BL_EPS)
            // First value
        {
            int prevIdx = i - 1;
            if (prevIdx > 0)
            {
                mNoiseEnvelope.Get()[prevIdx] = BL_EPS;
            }
            
            break;
        }
    }
    
#if THRESHOLD_NOISE_ISLES 
    // Threshold peak isles
    ThresholdNoiseIsles(&mNoiseEnvelope);
#endif
    
#if PROCESS_MUS_NOISE
    // Works well, but do not remove all musical noise
    ProcessMusicalNoise(&mNoiseEnvelope);
#endif
    
#if !DEBUG_DONT_FILL_NOISE_ENV
    // Create an envelope
    // NOTE: good for "oohoo", not good for "alphabet A"
    BLUtils::FillMissingValues2(&mNoiseEnvelope, false, (BL_FLOAT)0.0);
#endif
}

// Supress musical noise in the raw noise (not filled)
void
PartialTracker5::ProcessMusicalNoise(WDL_TypedBuf<BL_FLOAT> *noise)
{
// Must choose bigger value than 1e-15
// (otherwise the threshold won't work)
#define MUS_NOISE_EPS 1e-8
    
    // Better with an history
    // => suppress only spots that are in zone where partials are erased
    
    // If history is small => remove more spots (but unwanted ones)
    // If history too big => keep some spots that should have been erased
    if (mPrevNoiseMasks.size() < HISTORY_SIZE_MUS_NOISE)
    {
        mPrevNoiseMasks.push_back(*noise);

        if (mPrevNoiseMasks.size() == HISTORY_SIZE_MUS_NOISE)
            mPrevNoiseMasks.freeze();

        //if (mPrevNoiseMasks.size() > HISTORY_SIZE_MUS_NOISE)
        //    mPrevNoiseMasks.pop_front();
        
        return;
    }
    
    //const WDL_TypedBuf<BL_FLOAT> noiseCopy = *noise;
    WDL_TypedBuf<BL_FLOAT> &noiseCopy = mTmpBuf2;
    noiseCopy = *noise;
    
    // Search for begin of first isle: values with zero borders
    //
    int startIdx = 0;
    while (startIdx < noise->GetSize())
    {
        BL_FLOAT val = noise->Get()[startIdx];
        if (val < MUS_NOISE_EPS)
        // Zero
        {
            break;
        }
        
        startIdx++;
    }
    
    // Loop to search for isles
    //
    while(startIdx < noise->GetSize())
    {
        // Find "isles" in the current noise
        int startIdxIsle = startIdx;
        while (startIdxIsle < noise->GetSize())
        {
            BL_FLOAT val = noise->Get()[startIdxIsle];
            if (val > MUS_NOISE_EPS)
            // One
            {
                // Start of isle found
                break;
            }
            
            startIdxIsle++;
        }
        
        // Search for isles: values with zero borders
        int endIdxIsle = startIdxIsle;
        while (endIdxIsle < noise->GetSize())
        {
            BL_FLOAT val = noise->Get()[endIdxIsle];
            if (val < MUS_NOISE_EPS)
            // Zero
            {
                // End of isle found
                
                // Adjust the index to the last zero value
                if (endIdxIsle > startIdxIsle)
                    endIdxIsle--;
                
                break;
            }
            
            endIdxIsle++;
        }
        
        // Check that the prev mask is all zero
        // at in front of the isle
        bool prevMaskZero = true;
        for (int i = 0; i < mPrevNoiseMasks.size(); i++)
        {
            const WDL_TypedBuf<BL_FLOAT> &mask = mPrevNoiseMasks[i];
            
            for (int j = startIdxIsle; j <= endIdxIsle; j++)
            {
                BL_FLOAT prevVal = mask.Get()[j];
                if (prevVal > MUS_NOISE_EPS)
                {
                    prevMaskZero = false;
                
                    break;
                }
            }
            
            if (!prevMaskZero)
                break;
        }
        
        if (prevMaskZero)
        // We have a real isle
        {
            // TODO: check isle size ?
            
            // Earse the isle
            for (int i = startIdxIsle; i <= endIdxIsle; i++)
            {
                noise->Get()[i] = 0.0;
            }
        }
        
        startIdx = endIdxIsle + 1;
    }
    
    // Fill the history at the end
    // (to avoid having processing the current noise as history
    //mPrevNoiseMasks.push_back(noiseCopy);
    //if (mPrevNoiseMasks.size() > HISTORY_SIZE_MUS_NOISE)
    //    mPrevNoiseMasks.pop_front();
    mPrevNoiseMasks.push_pop(noiseCopy);
}

void
PartialTracker5::ThresholdNoiseIsles(WDL_TypedBuf<BL_FLOAT> *noise)
{
    // Must choose bigger value than 1e-15
    // (otherwise the threshold won't work)
#define ISLE_THRS_EPS 1e-8
    
#define ISLE_THRESHOLD -60.0
    
    // Search for begin of first isle: values with zero borders
    //
    int startIdx = 0;
    while (startIdx < noise->GetSize())
    {
        BL_FLOAT val = noise->Get()[startIdx];
        if (val < ISLE_THRS_EPS)
        // Zero
        {
            break;
        }
        
        startIdx++;
    }
    
    // Loop to search for isles
    //
    while(startIdx < noise->GetSize())
    {
        // Find "isles" in the current noise
        int startIdxIsle = startIdx;
        while (startIdxIsle < noise->GetSize())
        {
            BL_FLOAT val = noise->Get()[startIdxIsle];
            if (val > ISLE_THRS_EPS)
            // One
            {
                // Start of isle found
                break;
            }
            
            startIdxIsle++;
        }
        
        // Search for isles: values with zero borders
        int endIdxIsle = startIdxIsle;
        while (endIdxIsle < noise->GetSize())
        {
            BL_FLOAT val = noise->Get()[endIdxIsle];
            if (val < ISLE_THRS_EPS)
            // Zero
            {
                // End of isle found
                
                // Adjust the index to the last zero value
                if (endIdxIsle > startIdxIsle)
                    endIdxIsle--;
                
                break;
            }
            
            endIdxIsle++;
        }
        
        // Threshold
        BL_FLOAT maxVal = -BL_INF;
        for (int i = startIdxIsle; i <= endIdxIsle; i++)
        {
            BL_FLOAT val = noise->Get()[i];
            if (val > maxVal)
                maxVal = val;
        }
        
        if (maxVal > ISLE_THRESHOLD)
        // Above the threshold
        {
            // Suppress the isle
            for (int i = startIdxIsle; i <= endIdxIsle; i++)
            {
                noise->Get()[i] = 0.0;
            }
        }
        
        startIdx = endIdxIsle + 1;
    }
}

// Set to 0 until the next minimum
// Avoids "half partials" at the beginning that would
// not be detected, and finish in the noise envelope
void
PartialTracker5::ZeroToNextNoiseMinimum(WDL_TypedBuf<BL_FLOAT> *noise)
{
    if (noise->GetSize() == 0)
        return;
    
    BL_FLOAT prevVal = noise->Get()[0];
    for (int i = 0; i < noise->GetSize(); i++)
    {
        BL_FLOAT val = noise->Get()[i];
        
        if (i < DETECT_PARTIALS_START_INDEX)
        {
            noise->Get()[i] = 0.0;
        }
        else
        {
            if (val > prevVal)
            {
                // We are just after the minimum. Stop.
                break;
            }
            
            noise->Get()[i] = 0.0;
        }
        
        prevVal = val;
    }
}

void
PartialTracker5::SmoothNoiseEnvelope(WDL_TypedBuf<BL_FLOAT> *noise)
{
#define NOISE_SMOOTH_WIN_SIZE 27 //9
    
    // Get a smoothed version of the magns
    if (mSmoothWinNoise.GetSize() != NOISE_SMOOTH_WIN_SIZE)
    {
        //Window::MakeHanning(NOISE_SMOOTH_WIN_SIZE, &mSmoothWinNoise);
        
        // Works well too
        //
        // See: https://en.wikipedia.org/wiki/Window_function
        //
        BL_FLOAT sigma = 0.1;
        Window::MakeGaussian2(sigma, NOISE_SMOOTH_WIN_SIZE, &mSmoothWinNoise);
    }
    
    WDL_TypedBuf<BL_FLOAT> &smoothNoise = mTmpBuf3;
    BLUtils::SmoothDataWin(&smoothNoise, *noise, mSmoothWinNoise);
    
    *noise = smoothNoise;
}

void
PartialTracker5::SmoothNoiseEnvelopeTime(WDL_TypedBuf<BL_FLOAT> *noise)
{
#define NOISE_SMOOTH_TIME_COEFF 0.5
    
    if (mPrevNoiseEnvelope.GetSize() != noise->GetSize())
    {
        mPrevNoiseEnvelope = *noise;
        
        return;
    }
    
    for (int i = 0; i < noise->GetSize(); i++)
    {
        BL_FLOAT val = noise->Get()[i];
        BL_FLOAT prevVal = mPrevNoiseEnvelope.Get()[i];
        
        BL_FLOAT newVal =
            (1.0 - NOISE_SMOOTH_TIME_COEFF)*val + NOISE_SMOOTH_TIME_COEFF*prevVal;
        
        noise->Get()[i] = newVal;
    }
    
    mPrevNoiseEnvelope = *noise;
}

void
PartialTracker5::CutPartials(const vector<Partial> &partials,
                            WDL_TypedBuf<BL_FLOAT> *magns)
{
    // Take each partial, and "cut" it from the full data
    for (int i = 0; i < partials.size(); i++)
    {
        const Partial &partial = partials[i];
        
        int minIdx = partial.mLeftIndex;
        if (minIdx >= magns->GetSize())
            continue;
        
        int maxIdx = partial.mRightIndex;
        if (maxIdx >= magns->GetSize())
            continue;
        
        BL_FLOAT minVal = magns->Get()[minIdx];
        BL_FLOAT maxVal = magns->Get()[maxIdx];
        
        int numIdx = maxIdx - minIdx + 1;
        for (int j = 0; j < numIdx; j++)
        {
            BL_FLOAT t = 0.0;
            if (numIdx > 1)
                t = ((BL_FLOAT)j)/(numIdx - 1);
            
            BL_FLOAT val = (1.0 - t)*minVal + t*maxVal;
            if (minIdx + j >= magns->GetSize())
                continue;
            
            magns->Get()[minIdx + j] = val;
        }
    }
}

void
PartialTracker5::KeepOnlyPartials(const vector<Partial> &partials,
                                  WDL_TypedBuf<BL_FLOAT> *magns)
{
    WDL_TypedBuf<BL_FLOAT> &result = mTmpBuf4;
    //BLUtils::ResizeFillValue(&result, magns->GetSize(), (BL_FLOAT)0.0);
    result.Resize(magns->GetSize());
    BLUtils::FillAllZero(&result);
                   
    for (int i = 0; i < partials.size(); i++)
    {
        const Partial &partial = partials[i];
        
        int minIdx = partial.mLeftIndex;
        if (minIdx >= magns->GetSize())
            continue;
        
        int maxIdx = partial.mRightIndex;
        if (maxIdx >= magns->GetSize())
            continue;
        
        for (int j = minIdx; j <= maxIdx; j++)
        {
            BL_FLOAT val = magns->Get()[j];
            result.Get()[j] = val;
        }
    }
                           
    *magns = result;
}

// Compute the envelope of the minima
void
PartialTracker5::CutPartialsMinEnv(WDL_TypedBuf<BL_FLOAT> *magns)
{
    // Detect the minima
    vector<int> minIndices;
    for (int i = 1; i < magns->GetSize() - 1; i++)
    {
        BL_FLOAT val0 = magns->Get()[i - 1];
        BL_FLOAT val1 = magns->Get()[i];
        BL_FLOAT val2 = magns->Get()[i + 1];
        
        if ((val1 < val0) && (val1 < val2))
        // Minimum detected
        {
            minIndices.push_back(i);
        }
    }

    // Compute envelope
    WDL_TypedBuf<BL_FLOAT> &minEnv = mTmpBuf5;
    //BLUtils::ResizeFillZeros(&minEnv, magns->GetSize());
    minEnv.Resize(magns->GetSize());
    BLUtils::FillAllZero(&minEnv);
    
    for (int i = 0; i < minIndices.size(); i++)
    {
        int idx = minIndices[i];
        BL_FLOAT val = magns->Get()[idx];
        
        minEnv.Get()[idx] = val;
    }
    
    BLUtils::FillMissingValues(&minEnv, false, (BL_FLOAT)0.0);
    
    *magns = minEnv;
}

void
PartialTracker5::FilterPartials()
{    
#if !FILTER_PARTIALS
    mResult = mPartials[0];
#else // Filtered
    FilterPartials(&mResult);
#endif
}

// For noise envelope extraction, the
// state must be ALIVE, and not mWasAlive
bool
PartialTracker5::GetAlivePartials(vector<Partial> *partials)
{
    if (mPartials.empty())
        return false;
    
    partials->clear();
    
    for (int i = 0; i < mPartials[0].size(); i++)
    {
        const Partial &p = mPartials[0][i];
        if (p.mState == Partial::ALIVE)
        {
            partials->push_back(p);
        }
    }
    
    return true;
}

void
PartialTracker5::RemoveRealDeadPartials(vector<Partial> *partials)
{
    vector<Partial> &result = mTmpPartials5;
    result.resize(0);
    
    for (int i = 0; i < partials->size(); i++)
    {
        const Partial &p = (*partials)[i];
        if (p.mWasAlive)
            result.push_back(p);
    }
    
    *partials = result;
}

void
PartialTracker5::GetPartials(vector<Partial> *partials)
{
    *partials = mResult;
    
#if USE_KALMAN_FOR_RESULT
    for (int i = 0; i < partials->size(); i++)
    {
        (*partials)[i].mFreq = (*partials)[i].mPredictedFreq;
    }
#endif
    
    // For sending good result to SASFrame
    RemoveRealDeadPartials(partials);
}

void
PartialTracker5::GetPartialsRAW(vector<Partial> *partials)
{
    *partials = mResult;
}

void
PartialTracker5::ClearResult()
{
    mResult.clear();
}

void
PartialTracker5::GetNoiseEnvelope(WDL_TypedBuf<BL_FLOAT> *noiseEnv)
{
    *noiseEnv = mNoiseEnvelope;
}

void
PartialTracker5::GetHarmonicEnvelope(WDL_TypedBuf<BL_FLOAT> *harmoEnv)
{
    *harmoEnv = mHarmonicEnvelope;
}

void
PartialTracker5::SetMaxDetectFreq(BL_FLOAT maxFreq)
{
    mMaxDetectFreq = maxFreq;
}

// Optimized version (original version removed)
void
PartialTracker5::DetectPartials(const WDL_TypedBuf<BL_FLOAT> &magns,
                                const WDL_TypedBuf<BL_FLOAT> &phases,
                                vector<Partial> *outPartials)
{
    outPartials->clear();
    
    // prevIndex, currentIndex, nextIndex
    
    // Skip the first ones
    // (to avoid artifacts of very low freq partial)
    //int currentIndex = 0;
    int currentIndex = DETECT_PARTIALS_START_INDEX;
    
    BL_FLOAT prevVal = 0.0;
    BL_FLOAT nextVal = 0.0;
    BL_FLOAT currentVal = 0.0;
    
    int maxDetectIndex = magns.GetSize() - 1;
    
    if (mMaxDetectFreq > 0.0)
    {
        maxDetectIndex = mMaxDetectFreq*mBufferSize*0.5;
    }
    
    if (maxDetectIndex > magns.GetSize() - 1)
        maxDetectIndex = magns.GetSize() - 1;
    
    while(currentIndex </*=*/ maxDetectIndex)
    {
        if ((currentVal > prevVal) && (currentVal >= nextVal))
        // Maximum found
        {
            if (currentIndex - 1 >= 0)
            {
                // Take the left and right "feets" of the partial,
                // then the middle.
                // (in order to be more precise)
                
                // Left
                int leftIndex = currentIndex;
                if (leftIndex > 0)
                {
                    BL_FLOAT prevLeftVal = magns.Get()[leftIndex];
                    while(leftIndex > 0)
                    {
                        leftIndex--;
                        
                        BL_FLOAT leftVal = magns.Get()[leftIndex];
                        
                        // Stop if we reach 0 or if it goes up again
                        if ((leftVal < MIN_NORM_AMP) || (leftVal > prevLeftVal))
                        {
                            if (leftVal >= prevLeftVal)
                                leftIndex++;
                            
                            // Check bounds
                            if (leftIndex < 0)
                                leftIndex = 0;
                            
                            if (leftIndex > maxDetectIndex)
                                leftIndex = maxDetectIndex;
                            
                            break;
                        }
                        
                        prevLeftVal = leftVal;
                    }
                }
                
                // Right
                int rightIndex = currentIndex;
                
                if (rightIndex <= maxDetectIndex)
                {
                    BL_FLOAT prevRightVal = magns.Get()[rightIndex];
                    
                    while(rightIndex < maxDetectIndex)
                    {
                        rightIndex++;
                                
                        BL_FLOAT rightVal = magns.Get()[rightIndex];
                                
                        // Stop if we reach 0 or if it goes up again
                        if ((rightVal < MIN_NORM_AMP) || (rightVal > prevRightVal))
                        {
                            if (rightVal >= prevRightVal)
                                rightIndex--;
                                    
                            // Check bounds
                            if (rightIndex < 0)
                                rightIndex = 0;
                                    
                            if (rightIndex > maxDetectIndex)
                                rightIndex = maxDetectIndex;
                                    
                            break;
                        }
                                
                        prevRightVal = rightVal;
                    }
                }
                
                // Take the max (better than taking the middle)
                int peakIndex = currentIndex;
                
                if ((peakIndex < 0) || (peakIndex > maxDetectIndex))
                // Out of bounds
                    continue;
                
                bool discard = false;
    
#if DISCARD_INVALID_PEAKS
                if (!discard)
                {
                    discard = DiscardInvalidPeaks(magns, peakIndex, leftIndex, rightIndex);
                }
#endif
                
                if (!discard)
                {
                    // Create new partial
                    //
                    Partial p;
                    p.mLeftIndex = leftIndex;
                    p.mRightIndex = rightIndex;

                    if (!mComputeAccurateFreqs) // Do not recompute 2 times!
                    {
#if COMPUTE_PEAKS_AVG
                        // Make smooth partials change
                        // But makes wobble in the sound volume
                        BL_FLOAT peakIndexF =
                            ComputePeakIndexAvg(magns, leftIndex, rightIndex);
#endif
#if COMPUTE_PEAKS_PARABOLA
                        // Won't work well with peaks with almost flat top ?
                        //
                        // Make smooth partials change
                        // But makes wobble in the sound volume
                        BL_FLOAT peakIndexF =
                            ComputePeakIndexParabola(magns, peakIndex);
#endif
                    
#if COMPUTE_PEAKS_SIMPLE
                        BL_FLOAT peakIndexF = (leftIndex + rightIndex)*0.5;
#endif

#if COMPUTE_PEAKS_HALF_PROMINENCE_AVG
                        BL_FLOAT peakIndexF =
                            ComputePeakIndexHalfProminenceAvg(magns,
                                                              peakIndex,
                                                              p.mLeftIndex,
                                                              p.mRightIndex);
#endif

                        p.mPeakIndex = bl_round(peakIndexF);
                        if (p.mPeakIndex < 0)
                            p.mPeakIndex = 0;
                    
                        if (p.mPeakIndex > maxDetectIndex)
                            p.mPeakIndex = maxDetectIndex;

                        // Remainder: freq is normalized here
                        BL_FLOAT peakFreq = peakIndexF/(mBufferSize*0.5);
                        p.mFreq = peakFreq;
                    
                        // Kalman
                        //
                        // Update the estimate with the first value
                        //p.mKf.updateEstimate(p.mFreq);
                        p.mKf.initEstimate(p.mFreq);
                    
                        // For predicted freq to be freq for the first value
                        p.mPredictedFreq = p.mFreq;
                        
                        // Default value. Will be overwritten
                        //BL_FLOAT peakAmp = magns.Get()[(int)peakIndexF];
                    
#if !INTERPOLATE_PHASES
                        // Magn
                        p.mAmp = ComputePeakAmpInterp(magns, peakFreq);
                        
                        // Phase
                        p.mPhase = phases.Get()[(int)peakIndexF];
#else
                        ComputePeakMagnPhaseInterp(magns, phases, peakFreq,
                                                   &p.mAmp, &p.mPhase);
#endif
                    } // end mComputeAccurateFreqs
                    
                    outPartials->push_back(p);
                }
                
                // Go just after the right foot of the partial
                currentIndex = rightIndex;
            }
        }
        else
            // No maximum found, continue 1 step
            currentIndex++;
        
        // Update the values
        currentVal = magns.Get()[currentIndex];
        
        if (currentIndex - 1 >= 0)
            prevVal = magns.Get()[currentIndex - 1];
        
        if (currentIndex + 1 <= maxDetectIndex)
            nextVal = magns.Get()[currentIndex + 1];
    }
}

// From GlueTwinPartials()
bool
PartialTracker5::GluePartialBarbs(const WDL_TypedBuf<BL_FLOAT> &magns,
                                  vector<Partial> *partials)
{
    vector<Partial> &result = mTmpPartials6;
    result.resize(0);
    bool glued = false;
    
    sort(partials->begin(), partials->end(), Partial::FreqLess);
    
    int idx = 0;
    while(idx < partials->size())
    {
        Partial currentPartial = (*partials)[idx];
        
        vector<Partial> &twinPartials = mTmpPartials7;
        twinPartials.resize(0);
        
        twinPartials.push_back(currentPartial);
        
        for (int j = idx + 1; j < partials->size(); j++)
        {
            const Partial &otherPartial = (*partials)[j];
            
            if (otherPartial.mLeftIndex == currentPartial.mRightIndex)
            // This is a twin partial...
            {
                BL_FLOAT promCur = ComputePeakProminence(magns,
                                                         currentPartial.mPeakIndex,
                                                         currentPartial.mLeftIndex,
                                                         currentPartial.mRightIndex);
                
                BL_FLOAT promOther = ComputePeakProminence(magns,
                                                         otherPartial.mPeakIndex,
                                                         otherPartial.mLeftIndex,
                                                         otherPartial.mRightIndex);
                
                // Default ratio value
                // If it keeps this value, this is ok, this will be glued
                BL_FLOAT ratio = 0.0;
                if (promOther > BL_EPS)
                {
                    ratio = promCur/promOther;
                    if ((ratio > GLUE_BARBS_AMP_RATIO) || (ratio < 1.0/GLUE_BARBS_AMP_RATIO))
                    // ... with a big amp ratio
                    {
                        // Check that the barb is "in the middle" of a side of the main partial
                        // (in height)
                        bool inTheMiddle = false;
                        bool onTheSide = false;
                        if (promCur < promOther)
                        {
                            BL_FLOAT hf = ComputePeakHigherFoot(magns,
                                                              currentPartial.mLeftIndex,
                                                              currentPartial.mRightIndex);

                            
                            BL_FLOAT lf = ComputePeakLowerFoot(magns,
                                                             otherPartial.mLeftIndex,
                                                             otherPartial.mRightIndex);
                            
                            if ((hf > lf) && (hf < otherPartial.mAmp))
                                inTheMiddle = true;
                            
                            // Check that the barb is on the right side
                            BL_FLOAT otherLeftFoot = magns.Get()[otherPartial.mLeftIndex];
                            BL_FLOAT otherRightFoot = magns.Get()[otherPartial.mRightIndex];
                            if (otherLeftFoot > otherRightFoot)
                                onTheSide = true;
                            
                        }
                        else
                        {
                            BL_FLOAT hf =
                                ComputePeakHigherFoot(magns,
                                                      otherPartial.mLeftIndex,
                                                      otherPartial.mRightIndex);
                            
                            
                            BL_FLOAT lf =
                                ComputePeakLowerFoot(magns,
                                                     currentPartial.mLeftIndex,
                                                     currentPartial.mRightIndex);
                            
                            if ((hf > lf) && (hf < currentPartial.mAmp))
                                inTheMiddle = true;
                            
                            // Check that the barb is on the right side
                            BL_FLOAT curLeftFoot =
                                magns.Get()[currentPartial.mLeftIndex];
                            BL_FLOAT curRightFoot =
                                magns.Get()[currentPartial.mRightIndex];
                            if (curLeftFoot < curRightFoot)
                                onTheSide = true;
                        }
                            
                        if (inTheMiddle && onTheSide)
                            // This tween partial is a barb ! 
                            twinPartials.push_back(otherPartial);
                    }
                }
            }
        }
        
        // Glue ?
        //
        
        if (twinPartials.size() > 1)
        {
            glued = true;
            
            // Compute glued partial
            int leftIndex = twinPartials[0].mLeftIndex;
            int rightIndex = twinPartials[twinPartials.size() - 1].mRightIndex;
            
            BL_FLOAT peakIndex = ComputePeakIndexAvg(magns, leftIndex, rightIndex);
            
            // For peak amp, take max amp
            BL_FLOAT maxAmp = -BL_INF;
            for (int k = 0; k < twinPartials.size(); k++)
            {
                BL_FLOAT amp = twinPartials[k].mAmp;
                if (amp > maxAmp)
                    maxAmp = amp;
            }
            
            Partial res;
            res.mLeftIndex = leftIndex;
            res.mRightIndex = rightIndex;
            
            // Artificial peak
            res.mPeakIndex = peakIndex;
            
            BL_FLOAT peakFreq = peakIndex/(mBufferSize*0.5);
            res.mFreq = peakFreq;
            res.mAmp = maxAmp;
            
            // Kalman
            res.mKf.initEstimate(res.mFreq);
            res.mPredictedFreq = res.mFreq;
            
            // Do not set mPhase for now
            
            result.push_back(res);
        }
        else
            // Not twin, simply add the partial
        {
            result.push_back(twinPartials[0]);
        }
        
        // 1 or more
        idx += twinPartials.size();
    }
    
    *partials = result;
    
    return glued;
}

// Avoid the partial foot to leak on the left and right
// with very small amplitudes
void
PartialTracker5::NarrowPartialFoot(const WDL_TypedBuf<BL_FLOAT> &magns,
                                   int peakIndex,
                                   int *leftIndex, int *rightIndex)
{
    BL_FLOAT peakAmp = magns.Get()[peakIndex];
    
    // We will stop if we are under minAmp
    BL_FLOAT minAmp = peakAmp/NARROW_PARTIAL_FOOT_COEFF;
    
    // Left
    int newLeftIndex = peakIndex;
    while(newLeftIndex > *leftIndex)
    {
        BL_FLOAT ampLeft = magns.Get()[newLeftIndex];
        if (ampLeft <= minAmp)
            break;
        
        newLeftIndex--;
    }
    *leftIndex = newLeftIndex;
    
    // Right
    int newRightIndex = peakIndex;
    while(newRightIndex < *rightIndex)
    {
        BL_FLOAT ampRight = magns.Get()[newRightIndex];
        if (ampRight <= minAmp)
            break;
        
        newRightIndex++;
    }
    *rightIndex = newRightIndex;
}

void
PartialTracker5::NarrowPartialFoot(const WDL_TypedBuf<BL_FLOAT> &magns,
                                   vector<Partial> *partials)
{
    for (int i = 0; i < partials->size(); i++)
    {
        Partial &partial = (*partials)[i];
        
        NarrowPartialFoot(magns, partial.mPeakIndex,
                          &partial.mLeftIndex, &partial.mRightIndex);
    }
}

bool
PartialTracker5::DiscardFlatPartial(const WDL_TypedBuf<BL_FLOAT> &magns,
                                    int peakIndex, int leftIndex, int rightIndex)
{
    BL_FLOAT amp = magns.Get()[peakIndex];
    
    BL_FLOAT binDiff = rightIndex - leftIndex;
    
    BL_FLOAT coeff = binDiff/amp;
    
    bool result = (coeff > DISCARD_FLAT_PARTIAL_COEFF);
    
    return result;
}

void
PartialTracker5::DiscardFlatPartials(const WDL_TypedBuf<BL_FLOAT> &magns,
                                     vector<Partial> *partials)
{
    vector<Partial> &result = mTmpPartials8;
    result.resize(0);
    
    for (int i = 0; i < partials->size(); i++)
    {
        const Partial &partial = (*partials)[i];
            
        bool discard = DiscardFlatPartial(magns,
                                          partial.mPeakIndex,
                                          partial.mLeftIndex,
                                          partial.mRightIndex);
        
        if (!discard)
            result.push_back(partial);
    }
    
    *partials = result;
}

bool
PartialTracker5::DiscardInvalidPeaks(const WDL_TypedBuf<BL_FLOAT> &magns,
                                     int peakIndex, int leftIndex, int rightIndex)
{
    BL_FLOAT peakAmp = magns.Get()[peakIndex];
    BL_FLOAT leftAmp = magns.Get()[leftIndex];
    BL_FLOAT rightAmp = magns.Get()[rightIndex];
    
    if ((peakAmp > leftAmp) && (peakAmp > rightAmp))
        // Correct, do not discard
        return false;
    
    return true;
}

void
PartialTracker5::SuppressZeroFreqPartials(vector<Partial> *partials)
{
    vector<Partial> &result = mTmpPartials9;
    result.resize(0);
    
    for (int i = 0; i < partials->size(); i++)
    {
        const Partial &partial = (*partials)[i];
        
        BL_FLOAT peakFreq = partial.mFreq;
        
        // Zero frequency (because of very small magn) ?
        bool discard = false;
        if (peakFreq < BL_EPS)
            discard = true;
        
        if (!discard)
            result.push_back(partial);
    }
    
    *partials = result;
}

void
PartialTracker5::ThresholdPartialsPeakHeight(vector<Partial> *partials)
{
    vector<Partial> &result = mTmpPartials10;
    result.resize(0);
    
    for (int i = 0; i < partials->size(); i++)
    {
        const Partial &partial = (*partials)[i];
        
        BL_FLOAT height = partial.mPeakHeight;
        
        // Just in case
        if (height < 0.0)
            height = 0.0;
        
        // Threshold
        //
        
        int binNum = partial.mFreq*mBufferSize*0.5;
        BL_FLOAT thrsNorm = GetThreshold(binNum);
        
        if (height >= thrsNorm)
             result.push_back(partial);
    }
    
    *partials = result;
}

// Prominence
BL_FLOAT
PartialTracker5::ComputePeakProminence(const WDL_TypedBuf<BL_FLOAT> &magns,
                                       int peakIndex, int leftIndex, int rightIndex)
{
    // Compute prominence
    //
    // See: https://www.mathworks.com/help/signal/ref/findpeaks.html
    //
    BL_FLOAT maxFootAmp = magns.Get()[leftIndex];
    if (magns.Get()[rightIndex] > maxFootAmp)
        maxFootAmp = magns.Get()[rightIndex];
    
    BL_FLOAT peakAmp = magns.Get()[peakIndex];
    
    BL_FLOAT prominence = peakAmp - maxFootAmp;
    
    return prominence;
}

BL_FLOAT
PartialTracker5::
ComputePeakIndexHalfProminenceAvg(const WDL_TypedBuf<BL_FLOAT> &magns,
                                  int peakIndex, int leftIndex, int rightIndex)
{
    // First step: find float indices corresponding to the half prominence
    //
    // Find float indices, and intermediate interpolated magns, for more accuracy
    //
    BL_FLOAT prominence =
        ComputePeakProminence(magns, peakIndex, leftIndex, rightIndex);

    // Half-prominence threshold
    BL_FLOAT thrs = magns.Get()[peakIndex] - prominence*0.5;
    
    // Left and right float points
    BL_FLOAT LP[2];
    LP[0] = leftIndex;
    LP[1] = magns.Get()[leftIndex];
    
    BL_FLOAT RP[2];
    RP[0] = rightIndex;
    RP[1] = magns.Get()[rightIndex];
    
    // Left
    while(LP[0] < peakIndex)
    {
        if (magns.Get()[(int)LP[0] + 1] > thrs)
        {
            BL_FLOAT m0 = magns.Get()[(int)LP[0]];
            BL_FLOAT m1 = magns.Get()[(int)LP[0] + 1];
            
            BL_FLOAT t = (thrs - m0)/(m1 - m0);
            
            LP[0] = LP[0] + t;
            LP[1] = m0 + t*(m1 - m0);
            
            break;
        }
        
        LP[0]++;
    }
    
    // Right
    while(RP[0] > peakIndex)
    {
        if (magns.Get()[(int)RP[0] - 1] > thrs)
        {
            BL_FLOAT m0 = magns.Get()[(int)RP[0]];
            BL_FLOAT m1 = magns.Get()[(int)RP[0] - 1];
            
            BL_FLOAT t = (thrs - m0)/(m1 - m0);
            
            RP[0] = RP[0] - t;
            RP[1] = m0 + t*(m1 - m0);
            
            break;
        }
        
        RP[0]--;
    }
    
    // Second step: compute the result float peak index (weighted avg)
    //
    // NOTE: Separate first and last float indices from the loop
    //
    BL_FLOAT sumMagns = 0.0;
    BL_FLOAT sumIndices = 0.0;
    
    // First float point
    sumMagns += LP[1];
    sumIndices += LP[0]*LP[1];
    
    // Middle points
    for (int i = (int)(LP[0] + 1); i <= (int)RP[0]; i++)
    {
        BL_FLOAT m = magns.Get()[i];
        
        sumMagns += m;
        sumIndices += i*m;
    }
    
    // Last float point
    sumMagns += RP[1];
    sumIndices += RP[0]*RP[1];
    
    if (sumMagns < BL_EPS)
        return 0.0;
    
    // Result
    BL_FLOAT indexF = sumIndices/sumMagns;
    
    return indexF;
}

// Inverse of prominence
BL_FLOAT
PartialTracker5::ComputePeakHeight(const WDL_TypedBuf<BL_FLOAT> &magns,
                                   int peakIndex, int leftIndex, int rightIndex)
{
    // Compute height
    //
    // See: https://www.mathworks.com/help/signal/ref/findpeaks.html
    //
    BL_FLOAT minFootAmp = magns.Get()[leftIndex];
    if (magns.Get()[rightIndex] < minFootAmp)
        minFootAmp = magns.Get()[rightIndex];
    
    BL_FLOAT peakAmp = magns.Get()[peakIndex];
    
    BL_FLOAT height = peakAmp - minFootAmp;
    
    return height;
}

// Compute difference in amp, then convert back to Db
BL_FLOAT
PartialTracker5::ComputePeakHeightDb(const WDL_TypedBuf<BL_FLOAT> &magns,
                                     int peakIndex, int leftIndex, int rightIndex,
                                     const Partial &partial)
{
    // Compute height
    //
    // See: https://www.mathworks.com/help/signal/ref/findpeaks.html
    //
    
    // Get in Db
    BL_FLOAT minFoot = magns.Get()[leftIndex];
    if (magns.Get()[rightIndex] < minFoot)
        minFoot = magns.Get()[rightIndex];
    
    BL_FLOAT peak = partial.mAmp;
    
    // Compute height
    BL_FLOAT height = peak - minFoot;
    
    return height;
}

BL_FLOAT
PartialTracker5::ComputePeakHigherFoot(const WDL_TypedBuf<BL_FLOAT> &magns,
                                       int leftIndex, int rightIndex)
{
    BL_FLOAT leftVal = magns.Get()[leftIndex];
    BL_FLOAT rightVal = magns.Get()[rightIndex];
    
    if (leftVal > rightVal)
        return leftVal;
    else
        return rightVal;
}

BL_FLOAT
PartialTracker5::ComputePeakLowerFoot(const WDL_TypedBuf<BL_FLOAT> &magns,
                                      int leftIndex, int rightIndex)
{
    BL_FLOAT leftVal = magns.Get()[leftIndex];
    BL_FLOAT rightVal = magns.Get()[rightIndex];
    
    if (leftVal < rightVal)
        return leftVal;
    else
        return rightVal;
}

void
PartialTracker5::ComputePeaksHeights(const WDL_TypedBuf<BL_FLOAT> &magns,
                                    vector<Partial> *partials)
{
    for (int i = 0; i < partials->size(); i++)
    {
        Partial &partial = (*partials)[i];
        
        BL_FLOAT height = ComputePeakHeight(magns,
                                            partial.mPeakIndex,
                                            partial.mLeftIndex,
                                            partial.mRightIndex);
        
        partial.mPeakHeight = height;
    }
}

void
PartialTracker5::SuppressBarbs(vector<Partial> *partials)
{
#define HEIGHT_COEFF 2.0
#define WIDTH_COEFF 1.0
    
    vector<Partial> &result = mTmpPartials11;
    result.resize(0);
    
    for (int i = 0; i < partials->size(); i++)
    {
        const Partial &partial = (*partials)[i];
    
        // Check if the partial is a barb
        bool isBarb = false;
        for (int j = 0; j < partials->size(); j++)
        {
            const Partial &other = (*partials)[j];
            
            if (other.mAmp < partial.mAmp*HEIGHT_COEFF)
                // Amplitudes of tested partial is not small enough
                // compared to the current partial
                // => Not a candidate for barb
                continue;
            
            int center = other.mPeakIndex;
            int size = other.mRightIndex - other.mLeftIndex;
            
            if ((partial.mPeakIndex > center - size*WIDTH_COEFF) &&
                (partial.mPeakIndex < center + size*WIDTH_COEFF))
            {
                // Tested partial peak is "inside" a margin around
                // the current partial.
                // And its amplitude is small compared to the current partial
                // => this is a barb !
                isBarb = true;
                
                break;
            }
        }
        
        // It is not a barb
        if (!isBarb)
            result.push_back(partial);
    }
    
    *partials = result;
}

// Filter
void
PartialTracker5::FilterPartials(vector<Partial> *result)
{
    result->clear();
    
    if (mPartials.empty())
        return;
    
    if (mPartials.size() == 1)
        // Assigne ids to the first series of partials
    {
        for (int j = 0; j < mPartials[0].size(); j++)
        {
            Partial &currentPartial = mPartials[0][j];
            currentPartial.GenNewId();
        }
        
        // Not enough partials to filter, need 2 series
        return;
    }
    
    if (mPartials.size() < 2)
        return;
    
    const vector<Partial> &prevPartials = mPartials[1];
    vector<Partial> &currentPartials = mTmpPartials12;
    currentPartials = mPartials[0];
    
    // Partials that was not associated at the end
    vector<Partial> &remainingPartials = mTmpPartials13;
    remainingPartials.resize(0);
    
    AssociatePartialsPARSHL(prevPartials, &currentPartials, &remainingPartials);
    
    // Add the new zombie and dead partials
    for (int i = 0; i < prevPartials.size(); i++)
    {
        const Partial &prevPartial = prevPartials[i];

        bool found = false;
        for (int j = 0; j < currentPartials.size(); j++)
        {
            const Partial &currentPartial = currentPartials[j];
            
            if (currentPartial.mId == prevPartial.mId)
            {
                found = true;
                
                break;
            }
        }

        if (!found)
        {
            if (prevPartial.mState == Partial::ALIVE)
            {
                // We set zombie for 1 frame only
                Partial newPartial = prevPartial;
                newPartial.mState = Partial::ZOMBIE;
                newPartial.mZombieAge = 0;
                
                // Kalman:
                // GOOD: extrapolate the zombies
                newPartial.mPredictedFreq =
                    newPartial.mKf.updateEstimate(newPartial.mFreq);
                
                currentPartials.push_back(newPartial);
            }
            else if (prevPartial.mState == Partial::ZOMBIE)
            {
                Partial newPartial = prevPartial;
                
                newPartial.mZombieAge++;
                if (newPartial.mZombieAge >= MAX_ZOMBIE_AGE)
                    newPartial.mState = Partial::DEAD;
  
                // Kalman
                // GOOD: extrapolate the zombies
                newPartial.mPredictedFreq =
                    newPartial.mKf.updateEstimate(newPartial.mFreq);

                currentPartials.push_back(newPartial);
            }
            
            // If DEAD, do not add, forget it
        }
    }
    
    // Get the result here
    // So we get the partials that are well tracked over time
    *result = currentPartials;
    
    // At the end, there remains the partial that have not been matched
    //
    // Add them at to the history for next time
    //
    for (int i = 0; i < remainingPartials.size(); i++)
    {
        Partial p = remainingPartials[i];
        
        p.GenNewId();
        
        currentPartials.push_back(p);
    }
    
    // Then sort the new partials by frequency
    sort(currentPartials.begin(), currentPartials.end(), Partial::FreqLess);
    
    //
    // Update: add the partials to the history
    // (except the dead ones)
    mPartials[0].clear();
    for (int i = 0; i < currentPartials.size(); i++)
    {
        const Partial &currentPartial = currentPartials[i];
        
        // TEST: do not skip the dead partials:
        // they will be used for fade out !
        //if (currentPartial.mState != Partial::DEAD)
        mPartials[0].push_back(currentPartial);
    }
}

// Better than "Simple" => do not make jumps between bins
BL_FLOAT
PartialTracker5::ComputePeakIndexAvg(const WDL_TypedBuf<BL_FLOAT> &magns,
                                     int leftIndex, int rightIndex)
{
    // Pow coeff, to select preferably the high amp values
    // With 2.0, makes smoother freq change
    // With 3.0, make just a little smoother than 2.0
#define COEFF 3.0
    
    BL_FLOAT sumIndex = 0.0;
    BL_FLOAT sumMagns = 0.0;
    
    for (int i = leftIndex; i <= rightIndex; i++)
    {
        BL_FLOAT magn = magns.Get()[i];
        
        magn = std::pow(magn, COEFF);
        
        sumIndex += i*magn;
        sumMagns += magn;
    }
    
    if (sumMagns < BL_EPS)
        return 0.0;
    
    BL_FLOAT result = sumIndex/sumMagns;
    
    return result;
}

BL_FLOAT
PartialTracker5::ComputePeakIndexAvgSimple(const WDL_TypedBuf<BL_FLOAT> &magns,
                                           int leftIndex, int rightIndex)
{
    BL_FLOAT sumIndex = 0.0;
    BL_FLOAT sumMagns = 0.0;
    for (int i = leftIndex; i <= rightIndex; i++)
    {
        BL_FLOAT magn = magns.Get()[i];
        
        // IDEA: convert to amp?
        
        sumIndex += i*magn;
        sumMagns += magn;
    }
    
    if (sumMagns < BL_EPS)
        return 0.0;
    
    BL_FLOAT result = sumIndex/sumMagns;
    
    return result;
}

// Parabola peak center detection
// Works well (but I prefer my method) 
//
// See: http://eprints.maynoothuniversity.ie/4523/1/thesis.pdf (p32)
//
// and: https://ccrma.stanford.edu/~jos/parshl/Peak_Detection_Steps_3.html#sec:peakdet
//
BL_FLOAT
PartialTracker5::ComputePeakIndexParabola(const WDL_TypedBuf<BL_FLOAT> &magns,
                                          int peakIndex)
{
    if ((peakIndex - 1 < 0) || (peakIndex + 1 >= magns.GetSize()))
        return peakIndex;
    
    // magns are in DB
    // => no need to convert !
    BL_FLOAT alpha = magns.Get()[peakIndex - 1];
    BL_FLOAT beta = magns.Get()[peakIndex];
    BL_FLOAT gamma = magns.Get()[peakIndex + 1];

    // Will avoid wrong negative result
    if ((beta < alpha) || (beta < gamma))
        return peakIndex;
    
    // Center
    BL_FLOAT denom = (alpha - 2.0*beta + gamma);
    if (std::fabs(denom) < BL_EPS)
        return peakIndex;
    
    BL_FLOAT c = 0.5*((alpha - gamma)/denom);
    
    BL_FLOAT result = peakIndex + c;
    
    return result;
}

// Simple method
BL_FLOAT
PartialTracker5::ComputePeakAmpInterp(const WDL_TypedBuf<BL_FLOAT> &magns,
                                      BL_FLOAT peakFreq)
{
    BL_FLOAT bin = peakFreq*mBufferSize*0.5;
    
    int prevBin = (int)bin;
    int nextBin = (int)bin + 1;
    
    if (nextBin >= magns.GetSize())
    {
        BL_FLOAT peakAmp = magns.Get()[prevBin];
        
        return peakAmp;
    }
    
    BL_FLOAT prevAmp = magns.Get()[prevBin];
    BL_FLOAT nextAmp = magns.Get()[nextBin];
    
    BL_FLOAT t = bin - prevBin;
    
    BL_FLOAT peakAmp = (1.0 - t)*prevAmp + t*nextAmp;
    
    return peakAmp;
}

// VERY GOOD!
// NOTE: use unwraped phases
// NOTE: previouslt tested with complex interpolation => that was bad.
void
PartialTracker5::ComputePeakMagnPhaseInterp(const WDL_TypedBuf<BL_FLOAT> &magns,
                                            const WDL_TypedBuf<BL_FLOAT> &uwPhases,
                                            BL_FLOAT peakFreq,
                                            BL_FLOAT *peakAmp, BL_FLOAT *peakPhase)
{
    // TEST: tested that phases are unwrapped here: ok
    
    BL_FLOAT bin = peakFreq*mBufferSize*0.5;
    
    int prevBin = (int)bin;
    int nextBin = (int)bin + 1;
    
    if (nextBin >= magns.GetSize())
    {
        *peakAmp = magns.Get()[prevBin];
        *peakPhase = uwPhases.Get()[prevBin];
        
        return;
    }
    
    // Interpolate
    BL_FLOAT t = bin - prevBin;
    
    *peakAmp = (1.0 - t)*magns.Get()[prevBin] + t*magns.Get()[nextBin];
    *peakPhase = (1.0 - t)*uwPhases.Get()[prevBin] + t*uwPhases.Get()[nextBin];
}

int
PartialTracker5::FindPartialById(const vector<PartialTracker5::Partial> &partials,
                                 int idx)
{
    for (int i = 0; i < partials.size(); i++)
    {
        const Partial &partial = partials[i];
        
        if (partial.mId == idx)
            return i;
    }
    
    return -1;
}

// Use method similar to SAS
void
PartialTracker5::
AssociatePartials(const vector<PartialTracker5::Partial> &prevPartials,
                  vector<PartialTracker5::Partial> *currentPartials,
                  vector<PartialTracker5::Partial> *remainingPartials)
{
    // Sort current partials and prev partials by decreasing amplitude
    vector<Partial> &currentPartialsSort = mTmpPartials14;
    currentPartialsSort = *currentPartials;
    sort(currentPartialsSort.begin(), currentPartialsSort.end(), Partial::AmpLess);
    reverse(currentPartialsSort.begin(), currentPartialsSort.end());
    
    vector<Partial> &prevPartialsSort = mTmpPartials15;
    prevPartialsSort = prevPartials;
    
    sort(prevPartialsSort.begin(), prevPartialsSort.end(), Partial::AmpLess);
    reverse(prevPartialsSort.begin(), prevPartialsSort.end());
 
    // Associate
    
    // Associated partials
    vector<Partial> &currentPartialsAssoc = mTmpPartials16;
    currentPartialsAssoc.resize(0);
    
    for (int i = 0; i < prevPartialsSort.size(); i++)
    {
        const Partial &prevPartial = prevPartialsSort[i];
        
        for (int j = 0; j < currentPartialsSort.size(); j++)
        {
            Partial &currentPartial = currentPartialsSort[j];
            
            if (currentPartial.mId != -1)
                // Already assigned
                continue;
            
#if USE_KALMAN_FOR_ASSOC
            BL_FLOAT diffFreq = std::fabs(prevPartial.mPredictedFreq - currentPartial.mFreq);
#else
            BL_FLOAT diffFreq = std::fabs(prevPartial.mFreq - currentPartial.mFreq);
#endif
            
            int binNum = currentPartial.mFreq*mBufferSize*0.5;
            BL_FLOAT diffCoeff = GetDeltaFreqCoeff(binNum);
            if (diffFreq < DELTA_FREQ_ASSOC*diffCoeff*mDbgParam)
            // Associated !
            {
                currentPartial.mId = prevPartial.mId;
                currentPartial.mState = Partial::ALIVE;
                currentPartial.mWasAlive = true;
                
                currentPartial.mAge = prevPartial.mAge + 1;
            
                // Kalman
                currentPartial.mKf = prevPartial.mKf;
                currentPartial.mPredictedFreq =
                            currentPartial.mKf.updateEstimate(currentPartial.mFreq);

                currentPartialsAssoc.push_back(currentPartial);
                
                // We have associated to the prev partial
                // We are done!
                // Stop the search here.
                break;
            }
        }
    }
    
    sort(currentPartialsAssoc.begin(), currentPartialsAssoc.end(), Partial::IdLess);
     *currentPartials = currentPartialsAssoc;
    
    // Add the remaining partials
    remainingPartials->clear();
    for (int i = 0; i < currentPartialsSort.size(); i++)
    {
        const Partial &p = currentPartialsSort[i];
        if (p.mId == -1)
            remainingPartials->push_back(p);
    }
}

// Use PARSHL method
void
PartialTracker5::
AssociatePartialsPARSHL(const vector<PartialTracker5::Partial> &prevPartials,
                        vector<PartialTracker5::Partial> *currentPartials,
                        vector<PartialTracker5::Partial> *remainingPartials)
{
    // Sort current partials and prev partials by increasing frequency
    sort(currentPartials->begin(), currentPartials->end(), Partial::FreqLess);
    
    vector<PartialTracker5::Partial> &prevPartials0 = mTmpPartials17;
    prevPartials0 = prevPartials;
    sort(prevPartials0.begin(), prevPartials0.end(), Partial::FreqLess);
    
    // Associated partials
    bool stopFlag = true;
    do {
        stopFlag = true;
        
        for (int i = 0; i < prevPartials0.size(); i++)
        {
            const Partial &prevPartial = prevPartials0[i];
            for (int j = 0; j < currentPartials->size(); j++)
            {
                Partial &currentPartial = (*currentPartials)[j];
                if (currentPartial.mId != -1)
                    // Already associated, nothing to do on this step!
                    continue;
                
#if USE_KALMAN_FOR_ASSOC
                BL_FLOAT diffFreq =
                    std::fabs(prevPartial.mPredictedFreq - currentPartial.mFreq);
#else
                BL_FLOAT diffFreq =
                    std::fabs(prevPartial.mFreq - currentPartial.mFreq);
#endif
                int binNum = currentPartial.mFreq*mBufferSize*0.5;
                BL_FLOAT diffCoeff = GetDeltaFreqCoeff(binNum);
            
                if (diffFreq < DELTA_FREQ_ASSOC*diffCoeff*mDbgParam)
                    // Associate!
                {
                    int otherIdx =
                        FindPartialById(*currentPartials, (int)prevPartial.mId);
                    
                    if (otherIdx == -1)
                        // This partial is not yet associated
                        // => No fight
                    {
                        currentPartial.mId = prevPartial.mId;
                        currentPartial.mAge = prevPartial.mAge;
                        currentPartial.mKf = prevPartial.mKf; //
                        
                        stopFlag = false;
                    }
                    else // Fight!
                    {
                        Partial &otherPartial = (*currentPartials)[otherIdx];
                        
#if USE_KALMAN_FOR_ASSOC
                        BL_FLOAT otherDiffFreq =
                                std::fabs(prevPartial.mPredictedFreq - otherPartial.mFreq);
#else
                        BL_FLOAT otherDiffFreq =
                            std::fabs(prevPartial.mFreq - otherPartial.mFreq);
#endif
                        
                        if (diffFreq < otherDiffFreq)
                        // Current partial won
                        {
                            currentPartial.mId = prevPartial.mId;
                            currentPartial.mAge = prevPartial.mAge;
                            currentPartial.mKf = prevPartial.mKf; //
                            
                            // Detach the other
                            otherPartial.mId = -1;
                            
                            stopFlag = false;
                        }
                        else
                        // Other partial won
                        {
                            // Just keep it like it is!
                        }
                    }
                }
            }
        }
    } while (!stopFlag);
    
    
    // Update partials
    vector<PartialTracker5::Partial> &newPartials = mTmpPartials18;
    newPartials.resize(0);
    
    for (int j = 0; j < currentPartials->size(); j++)
    {
        Partial &currentPartial = (*currentPartials)[j];
        
        if (currentPartial.mId != -1)
        {
            currentPartial.mState = Partial::ALIVE;
            currentPartial.mWasAlive = true;
    
            // Increment age
            currentPartial.mAge = currentPartial.mAge + 1;
            currentPartial.mPredictedFreq =
                    currentPartial.mKf.updateEstimate(currentPartial.mFreq);
    
            newPartials.push_back(currentPartial);
        }
    }
    
    // Add the remaining partials
    remainingPartials->clear();
    for (int i = 0; i < currentPartials->size(); i++)
    {
        const Partial &p = (*currentPartials)[i];
        if (p.mId == -1)
            remainingPartials->push_back(p);
    }
    
    // Update current partials
    *currentPartials = newPartials;
}

void
PartialTracker5::DBG_DumpPartials(const char *fileName,
                                  const vector<Partial> &partials,
                                  int bufferSize)
{
    WDL_TypedBuf<BL_FLOAT> result;
    result.Resize(bufferSize);
    BLUtils::FillAllZero(&result);
    
    for (int i = 0; i < partials.size(); i++)
    {
        const PartialTracker5::Partial &p = partials[i];
        
        // Width
        for (int j = p.mLeftIndex; j <= p.mRightIndex; j++)
        {
            result.Get()[j] = p.mAmp*0.5;
        }
        
        // Peak
        result.Get()[p.mPeakIndex] = p.mAmp;
    }
    
    BLDebug::DumpData(fileName, result);
}

void
PartialTracker5::DBG_SetDbgParam(BL_FLOAT param)
{
    mDbgParam = param;
}

BL_FLOAT
PartialTracker5::GetThreshold(int binNum)
{
    BL_FLOAT thrsNorm = -(MIN_AMP_DB - mThreshold)/(-MIN_AMP_DB);
    
    return thrsNorm;
}

BL_FLOAT
PartialTracker5::GetDeltaFreqCoeff(int binNum)
{
#define END_COEFF 0.25
    
    BL_FLOAT t = ((BL_FLOAT)binNum)/(mBufferSize*0.5);
    BL_FLOAT diffCoeff = 1.0 - (1.0 - END_COEFF)*t;
    
    return diffCoeff;
}

void
PartialTracker5::PreProcessDataX(WDL_TypedBuf<BL_FLOAT> *data)
{
#if !USE_FILTER_BANKS
    // ORIGIN scale: use MelScale internally
    // X
    mScale->ApplyScale(mXScale, data, (BL_FLOAT)0.0, (BL_FLOAT)(mSampleRate*0.5));
#else
    // NEW version; use FilterBank internally (avoid stairs effect)
    WDL_TypedBuf<BL_FLOAT> &scaledData = mTmpBuf8;
    Scale::FilterBankType type = mScale->TypeToFilterBankType(mXScale);
    mScale->ApplyScaleFilterBank(type, &scaledData, *data,
                                 mSampleRate, data->GetSize());
    *data = scaledData;
#endif
}

void
PartialTracker5::PreProcessDataY(WDL_TypedBuf<BL_FLOAT> *data)
{
    // Y
#if 0 // one by one
    for (int i = 0; i < data->GetSize(); i++)
    {
        BL_FLOAT d = data->Get()[i];
        d = mScale->ApplyScale(mYScale, d, (BL_FLOAT)MIN_AMP_DB, (BL_FLOAT)0.0);
        data->Get()[i] = d;
    }
#endif
#if 1 // OPTIM; in block
    mScale->ApplyScaleForEach(mYScale, data, (BL_FLOAT)MIN_AMP_DB, (BL_FLOAT)0.0);
#endif
    
    // Better tracking on high frequencies with this!
    PreProcessAWeighting(data, true);
}

void
PartialTracker5::PreProcessDataXY(WDL_TypedBuf<BL_FLOAT> *data)
{
    // Origin: process Y first
    PreProcessDataY(data);
    PreProcessDataX(data);
}
    
// Unwrap phase before converting to mel => more correct!
void
PartialTracker5::PreProcess(WDL_TypedBuf<BL_FLOAT> *magns,
                            WDL_TypedBuf<BL_FLOAT> *phases)
{
    // ORIGIN: smooth only magns
    // NOTE: tested smooting on complex => gave more noisy result
    PreProcessTimeSmooth(magns);

    // Use time smooth on raw magns too
    // (time smoothed, but linearly scaled)
    mLinearMagns = *magns;
    PreProcessDataY(&mLinearMagns); // We want raw data in dB (just keep linear on x)
    
#if SQUARE_MAGNS
    BLUtils::ComputeSquare(magns);
#endif
    
    PreProcessDataXY(magns);
    
#if MEL_UNWRAP_PHASES
    BLUtilsPhases::UnwrapPhases(phases);
#endif

    // Phases

    // ORIGIN version, use MelScale internally
    //mScale->ApplyScale(mXScale, phases, (BL_FLOAT)0.0, (BL_FLOAT)(mSampleRate*0.5));

    WDL_TypedBuf<BL_FLOAT> &scaledPhases = mTmpBuf9;
    Scale::FilterBankType type = mScale->TypeToFilterBankType(mXScale);
    mScale->ApplyScaleFilterBank(type, &scaledPhases, *phases,
                                 mSampleRate, phases->GetSize());
    *phases = scaledPhases;
    
#if MEL_UNWRAP_PHASES
    // NOTE: commented, because we will need unwrapped phases later!
    // (in ComputePeakMagnPhaseInterp())
    
    // With or without this line, we get the same result
    //BLUtils::MapToPi(phases);
#endif
}

void
PartialTracker5::SetTimeSmoothCoeff(BL_FLOAT coeff)
{
    mTimeSmoothCoeff = coeff;
}

void
PartialTracker5::SetTimeSmoothNoiseCoeff(BL_FLOAT coeff)
{
    mTimeSmoothNoiseCoeff = coeff;
}

// Time smooth
void
PartialTracker5::PreProcessTimeSmooth(WDL_TypedBuf<BL_FLOAT> *magns)
{
    if (mTimeSmoothPrevMagns.GetSize() == 0)
    {
        mTimeSmoothPrevMagns = *magns;
        
        return;
    }
    
    for (int i = 0; i < magns->GetSize(); i++)
    {
        BL_FLOAT val = magns->Get()[i];
        BL_FLOAT prevVal = mTimeSmoothPrevMagns.Get()[i];
        
        BL_FLOAT newVal = (1.0 - mTimeSmoothCoeff)*val + mTimeSmoothCoeff*prevVal;
        
        magns->Get()[i] = newVal;
    }
    
    mTimeSmoothPrevMagns = *magns;
}

// Time smooth noise
void
PartialTracker5::TimeSmoothNoise(WDL_TypedBuf<BL_FLOAT> *noise)
{
    if (mTimeSmoothPrevNoise.GetSize() == 0)
    {
        mTimeSmoothPrevNoise = *noise;
        
        return;
    }
    
    for (int i = 0; i < noise->GetSize(); i++)
    {
        BL_FLOAT val = noise->Get()[i];
        BL_FLOAT prevVal = mTimeSmoothPrevNoise.Get()[i];
        
        BL_FLOAT newVal = (1.0 - mTimeSmoothNoiseCoeff)*val + mTimeSmoothNoiseCoeff*prevVal;
        
        noise->Get()[i] = newVal;
    }
    
    mTimeSmoothPrevNoise = *noise;
}

void
PartialTracker5::DenormPartials(vector<PartialTracker5::Partial> *partials)
{
    BL_FLOAT hzPerBin =  mSampleRate/mBufferSize;
    
    for (int i = 0; i < partials->size(); i++)
    {
        PartialTracker5::Partial &partial = (*partials)[i];
        
        // Reverse Mel
        BL_FLOAT freq = partial.mFreq;
        freq = mScale->ApplyScale(mXScaleInv, freq, (BL_FLOAT)0.0,
                                  (BL_FLOAT)(mSampleRate*0.5));
        partial.mFreq = freq;
        
        // Convert to real freqs
        partial.mFreq *= mSampleRate*0.5;
        
        // Reverse AWeighting
        int binNum = partial.mFreq/hzPerBin;
        partial.mAmp = ProcessAWeighting(binNum, mBufferSize*0.5,
                                         partial.mAmp, false);
    
        // Y
        partial.mAmp = mScale->ApplyScale(mYScaleInv, partial.mAmp,
                                          (BL_FLOAT)MIN_AMP_DB, (BL_FLOAT)0.0);

#if DENORM_PARTIAL_INDICES
        partial.mLeftIndex = DenormBinIndex(partial.mLeftIndex);
        partial.mPeakIndex = DenormBinIndex(partial.mPeakIndex);
        partial.mRightIndex = DenormBinIndex(partial.mRightIndex);
#endif
    }
}

void
PartialTracker5::DenormData(WDL_TypedBuf<BL_FLOAT> *data)
{
#if !USE_FILTER_BANKS
    // ORIGIN version: use MelFilter internally
    // X
    mScale->ApplyScale(mXScaleInv, data, (BL_FLOAT)0.0, (BL_FLOAT)(mSampleRate*0.5));
#else
    // NEW version; use FilterBank internally (avoid stairs effect)
    WDL_TypedBuf<BL_FLOAT> &scaledData = mTmpBuf7;
    Scale::FilterBankType type = mScale->TypeToFilterBankType(mXScale);
    mScale->ApplyScaleFilterBankInv(type, &scaledData, *data,
                                    mSampleRate, data->GetSize());
    *data = scaledData;
#endif
    
    // A-Weighting
    PreProcessAWeighting(data, false);
    
    // Y
#if 0 // one by one
    for (int i = 0; i < data->GetSize(); i++)
    {
        BL_FLOAT d = data->Get()[i];
        d = mScale->ApplyScale(mYScaleInv, d, (BL_FLOAT)MIN_AMP_DB, (BL_FLOAT)0.0);
        data->Get()[i] = d;
    }
#endif
#if 1 // OPTIM: process by block
    mScale->ApplyScaleForEach(mYScaleInv, data, (BL_FLOAT)MIN_AMP_DB, (BL_FLOAT)0.0);
#endif
}

void
PartialTracker5::PartialsAmpToAmpDB(vector<PartialTracker5::Partial> *partials)
{
    for (int i = 0; i < partials->size(); i++)
    {
        PartialTracker5::Partial &partial = (*partials)[i];
        
        partial.mAmpDB = BLUtils::AmpToDB(partial.mAmp);
    }
}

void
PartialTracker5::PreProcessAWeighting(WDL_TypedBuf<BL_FLOAT> *magns,
                                      bool reverse)
{
    // Input magns are in normalized dB
    
    WDL_TypedBuf<BL_FLOAT> &weights = mTmpBuf6;
    //int numBins = magns->GetSize();
    //AWeighting::ComputeAWeights(&weights, numBins, mSampleRate);
    weights = mAWeights;
    
    BL_FLOAT hzPerBin = 0.5*mSampleRate/magns->GetSize();
    
    // W-Weighting property: 0dB at 1000Hz!
    BL_FLOAT zeroDbFreq = 1000.0;
    int zeroDbBin = zeroDbFreq/hzPerBin;
    
    for (int i = zeroDbBin; i < magns->GetSize(); i++)
    {
        BL_FLOAT a = weights.Get()[i];
        
        BL_FLOAT normDbMagn = magns->Get()[i];
        BL_FLOAT dbMagn = (1.0 - normDbMagn)*MIN_AMP_DB;
        
        if (reverse)
            dbMagn -= a;
        else
            dbMagn += a;
        
        normDbMagn = 1.0 - dbMagn/MIN_AMP_DB;
        
        if (normDbMagn < 0.0)
            normDbMagn = 0.0;
        if (normDbMagn > 1.0)
            normDbMagn = 1.0;
        
        magns->Get()[i] = normDbMagn;
    }
}

BL_FLOAT
PartialTracker5::ProcessAWeighting(int binNum, int numBins,
                                   BL_FLOAT magn, bool reverse)
{
    // Input magn is in normalized dB
    
    BL_FLOAT hzPerBin = 0.5*mSampleRate/numBins;
    
    // W-Weighting property: 0dB at 1000Hz!
    BL_FLOAT zeroDbFreq = 1000.0;
    int zeroDbBin = zeroDbFreq/hzPerBin;
    
    if (binNum <= zeroDbBin)
        // Do nothing
        return magn;
    
    //BL_FLOAT a = AWeighting::ComputeAWeight(binNum, numBins, mSampleRate);
    BL_FLOAT a = mAWeights.Get()[binNum];
    
    BL_FLOAT normDbMagn = magn;
    BL_FLOAT dbMagn = (1.0 - normDbMagn)*MIN_AMP_DB;
        
    if (reverse)
        dbMagn -= a;
    else
        dbMagn += a;
        
    normDbMagn = 1.0 - dbMagn/MIN_AMP_DB;
        
    if (normDbMagn < 0.0)
        normDbMagn = 0.0;
    if (normDbMagn > 1.0)
        normDbMagn = 1.0;
        
    return normDbMagn;
}

// Optim: pre-compute a weights
void
PartialTracker5::ComputeAWeights(int numBins, BL_FLOAT sampleRate)
{
    AWeighting::ComputeAWeights(&mAWeights, numBins, sampleRate);
}

void
PartialTracker5::ReserveTmpBufs()
{
    // Doesn't seem to improve.
    // Maybe the defaut behavior of std::vector() is sufficient
    // to manage well our case.
    //
    // So disable
    return;
    
    int reserveSize = mBufferSize/2;
    
    mTmpPartials0.reserve(reserveSize);
    mTmpPartials1.reserve(reserveSize);
    mTmpPartials2.reserve(reserveSize);
    mTmpPartials3.reserve(reserveSize);
    mTmpPartials4.reserve(reserveSize);
    mTmpPartials5.reserve(reserveSize);
    mTmpPartials6.reserve(reserveSize);
    mTmpPartials7.reserve(reserveSize);
    mTmpPartials8.reserve(reserveSize);
    mTmpPartials9.reserve(reserveSize);
    mTmpPartials10.reserve(reserveSize);
    mTmpPartials11.reserve(reserveSize);
    mTmpPartials12.reserve(reserveSize);
    mTmpPartials13.reserve(reserveSize);
    mTmpPartials14.reserve(reserveSize);
    mTmpPartials15.reserve(reserveSize);
    mTmpPartials16.reserve(reserveSize);
    mTmpPartials17.reserve(reserveSize);
    mTmpPartials18.reserve(reserveSize);
}

int
PartialTracker5::DenormBinIndex(int idx)
{
    BL_FLOAT freq = ((BL_FLOAT)idx)/(mBufferSize*0.5);
    freq = mScale->ApplyScale(mXScaleInv, freq, (BL_FLOAT)0.0,
                              (BL_FLOAT)(mSampleRate*0.5));

    BL_FLOAT res = freq*(mBufferSize*0.5);

    int resi = bl_round(res);
    if (resi < 0)
        resi = 0;
    if (resi > mBufferSize*0.5 - 1)
        resi = mBufferSize*0.5 - 1;

    return resi;
}

// NOTE: freq/index conversions should use (bufferSize*0.5 - 1) as a factor...
void
PartialTracker5::ComputeAccurateFreqs(vector<Partial> *partials)
{    
    // The most accurate frequencies are acheived using linear scale on x,
    // db scale on y, and parabola peak finding,
    // (and with and offset of 0.5 to get centered on bins) <- mistake ?

    // With this method, we get an accuracy of less than 1Hz!
    
    // Compute the frequencies using this most accurate method
    for (int i = 0; i < partials->size(); i++)
    {
        Partial &p = (*partials)[i];

        // First, find the left and right indices, scaled to linear
        BL_FLOAT leftIndexF = ((BL_FLOAT)p.mLeftIndex)/(mBufferSize*0.5);
        BL_FLOAT rightIndexF = ((BL_FLOAT)p.mRightIndex)/(mBufferSize*0.5);

        leftIndexF = mScale->ApplyScale(mXScaleInv, leftIndexF, (BL_FLOAT)0.0,
                                        (BL_FLOAT)(mSampleRate*0.5));
        rightIndexF = mScale->ApplyScale(mXScaleInv, rightIndexF, (BL_FLOAT)0.0,
                                         (BL_FLOAT)(mSampleRate*0.5));

        BL_FLOAT leftIndex0 = leftIndexF*(mBufferSize*0.5);
        BL_FLOAT rightIndex0 = rightIndexF*(mBufferSize*0.5);

        // Necessary to round(), otherwise we have the risk to have several peaks in
        // the range [leftIndex, leftIndex] (due to inaccurate L/R bounds)
        int leftIndex = bl_round(leftIndex0);
        int rightIndex = bl_round(rightIndex0);
        
        // Then find the integer peak index, still in linear scale
        // Use the raw magns we previously kept (possibly time smoothed) 
        int peakIndex = BLUtils::FindMaxIndex(mLinearMagns, leftIndex, rightIndex);

        // Won't work well with peaks with almost flat top ?
        //
        // Make smooth partials change
        // But makes wobble in the sound volume
        BL_FLOAT peakIndexF = ComputePeakIndexParabola(mLinearMagns, peakIndex);
        
        // NOTE: this seemed to be a mistake, fixed here by a hack
        //
        // Then adjust to be centered on the bins (the magic comes here :)
        // => this way we get the right result
        //peakIndexF = peakIndexF + 0.5;

        // Update the partial peak index (just in case)
        p.mPeakIndex = bl_round(peakIndexF);
        
        // Get the normalized frequency (linear scale)
        BL_FLOAT peakFreq = peakIndexF/(mBufferSize*0.5);
                        
        // Rescale it to the current scale
        peakFreq = mScale->ApplyScale(mXScale, peakFreq, (BL_FLOAT)0.0,
                                     (BL_FLOAT)(mSampleRate*0.5));

        // And finally, update the partial
        p.mFreq = peakFreq;

        // Some updates
        //

        // NOTE: not sure this computation is very exact...
        //
        // Update the partial peak index (just in case)
        BL_FLOAT newPeakIndex = peakFreq*(mBufferSize*0.5);
        p.mPeakIndex = bl_round(newPeakIndex);
        if (p.mPeakIndex < 0)
            p.mPeakIndex = 0;
        
        int maxDetectIndex = mCurrentMagns.GetSize();
        if (mMaxDetectFreq > 0.0)
            maxDetectIndex = mMaxDetectFreq*(mBufferSize*0.5);
        if (maxDetectIndex > mCurrentMagns.GetSize() - 1)
            maxDetectIndex = mCurrentMagns.GetSize() - 1;
        if (p.mPeakIndex > maxDetectIndex)
            p.mPeakIndex = maxDetectIndex;
    
        // Kalman
        
        // Update the estimate with the first value
        //p.mKf.updateEstimate(p.mFreq);
        p.mKf.initEstimate(p.mFreq);
                    
        // For predicted freq to be freq for the first value
        p.mPredictedFreq = p.mFreq;

        // (Re)compute amp more accurately
#if !INTERPOLATE_PHASES
        // Magn
        p.mAmp = ComputePeakAmpInterp(mCurrentMagns, peakFreq);
                    
        // Phase
        p.mPhase = mCurrentPhases.Get()[(int)peakIndexF];
#else
        ComputePeakMagnPhaseInterp(mCurrentMagns, mCurrentPhases, peakFreq,
                                   &p.mAmp, &p.mPhase);
#endif
    }
}
