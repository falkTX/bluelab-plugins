//
//  PartialTracker6.cpp
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

#include <PartialFilterMarchand.h>
#include <PartialFilterAMFM.h>

#include <BLUtils.h>
#include <BLUtilsPhases.h>
#include <BLUtilsMath.h>

#include <BLDebug.h>

#include <PeakDetectorBL.h>
#include <PeakDetectorBillauer.h>

#include <QIFFT.h>
#include <PhasesUnwrapper.h>

#include "PartialTracker6.h"

#define MIN_AMP_DB -120.0
#define MIN_NORM_AMP 1e-15

// As described in: https://www.dsprelated.com/freebooks/sasp/PARSHL_Program.html#app:parshlapp
#define SQUARE_MAGNS 0 //1


// Detect partials
//
#if USE_BL_PEAK_DETECTOR
#define DETECT_PARTIALS_START_INDEX 2
#else
#define DETECT_PARTIALS_START_INDEX 1
#endif

#define DISCARD_FLAT_PARTIAL 1
#define DISCARD_FLAT_PARTIAL_COEFF 25000.0 //30000.0

//#define DISCARD_INVALID_PEAKS 1

//
#define GLUE_BARBS              1 //0
#define GLUE_BARBS_AMP_RATIO    10.0 //4.0

// Filter
//

// Do we filter ?
#define FILTER_PARTIALS 1

// GOOD: avoid many small zig-zags on the result lines
//#define USE_KALMAN_FOR_RESULT 1

#define PROCESS_MUS_NOISE      1 // Working (but dos not remove all mus noise)
// 4 seems better than 2
#define HISTORY_SIZE_MUS_NOISE 4 //2 //4

// NOTE: See: https://www.dsprelated.com/freebooks/sasp/Spectral_Modeling_Synthesis.html
//
// and: https://www.dsprelated.com/freebooks/sasp/PARSHL_Program.html#app:parshlapp
//
// and: https://ccrma.stanford.edu/~jos/parshl/

// Get the precision when interpolating peak magns, but also for phases
#define INTERPOLATE_PHASES 1

// Better mel filtering of phase if they are unwrapped!
#define MEL_UNWRAP_PHASES 1

#define USE_FILTER_BANKS 1

// NEW
#define DENORM_PARTIAL_INDICES 1

#define USE_QIFFT 1

// It is better with log then just with dB!
#define USE_QIFFT_YLOG 1

PartialTracker6::PartialTracker6(int bufferSize, BL_FLOAT sampleRate,
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
    
    mXScale = Scale::LINEAR;
    //mXScale = Scale::MEL;
    //mXScale = Scale::MEL_FILTER;
    
    //mYScale = Scale::LINEAR;
    mYScale = Scale::DB;

#if USE_QIFFT_YLOG
    mYScale2 = Scale::LOG_NO_NORM;
#endif
    
    //
    mXScaleInv = Scale::LINEAR;
    //mXScaleInv = Scale::MEL_INV;
    //mXScaleInv = Scale::MEL_FILTER_INV;

    mYScaleInv = Scale::DB_INV;
#if USE_QIFFT_YLOG
    mYScaleInv2 = Scale::LOG_NO_NORM_INV;
#endif
    
    mTimeSmoothCoeff = 0.5;
    mTimeSmoothNoiseCoeff = 0.5;
    
    // Optim
    ComputeAWeights(bufferSize/2, sampleRate);

#if USE_BL_PEAK_DETECTOR
    mPeakDetector = new PeakDetectorBL();
#endif
#if USE_BILLAUER_PEAK_DETECTOR
    BL_FLOAT maxDelta = (USE_QIFFT_YLOG == 0) ? 1.0 : -MIN_AMP_DB/4;
    mPeakDetector = new PeakDetectorBillauer(maxDelta);
#endif

#if USE_PARTIAL_FILTER_MARCHAND
    mPartialFilter = new PartialFilterMarchand(bufferSize, sampleRate);
#endif
#if USE_PARTIAL_FILTER_AMFM
    mPartialFilter = new PartialFilterAMFM(bufferSize, sampleRate);
#endif
}

PartialTracker6::~PartialTracker6()
{
    delete mScale;
    delete mPeakDetector;
    delete mPartialFilter;
}

void
PartialTracker6::Reset()
{
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

    if (mPartialFilter != NULL)
        mPartialFilter->Reset(mBufferSize, mSampleRate);
}

void
PartialTracker6::Reset(int bufferSize, BL_FLOAT sampleRate)
{
    mBufferSize = bufferSize;
    mSampleRate = sampleRate;
    
    Reset();

    // Optim
    ComputeAWeights(bufferSize/2, sampleRate);
}

BL_FLOAT
PartialTracker6::GetMinAmpDB()
{
    return MIN_AMP_DB;
}

void
PartialTracker6::SetThreshold(BL_FLOAT threshold)
{
    mThreshold = threshold;

    mPeakDetector->SetThreshold(threshold);
}

void
PartialTracker6::SetData(const WDL_TypedBuf<BL_FLOAT> &magns,
                         const WDL_TypedBuf<BL_FLOAT> &phases)
{
    mCurrentMagns = magns;
    mCurrentPhases = phases;

    // Not smoothed (will be overwritten later)
    //mLinearMagns = magns;
    
    PreProcess(&mCurrentMagns, &mCurrentPhases);
}

void
PartialTracker6::GetPreProcessedMagns(WDL_TypedBuf<BL_FLOAT> *magns)
{
    *magns = mCurrentMagns;
}

void
PartialTracker6::DetectPartials()
{
    WDL_TypedBuf<BL_FLOAT> &magns0 = mTmpBuf0;
    magns0 = mCurrentMagns;
    
    vector<Partial> &partials = mTmpPartials0;
    partials.resize(0);
    DetectPartials(magns0, mCurrentPhases, &partials);

#if USE_BL_PEAK_DETECTOR
    // For first partial detection
    PostProcessPartials(magns0, &partials);
#endif
    
    mResult = partials;
}

void
PartialTracker6::ExtractNoiseEnvelope()
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
    
#if PROCESS_MUS_NOISE
    // Works well, but do not remove all musical noise
    ProcessMusicalNoise(&mNoiseEnvelope);
#endif
    
    // Create an envelope
    // NOTE: good for "oohoo", not good for "alphabet A"
    BLUtils::FillMissingValues2(&mNoiseEnvelope, false, (BL_FLOAT)0.0);
    
    //
    TimeSmoothNoise(&mNoiseEnvelope);
}

// Supress musical noise in the raw noise (not filled)
void
PartialTracker6::ProcessMusicalNoise(WDL_TypedBuf<BL_FLOAT> *noise)
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
PartialTracker6::SmoothNoiseEnvelope(WDL_TypedBuf<BL_FLOAT> *noise)
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
        Window::MakeGaussian2(NOISE_SMOOTH_WIN_SIZE, sigma, &mSmoothWinNoise);
    }
    
    WDL_TypedBuf<BL_FLOAT> &smoothNoise = mTmpBuf3;
    BLUtils::SmoothDataWin(&smoothNoise, *noise, mSmoothWinNoise);
    
    *noise = smoothNoise;
}

void
PartialTracker6::KeepOnlyPartials(const vector<Partial> &partials,
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

void
PartialTracker6::FilterPartials()
{    
#if FILTER_PARTIALS
    //FilterPartials(&mResult);

#if USE_PARTIAL_FILTER_AMFM
    // Adjust the scale
    for (int i = 0; i < mResult.size(); i++)
    {
        Partial &p = mResult[i];

        BL_FLOAT amp =
            mScale->ApplyScale(mYScaleInv, p.mAmp,
                               (BL_FLOAT)MIN_AMP_DB, (BL_FLOAT)0.0);
        
        BL_FLOAT ampNorm =
            mScale->ApplyScale(mYScale2, amp,
                               (BL_FLOAT)MIN_AMP_DB, (BL_FLOAT)0.0);
        p.mAmp = ampNorm;
    }
#endif
    
    mPartialFilter->FilterPartials(&mResult);

#if USE_PARTIAL_FILTER_AMFM
    // Adjust the scale
    for (int i = 0; i < mResult.size(); i++)
    {
        Partial &p = mResult[i];
        BL_FLOAT ampNorm =
            mScale->ApplyScale(mYScaleInv2, p.mAmp,
                               (BL_FLOAT)MIN_AMP_DB, (BL_FLOAT)0.0);
        BL_FLOAT ampDbNorm =
            mScale->ApplyScale(mYScale, ampNorm,
                               (BL_FLOAT)MIN_AMP_DB, (BL_FLOAT)0.0);
        p.mAmp = ampDbNorm;
    }
#endif
    
#endif
}

// For noise envelope extraction, the
// state must be ALIVE, and not mWasAlive
bool
PartialTracker6::GetAlivePartials(vector<Partial> *partials)
{
    //if (mPartials.empty())
    if (mResult.empty())
        return false;
    
    partials->clear();
    
    for (int i = 0; i < mResult.size(); i++)
    {
        const Partial &p = mResult[i];
        if (p.mState == Partial::ALIVE)
        {
            partials->push_back(p);
        }
    }
    
    return true;
}

void
PartialTracker6::RemoveRealDeadPartials(vector<Partial> *partials)
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
PartialTracker6::GetPartials(vector<Partial> *partials)
{
    *partials = mResult;
    
    /*#if USE_KALMAN_FOR_RESULT
      for (int i = 0; i < partials->size(); i++)
      {
      (*partials)[i].mFreq = (*partials)[i].mPredictedFreq;
      }
      #endif*/
    
    // For sending good result to SASFrame
    RemoveRealDeadPartials(partials);
}

void
PartialTracker6::GetPartialsRAW(vector<Partial> *partials)
{
    *partials = mResult;
}

void
PartialTracker6::ClearResult()
{
    mResult.clear();
}

void
PartialTracker6::GetNoiseEnvelope(WDL_TypedBuf<BL_FLOAT> *noiseEnv)
{
    *noiseEnv = mNoiseEnvelope;
}

void
PartialTracker6::GetHarmonicEnvelope(WDL_TypedBuf<BL_FLOAT> *harmoEnv)
{
    *harmoEnv = mHarmonicEnvelope;
}

void
PartialTracker6::SetMaxDetectFreq(BL_FLOAT maxFreq)
{
    mMaxDetectFreq = maxFreq;
}

// Optimized version (original version removed)
void
PartialTracker6::DetectPartials(const WDL_TypedBuf<BL_FLOAT> &magns,
                                const WDL_TypedBuf<BL_FLOAT> &phases,
                                vector<Partial> *outPartials)
{
    int maxIndex = magns.GetSize() - 1;
    if (mMaxDetectFreq > 0.0)
        maxIndex = mMaxDetectFreq*mBufferSize*0.5;
    if (maxIndex > magns.GetSize() - 1)
        maxIndex = magns.GetSize() - 1;
    
    vector<PeakDetector::Peak> peaks;
    //DBG_DumpPeaks(magns, peaks);

#if !USE_QIFFT_YLOG
    mPeakDetector->DetectPeaks(magns, &peaks,
                               DETECT_PARTIALS_START_INDEX, maxIndex);
    
    ComputePartials(peaks, magns, phases, outPartials);
#else
    mPeakDetector->DetectPeaks(mLogMagns, &peaks,
                               DETECT_PARTIALS_START_INDEX, maxIndex);

    // Log
    ComputePartials(peaks, mLogMagns, phases, outPartials);

    // Adjust the scale
    //
    // NOTE: we keep alpha0 in log scale
    for (int i = 0; i < outPartials->size(); i++)
    {
        Partial &p = (*outPartials)[i];
        BL_FLOAT ampNorm =
            mScale->ApplyScale(mYScaleInv2, p.mAmp,
                               (BL_FLOAT)MIN_AMP_DB, (BL_FLOAT)0.0);
        BL_FLOAT ampDbNorm =
            mScale->ApplyScale(mYScale, ampNorm,
                               (BL_FLOAT)MIN_AMP_DB, (BL_FLOAT)0.0);
        p.mAmp = ampDbNorm;
    }
#endif

    // DEBUG
    //DBG_DumpPartials(magns, *outPartials);
}

// From GlueTwinPartials()
bool
PartialTracker6::GluePartialBarbs(const WDL_TypedBuf<BL_FLOAT> &magns,
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
                    if ((ratio > GLUE_BARBS_AMP_RATIO) ||
                        (ratio < 1.0/GLUE_BARBS_AMP_RATIO))
                    // ... with a big amp ratio
                    {
                        // Check that the barb is "in the middle" of a side
                        // of the main partial (in height)
                        bool inTheMiddle = false;
                        bool onTheSide = false;
                        if (promCur < promOther)
                        {
                            BL_FLOAT hf =
                                ComputePeakHigherFoot(magns,
                                                      currentPartial.mLeftIndex,
                                                      currentPartial.mRightIndex);

                            
                            BL_FLOAT lf =
                                ComputePeakLowerFoot(magns,
                                                     otherPartial.mLeftIndex,
                                                     otherPartial.mRightIndex);
                            
                            if ((hf > lf) && (hf < otherPartial.mAmp))
                                inTheMiddle = true;
                            
                            // Check that the barb is on the right side
                            BL_FLOAT otherLeftFoot =
                                magns.Get()[otherPartial.mLeftIndex];
                            BL_FLOAT otherRightFoot =
                                magns.Get()[otherPartial.mRightIndex];
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
            //res.mPredictedFreq = res.mFreq;
            
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

bool
PartialTracker6::DiscardFlatPartial(const WDL_TypedBuf<BL_FLOAT> &magns,
                                    int peakIndex, int leftIndex, int rightIndex)
{
    BL_FLOAT amp = magns.Get()[peakIndex];
    
    BL_FLOAT binDiff = rightIndex - leftIndex;
    
    BL_FLOAT coeff = binDiff/amp;
    
    bool result = (coeff > DISCARD_FLAT_PARTIAL_COEFF);
    
    return result;
}

void
PartialTracker6::DiscardFlatPartials(const WDL_TypedBuf<BL_FLOAT> &magns,
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

void
PartialTracker6::SuppressZeroFreqPartials(vector<Partial> *partials)
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
PartialTracker6::ThresholdPartialsPeakHeight(const WDL_TypedBuf<BL_FLOAT> &magns,
                                             vector<Partial> *partials)
{
    vector<Partial> &result = mTmpPartials10;
    result.resize(0);
    
    for (int i = 0; i < partials->size(); i++)
    {
        const Partial &partial = (*partials)[i];
        
        BL_FLOAT height = ComputePeakHeight(magns,
                                            partial.mPeakIndex,
                                            partial.mLeftIndex,
                                            partial.mRightIndex);
        
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
PartialTracker6::ComputePeakProminence(const WDL_TypedBuf<BL_FLOAT> &magns,
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

// Inverse of prominence
BL_FLOAT
PartialTracker6::ComputePeakHeight(const WDL_TypedBuf<BL_FLOAT> &magns,
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

BL_FLOAT
PartialTracker6::ComputePeakHigherFoot(const WDL_TypedBuf<BL_FLOAT> &magns,
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
PartialTracker6::ComputePeakLowerFoot(const WDL_TypedBuf<BL_FLOAT> &magns,
                                      int leftIndex, int rightIndex)
{
    BL_FLOAT leftVal = magns.Get()[leftIndex];
    BL_FLOAT rightVal = magns.Get()[rightIndex];
    
    if (leftVal < rightVal)
        return leftVal;
    else
        return rightVal;
}

// Better than "Simple" => do not make jumps between bins
BL_FLOAT
PartialTracker6::ComputePeakIndexAvg(const WDL_TypedBuf<BL_FLOAT> &magns,
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

// Parabola peak center detection
// Works well
//
// See: http://eprints.maynoothuniversity.ie/4523/1/thesis.pdf (p32)
//
// and: https://ccrma.stanford.edu/~jos/parshl/Peak_Detection_Steps_3.html#sec:peakdet
//
BL_FLOAT
PartialTracker6::ComputePeakIndexParabola(const WDL_TypedBuf<BL_FLOAT> &magns,
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
PartialTracker6::ComputePeakAmpInterp(const WDL_TypedBuf<BL_FLOAT> &magns,
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
PartialTracker6::ComputePeakMagnPhaseInterp(const WDL_TypedBuf<BL_FLOAT> &magns,
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

void
PartialTracker6::DBG_DumpPartials(const char *fileName,
                                  const vector<Partial> &partials,
                                  int bufferSize)
{
    WDL_TypedBuf<BL_FLOAT> result;
    result.Resize(bufferSize);
    BLUtils::FillAllZero(&result);
    
    for (int i = 0; i < partials.size(); i++)
    {
        const Partial &p = partials[i];
        
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

BL_FLOAT
PartialTracker6::GetThreshold(int binNum)
{
#if USE_BL_PEAK_DETECTOR
    BL_FLOAT thrsNorm = -(MIN_AMP_DB - mThreshold)/(-MIN_AMP_DB);
#else // For debugging
    const BL_FLOAT defaultThrs = -100.0;
    BL_FLOAT thrsNorm = -(MIN_AMP_DB - defaultThrs)/(-MIN_AMP_DB);
#endif
    
    return thrsNorm;
}

void
PartialTracker6::PreProcessDataX(WDL_TypedBuf<BL_FLOAT> *data)
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
PartialTracker6::PreProcessDataY(WDL_TypedBuf<BL_FLOAT> *data)
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
PartialTracker6::PreProcessDataXY(WDL_TypedBuf<BL_FLOAT> *data)
{
    // Origin: process Y first
    PreProcessDataY(data);
    PreProcessDataX(data);
}
    
// Unwrap phase before converting to mel => more correct!
void
PartialTracker6::PreProcess(WDL_TypedBuf<BL_FLOAT> *magns,
                            WDL_TypedBuf<BL_FLOAT> *phases)
{
    // ORIGIN: smooth only magns
    // NOTE: tested smooting on complex => gave more noisy result
    PreProcessTimeSmooth(magns);
    
    // Use time smooth on raw magns too
    // (time smoothed, but linearly scaled)
    mLinearMagns = *magns;
    PreProcessDataY(&mLinearMagns); // We want raw data in dB (just keep linear on x)
    
#if USE_QIFFT_YLOG
    mLogMagns = *magns;
    mScale->ApplyScaleForEach(mYScale2, &mLogMagns);
#endif
    
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
PartialTracker6::SetTimeSmoothCoeff(BL_FLOAT coeff)
{
    mTimeSmoothCoeff = coeff;
}

void
PartialTracker6::SetTimeSmoothNoiseCoeff(BL_FLOAT coeff)
{
    mTimeSmoothNoiseCoeff = coeff;
}

// Time smooth
//
// NOTE: Makes PartialTacker6/QIFFT fail if > 0
// If need to smooth, we will have to smooth in complex domain!
void
PartialTracker6::PreProcessTimeSmooth(WDL_TypedBuf<BL_FLOAT> *magns)
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
PartialTracker6::TimeSmoothNoise(WDL_TypedBuf<BL_FLOAT> *noise)
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
PartialTracker6::DenormPartials(vector<Partial> *partials)
{
    BL_FLOAT hzPerBin =  mSampleRate/mBufferSize;
    
    for (int i = 0; i < partials->size(); i++)
    {
        Partial &partial = (*partials)[i];
        
        // Reverse Mel
        BL_FLOAT freq = partial.mFreq;
        freq = mScale->ApplyScale(mXScaleInv, freq, (BL_FLOAT)0.0,
                                  (BL_FLOAT)(mSampleRate*0.5));
        partial.mFreq = freq;
        
        // Convert to real freqs
        partial.mFreq *= mSampleRate*0.5;

#if !USE_QIFFT_YLOG
        // Reverse AWeighting
        int binNum = partial.mFreq/hzPerBin;
        partial.mAmp = ProcessAWeighting(binNum, mBufferSize*0.5,
                                         partial.mAmp, false);
    
        // Y
        partial.mAmp = mScale->ApplyScale(mYScaleInv, partial.mAmp,
                                          (BL_FLOAT)MIN_AMP_DB, (BL_FLOAT)0.0);
#endif
#if USE_QIFFT_YLOG
        // Y
        partial.mAmp = mScale->ApplyScale(mYScaleInv/*2*/, partial.mAmp,
                                          (BL_FLOAT)MIN_AMP_DB, (BL_FLOAT)0.0);
#endif

#if DENORM_PARTIAL_INDICES
        partial.mLeftIndex = DenormBinIndex(partial.mLeftIndex);
        partial.mPeakIndex = DenormBinIndex(partial.mPeakIndex);
        partial.mRightIndex = DenormBinIndex(partial.mRightIndex);
#endif
    }
}

void
PartialTracker6::DenormData(WDL_TypedBuf<BL_FLOAT> *data)
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
PartialsAmpToAmpDB(vector<Partial> *partials)
{
    for (int i = 0; i < partials->size(); i++)
    {
        Partial &partial = (*partials)[i];
        
        partial.mAmpDB = BLUtils::AmpToDB(partial.mAmp);
    }
}

BL_FLOAT
PartialTracker6::PartialScaleToQIFFTScale(BL_FLOAT ampDbNorm)
{
    BL_FLOAT amp =
        mScale->ApplyScale(mYScaleInv, ampDbNorm,
                           (BL_FLOAT)MIN_AMP_DB, (BL_FLOAT)0.0);

    BL_FLOAT ampLog =
        mScale->ApplyScale(mYScale2, amp,
                           (BL_FLOAT)MIN_AMP_DB, (BL_FLOAT)0.0);

    return ampLog;
}

BL_FLOAT
PartialTracker6::QIFFTScaleToPartialScale(BL_FLOAT ampLog)
{
    BL_FLOAT amp =
        mScale->ApplyScale(mYScaleInv2, ampLog,
                           (BL_FLOAT)MIN_AMP_DB, (BL_FLOAT)0.0);
    
    BL_FLOAT ampDbNorm =
        mScale->ApplyScale(mYScale, amp,
                           (BL_FLOAT)MIN_AMP_DB, (BL_FLOAT)0.0);
    
    return ampDbNorm;
}

void
PartialTracker6::PreProcessAWeighting(WDL_TypedBuf<BL_FLOAT> *magns,
                                      bool reverse)
{
    // Input magns are in normalized dB
    
    WDL_TypedBuf<BL_FLOAT> &weights = mTmpBuf6;
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
PartialTracker6::ProcessAWeighting(int binNum, int numBins,
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
PartialTracker6::ComputeAWeights(int numBins, BL_FLOAT sampleRate)
{
    AWeighting::ComputeAWeights(&mAWeights, numBins, sampleRate);
}

void
PartialTracker6::ComputePartials(const vector<PeakDetector::Peak> &peaks,
                                 const WDL_TypedBuf<BL_FLOAT> &magns,
                                 const WDL_TypedBuf<BL_FLOAT> &phases,
                                 vector<Partial> *partials)
{
    partials->resize(peaks.size());

    WDL_TypedBuf<BL_FLOAT> &phasesUW = mTmpBuf10;
    phasesUW = phases;
    // NOTE: phases are already unwrapped if MEL_UNWRAP_PHASES is 1
    PhasesUnwrapper::UnwrapPhasesFreq(&phasesUW);
    
    for (int i = 0; i < peaks.size(); i++)
    {
        const PeakDetector::Peak &peak = peaks[i];
        Partial &p = (*partials)[i];

        // Indices
        p.mPeakIndex = peak.mPeakIndex;
        p.mLeftIndex = peak.mLeftIndex;
        p.mRightIndex = peak.mRightIndex;

        
#if !USE_QIFFT
        BL_FLOAT peakIndexF = ComputePeakIndexParabola(magns, p.mPeakIndex);
#else
        QIFFT::Peak qifftPeak;
        QIFFT::FindPeak(magns, phasesUW, mBufferSize, peak.mPeakIndex, &qifftPeak);
        // QIFFT::FindPeak2() is not fixed yet...
        //QIFFT::FindPeak2(magns, phasesUW, peak.mPeakIndex, &qifftPeak);

        // Apply empirical coeffs, so that next values will match
        // current values + deta.
        //
        // NOTE: this may depend on window type, maybe on overlap too..
        qifftPeak.mAlpha0 *= EMPIR_ALPHA0_COEFF;
        qifftPeak.mBeta0 *= EMPIR_BETA0_COEFF;
            
        p.mBinIdxF = qifftPeak.mBinIdx;
        p.mFreq = qifftPeak.mFreq;
        p.mAmp = qifftPeak.mAmp;
        p.mPhase = qifftPeak.mPhase;
        p.mAlpha0 = qifftPeak.mAlpha0;
        p.mBeta0 = qifftPeak.mBeta0;

        BL_FLOAT peakIndexF = qifftPeak.mBinIdx;
#endif
        
        p.mPeakIndex = bl_round(peakIndexF);
        if (p.mPeakIndex < 0)
            p.mPeakIndex = 0;
            
        //if (p.mPeakIndex > maxIndex)
        //    p.mPeakIndex = maxIndex;
        if (p.mPeakIndex > magns.GetSize() - 1)
            p.mPeakIndex = magns.GetSize() - 1;

#if !USE_QIFFT
        // Remainder: freq is normalized here
        BL_FLOAT peakFreq = peakIndexF/(mBufferSize*0.5);
        p.mFreq = peakFreq;
#endif
        
        // Kalman
        //
        // Update the estimate with the first value
        //p.mKf.updateEstimate(p.mFreq);
        p.mKf.initEstimate(p.mFreq);
        
        // For predicted freq to be freq for the first value
        //p.mPredictedFreq = p.mFreq;
        
        // Default value. Will be overwritten
        //BL_FLOAT peakAmp = data.Get()[(int)peakIndexF];

#if !USE_QIFFT
        
#if !INTERPOLATE_PHASES
        // Magn
        p.mAmp = ComputePeakAmpInterp(magns, peakFreq);
        
        // Phase
        p.mPhase = phases.Get()[(int)peakIndexF];
#else
        ComputePeakMagnPhaseInterp(magns, phases, peakFreq,
                                   &p.mAmp, &p.mPhase);
#endif

#endif
    }
}

int
PartialTracker6::DenormBinIndex(int idx)
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

void
PartialTracker6::PostProcessPartials(const WDL_TypedBuf<BL_FLOAT> &magns,
                                     vector<Partial> *partials)
{
    SuppressZeroFreqPartials(partials);

    // Some operations
#if GLUE_BARBS
    vector<Partial> &prev = mTmpPartials1;
    prev = *partials;
    
    GluePartialBarbs(magns, partials);
#endif

#if DISCARD_FLAT_PARTIAL
    DiscardFlatPartials(magns, partials);
#endif

    // Threshold
    ThresholdPartialsPeakHeight(magns, partials);
}

void
PartialTracker6::DBG_DumpPeaks(const WDL_TypedBuf<BL_FLOAT> &data,
                               const vector<PeakDetector::Peak> &peaks)
{
    for (int i = 0; i < peaks.size(); i++)
    {
        const PeakDetector::Peak &peak = peaks[i];

        // Select peak
        if (peak.mPeakIndex >= 2)
        {
            // Dump peak amp
            BL_FLOAT peakAmp = data.Get()[peak.mPeakIndex];

            //BLDebug::AppendValue("peaks.txt", peakAmp);
            
            break;
        }
    }
}

#if 1 //0 // Display only the first partial
void
PartialTracker6::DBG_DumpPartials(const WDL_TypedBuf<BL_FLOAT> &magns,
                                  const vector<Partial> &partials)
{
    static bool firstTime = true;
    if (firstTime)
    {
        BLDebug::ResetFile("ref-amp.txt");
        BLDebug::ResetFile("ref-freq.txt");
        BLDebug::ResetFile("amp0.txt");
        BLDebug::ResetFile("amp1.txt");
        BLDebug::ResetFile("freq0.txt");
        BLDebug::ResetFile("freq1.txt");

        firstTime = false;
    }
    
    if (partials.empty())
        return;

    // Dump only the first partial
    const Partial &p = partials[0];

    // Ref amp
    BLDebug::AppendValue("ref-amp.txt", magns.Get()[p.mPeakIndex]);

    // Ref freq
    BL_FLOAT refFreq = ((BL_FLOAT)p.mPeakIndex)/magns.GetSize();
    BLDebug::AppendValue("ref-freq.txt", refFreq);
        
    // Real amp
    BLDebug::AppendValue("amp0.txt", p.mAmp);
    
    // Estimated next amp
    BL_FLOAT amp1 = p.mAmp + p.mAlpha0;
    BLDebug::AppendValue("amp1.txt", amp1);

    // Real freq
    BLDebug::AppendValue("freq0.txt", p.mFreq);

    // Estimated next freq
    BL_FLOAT freq1 = p.mFreq + p.mBeta0;
    BLDebug::AppendValue("freq1.txt", freq1);
}
#endif

// Specific; for displaying two sines crossing
#if 0 //1 // Dump all th epartials
void
PartialTracker6::DBG_DumpPartials(const WDL_TypedBuf<BL_FLOAT> &magns,
                                  const vector<Partial> &partials)
{
    static bool firstTime = true;
    if (firstTime)
    {
        //BLDebug::ResetFile("ref-amp-0.txt");
        //BLDebug::ResetFile("ref-freq-0.txt");
        BLDebug::ResetFile("amp0-0.txt");
        BLDebug::ResetFile("amp1-0.txt");
        BLDebug::ResetFile("freq0-0.txt");
        BLDebug::ResetFile("freq1-0.txt");

        //BLDebug::ResetFile("ref-amp-1.txt");
        //BLDebug::ResetFile("ref-freq-1.txt");
        BLDebug::ResetFile("amp0-1.txt");
        BLDebug::ResetFile("amp1-1.txt");
        BLDebug::ResetFile("freq0-1.txt");
        BLDebug::ResetFile("freq1-1.txt");
        
        firstTime = false;
    }
    
    if (partials.empty())
        return;

    for (int i = 0; i < 2; i++)
    {
        BL_FLOAT amp0 = -1.0;
        BL_FLOAT amp1 = -1.0;
        BL_FLOAT freq0 = -1.0;
        BL_FLOAT freq1 = -1.0;

        if (i < partials.size())
        {
            // Dump only the first partial
            const Partial &p = partials[i];

            amp0 = p.mAmp;
            amp1 = p.mAmp + p.mAlpha0;

            freq0 = p.mFreq;
            freq1 = p.mFreq + p.mBeta0;
        }

        if (i == 0)
        {
            // Real amp
            BLDebug::AppendValue("amp0-0.txt", amp0);
            
            // Estimated next amp
            BLDebug::AppendValue("amp1-0.txt", amp1);
            
            // Real freq
            BLDebug::AppendValue("freq0-0.txt", freq0);
            
            // Estimated next freq
            BLDebug::AppendValue("freq1-0.txt", freq1);
        }

        if (i == 1)
        {
            // Real amp
            BLDebug::AppendValue("amp0-1.txt", amp0);
            
            // Estimated next amp
            BLDebug::AppendValue("amp1-1.txt", amp1);
            
            // Real freq
            BLDebug::AppendValue("freq0-1.txt", freq0);
            
            // Estimated next freq
            BLDebug::AppendValue("freq1-1.txt", freq1);
        }
    }
}
#endif
