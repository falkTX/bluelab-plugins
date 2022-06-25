/* Copyright (C) 2022 Nicolas Dittlo <deadlab.plugins@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this software; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
 
//
//  PartialTracker.cpp
//  BL-SASViewer
//
//  Created by applematuer on 2/2/19.
//
//

#include <algorithm>
using namespace std;

#include <SASViewerProcess.h>
#include <FreqAdjustObj3.h>

#include <CMASmoother.h>

#include <BLUtils.h>
#include <BLUtilsMath.h>

#include <BLDebug.h>

#include "PartialTracker.h"

//#define EPS 1e-15
//#define INF 1e15

#define HISTORY_SIZE 2

// Discarding
#define EPS_DB 1e-15
//#define MIN_DB -80.0 // makes many partials, and thacking gets lost
#define MIN_DB -60.0 // makes few partials

#define MIN_AMP 0.01

//#define MIN_AMP 0.01
//#define MIN_AMP 0.0

#define BARB_COEFF 2.0 //2.0 //4.0 //10.0 //2.0

#define MAX_FREQ_DIFF 200.0 //100.0 //22050.0 //1000.0 //100.0
//#define MAX_AMP_DIFF 0.01 //1.0 //0.01 //1.0 //0.2 // TODO

// 10.0: works well, but the 2nd partial gets lost for 1 step
// on "oohoo"
//
// 2.0: seems to be better (no tracking lost)
//
// 4.0: worse
//
// 1.0: not so good
//
// 1.5: good too
#define MAX_AMP_DIFF_RATIO 2.0 //1.5 //2.0 //1.0 //4.0 //2.0 //10.0

// Compute simple, avg, or parabola ?
// (Avg seems a little more smooth than parabola)
#define COMPUTE_PEAKS_SIMPLE 0 //0 //1
#define COMPUTE_PEAKS_AVG 1 //0 //1
#define COMPUTE_PEAKS_PARABOLA 0 //1
#define COMPUTE_PEAKS_AVG2 0 // 0 // 1

// Do we filter ?
#define FILTER_PARTIALS 1 //0 // 1

// Do we smooth when filter ?
#define FILTER_SMOOTH 0 //0 //1
#define FILTER_SMOOTH_COEFF 0.9 //0.8 //0.9

#define HARMONIC_SELECT 0

// Makes wobbling frequencies
#define USE_FREQ_OBJ 0 //0 //1

#define MAGNS_TEMPORAL_SMOOTH 0 //1
#define MAGNS_TEMPORAL_SMOOTH_COEFF 0.8

#define DEBUG_KEEP_SINGLE_PARTIAL 0 //0 //1

#define ASSOC_PERMUT_MAX_NEIGHBOURS 2

#define MAX_ZOMBIE_AGE 2

// 1 gives good results for "Ooohoo" (method "Min")
// 2 gives good results for "Ti Tsu Koi" (method "Min")
#define NUM_ITER_EXTRACT_NOISE 4 //1 //4 //1 //2

// TODO: maybe make a param "extract ratio"

// If we incrase, keep more
#define DEFAULT_SHARPNESS_EXTRACT_NOISE 0.000025 //0.00012 //0.00001 //0.000025
// 0.00002 => need to keep
// 0.00012 => need to cut

#define DEBUG_COEFF_EXTRACT_HARMO 1.0 //4.0

// Minimum spacing between partials in bins
#define MIN_SPACING_BINS 0 //8 //16

// 0.01 => gives good result (avoids "tss" detected as partial)
// 0.005 => avoids missing partials in heigh frequencies
//#define MIN_SPACING_NORM_MEL 0.01
//#define MIN_SPACING_NORM_MEL 0.005
#define MIN_SPACING_NORM_MEL 0.001 // in test...


// NOTE: See: https://www.dsprelated.com/freebooks/sasp/Spectral_Modeling_Synthesis.html
//
// and: https://www.dsprelated.com/freebooks/sasp/PARSHL_Program.html#app:parshlapp
//

unsigned long PartialTracker::Partial::mCurrentId = 0;



PartialTracker::Partial::Partial()
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
}
    
    
PartialTracker::Partial::Partial(const Partial &other)
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
}
    
PartialTracker::Partial::~Partial() {}

void
PartialTracker::Partial::GenNewId()
{
    mId = mCurrentId++;
}
    
BL_FLOAT
PartialTracker::Partial::ComputeDistance2(const Partial &partial0,
                                          const Partial &partial1,
                                          BL_FLOAT sampleRate)
{
#define AMP_COEFF 0.0 //0.25
  BL_FLOAT diffBin = std::fabs(partial0.mFreq - partial1.mFreq)/sampleRate;
        
  BL_FLOAT diffAmp = std::fabs(partial0.mAmp - partial1.mAmp)/partial0.mAmp;
    
    diffAmp *= AMP_COEFF;
    
    BL_FLOAT dist2 = diffBin*diffBin + diffAmp*diffAmp;
    
    return dist2;
}
    
bool
PartialTracker::Partial::FreqLess(const Partial &p1, const Partial &p2)
{
    return (p1.mFreq < p2.mFreq);
}

bool
PartialTracker::Partial::IdLess(const Partial &p1, const Partial &p2)
{
    return (p1.mId < p2.mId);
}

bool
PartialTracker::Partial::IsEqual(const Partial &p1, const Partial &p2)
{
    // Keep it commented !!
    // (otherwise it will overwrite the global EPS, and
    // make mistakes with dB
    //#define EPS 1e-8
    
    if ((std::fabs(p1.mFreq - p2.mFreq) < BL_EPS) &&
        (std::fabs(p1.mAmp - p2.mAmp) < BL_EPS))
        return true;
    
    return false;

}

bool
PartialTracker::Partial::operator==(const Partial &other)
{
    return IsEqual(*this, other);
}
    
bool
PartialTracker::Partial::operator!=(const Partial &other)
{
    return !(*this == other);
}

PartialTracker::PartialTracker(int bufferSize, BL_FLOAT sampleRate,
                               BL_FLOAT overlapping)
{
    mBufferSize = bufferSize;
    mSampleRate = sampleRate;
    mOverlapping = overlapping;
    
    mThreshold = -60.0;
    
    mFreqObj = NULL;
    
#if USE_FREQ_OBJ
    int freqRes = 1;
    mFreqObj = new FreqAdjustObj3(bufferSize, overlapping,
                                  freqRes, sampleRate);
#endif
    
     mSharpnessExtractNoise = DEFAULT_SHARPNESS_EXTRACT_NOISE;

     int dummyWinSize = 5;
     mSmoother = new CMASmoother(bufferSize, dummyWinSize);
}

PartialTracker::~PartialTracker()
{
    if (mFreqObj != NULL)
        delete mFreqObj;

    delete mSmoother;
}

void
PartialTracker::Reset()
{
    if (mFreqObj != NULL)
    {
        int freqRes = 1;
        mFreqObj->Reset(mBufferSize, mOverlapping,
                        freqRes, mSampleRate);
    }
    
    //mPrevPhases.Resize(0);
}

void
PartialTracker::SetThreshold(BL_FLOAT threshold)
{
    mThreshold = threshold;
}

#if 0 // OLD: all in one
void
PartialTracker::AddData(const WDL_TypedBuf<BL_FLOAT> &magns,
                        const WDL_TypedBuf<BL_FLOAT> &phases)
{
    WDL_TypedBuf<BL_FLOAT> magns0 = magns;
    
#if MAGNS_TEMPORAL_SMOOTH
    if (mPrevMagns.GetSize() != magns0.GetSize())
    {
        mPrevMagns = magns0;
    }
    else
    {
        for (int i = 0; i < magns0.GetSize(); i++)
        {
            BL_FLOAT magn = magns0.Get()[i];
            BL_FLOAT prevMagn = mPrevMagns.Get()[i];
            
            BL_FLOAT newMagn = MAGNS_TEMPORAL_SMOOTH_COEFF*prevMagn +
                            (1.0 - MAGNS_TEMPORAL_SMOOTH_COEFF)*magn;
            
            mPrevMagns.Get()[i] = newMagn;
            
            magns0.Get()[i] = newMagn;
        }
    }
#endif
    
    if (mFreqObj != NULL)
    {
        // See: http://blogs.zynaptiq.com/bernsee/pitch-shifting-using-the-ft/
        mFreqObj->ComputeRealFrequencies(phases, &mRealFreqs);
    }
    
    vector<Partial> partials;
    DetectPartials(magns0, phases, &partials);
    
    SuppressBarbs(&partials);
    
    // DEBUG
    //partials.resize(1);
    
    mPartials.push_front(partials);
    
    while(mPartials.size() > HISTORY_SIZE)
          mPartials.pop_back();
    
#if !FILTER_PARTIALS // Not filtered
    mResult = mPartials[0];
#else // Filtered
    FilterPartials(&mResult);
#endif
    
#if HARMONIC_SELECT
    HarmonicSelect(&mResult);
#endif
}
#endif

void
PartialTracker::SetData(const WDL_TypedBuf<BL_FLOAT> &magns,
                        const WDL_TypedBuf<BL_FLOAT> &phases)
{
    mCurrentMagns = magns;
    mCurrentPhases = phases;
}

void
PartialTracker::DetectPartials()
{
    WDL_TypedBuf<BL_FLOAT> magns0 = mCurrentMagns;
    
#if MAGNS_TEMPORAL_SMOOTH
    if (mPrevMagns.GetSize() != magns0.GetSize())
    {
        mPrevMagns = magns0;
    }
    else
    {
        for (int i = 0; i < magns0.GetSize(); i++)
        {
            BL_FLOAT magn = magns0.Get()[i];
            BL_FLOAT prevMagn = mPrevMagns.Get()[i];
            
            BL_FLOAT newMagn = MAGNS_TEMPORAL_SMOOTH_COEFF*prevMagn +
            (1.0 - MAGNS_TEMPORAL_SMOOTH_COEFF)*magn;
            
            mPrevMagns.Get()[i] = newMagn;
            
            magns0.Get()[i] = newMagn;
        }
    }
#endif
    
    if (mFreqObj != NULL)
    {
        // See: http://blogs.zynaptiq.com/bernsee/pitch-shifting-using-the-ft/
        mFreqObj->ComputeRealFrequencies(mCurrentPhases, &mRealFreqs);
    }
    
    vector<Partial> partials;
    DetectPartials(magns0, mCurrentPhases, &partials);
    
    //SuppressBarbs(&partials);
    
    mPartials.push_front(partials);
    
    while(mPartials.size() > HISTORY_SIZE)
        mPartials.pop_back();
    
    // TEST
    mResult = partials;
}

void
PartialTracker::ExtractNoiseEnvelope()
{
#if 0
    ExtractNoiseEnvelopeMax();
#endif
    
#if 0
    // Doesn't work well
    ExtractNoiseEnvelopeSmooth();
#endif
    
#if 0
    ExtractNoiseEnvelopeSelect();
#endif

#if 1
    ExtractNoiseEnvelopeTrack();
    //SuppressZeroFreqPartialEnv();
#endif
}

void
PartialTracker::ExtractNoiseEnvelopeMax()
{
    // If not partial detected, noise envelope is simply the input magns
    mNoiseEnvelope = mCurrentMagns;
    
    if (mPartials.empty())
        return;
    
    // NOTE: may be better in the loop
    // (to avoid special case for first iteration)
    CutPartials(mPartials[0], &mNoiseEnvelope);
    
    // Iterate
    WDL_TypedBuf<BL_FLOAT> dummyPhases;
    BLUtils::ResizeFillZeros(&dummyPhases, mNoiseEnvelope.GetSize());
    for (int i = 0; i < NUM_ITER_EXTRACT_NOISE - 1; i++)
    {
        vector<Partial> partials;
        DetectPartials(mNoiseEnvelope, dummyPhases, &partials);
        
        CutPartials(partials, &mNoiseEnvelope);
    }
}

void
PartialTracker::ExtractNoiseEnvelopeSelect()
{
    // If not partial detected, noise envelope is simply the input magns
    mNoiseEnvelope = mCurrentMagns;
    
    // Iterate
    WDL_TypedBuf<BL_FLOAT> dummyPhases;
    BLUtils::ResizeFillZeros(&dummyPhases, mNoiseEnvelope.GetSize());
    for (int i = 0; i < NUM_ITER_EXTRACT_NOISE; i++)
    {
        vector<Partial> partials;
        DetectPartials(mNoiseEnvelope, dummyPhases, &partials);
        
        // Select partials
        vector<Partial> selectedPartials;
        for (int j = 0; j < partials.size(); j++)
        {
            const Partial &partial = partials[j];
            
            int dIdx = partial.mRightIndex - partial.mLeftIndex;
            BL_FLOAT amp = partial.mAmp;
            
            BL_FLOAT sharpness = amp/dIdx;
            
            if (sharpness > mSharpnessExtractNoise)
                selectedPartials.push_back(partial);
        }
        
        CutPartials(selectedPartials, &mNoiseEnvelope);
    }
    
    WDL_TypedBuf<BL_FLOAT> noiseEnv = mNoiseEnvelope;
    BLUtils::MultValues(&noiseEnv, (BL_FLOAT)DEBUG_COEFF_EXTRACT_HARMO);
    
    // Compute harmonic envelope
    // (origin signal less noise)
    mHarmonicEnvelope = mCurrentMagns;
    BLUtils::SubstractValues(&mHarmonicEnvelope, noiseEnv);
    
    BLUtils::ClipMin(&mHarmonicEnvelope, (BL_FLOAT)0.0);
}

void
PartialTracker::ExtractNoiseEnvelopeTrack()
{
    // If not partial detected, noise envelope is simply the input magns
    mNoiseEnvelope = mCurrentMagns;
    
    //BLDebug::DumpData("magns.txt", mCurrentMagns);
    
    vector<Partial> partials;
    GetPartials(&partials);
    
    // Select partials
    vector<Partial> selectedPartials;
    for (int i = 0; i < partials.size(); i++)
    {
        const Partial &partial = partials[i];
        
        // to TEST
        //if (partial.mState != Partial::ALIVE)
        //    continue;
        
        // Commented
        // Do not use sharpness if we filter partials
        // (so we have all tracked partials with dB threshold at -70)
#if 0
        int dIdx = partial.mRightIndex - partial.mLeftIndex;
        BL_FLOAT amp = partial.mAmp;
        
        BL_FLOAT sharpness = amp/dIdx;
            
        if (sharpness > mSharpnessExtractNoise)
            selectedPartials.push_back(partial);
#endif
        
        selectedPartials.push_back(partial);
    }
    
    CutPartials(selectedPartials, &mNoiseEnvelope);
    
    //BLDebug::DumpData("noise.txt", mNoiseEnvelope);
    
    WDL_TypedBuf<BL_FLOAT> noiseEnv = mNoiseEnvelope;
    BLUtils::MultValues(&noiseEnv, (BL_FLOAT)DEBUG_COEFF_EXTRACT_HARMO);
    
    // Compute harmonic envelope
    // (origin signal less noise)
    mHarmonicEnvelope = mCurrentMagns;
    BLUtils::SubstractValues(&mHarmonicEnvelope, noiseEnv);
    
    BLUtils::ClipMin(&mHarmonicEnvelope, (BL_FLOAT)0.0);
}

#if 0
// Zero the noise and harmonic envelopes if the
// first partial is not correct (no minimum on the left)
void
PartialTracker::SuppressZeroFreqPartialEnv()
{
    // TEST: empty the first 3 bins...
    for (int i = 0; i < 3; i++)
    {
        mNoiseEnvelope.Get()[i] = 0.0;
        mHarmonicEnvelope.Get()[i] = 0.0;
    }
}
#endif

// NOTE: Doesn't work well
void
PartialTracker::ExtractNoiseEnvelopeSmooth()
{
#define CMA_WINDOW_SIZE 40
    
    // If not partial detected, noise envelope is simply the input magns
    mNoiseEnvelope = mCurrentMagns;
    
    WDL_TypedBuf<BL_FLOAT> smoothEnv;
    smoothEnv.Resize(mNoiseEnvelope.GetSize());
    mSmoother->ProcessOne(mNoiseEnvelope.Get(), smoothEnv.Get(),
                          mNoiseEnvelope.GetSize(), CMA_WINDOW_SIZE);
    
    mNoiseEnvelope = smoothEnv;
}

void
PartialTracker::CutPartials(const vector<Partial> &partials,
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
        
        int numIdx = maxIdx - minIdx;
        for (int j = 0; j <= numIdx; j++)
        {
            BL_FLOAT t = ((BL_FLOAT)j)/numIdx;
            
            BL_FLOAT val = (1.0 - t)*minVal + t*maxVal;
            if (minIdx + j >= magns->GetSize())
                continue;
            
            magns->Get()[minIdx + j] = val;
        }
    }
}

void
PartialTracker::FilterPartials()
{
#if !FILTER_PARTIALS // Not filtered
    mResult = mPartials[0];
#else // Filtered
    FilterPartials(&mResult);
#endif
    
#if HARMONIC_SELECT
    HarmonicSelect(&mResult);
#endif
}

bool
PartialTracker::GetPartials(vector<Partial> *partials)
{
    if (mPartials.empty())
        return false;
    
    *partials = mResult;
    
#if DEBUG_KEEP_SINGLE_PARTIAL
    if (!partials->empty())
    {
        Partial p = (*partials)[0];
        partials->clear();
        partials->push_back(p);
    }
#endif
    
    return true;
}

void
PartialTracker::GetNoiseEnvelope(WDL_TypedBuf<BL_FLOAT> *noiseEnv)
{
    *noiseEnv = mNoiseEnvelope;
}

void
PartialTracker::GetHarmonicEnvelope(WDL_TypedBuf<BL_FLOAT> *harmoEnv)
{
    *harmoEnv = mHarmonicEnvelope;
}

void
PartialTracker::SetSharpnessExtractNoise(BL_FLOAT sharpness)
{
    mSharpnessExtractNoise = sharpness;
}

void
PartialTracker::RemoveRealDeadPartials(vector<Partial> *partials)
{
    vector<Partial> result;
    for (int i = 0; i < partials->size(); i++)
    {
        const Partial &p = (*partials)[i];
        if (p.mWasAlive)
            result.push_back(p);
    }
    
    *partials = result;
}

void
PartialTracker::DBG_DumpPartials(const vector<Partial> &partials,
                                 int maxNumPartials)
{
    vector<Partial> partials0 = partials;
    
    sort(partials0.begin(), partials0.end(), Partial::IdLess);
    
    for (int i = 0; i < maxNumPartials; i++)
    {
        char fileName[255];
        sprintf(fileName, "partial-%d.txt", i);
        
        if (i < partials0.size())
        {
            const Partial &p = partials0[i];
            BLDebug::AppendValue(fileName, p.mFreq);
        }
        else
            // Append zeros to stay synchro
        {
            BLDebug::AppendValue(fileName, 0.0);
        }
    }
}

void
PartialTracker::DBG_DumpPartialsAmp(const vector<Partial> &partials,
                                    int bufferSize, BL_FLOAT sampleRate)
{
#define NUM_MAGNS 1024
    
    BL_FLOAT hzPerBin = sampleRate/bufferSize;
    
    WDL_TypedBuf<BL_FLOAT> result;
    result.Resize(NUM_MAGNS);
    BLUtils::FillAllZero(&result);
    
    for (int i = 0; i < partials.size(); i++)
    {
        const PartialTracker::Partial &p = partials[i];
        
        BL_FLOAT binNum = p.mFreq/hzPerBin;
        BL_FLOAT amp = p.mAmp;
        
        if (binNum < result.GetSize())
        {
            result.Get()[(int)binNum] = amp;
        }
    }
    
    BLDebug::DumpData("partials.txt", result);
}

// Same method as above (implemented twice...)
// (but better code)
void
PartialTracker::DBG_DumpPartials2(const vector<Partial> &partials)
{
    BL_FLOAT hzPerBin = mSampleRate/mBufferSize;
    
    WDL_TypedBuf<BL_FLOAT> buffer;
    BLUtils::ResizeFillZeros(&buffer, mBufferSize/2);
    
    for (int i = 0; i < partials.size(); i++)
    {
        const PartialTracker::Partial &p = partials[i];
        
        BL_FLOAT binNum = p.mFreq/hzPerBin;
        BL_FLOAT amp = p.mAmp;
        
        buffer.Get()[(int)binNum] = amp;
    }
    
    BLDebug::DumpData("partials.txt", buffer);
}

void
PartialTracker::DetectPartials(const WDL_TypedBuf<BL_FLOAT> &magns,
                               const WDL_TypedBuf<BL_FLOAT> &phases,
                               vector<Partial> *outPartials)
{    
    outPartials->clear();
    
    //
    // prevIndex0 prevIndex1 (max), current
    
    // Skip the first ones
    // (to avoid artifacts of very low freq partial)
    //int currentIndex = 0;
    int currentIndex = 2;
    
    BL_FLOAT prevVal0 = 0.0;
    BL_FLOAT prevVal1 = 0.0;
    while(currentIndex < magns.GetSize())
    {
        BL_FLOAT currentVal = magns.Get()[currentIndex];
        
        if ((prevVal1 > prevVal0) && (prevVal1 > currentVal))
            // Maximum found
        {
            if (currentIndex - 1 >= 0)
            {
                // Take the left and right "feets" of the partial,
                // then the middle.
                // (in order to be more precise)
            
                // Left
                int leftIndex = currentIndex - 2;
                if (leftIndex > 0)
                {
                    BL_FLOAT prevLeftVal = magns.Get()[leftIndex];
                    while(leftIndex > 0)
                    {
                        leftIndex--;
                    
                        BL_FLOAT leftVal = magns.Get()[leftIndex];
                    
                        // Stop if we reach 0 or if it goes up again
                        if ((leftVal < BL_EPS) || (leftVal >= prevLeftVal))
                        {
                            leftIndex++;
                        
                            // Check bounds
                            if (leftIndex < 0)
                                leftIndex = 0;
                            if (leftIndex >= magns.GetSize())
                                leftIndex = magns.GetSize() - 1;
                        
                            break;
                        }
                        
                        prevLeftVal = leftVal;
                    }
                }
            
                // Right
                int rightIndex = currentIndex;
                if (rightIndex < magns.GetSize())
                {
                    BL_FLOAT prevRightVal = magns.Get()[rightIndex];
                    while(rightIndex < magns.GetSize() - 1)
                    {
                        rightIndex++;
                    
                        BL_FLOAT rightVal = magns.Get()[rightIndex];
                    
                        // Stop if we reach 0 or if it goes up again
                        if ((rightVal < BL_EPS) || (rightVal >= prevRightVal))
                        {
                            rightIndex--;
                        
                            // Check bounds
                            if (rightIndex < 0)
                                rightIndex = 0;
                            if (rightIndex >= magns.GetSize())
                                rightIndex = magns.GetSize() - 1;
                        
                            break;
                        }
                        
                        prevRightVal = rightVal;
                    }
                }
            
#if 0 // Take the center
                int peakIndex = (leftIndex + rightIndex)/2;
#else // Take the max
      // BETTER: avoids shifting the center in the case the left or right
      // foot extends too much on one direction
                int peakIndex = currentIndex - 1;
#endif
            
                if ((peakIndex < 0) || (peakIndex >= magns.GetSize()))
                    // Out of bounds
                    continue;
            
                BL_FLOAT peakAmp = magns.Get()[peakIndex];
     
                // Discard partial ?
                bool discard = false;
                
#if 0 // Moved this code at the beging of filter
      // For the noise envelope to get also the small partials
                // Too small amp ?
                BL_FLOAT peakAmpDB = BLUtils::AmpToDBNorm(peakAmp, EPS_DB, mThreshold);
                if (peakAmpDB < MIN_AMP)
                    // Amp is not big enough, discard the partial
                    discard = true;
#endif
                
#if 0 // Not working well
                // Is a barb on the side of a real partial ?
                BL_FLOAT leftAmp = magns.Get()[leftIndex];
                BL_FLOAT rightAmp = magns.Get()[rightIndex];
                BL_FLOAT maxSideAmp = (leftAmp > rightAmp) ? leftAmp : rightAmp;
            
                if (std::fabs(leftAmp - rightAmp)*BARB_COEFF > (peakAmp - maxSideAmp))
                    discard = true;
#endif
                
                // Create new partial
                Partial p;
            
                p.mPeakIndex = peakIndex;
                p.mLeftIndex = leftIndex;
                p.mRightIndex = rightIndex;
                
#if COMPUTE_PEAKS_SIMPLE // Makes some jumps between frequencies
      // But resynthetize well
                BL_FLOAT peakFreq = ComputePeakFreqSimple(peakIndex);
#endif
#if COMPUTE_PEAKS_AVG
      // Make smooth partials change
      // But makes wobble in the sound volume
                BL_FLOAT peakFreq = ComputePeakFreqAvg(magns, leftIndex, rightIndex);
#endif
#if COMPUTE_PEAKS_PARABOLA
                // Make smooth partials change
                // But makes wobble in the sound volume
                BL_FLOAT peakFreq = ComputePeakFreqParabola(magns, peakIndex);
#endif
#if COMPUTE_PEAKS_AVG2
                BL_FLOAT peakFreq = ComputePeakFreqAvg2(magns, leftIndex, rightIndex);
#endif
                
                p.mFreq = peakFreq;
      
                // If magns are 0, ComputePeakFreqAvg() will return a frequency at 0
                // (which means invalid partial)
                //if (peakFreq < EPS)
                //    discard = true;
                
                if (mFreqObj == NULL)
                    peakAmp = ComputePeakAmpAvg(magns, peakFreq);
                else
                    peakAmp = ComputePeakAmpAvgFreqObj(magns, peakFreq);
            
                p.mAmp = peakAmp;
                
                //p.mPhase = phases.Get()[peakIndex];
            
                if (!discard)
                    outPartials->push_back(p);
            
                // Go just after the right foot of the partial
                currentIndex = rightIndex;
            
                //TODO: test this !
                // Go one step before
                // Case: tiny peak just before a big one: we detect the tiny one but not the big one
                //currentIndex = rightIndex - 1;
            }
        }
        
        currentIndex++;
        
        if (currentIndex - 1 >= 0)
            prevVal1 = magns.Get()[currentIndex - 1];
        
        if (currentIndex - 2 >= 0)
            prevVal0 = magns.Get()[currentIndex - 2];
    }
}

// Doesn't work...
#if 0 // TEST 1
void
PartialTracker::SuppressNoisyPartials(vector<Partial> *partials)
{
    vector<Partial> result;
    for (int i = 0; i < partials->size(); i++)
    {
        const Partial &partial = (*partials)[i];
        
        int peakIndex = partial.mPeakIndex;
        
        BL_FLOAT currentPhase = mCurrentPhases.Get()[peakIndex];
        currentPhase = BLUtils::MapToPi(currentPhase);
        
        BL_FLOAT prevPhase = currentPhase;
        if (peakIndex - 1 >= 0)
            prevPhase = mCurrentPhases.Get()[peakIndex - 1];
        prevPhase = BLUtils::MapToPi(prevPhase);
        
        BL_FLOAT nextPhase = currentPhase;
        if (peakIndex + 1 < mCurrentPhases.GetSize())
            nextPhase = mCurrentPhases.Get()[peakIndex + 1];
        nextPhase = BLUtils::MapToPi (nextPhase);
        
        BL_FLOAT diff0 = std::fabs(currentPhase - prevPhase);
        BL_FLOAT diff1 = std::fabs(currentPhase - nextPhase);
        
        BL_FLOAT diff = std::sqrt(diff0*diff0 + diff1*diff1);
        
        // Zero frequency (because of very small magn) ?
        bool discard = false;
        
        // TEST here
        //fprintf(stderr, "diff: %g\n", diff);
        //if (diff > 3.0)
        //    discard = true;
        
        if (!discard)
            result.push_back(partial);
    }
    
    *partials = result;
}
#endif

#if 0
// Doesn't work...
void
PartialTracker::SuppressNoisyPartials(vector<Partial> *partials)
{
    if (mPrevPhases.GetSize() != mCurrentPhases.GetSize())
    {
        mPrevPhases = mCurrentPhases;
        
        return;
    }
    
    vector<Partial> result;
    for (int i = 0; i < partials->size(); i++)
    {
        const Partial &partial = (*partials)[i];
        
        int peakIndex = partial.mPeakIndex;
        
        BL_FLOAT phase0 = mPrevPhases.Get()[peakIndex];
        BL_FLOAT phase1 = mCurrentPhases.Get()[peakIndex];
        
        // See TransientLib4
        
        // Ensure that phase1 is greater than phase0
        while(phase1 < phase0)
            phase1 += 2.0*M_PI;
        
        BL_FLOAT delta = phase1 - phase0;
        
        delta = fmod(delta, 2.0*M_PI);
        
        if (delta > M_PI)
            delta = 2.0*M_PI - delta;

        // Zero frequency (because of very small magn) ?
        bool discard = false;
        
        // TEST here
        //fprintf(stderr, "delta: %g\n", delta);
        //if (delta > 0.01)
        //    discard = true;
        
        if (!discard)
            result.push_back(partial);
    }
    
    *partials = result;
    
    mPrevPhases = mCurrentPhases;
}
#endif

// Does not work very well
// (the Mel version works better)
void
PartialTracker::ApplyMinSpacing(vector<Partial> *partials)
{
    vector<Partial> result;
    for (int i = 0; i < partials->size(); i++)
    {
        const Partial &partial0 = (*partials)[i];
        int peakIndex0 = partial0.mPeakIndex;
    
        bool discard = false;
        for (int j = 0; j < partials->size(); j++)
        {
            if (j == i)
                continue;
            
            const Partial &partial1 = (*partials)[j];
            int peakIndex1 = partial1.mPeakIndex;
            
            if (std::fabs(peakIndex0 - peakIndex1) < MIN_SPACING_BINS)
            {
                discard = true;
                break;
            }
        }
        
        if (!discard)
            result.push_back(partial0);
    }
    
    *partials = result;
}

// Works well to suppress the "tsss"
// while keeping the low frequencies partials
void
PartialTracker::ApplyMinSpacingMel(vector<Partial> *partials)
{
    BL_FLOAT hzPerBin = mSampleRate/mBufferSize;
    
    vector<Partial> result;
    for (int i = 0; i < partials->size(); i++)
    {
        const Partial &partial0 = (*partials)[i];
        BL_FLOAT peakFreq0 = partial0.mFreq;
        BL_FLOAT peakFreqMel0 = BLUtils::FreqToMelNorm(peakFreq0, hzPerBin, mBufferSize);
        
        bool discard = false;
        for (int j = 0; j < partials->size(); j++)
        {
            if (j == i)
                continue;
            
            const Partial &partial1 = (*partials)[j];
            BL_FLOAT peakFreq1 = partial1.mFreq;
            BL_FLOAT peakFreqMel1 = BLUtils::FreqToMelNorm(peakFreq1, hzPerBin, mBufferSize);
            
            if (std::fabs(peakFreqMel0 - peakFreqMel1) < MIN_SPACING_NORM_MEL)
            {
                discard = true;
                
                break;
            }
        }
        
        if (!discard)
            result.push_back(partial0);
    }
    
    *partials = result;
}

void
PartialTracker::SuppressBadPartials(vector<Partial> *partials)
{
    vector<Partial> result;
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
PartialTracker::SuppressSmallPartials(vector<Partial> *partials)
{
    vector<Partial> result;
    for (int i = 0; i < partials->size(); i++)
    {
        const Partial &partial = (*partials)[i];
    
        BL_FLOAT peakAmp = partial.mAmp;
        
        // Too small amp ?
        bool discard = false;
        BL_FLOAT peakAmpDB = BLUtils::AmpToDBNorm(peakAmp, (BL_FLOAT)EPS_DB, mThreshold);
        if (peakAmpDB < MIN_AMP)
            // Amp is not big enough, discard the partial
            discard = true;
        
        if (!discard)
            result.push_back(partial);
    }
    
    *partials = result;
}

void
PartialTracker::SuppressBarbs(vector<Partial> *partials)
{
#define HEIGHT_COEFF 2.0
#define WIDTH_COEFF 1.0
    
    vector<Partial> result;
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

// Make a tracking for smoothing
void
PartialTracker::FilterPartials(vector<Partial> *result)
{
#define INF 1e15
    
    // TEST
    result->clear();
    
    if (mPartials.empty())
        return;
    
    // TEST
    //SuppressNoisyPartials(&mPartials[0]);
    
    // Suppress partials with zero freqs (due to very small magns)
    SuppressBadPartials(&mPartials[0]);
    
    // Check small magns
    SuppressSmallPartials(&mPartials[0]);
    
    // Suppress the barbs on the last detected series
    // (just added by DetectPartials)
    SuppressBarbs(&mPartials[0]);
    
    // TEST
    //ApplyMinSpacing(&mPartials[0]);
    
    // TEST 2
    ApplyMinSpacingMel(&mPartials[0]);
    
    if (mPartials.size() < 2)
        return;
    
    const vector<Partial> &prevPartials = mPartials[1];
    vector<Partial> currentPartials = mPartials[0];
    
    // Partials that was not associated at the end
    vector<Partial> remainingPartials;
    
    AssociatePartialsMin(prevPartials, &currentPartials,
                         &remainingPartials);
    
    //AssociatePartialsPermut(prevPartials, &currentPartials,
    //                        &remainingPartials);
                         
#if FILTER_SMOOTH
    SmoothPartials(prevPartials, &currentPartials);
#endif
    
    // Add the new zombie and dead partials
    for (int i = 0; i < prevPartials.size(); i++)
    {
        const Partial &prevPartial = prevPartials[i];

        bool found = false;
        for (int j = 0; j < currentPartials.size(); j++)
        {
            const Partial &currentPartial = currentPartials[j];
            
            // Avoid managing partials we just added above
            //if (currentPartial.mState != Partial::ALIVE)
            //    continue;
            
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
                currentPartials.push_back(newPartial);
            }
            else if (prevPartial.mState == Partial::ZOMBIE)
            {
                Partial newPartial = prevPartial;
                
                newPartial.mZombieAge++;
                if (newPartial.mZombieAge >= MAX_ZOMBIE_AGE)
                    newPartial.mState = Partial::DEAD;
                
                currentPartials.push_back(newPartial);
            }
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
        
        if (currentPartial.mState != Partial::DEAD)
            mPartials[0].push_back(currentPartial);
    }
}

// Discard partial association if the amps ratio is too big
// (this may avoid associated a new partial with a barb on the side of
// the previous partial)
bool
PartialTracker::TestDiscardByAmp(const Partial &p0, const Partial &p1)
{
    bool result = false;
    BL_FLOAT ampRatio = 1.0;
    
    if (p0.mAmp > BL_EPS)
        ampRatio = p1.mAmp/p0.mAmp;
    
    if ((ampRatio > MAX_AMP_DIFF_RATIO) ||
        (ampRatio < 1.0/MAX_AMP_DIFF_RATIO))
        result = true;
    
    return result;;
}

// Remove the detected partials that are between two
// valid partials (almost harmonic)
void
PartialTracker::HarmonicSelect(vector<Partial> *result)
{
    //#define INF 1e8
    
    if (result->empty())
        return;
    
    vector<Partial> newPartials;
    
    BL_FLOAT freq0 = (*result)[0].mFreq;
    newPartials.push_back((*result)[0]);
    
    BL_FLOAT freq = freq0;
    while((freq < mSampleRate/2.0) && !result->empty())
    {
        // Find the nearest partial
        BL_FLOAT minDiffFreq = BL_INF8;
        BL_FLOAT minIdx = -1;
        for (int i = 0; i < result->size(); i++)
        {
            const Partial &p = (*result)[i];
            
            BL_FLOAT diffFreq = std::fabs(p.mFreq - freq); // TODO: ComputeDist2 ?
            if (diffFreq < minDiffFreq)
            {
                minDiffFreq = diffFreq;
                minIdx = i;
            }
        }
        
        if (minIdx >= 0)
        {
            Partial &p = (*result)[minIdx];
            newPartials.push_back(p);
        }
        
        freq += freq0;
    }
    
    // Sort
    sort(newPartials.begin(), newPartials.end(), Partial::FreqLess);
    
    // Remove potential duplicates
    //vector<Partial>::iterator last = unique(newPartials.begin(), newPartials.end());
    vector<Partial>::iterator last = unique(newPartials.begin(), newPartials.end(),
                                            PartialTracker::Partial::IsEqual);
    
    newPartials.erase(last, newPartials.end());
    
    *result = newPartials;
}

// Makes jumps between bins if not filtered
BL_FLOAT
PartialTracker::ComputePeakFreqSimple(int peakIndex)
{
    BL_FLOAT result = GetFrequency(peakIndex);
    
    return result;
}

// Better than "Simple" => do not make jups between bins
BL_FLOAT
PartialTracker::ComputePeakFreqAvg(const WDL_TypedBuf<BL_FLOAT> &magns,
                                   int leftIndex, int rightIndex)
{
    // Pow coeff, to select preferably the high amp values
    // With 2.0, makes smoother freq change
    // With 3.0, make just a littel smoother than 2.0
#define COEFF 3.0 //2.0
    
    BL_FLOAT sumFreqs = 0.0;
    BL_FLOAT sumMagns = 0.0;
    
    for (int i = leftIndex; i <= rightIndex; i++)
    {
        BL_FLOAT freq = GetFrequency(i);
        
        BL_FLOAT magn = magns.Get()[i];
        
        magn = std::pow(magn, COEFF);
        
        sumFreqs += freq*magn;
        sumMagns += magn;
    }
    
    if (sumMagns < BL_EPS)
        return 0.0;
    
    BL_FLOAT result = sumFreqs/sumMagns;
    
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
PartialTracker::ComputePeakFreqParabola(const WDL_TypedBuf<BL_FLOAT> &magns,
                                        int peakIndex)
{
    BL_FLOAT hzPerBin = mSampleRate/mBufferSize;
    
    if ((peakIndex - 1 < 0) || (peakIndex >= magns.GetSize()))
    {
        BL_FLOAT result = peakIndex*hzPerBin;
        
        return result;
    }
    
    BL_FLOAT alpha = magns.Get()[peakIndex - 1];
    alpha = AmpToDB(alpha);
    
    BL_FLOAT beta = magns.Get()[peakIndex];
    beta = AmpToDB(beta);
    
    BL_FLOAT gamma = magns.Get()[peakIndex + 1];
    gamma = AmpToDB(gamma);
    
    // Center
    BL_FLOAT c = 0.5*((alpha - gamma)/(alpha - 2.0*beta + gamma));
    
    BL_FLOAT binNum = peakIndex + c;
                    
    BL_FLOAT result = binNum*hzPerBin;
    
    return result;
}

// Better than "Simple" => do not make jups between bins
BL_FLOAT
PartialTracker::ComputePeakFreqAvg2(const WDL_TypedBuf<BL_FLOAT> &magns,
                                   int leftIndex, int rightIndex)
{
    // Pow coeff, to select preferably the high amp values
    // With 2.0, makes smoother freq change
    // With 3.0, make just a littel smoother than 2.0
#define COEFF 3.0 //2.0
    
    BL_FLOAT sumFreqs = 0.0;
    BL_FLOAT sumMagns = 0.0;
    
    for (int i = leftIndex; i <= rightIndex; i++)
    {
        BL_FLOAT freq = GetFrequency(i);
        
        BL_FLOAT magn = magns.Get()[i];
        
        magn = std::pow(magn, COEFF);
        
        sumFreqs += freq*magn;
        sumMagns += magn;
    }
    
    if (sumMagns < BL_EPS)
        return 0.0;
    
    BL_FLOAT freq = sumFreqs/sumMagns;
    
    // Take the nearest bin
    BL_FLOAT result = 0.0;
    BL_FLOAT minDiff = BL_INF8;
    for (int i = 0; i < magns.GetSize(); i++)
    {
        BL_FLOAT freq0 = GetFrequency(i);
        
        BL_FLOAT diff0 = std::fabs(freq - freq0);
        if (diff0 < minDiff)
        {
            minDiff = diff0;
            result = freq0;
        }
    }
    return result;
}

// Simple method
BL_FLOAT
PartialTracker::ComputePeakAmpAvg(const WDL_TypedBuf<BL_FLOAT> &magns,
                                  BL_FLOAT peakFreq)
{
    BL_FLOAT hzPerBin = mSampleRate/mBufferSize;
    
    BL_FLOAT bin = peakFreq/hzPerBin;
    
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

BL_FLOAT
PartialTracker::ComputePeakAmpAvgFreqObj(const WDL_TypedBuf<BL_FLOAT> &magns,
                                         BL_FLOAT peakFreq)
{
    int idx0 = 0;
    for (int i = 0; i < mRealFreqs.GetSize(); i++)
    {
        BL_FLOAT realFreq = mRealFreqs.Get()[i];
        
        if (realFreq > peakFreq)
        {
            idx0 = i - 1;
            if (idx0 < 0)
                idx0 = 0;
            
            break;
        }
    }
    
    BL_FLOAT freq0 = mRealFreqs.Get()[idx0];
    
    int idx1 = idx0 + 1;
    if (idx1 >= mRealFreqs.GetSize())
    {
        BL_FLOAT result = magns.Get()[idx0];
        
        return result;
    }
    BL_FLOAT freq1 = mRealFreqs.Get()[idx1];
    
    BL_FLOAT t = (peakFreq - freq0)/(freq1 - freq0);
    
    BL_FLOAT prevAmp = magns.Get()[idx0];
    BL_FLOAT nextAmp = magns.Get()[idx1];
    
    BL_FLOAT peakAmp = (1.0 - t)*prevAmp + t*nextAmp;
    
    return peakAmp;
}

BL_FLOAT
PartialTracker::GetFrequency(int binIndex)
{
    if ((mFreqObj == NULL) || (binIndex >= mRealFreqs.GetSize()))
    {
        BL_FLOAT hzPerBin = mSampleRate/mBufferSize;
        BL_FLOAT result = binIndex*hzPerBin;
        
        return result;
    }
    else
    {
        BL_FLOAT result = mRealFreqs.Get()[binIndex];
        
        return result;
    }
}

int
PartialTracker::FindPartialById(const vector<PartialTracker::Partial> &partials,
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

void
PartialTracker::AssociatePartialsMin(const vector<PartialTracker::Partial> &prevPartials,
                                     vector<PartialTracker::Partial> *currentPartials,
                                     vector<PartialTracker::Partial> *remainingPartials)
{
    if (prevPartials.size() <= currentPartials->size())
        AssociatePartialsMinAux1(prevPartials, currentPartials, remainingPartials);
    else
        AssociatePartialsMinAux2(prevPartials, currentPartials, remainingPartials);
}

void
PartialTracker::AssociatePartialsMinAux1(const vector<PartialTracker::Partial> &prevPartials,
                                         vector<PartialTracker::Partial> *currentPartials,
                                         vector<PartialTracker::Partial> *remainingPartials)
{
    // Associate partials
    vector<Partial> currentPartialsAssoc;
    for (int i = 0; i < prevPartials.size(); i++)
    {
        const Partial &prevPartial = prevPartials[i];
        
        // Find the nearest partial (using frequency)
        BL_FLOAT minFreqDiff = BL_INF8;
        int minPartialIdx = -1;
        for (int j = 0; j < currentPartials->size(); j++)
        {
            Partial &currentPartial = (*currentPartials)[j];
            if (currentPartial.mId != -1)
                continue;
            
            //BL_FLOAT ampDiff = std::fabs(prevPartial.mAmp - currentPartial.mAmp);
            bool discardByAmp = TestDiscardByAmp(prevPartial, currentPartial);
            BL_FLOAT freqDiff = std::fabs(currentPartial.mFreq - prevPartial.mFreq);
            
            if (//(ampDiff < MAX_AMP_DIFF) &&
                !discardByAmp &&
                (freqDiff < MAX_FREQ_DIFF) &&
                (freqDiff < minFreqDiff))
            {
                minFreqDiff = freqDiff;
                minPartialIdx = j;
            }
        }
        
        if (minPartialIdx != -1)
            // Found
        {
            Partial &minPartial = (*currentPartials)[minPartialIdx];
            
            minPartial.mId = prevPartial.mId;
            minPartial.mState = Partial::ALIVE;
            minPartial.mWasAlive = true;
            
            currentPartialsAssoc.push_back(minPartial);
        }
    }
    
    remainingPartials->clear();
    for (int i = 0; i < currentPartials->size(); i++)
    {
        const Partial &p = (*currentPartials)[i];
        if (p.mId == -1)
            remainingPartials->push_back(p);
    }
    
    *currentPartials = currentPartialsAssoc;
}

void
PartialTracker::AssociatePartialsMinAux2(const vector<PartialTracker::Partial> &prevPartials,
                                         vector<PartialTracker::Partial> *currentPartials,
                                         vector<PartialTracker::Partial> *remainingPartials)
{
    // Associate partials
    vector<Partial> currentPartialsAssoc;
    
    for (int i = 0; i < currentPartials->size(); i++)
    {
        Partial &currentPartial = (*currentPartials)[i];
        if (currentPartial.mId != -1)
            continue;
        
        // Find the nearest partial (using frequency)
        BL_FLOAT minFreqDiff = BL_INF8;
        int minPartialIdx = -1;
        for (int j = 0; j < prevPartials.size(); j++)
        {
            const Partial &prevPartial = prevPartials[j];
     
            // Check of already associated
            bool alreadyAssociated = false;
            for (int k = 0; k < currentPartialsAssoc.size(); k++)
            {
                if (currentPartialsAssoc[k].mId == prevPartial.mId)
                {
                    alreadyAssociated = true;
                    break;
                }
            }
            if (alreadyAssociated)
                continue;
            
            //BL_FLOAT ampDiff = std::fabs(prevPartial.mAmp - currentPartial.mAmp);
            bool discardByAmp = TestDiscardByAmp(prevPartial, currentPartial);
            BL_FLOAT freqDiff = std::fabs(currentPartial.mFreq - prevPartial.mFreq);
            
            if (//(ampDiff < MAX_AMP_DIFF) &&
                !discardByAmp &&
                (freqDiff < MAX_FREQ_DIFF) &&
                (freqDiff < minFreqDiff))
            {
                minFreqDiff = freqDiff;
                
                minPartialIdx = j;
            }
        }
        
        if (minPartialIdx != -1)
            // Found
        {
            const Partial &prevPartial = prevPartials/*0*/[minPartialIdx];
            
            currentPartial.mId = prevPartial.mId;
            currentPartial.mState = Partial::ALIVE;
            currentPartial.mWasAlive = true;
            
            currentPartialsAssoc.push_back(currentPartial);
        }
    }
    
    remainingPartials->clear();
    for (int i = 0; i < currentPartials->size(); i++)
    {
        const Partial &p = (*currentPartials)[i];
        if (p.mId == -1)
            remainingPartials->push_back(p);
    }
    
    *currentPartials = currentPartialsAssoc;
}

#if 0
void
PartialTracker::AssociatePartialsPermut(const vector<PartialTracker::Partial> &prevPartials,
                                        vector<PartialTracker::Partial> *currentPartials,
                                        vector<PartialTracker::Partial> *remainingPartials)
{
    AssociatePartialsPermutAux1(prevPartials, currentPartials, remainingPartials);
}

void
PartialTracker::AssociatePartialsPermutAux1(const vector<PartialTracker::Partial> &prevPartials,
                                            vector<PartialTracker::Partial> *currentPartials,
                                            vector<PartialTracker::Partial> *remainingPartials)
{
    vector<vector<BL_FLOAT> > distances;
    vector<vector<int> > ids;
    ComputeDistances(prevPartials, *currentPartials, &distances, &ids);
    
    // For 50 partials, and max neightbors 2 this would lead to 2^50 possibilities...
    // => Note possible
}

void
PartialTracker::ComputeDistances(const vector<PartialTracker::Partial> &prevPartials,
                                 const vector<PartialTracker::Partial> &currentPartials,
                                 vector<vector<BL_FLOAT> > *distances,
                                 vector<vector<int> > *ids)
{
    //#define INF 1e15
    
    int numDistances = currentPartials.size();
    if (numDistances > ASSOC_PERMUT_MAX_NEIGHBOURS)
        numDistances = ASSOC_PERMUT_MAX_NEIGHBOURS;
    
    for (int i = 0; i < prevPartials.size(); i++)
    {
        const Partial &prevPartial = prevPartials[i];
        
        // Distances and ids for the current partial
        vector<BL_FLOAT> distances0;
        vector<int> ids0;
        
        // Compute the list of distances for the current partial
        while(distances0.size() < numDistances)
        {
            BL_FLOAT minDist2 = BL_INF;
            BL_FLOAT minId = -1;
            for (int j = 0; j < currentPartials.size(); j++)
            {
                const Partial &currentPartial = currentPartials[i];
                
                // Check if we already have this distance in the list
                bool alreadyComputed = false;
                for (int k = 0; k < ids0.size(); k++)
                {
                    if (ids0[k] == j)
                    {
                        alreadyComputed = true;
                        break;
                    }
                }
                
                if (alreadyComputed)
                    continue;
                
                BL_FLOAT dist = std::fabs(currentPartial.mFreq - prevPartial.mFreq);
                BL_FLOAT dist2 = dist*dist;
                
                if (dist2 < minDist2)
                {
                    minDist2 = dist2;
                    minId = j;
                }
            }
            
            distances0.push_back(minDist2);
            ids0.push_back(minId);
        }
        
        distances->push_back(distances0);
        ids->push_back(ids0);
    }
}
#endif

void
PartialTracker::SmoothPartials(const vector<PartialTracker::Partial> &prevPartials,
                               vector<PartialTracker::Partial> *currentPartials)
{
    for (int i = 0; i < currentPartials->size(); i++)
    {
        Partial &currentPartial = (*currentPartials)[i];
        
        int prevIdx = FindPartialById(prevPartials, currentPartial.mId);
        
        if (prevIdx != -1)
        {
            const Partial &prevPartial = prevPartials[prevIdx];
            
            // Smooth
            
            // Smooth freq
            BL_FLOAT freq0 = prevPartial.mFreq;
            BL_FLOAT freq1 = currentPartial.mFreq;
            
            BL_FLOAT newFreq = FILTER_SMOOTH_COEFF*freq0 + (1.0 - FILTER_SMOOTH_COEFF)*freq1;
            
            currentPartial.mFreq = newFreq;
            
            // Smooth amp
            BL_FLOAT amp0 = prevPartial.mAmp;
            BL_FLOAT amp1 = currentPartial.mAmp;
            
            BL_FLOAT newAmp = FILTER_SMOOTH_COEFF*amp0 + (1.0 - FILTER_SMOOTH_COEFF)*amp1;
            
            currentPartial.mAmp = newAmp;
        }
    }
}

