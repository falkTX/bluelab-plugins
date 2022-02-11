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

#include <Window.h>

#include <BLUtils.h>
#include <BLUtilsMath.h>

#include <BLDebug.h>

#include <CMASmoother.h>

#include "PartialTracker2.h"

#define EPS 1e-15
#define INF 1e15

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
//#define MAX_AMP_DIFF_RATIO 2.0 //1.5 //2.0 //1.0 //4.0 //2.0 //10.0

// Compute simple, avg, or parabola ?
// (Avg seems a little more smooth than parabola)
#define COMPUTE_PEAKS_SIMPLE 0 //0 //1
#define COMPUTE_PEAKS_AVG 1 //0 //1 //0 //1 // prev
#define COMPUTE_PEAKS_PARABOLA 0 //0 //1
#define COMPUTE_PEAKS_AVG2 0 // 0 // 1

// Do we filter ?
#define FILTER_PARTIALS 1 //0 // 1

// Do we smooth when filter ?
#define FILTER_SMOOTH 0 //0 //1
#define FILTER_SMOOTH_COEFF 0.9 //0.8 //0.9

#define HARMONIC_SELECT 0

// Makes wobbling frequencies
#define USE_FREQ_OBJ 0 //0 //1


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

// Minimum spacing between partials in bins
#define APPLY_MIN_SPACING 0
#define MIN_SPACING_BINS  0 //8 //16

// 0.01 => gives good result (avoids "tss" detected as partial)
// 0.005 => avoids missing partials in heigh frequencies
//#define MIN_SPACING_NORM_MEL 0.01
//#define MIN_SPACING_NORM_MEL 0.005
#define MIN_SPACING_NORM_MEL 0.001 // in test...

// 100 Hz
#define MAX_FREQ_DIFF_ASSOC 100.0


// Detect partials
//
#define DETECT_PARTIALS_START_INDEX 2

#define DETECT_PARTIALS_SMOOTH 0

// Take odd size ! (otherwise there will be a shift on the right)
//
// 5: good, but very high freqs are not well detected
// 3: detect well high frequencies, but dos not smooth enough low freqs
//
#define DETECT_PARTIALS_SMOOTH_SIZE 5

// Doesn't work... => TODO: delete this
#define SYMETRISE_PARTIAL_FOOT 0

#define GLUE_BARBS              1 //0 // 1
#define GLUE_BARBS_AMP_RATIO    10.0 //4.0

//#define GLUE_BARBS_WIDTH_COEFF  1.0
//#define GLUE_BARBS_HEIGHT_COEFF 4.0 //2.0

// With 1, that made more defined partials
// With 0, avoids partial leaking in noise ("oohoo")
#define NARROW_PARTIAL_FOOT 0 //1 //0
#define NARROW_PARTIAL_FOOT_COEFF 20.0 //40.0

#define DISCARD_FLAT_PARTIAL 1
#define DISCARD_FLAT_PARTIAL_COEFF 25000.0 //30000.0 //20000.0 //10000.0 //30000.0 //20000.0 //10000.0 //20000.0 // 40000.0


// Threshold partials
//
#define THRESHOLD_MIN        1 // good
#define THRESHOLD_PROMINENCE 0 // better (but suppresses partial in case of "double head"
#define THRESHOLD_SMOOTH     0 // ? => some better, some worse ##
#define THRESHOLD_AUTO       0 // Not working

#define THRESHOLD_PARTIALS_SMOOTH_SIZE   21
#define THRESHOLD_PARTIALS_AUTO_WIN_SIZE 21

#define GLUE_TWIN_PARTIALS 0

// Extract noise envelope
//
#define EXTRACT_NOISE_ENVELOPE_MAX    0
#define EXTRACT_NOISE_ENVELOPE_SMOOTH 0 // not working well
#define EXTRACT_NOISE_ENVELOPE_SELECT 0
#define EXTRACT_NOISE_ENVELOPE_TRACK  0 // prev
#define EXTRACT_NOISE_ENVELOPE_TEST   1 // new (fixed)



// NOTE: See: https://www.dsprelated.com/freebooks/sasp/Spectral_Modeling_Synthesis.html
//
// and: https://www.dsprelated.com/freebooks/sasp/PARSHL_Program.html#app:parshlapp
//

unsigned long PartialTracker2::Partial::mCurrentId = 0;



PartialTracker2::Partial::Partial()
#if PT2_PREDICTIVE
: mKf(KF_E_MEA, KF_E_EST, KF_Q)
#endif
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
    
#if PT2_PREDICTIVE
    mPredictedFreq = 0.0;
#endif
}

    
PartialTracker2::Partial::Partial(const Partial &other)
#if PT2_PREDICTIVE
: mKf(other.mKf)
#endif
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
    
#if PT2_PREDICTIVE
    mPredictedFreq = other.mPredictedFreq;
#endif
}

PartialTracker2::Partial::~Partial() {}

void
PartialTracker2::Partial::GenNewId()
{
    mId = mCurrentId++;
}
    
BL_FLOAT
PartialTracker2::Partial::ComputeDistance2(const Partial &partial0,
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
PartialTracker2::Partial::FreqLess(const Partial &p1, const Partial &p2)
{
    return (p1.mFreq < p2.mFreq);
}

bool
PartialTracker2::Partial::AmpLess(const Partial &p1, const Partial &p2)
{
    return (p1.mAmp < p2.mAmp);
}

bool
PartialTracker2::Partial::IdLess(const Partial &p1, const Partial &p2)
{
    return (p1.mId < p2.mId);
}

PartialTracker2::PartialTracker2(int bufferSize, BL_FLOAT sampleRate,
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

    int dummyWinSize = 5;
    mSmoother = new CMASmoother(bufferSize, dummyWinSize);
}

PartialTracker2::~PartialTracker2()
{
    if (mFreqObj != NULL)
        delete mFreqObj;

    delete mSmoother;
}

void
PartialTracker2::Reset()
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
PartialTracker2::SetThreshold(BL_FLOAT threshold)
{
    mThreshold = threshold;
}

void
PartialTracker2::SetData(const WDL_TypedBuf<BL_FLOAT> &magns,
                        const WDL_TypedBuf<BL_FLOAT> &phases)
{
    mCurrentMagns = magns;
    mCurrentPhases = phases;
}

void
PartialTracker2::DetectPartials()
{
    WDL_TypedBuf<BL_FLOAT> magns0 = mCurrentMagns;
    
#if 0
    ////
    BLDebug::DumpData("magns.txt", magns0);
    
    WDL_TypedBuf<BL_FLOAT> magnsDB;
    BLUtils::AmpToDB(&magnsDB, magns0);
    BLDebug::DumpData("magns-db.txt", magnsDB);
    
    WDL_TypedBuf<BL_FLOAT> magnsMel;
    BL_FLOAT hzPerBin = mSampleRate/mBufferSize;
    BLUtils::FreqsToMelNorm(&magnsMel, magnsDB, hzPerBin);
    BLDebug::DumpData("magns-mel.txt", magnsMel);
    ////
#endif
    
    if (mFreqObj != NULL)
    {
        // See: http://blogs.zynaptiq.com/bernsee/pitch-shifting-using-the-ft/
        mFreqObj->ComputeRealFrequencies(mCurrentPhases, &mRealFreqs);
    }
    
    vector<Partial> partials;
    
//#if !DETECT_PARTIALS_SMOOTH
    DetectPartials(magns0, mCurrentPhases, &partials);
//#else
//    // Smoothed version
//    if (mSmoothWinDetect.GetSize() != DETECT_PARTIALS_SMOOTH_SIZE)
//    {
//        Window::MakeHanning(DETECT_PARTIALS_SMOOTH_SIZE, &mSmoothWinDetect);
//    }
//
//    BLUtils::SmoothDataWin(&mCurrentSmoothMagns, magns0, mSmoothWinDetect);
//    DetectPartialsSmooth(magns0, mCurrentPhases, mCurrentSmoothMagns, &partials);
//#endif
    
#if GLUE_BARBS
    vector<Partial> prev = partials;
    
    bool glued = GluePartialBarbs(magns0, &partials);
    
#if 0
    if (glued)
    {
        BLDebug::DumpData("magns.txt", mCurrentMagns);
        
        DBG_DumpPartials2("partials0.txt", prev);
        DBG_DumpPartialsBox("box0.txt", prev, mBufferSize, mSampleRate);
        
        DBG_DumpPartials2("partials1.txt", partials);
        DBG_DumpPartialsBox("box1.txt", partials, mBufferSize, mSampleRate);
        
        int dummy = 0;
    }
#endif
    
#endif
    
#if NARROW_PARTIAL_FOOT
    NarrowPartialFoot(magns0, &partials);
#endif
    
#if DISCARD_FLAT_PARTIAL
    //TEST DiscardFlatPartials(magns0, &partials);
#endif

    
    // Remove small partials before gluing the twins
    // => this avoids gluing many small partials together
#if THRESHOLD_MIN
    ThresholdPartialsAmp(&partials);
#endif

#if THRESHOLD_PROMINENCE
    ThresholdPartialsAmpProminence(magns0, &partials);
#endif

#if THRESHOLD_SMOOTH
    ThresholdPartialsAmpSmooth(magns0, &partials);
#endif

#if THRESHOLD_AUTO
    ThresholdPartialsAmpAuto(magns0, &partials);
#endif
    
    // "tibetan bell": avoids keeping a partial at 2KHz in the noise envelope
    //
    // With "bell" => we loose the first partial
    // (glued with the second)
#if 0
    GlueTwinPartials(magns0, &partials);
#endif
    
    mPartials.push_front(partials);
    
    while(mPartials.size() > HISTORY_SIZE)
        mPartials.pop_back();
    
    mResult = partials;
}

void
PartialTracker2::ExtractNoiseEnvelope()
{
#if EXTRACT_NOISE_ENVELOPE_MAX
    ExtractNoiseEnvelopeMax();
#endif
    
#if EXTRACT_NOISE_ENVELOPE_SMOOTH
    ExtractNoiseEnvelopeSmooth();
#endif
    
#if EXTRACT_NOISE_ENVELOPE_SELECT
    ExtractNoiseEnvelopeSelect();
#endif

#if EXTRACT_NOISE_ENVELOPE_TRACK
    ExtractNoiseEnvelopeTrack();
#endif
    
#if EXTRACT_NOISE_ENVELOPE_TEST
    ExtractNoiseEnvelopeTest();
#endif
}

void
PartialTracker2::ExtractNoiseEnvelopeMax()
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

#if 0
void
PartialTracker2::ExtractNoiseEnvelopeSelect()
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
    BLUtils::MultValues(&noiseEnv, DEBUG_COEFF_EXTRACT_HARMO);
    
    // Compute harmonic envelope
    // (origin signal less noise)
    mHarmonicEnvelope = mCurrentMagns;
    BLUtils::SubstractValues(&mHarmonicEnvelope, noiseEnv);
    
    BLUtils::ClipMin(&mHarmonicEnvelope, 0.0);
}
#endif

void
PartialTracker2::ExtractNoiseEnvelopeTrack()
{
    // If not partial detected, noise envelope is simply the input magns
    mNoiseEnvelope = mCurrentMagns;
    
    vector<Partial> partials;
    
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
    //GlueTwinPartials(&partials);
    GlueTwinPartials(mCurrentMagns, &partials);
#endif
    
    CutPartials(partials, &mNoiseEnvelope);
    // CutPartialsMinEnv(&mNoiseEnvelope); ##
    
    // Compute harmonic envelope
    // (origin signal less noise)
    mHarmonicEnvelope = mCurrentMagns;
    BLUtils::SubstractValues(&mHarmonicEnvelope, mNoiseEnvelope);
    
    BLUtils::ClipMin(&mHarmonicEnvelope, (BL_FLOAT)0.0);
}

void
PartialTracker2::ExtractNoiseEnvelopeTest()
{
#if 0
    static int count = 0;
    count++;
    if (count == 90)
    {
        int dummy = 0;
    }
    
    //fprintf(stderr, "count: %d\n", count);
#endif
    
    // If not partial detected, noise envelope is simply the input magns
    //mNoiseEnvelope = mCurrentMagns;
    
    vector<Partial> partials;
    
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
//#if GLUE_TWIN_PARTIALS
    //GlueTwinPartials(&partials);
    //GlueTwinPartials(mCurrentMagns, &partials);
//#endif
    
    //CutPartials(partials, &mNoiseEnvelope);
    // CutPartialsMinEnv(&mNoiseEnvelope); ## OLD
    
    mHarmonicEnvelope = mCurrentMagns;
    
    // Just in case
    for (int i = 0; i < DETECT_PARTIALS_START_INDEX; i++)
        mHarmonicEnvelope.Get()[i] = 0.0;
    
    KeepOnlyPartials(partials, &mHarmonicEnvelope);
    

    // Compute harmonic envelope
    // (origin signal less noise)
    mNoiseEnvelope = mCurrentMagns;
    BLUtils::SubstractValues(&mNoiseEnvelope, mHarmonicEnvelope);
    
    BLUtils::ClipMin(&mNoiseEnvelope,(BL_FLOAT)0.0);
    
    // Set the first values to 0
    // (avoids very big first value, that would make
    // a big and large noise envelope at the beginning)
    //for (int i = 0; i < DETECT_PARTIALS_START_INDEX; i++)
    //    mNoiseEnvelope.Get()[i] = 0.0;
    
    // BUG: some small spots remain ("alphabet, A")
    // TODO: re-inject the remaining in the harmonic envelope
    //ZeroToNextNoiseMinimum(&mNoiseEnvelope);
    
    // Avoids interpolation from 0 to the first valid index
    // (could have made an artificial increasing slope in the low freqs)
    for (int i = 0; i < mNoiseEnvelope.GetSize(); i++)
    {
        BL_FLOAT val = mNoiseEnvelope.Get()[i];
        if (val > EPS)
            // First value
        {
            int prevIdx = i - 1;
            if (prevIdx > 0)
            {
                mNoiseEnvelope.Get()[prevIdx] = EPS;
            }
            
            break;
        }
    }
    
    // Create an envelope
    // NOTE: good for "oohoo", not good for "alphabet A"
    BLUtils::FillMissingValues(&mNoiseEnvelope, false, (BL_FLOAT)0.0);
    
    //SmoothNoiseEnvelope(&mNoiseEnvelope);
}

// Set to 0 until the next minimum
// Avoids "half partials" at the beginning that would
// not be detected, and finish in the noise envelope
void
PartialTracker2::ZeroToNextNoiseMinimum(WDL_TypedBuf<BL_FLOAT> *noise)
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
PartialTracker2::SmoothNoiseEnvelope(WDL_TypedBuf<BL_FLOAT> *noise)
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
    
    WDL_TypedBuf<BL_FLOAT> smoothNoise;
    BLUtils::SmoothDataWin(&smoothNoise, *noise, mSmoothWinNoise);
    
    *noise = smoothNoise;
}

// NOTE: Doesn't work well
void
PartialTracker2::ExtractNoiseEnvelopeSmooth()
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
PartialTracker2::CutPartials(const vector<Partial> &partials,
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
PartialTracker2::KeepOnlyPartials(const vector<Partial> &partials,
                                  WDL_TypedBuf<BL_FLOAT> *magns)
{
    WDL_TypedBuf<BL_FLOAT> result;
    BLUtils::ResizeFillZeros(&result, magns->GetSize());
                   
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
PartialTracker2::CutPartialsMinEnv(WDL_TypedBuf<BL_FLOAT> *magns)
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
    WDL_TypedBuf<BL_FLOAT> minEnv;
    BLUtils::ResizeFillZeros(&minEnv, magns->GetSize());
    
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
PartialTracker2::FilterPartials()
{
    // DEBUG
    //mResult = mPartials[0];
    //return;
    
#if !FILTER_PARTIALS // Not filtered
    mResult = mPartials[0];
#else // Filtered
    FilterPartials(&mResult);
#endif
}

// For noise envelope extraction, the
// state must be ALIVE, and not mWasAlive
bool
PartialTracker2::GetAlivePartials(vector<Partial> *partials)
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

bool
PartialTracker2::GetPartials(vector<Partial> *partials)
{
    *partials = mResult;
    
    return true;
}

void
PartialTracker2::GetNoiseEnvelope(WDL_TypedBuf<BL_FLOAT> *noiseEnv)
{
    *noiseEnv = mNoiseEnvelope;
}

void
PartialTracker2::GetHarmonicEnvelope(WDL_TypedBuf<BL_FLOAT> *harmoEnv)
{
    *harmoEnv = mHarmonicEnvelope;
}

void
PartialTracker2::RemoveRealDeadPartials(vector<Partial> *partials)
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
PartialTracker2::DBG_DumpPartials(const vector<Partial> &partials,
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
PartialTracker2::DBG_DumpPartialsAmp(const vector<Partial> &partials,
                                     int bufferSize, BL_FLOAT sampleRate)
{
    DBG_DumpPartialsAmp("partials.txt", partials, bufferSize, sampleRate);
}

void
PartialTracker2::DBG_DumpPartialsAmp(const char *fileName,
                                     const vector<Partial> &partials,
                                     int bufferSize, BL_FLOAT sampleRate)
{
#define NUM_MAGNS 1024
    
    BL_FLOAT hzPerBin = sampleRate/bufferSize;
    
    WDL_TypedBuf<BL_FLOAT> result;
    result.Resize(NUM_MAGNS);
    BLUtils::FillAllZero(&result);
    
    for (int i = 0; i < partials.size(); i++)
    {
        const PartialTracker2::Partial &p = partials[i];
        
        BL_FLOAT amp = p.mAmp;
        
        BL_FLOAT binNum = p.mFreq/hzPerBin;
        
        // Take the nearest bin
        // (will be better for display)
        binNum = bl_round(binNum);
        
        if (binNum < result.GetSize())
        {
            result.Get()[(int)binNum] = amp;
        }
    }
    
    BLDebug::DumpData(fileName, result);
}

void
PartialTracker2::DBG_DumpPartialsBox(const vector<Partial> &partials,
                                     int bufferSize, BL_FLOAT sampleRate)
{
    DBG_DumpPartialsBox("partials-box.txt", partials,
                        bufferSize, sampleRate);
}

void
PartialTracker2::DBG_DumpPartialsBox(const char *fileName,
                                     const vector<Partial> &partials,
                                     int bufferSize, BL_FLOAT sampleRate)
{
#define NUM_MAGNS 1024
    
    BL_FLOAT hzPerBin = sampleRate/bufferSize;
    
    WDL_TypedBuf<BL_FLOAT> result;
    result.Resize(NUM_MAGNS);
    BLUtils::FillAllZero(&result);
    
    for (int i = 0; i < partials.size(); i++)
    {
        const PartialTracker2::Partial &p = partials[i];
        
        BL_FLOAT amp = p.mAmp;
        
        for (int j = p.mLeftIndex; j <= p.mRightIndex; j++)
        {
            result.Get()[j] = amp;
        }
    }
    
    BLDebug::DumpData(fileName, result);
}

void
PartialTracker2::DBG_DumpPartials2(const vector<Partial> &partials)
{
    DBG_DumpPartials2("partials.txt", partials);
}

// Same method as above (implemented twice...)
// (but better code)
void
PartialTracker2::DBG_DumpPartials2(const char *fileName,
                                   const vector<Partial> &partials)
{
    BL_FLOAT hzPerBin = mSampleRate/mBufferSize;
    
    WDL_TypedBuf<BL_FLOAT> buffer;
    BLUtils::ResizeFillZeros(&buffer, mBufferSize/2);
    
    for (int i = 0; i < partials.size(); i++)
    {
        const PartialTracker2::Partial &p = partials[i];
        
        BL_FLOAT binNum = p.mFreq/hzPerBin;
        binNum = bl_round(binNum);
        
        BL_FLOAT amp = p.mAmp;
        
        buffer.Get()[(int)binNum] = amp;
    }
    
    BLDebug::DumpData(fileName, buffer);
}

// Simple version
void
PartialTracker2::DetectPartials(const WDL_TypedBuf<BL_FLOAT> &magns,
                                const WDL_TypedBuf<BL_FLOAT> &phases,
                                vector<Partial> *outPartials)
{
#define EPS_PARTIAL 1e-8
    
    outPartials->clear();
    
    //
    WDL_TypedBuf<BL_FLOAT> smoothMagns = magns;
    
#if DETECT_PARTIALS_SMOOTH
    // Get a smoothed version of the magns
    if (mSmoothWinDetect.GetSize() != DETECT_PARTIALS_SMOOTH_SIZE)
    {
        Window::MakeHanning(DETECT_PARTIALS_SMOOTH_SIZE, &mSmoothWinDetect);
        
        // Works well too
        //
        // See: https://en.wikipedia.org/wiki/Window_function
        //
        //BL_FLOAT sigma = 0.1;
        //Window::MakeGaussian2(DETECT_PARTIALS_SMOOTH_SIZE, sigma, &mSmoothWinDetect);
    }
    
    BLUtils::SmoothDataWin(&smoothMagns, magns, mSmoothWinDetect);
    
#endif
    
    //
    // prevIndex0, prevIndex1 (max), current
    
    // Skip the first ones
    // (to avoid artifacts of very low freq partial)
    //int currentIndex = 0;
    int currentIndex = DETECT_PARTIALS_START_INDEX;
    
    BL_FLOAT prevVal0 = 0.0;
    BL_FLOAT prevVal1 = 0.0;
    while(currentIndex < smoothMagns.GetSize())
    {
        BL_FLOAT currentVal = smoothMagns.Get()[currentIndex];
        
        if ((prevVal1 > prevVal0) && (prevVal1 > currentVal))
            // Maximum found
        {
            if (currentIndex - 1 >= 0)
            {
                // Take the left and right "feets" of the partial,
                // then the middle.
                // (in order to be more precise)
            
                // Left
                int leftIndex = currentIndex - 2; //
                if (leftIndex > 0)
                {
                    BL_FLOAT prevLeftVal = smoothMagns.Get()[leftIndex];
                    while(leftIndex > 0)
                    {
                        leftIndex--;
                    
                        BL_FLOAT leftVal = smoothMagns.Get()[leftIndex];
                    
                        // Stop if we reach 0 or if it goes up again
                        if ((leftVal < EPS_PARTIAL) || (leftVal >= prevLeftVal))
                        {
                            if (leftVal >= prevLeftVal)
                                leftIndex++;
                        
                            // Check bounds
                            if (leftIndex < 0)
                                leftIndex = 0;
                            if (leftIndex >= smoothMagns.GetSize())
                                leftIndex = smoothMagns.GetSize() - 1;
                        
                            break;
                        }
                        
                        prevLeftVal = leftVal;
                    }
                }
            
                // Right
                int rightIndex = currentIndex; //
                if (rightIndex < smoothMagns.GetSize())
                {
                    BL_FLOAT prevRightVal = smoothMagns.Get()[rightIndex];
                    while(rightIndex < smoothMagns.GetSize() - 1)
                    {
                        rightIndex++;
                    
                        BL_FLOAT rightVal = smoothMagns.Get()[rightIndex];
                    
                        // Stop if we reach 0 or if it goes up again
                        if ((rightVal < EPS_PARTIAL) || (rightVal >= prevRightVal))
                        {
                            if (rightVal >= prevRightVal)
                                rightIndex--;
                        
                            // Check bounds
                            if (rightIndex < 0)
                                rightIndex = 0;
                            if (rightIndex >= smoothMagns.GetSize())
                                rightIndex = smoothMagns.GetSize() - 1;
                        
                            break;
                        }
                        
                        prevRightVal = rightVal;
                    }
                }
            
                // Take the max (better than taking the middle)
                int peakIndex = currentIndex - 1;
            
                if ((peakIndex < 0) || (peakIndex >= magns.GetSize()))
                    // Out of bounds
                    continue;
                
#if SYMETRISE_PARTIAL_FOOT
                SymetrisePartialFoot(peakIndex, &leftIndex, &rightIndex);
#endif
                
#if NARROW_PARTIAL_FOOT
                //NarrowPartialFoot(magns, peakIndex, &leftIndex, &rightIndex);
#endif
                
                bool discard = false;
                
#if DISCARD_FLAT_PARTIAL
                //discard = DiscardFlatPartial(magns, peakIndex, leftIndex, rightIndex);
#endif
                
                if (!discard)
                {
                    // Create new partial
                    //
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

#if PT2_PREDICTIVE
                    // Update the estimate with the first value
                    p.mKf.updateEstimate(p.mFreq);
                
                    // For predicted freq to be freq for the first value
                    p.mPredictedFreq = p.mFreq;
#endif
                
                    // Default value. will be overwritten
                    BL_FLOAT peakAmp = magns.Get()[peakIndex];
                
                    if (mFreqObj == NULL)
                        peakAmp = ComputePeakAmpInterp(magns, peakFreq);
                    else
                        peakAmp = ComputePeakAmpInterpFreqObj(magns, peakFreq);
            
                    p.mAmp = peakAmp;
                
                    // TODO: later, manage the phases, for transient in input signal
                    //p.mPhase = phases.Get()[peakIndex];
            
                    outPartials->push_back(p);
                }
                
                // Go just after the right foot of the partial
                currentIndex = rightIndex;
            }
        }
        
        currentIndex++;
        
        if (currentIndex - 1 >= 0)
            prevVal1 = magns.Get()[currentIndex - 1];
        
        if (currentIndex - 2 >= 0)
            prevVal0 = magns.Get()[currentIndex - 2];
    }
}

// Extend the foot that is closer to the peak,
// to get a symetric partial box (around the peak)
// (avoids one foot blocked by a barb)
void
PartialTracker2::SymetrisePartialFoot(int peakIndex,
                                      int *leftIndex, int *rightIndex)
{
    int diff0 = peakIndex - *leftIndex;
    int diff1 = *rightIndex - peakIndex;
    
    if (diff0 < diff1)
    {
        *leftIndex = peakIndex - diff1;
    }
    
    if (diff1 < diff0)
    {
        *rightIndex = peakIndex + diff1;
    }
}

#if 0 // not debugged
void
PartialTracker2::GluePartialBarbs(vector<Partial> *partials)
{    
    vector<Partial> result;
    vector<Partial> barbs;
    for (int i = 0; i < partials->size(); i++)
    {
        // Barb ?
        const Partial &partial = (*partials)[i];
        
        // Check if the partial is a barb
        bool isBarb = false;
        for (int j = 0; j < partials->size(); j++)
        {
            // Check other partials
            const Partial &other = (*partials)[j];
            
            if (partial.mAmp*GLUE_BARBS_HEIGHT_COEFF > other.mAmp)
                // Amplitudes of tested partial is not small enough
                // compared to the current partial
                // => Not a candidate for barb
                continue;
            
            int center = other.mPeakIndex;
            int size = other.mRightIndex - other.mLeftIndex;
            
            if ((partial.mPeakIndex > center - size*GLUE_BARBS_WIDTH_COEFF) &&
                (partial.mPeakIndex < center + size*GLUE_BARBS_WIDTH_COEFF))
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
        else
            barbs.push_back(partial);
    }
    
    // Glue the barbs
    for (int i = 0; i < barbs.size(); i++)
    {
        const Partial &barb = barbs[i];
        
        for (int j = 0; j < result.size(); j++)
        {
            Partial &partial = result[j];
            
            // Try to glue on the left
            if (barb.mRightIndex == partial.mLeftIndex)
            {
                partial.mLeftIndex = barb.mLeftIndex;
                
                break;
            }
            
            // Try to glue on the right
            if (barb.mLeftIndex == partial.mRightIndex)
            {
                partial.mRightIndex = barb.mRightIndex;
                
                break;
            }
        }
    }
    
    *partials = result;
}
#endif


// From GlueTwinPartials()
bool
PartialTracker2::GluePartialBarbs(const WDL_TypedBuf<BL_FLOAT> &magns,
                                  vector<Partial> *partials)
{
    vector<Partial> result;
    bool glued = false;
    
    BL_FLOAT hzPerBin = mSampleRate/mBufferSize;
    
    sort(partials->begin(), partials->end(), Partial::FreqLess);
    
    int idx = 0;
    while(idx < partials->size())
    {
        Partial currentPartial = (*partials)[idx];
        
        vector<Partial> twinPartials;
        twinPartials.push_back(currentPartial);
        
        for (int j = idx + 1; j < partials->size(); j++)
        {
            const Partial &otherPartial = (*partials)[j];
            
            if (otherPartial.mLeftIndex == currentPartial.mRightIndex)
            // This is a twin partial...
            {
                BL_FLOAT promCur = ComputeProminence2(magns,
                                                    currentPartial.mPeakIndex,
                                                    currentPartial.mLeftIndex,
                                                    currentPartial.mRightIndex);
                
                BL_FLOAT promOther = ComputeProminence2(magns,
                                                      otherPartial.mPeakIndex,
                                                      otherPartial.mLeftIndex,
                                                      otherPartial.mRightIndex);
                
                // Default ratio value
                // If it keeps this value, this is ok, this will be glued
                BL_FLOAT ratio = 0.0;
                if (promOther > EPS)
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
                            BL_FLOAT hf = ComputeHigherFoot(magns,
                                                          currentPartial.mLeftIndex,
                                                          currentPartial.mRightIndex);

                            
                            BL_FLOAT lf = ComputeLowerFoot(magns,
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
                            BL_FLOAT hf = ComputeHigherFoot(magns,
                                                          otherPartial.mLeftIndex,
                                                          otherPartial.mRightIndex);
                            
                            
                            BL_FLOAT lf = ComputeLowerFoot(magns,
                                                         currentPartial.mLeftIndex,
                                                         currentPartial.mRightIndex);
                            
                            if ((hf > lf) && (hf < currentPartial.mAmp))
                                inTheMiddle = true;
                            
                            // Check that the barb is on the right side
                            BL_FLOAT curLeftFoot = magns.Get()[currentPartial.mLeftIndex];
                            BL_FLOAT curRightFoot = magns.Get()[currentPartial.mRightIndex];
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
        
        //
        // Glue ?
        //
        
        if (twinPartials.size() > 1)
        {
            glued = true;
            
            // Compute glued partial
            int leftIndex = twinPartials[0].mLeftIndex;
            int rightIndex = twinPartials[twinPartials.size() - 1].mRightIndex;
            
            BL_FLOAT peakFreq = ComputePeakFreqAvg(magns, leftIndex, rightIndex);
            
            // For peak amp, take max amp
            BL_FLOAT maxAmp = 0.0;
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
            res.mPeakIndex = peakFreq/hzPerBin;
            
            res.mFreq = peakFreq;
            res.mAmp = maxAmp;
            
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
PartialTracker2::NarrowPartialFoot(const WDL_TypedBuf<BL_FLOAT> &magns,
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
PartialTracker2::NarrowPartialFoot(const WDL_TypedBuf<BL_FLOAT> &magns,
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
PartialTracker2::DiscardFlatPartial(const WDL_TypedBuf<BL_FLOAT> &magns,
                                    int peakIndex, int leftIndex, int rightIndex)
{
    // With prominence, this would suppress a partial if it has "2 heads"
    //BL_FLOAT amp = ComputeProminence2(magns, peakIndex,
    //                                leftIndex, rightIndex);
    
    BL_FLOAT amp = magns.Get()[peakIndex];
    
    BL_FLOAT binDiff = rightIndex - leftIndex;
    
    BL_FLOAT coeff = binDiff/amp;
    
    bool result = (coeff > DISCARD_FLAT_PARTIAL_COEFF);
    
    return result;
}

void
PartialTracker2::DiscardFlatPartials(const WDL_TypedBuf<BL_FLOAT> &magns,
                                     vector<Partial> *partials)
{
    vector<Partial> result;
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

#if 0
// Version that detect extrema on the smoothed magns data
//
// NOTE: better than simple version
// => avoids low freq leaks in noise envelope
void
PartialTracker2::DetectPartialsSmooth(const WDL_TypedBuf<BL_FLOAT> &magns,
                                      const WDL_TypedBuf<BL_FLOAT> &phases,
                                      const WDL_TypedBuf<BL_FLOAT> &smoothMagns,
                                      vector<Partial> *outPartials)
{
#define EPS_PARTIAL 1e-8
    
    outPartials->clear();
    
    //
    // prevIndex0, prevIndex1 (max), current
    
    // Skip the first ones
    // (to avoid artifacts of very low freq partial)
    //int currentIndex = 0;
    int currentIndex = 2;
    
    BL_FLOAT prevVal0 = 0.0;
    BL_FLOAT prevVal1 = 0.0;
    BL_FLOAT currentVal = 0.0;
    
    // Do not compute the last one
    // (because parabola peak computing needs the next value)
    while(currentIndex < smoothMagns.GetSize() - 1)
    {
        if ((prevVal1 > prevVal0) && (prevVal1 > currentVal))
            // Maximum found
        {
            if (currentIndex - 1 >= 0)
            {
                // Take the left and right "feets" of the partial,
                // then the middle.
                // (in order to be more precise)
                
                // Left
                int leftIndex = currentIndex - 2; //- 1;
                if (leftIndex > 0)
                {
                    BL_FLOAT prevLeftVal = smoothMagns.Get()[leftIndex];
                    while(leftIndex > 0)
                    {
                        leftIndex--;
                        
                        BL_FLOAT leftVal = smoothMagns.Get()[leftIndex];
                        
                        // Stop if we reach 0 or if it goes up again
                        if ((leftVal < EPS_PARTIAL) || (leftVal >= prevLeftVal))
                        {
                            if (leftVal >= prevLeftVal)
                                leftIndex++;
                            
                            // Check bounds
                            if (leftIndex < 0)
                                leftIndex = 0;
                            if (leftIndex >= smoothMagns.GetSize())
                                leftIndex = smoothMagns.GetSize() - 1;
                            
                            break;
                        }
                        
                        prevLeftVal = leftVal;
                    }
                }
                
                // Right
                int rightIndex = currentIndex; // + 1;
                if (rightIndex < smoothMagns.GetSize())
                {
                    BL_FLOAT prevRightVal = smoothMagns.Get()[rightIndex];
                    while(rightIndex < smoothMagns.GetSize() - 1)
                    {
                        rightIndex++;
                        
                        BL_FLOAT rightVal = smoothMagns.Get()[rightIndex];
                        
                        // Stop if we reach 0 or if it goes up again
                        if ((rightVal < EPS_PARTIAL) || (rightVal >= prevRightVal))
                        {
                            if (rightVal >= prevRightVal)
                                rightIndex--;
                            
                            // Check bounds
                            if (rightIndex < 0)
                                rightIndex = 0;
                            if (rightIndex >= smoothMagns.GetSize())
                                rightIndex = smoothMagns.GetSize() - 1;
                            
                            break;
                        }
                        
                        prevRightVal = rightVal;
                    }
                }
                
                // Take the max
                int peakIndex = currentIndex;
                
                if ((peakIndex < 0) || (peakIndex >= magns.GetSize()))
                    // Out of bounds
                    continue;
                
                // Discard partial ?
                bool discard = false;
                
                // Create new partial
                Partial p;
                
                // HACK !
                //
                // Don't know why we should remove -1 ...
                p.mPeakIndex = peakIndex - 1;
                if (p.mPeakIndex < 0)
                    p.mPeakIndex = 0;
                
                p.mLeftIndex = leftIndex - 1;
                if (p.mLeftIndex < 0)
                    p.mLeftIndex = 0;
                
                p.mRightIndex = rightIndex - 1;
                if (p.mRightIndex < 0)
                    p.mRightIndex = 0;
                
                // OLD
                // Make smooth partials change
                // But makes wobble in the sound volume
                //BL_FLOAT peakFreq = ComputePeakFreqAvg(magns, leftIndex, rightIndex);
                
                // More accurate than above
                // (Since we now use smooth mangns, and peaks can expand
                BL_FLOAT peakFreq = ComputePeakFreqParabola(smoothMagns, peakIndex);
                
                p.mFreq = peakFreq;
                
#if PT2_PREDICTIVE
                // Update the estimate with the first value
                p.mKf.updateEstimate(p.mFreq);
                
                // For predicted freq to be freq for the first value
                p.mPredictedFreq = p.mFreq;
#endif
                
                // Default value. will be overwritten
                BL_FLOAT peakAmp = magns.Get()[peakIndex];
                
                if (mFreqObj == NULL)
                    peakAmp = ComputePeakAmpInterp(magns, peakFreq);
                else
                    peakAmp = ComputePeakAmpInterpFreqObj(magns, peakFreq);
                
                p.mAmp = peakAmp;
                
                //p.mPhase = phases.Get()[peakIndex];
                
                if (!discard)
                    outPartials->push_back(p);
                
                // Go just after the right foot of the partial
                currentIndex = rightIndex;
            }
        }
        
        currentIndex++;
        
        // Update the values
        currentVal = smoothMagns.Get()[currentIndex];
        
        if (currentIndex - 1 >= 0)
            prevVal1 = smoothMagns.Get()[currentIndex - 1];
        
        if (currentIndex - 2 >= 0)
            prevVal0 = smoothMagns.Get()[currentIndex - 2];
    }
}
#endif


// Works well to suppress the "tsss"
// while keeping the low frequencies partials
// Mel works better than linear scale
void
PartialTracker2::ApplyMinSpacingMel(vector<Partial> *partials)
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

// For Air + keep only noise + Tibetan bowl: avoids a partial remaining at 2KHz
void
PartialTracker2::GlueTwinPartials(const WDL_TypedBuf<BL_FLOAT> &magns,
                                  vector<Partial> *partials)
{
    vector<Partial> result;
    
    BL_FLOAT hzPerBin = mSampleRate/mBufferSize;
    
    sort(partials->begin(), partials->end(), Partial::FreqLess);
    
    int idx = 0;
    while(idx < partials->size())
    {
        Partial currentPartial = (*partials)[idx];
        
        vector<Partial> twinPartials;
        twinPartials.push_back(currentPartial);
        
        for (int j = idx + 1; j < partials->size(); j++)
        {
            const Partial &other = (*partials)[j];
            
            if (other.mLeftIndex == currentPartial.mRightIndex)
                // This is a twin partial
            {
                twinPartials.push_back(other);
                
                // Iterate
                // (may glue more than 2 partials)
                currentPartial = other;
            }
            else
            {
                // Not twin, continue over next partials
                break;
            }
        }
        
        if (twinPartials.size() > 1)
        {
            // Compute glued partial
            int leftIndex = twinPartials[0].mLeftIndex;
            int rightIndex = twinPartials[twinPartials.size() - 1].mRightIndex;
            
            BL_FLOAT peakFreq = ComputePeakFreqAvg(magns, leftIndex, rightIndex);
            
            // For peak amp, take max amp
            BL_FLOAT maxAmp = 0.0;
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
            res.mPeakIndex = peakFreq/hzPerBin;
            
            res.mFreq = peakFreq;
            res.mAmp = maxAmp;
            
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
}

// Other version (works less)
// => center of partials are not correct
// (because not ponderated by magnitudes)
void
PartialTracker2::GlueTwinPartials(vector<Partial> *partials)
{
    vector<Partial> result;
    
    BL_FLOAT hzPerBin = mSampleRate/mBufferSize;
    
    sort(partials->begin(), partials->end(), Partial::FreqLess);
    
    int idx = 0;
    while(idx < partials->size())
    {
        Partial currentPartial = (*partials)[idx];
        
        vector<Partial> twinPartials;
        twinPartials.push_back(currentPartial);
        
        for (int j = idx + 1; j < partials->size(); j++)
        {
            const Partial &other = (*partials)[j];
            
            if (other.mLeftIndex == currentPartial.mRightIndex)
                // This is a twin partial
            {
                twinPartials.push_back(other);
                
                // Iterate
                // (may glue more than 2 partials)
                currentPartial = other;
            }
            else
            {
                // Not twin, continue over next partials
                break;
            }
        }
        
        if (twinPartials.size() > 1)
        {
            // Compute glued partial
            int leftIndex = twinPartials[0].mLeftIndex;
            int rightIndex = twinPartials[twinPartials.size() - 1].mRightIndex;
            
            int peakIndex = (leftIndex + rightIndex)/2;
            
            BL_FLOAT peakFreq = peakIndex*hzPerBin;
            
            // For peak amp, take max amp
            BL_FLOAT maxAmp = 0.0;
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
            
            res.mFreq = peakFreq;
            res.mAmp = maxAmp;
            
            // Do not set mPhase for now
            
            result.push_back(res);
        }
        else
            // Not twin, simply add the partial
        {
            result.push_back(currentPartial);
        }
        
        // 1 or more
        idx += twinPartials.size();
    }
    
    *partials = result;
}

void
PartialTracker2::SuppressBadPartials(vector<Partial> *partials)
{
    vector<Partial> result;
    for (int i = 0; i < partials->size(); i++)
    {
        const Partial &partial = (*partials)[i];
        
        BL_FLOAT peakFreq = partial.mFreq;
        
        // Zero frequency (because of very small magn) ?
        bool discard = false;
        if (peakFreq < EPS)
            discard = true;
        
        if (!discard)
            result.push_back(partial);
    }
    
    *partials = result;
}

void
PartialTracker2::ThresholdPartialsAmp(vector<Partial> *partials)
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
PartialTracker2::ThresholdPartialsAmpProminence(const WDL_TypedBuf<BL_FLOAT> &magns,
                                                vector<Partial> *partials)
{
    vector<Partial> result;
    for (int i = 0; i < partials->size(); i++)
    {
        const Partial &partial = (*partials)[i];
        
        BL_FLOAT prominence = ComputeProminence2(magns,
                                               partial.mPeakIndex,
                                               partial.mLeftIndex,
                                               partial.mRightIndex);
        
        // Just in case
        if (prominence < 0.0)
            prominence = 0.0;
    
        // Threshold
        BL_FLOAT prominenceDB = AmpToDB(prominence);
        if (prominenceDB >= mThreshold)
            result.push_back(partial);
    }
    
    *partials = result;
}

// BUGGY (this should be max foot)
BL_FLOAT
PartialTracker2::ComputeProminence(const WDL_TypedBuf<BL_FLOAT> &magns,
                                   int peakIndex, int leftIndex, int rightIndex)
{
    // Compute prominence
    //
    // See: https://www.mathworks.com/help/signal/ref/findpeaks.html
    //
    BL_FLOAT minFootAmp = magns.Get()[leftIndex];
    if (magns.Get()[rightIndex] < minFootAmp)
        minFootAmp = magns.Get()[rightIndex];
    
    BL_FLOAT peakAmp = magns.Get()[peakIndex];
    
    BL_FLOAT prominence = peakAmp - minFootAmp;
    
    return prominence;
}

// Fixed version
BL_FLOAT
PartialTracker2::ComputeProminence2(const WDL_TypedBuf<BL_FLOAT> &magns,
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
PartialTracker2::ComputeHigherFoot(const WDL_TypedBuf<BL_FLOAT> &magns,
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
PartialTracker2::ComputeLowerFoot(const WDL_TypedBuf<BL_FLOAT> &magns,
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
PartialTracker2::ThresholdPartialsAmpSmooth(const WDL_TypedBuf<BL_FLOAT> &magns,
                                            vector<Partial> *partials)
{
    if (mSmoothWinThreshold.GetSize() != THRESHOLD_PARTIALS_SMOOTH_SIZE)
    {
        Window::MakeHanning(THRESHOLD_PARTIALS_SMOOTH_SIZE, &mSmoothWinThreshold);
    }
    
    WDL_TypedBuf<BL_FLOAT> smoothMagns;
    BLUtils::SmoothDataWin(&smoothMagns, mCurrentMagns, mSmoothWinThreshold);
    
    WDL_TypedBuf<BL_FLOAT> magnsDiff = magns;
    BLUtils::SubstractValues(&magnsDiff, smoothMagns);
    
    BL_FLOAT hzPerBin = mSampleRate/mBufferSize;
    
    vector<Partial> result;
    for (int i = 0; i < partials->size(); i++)
    {
        const Partial &partial = (*partials)[i];
        
        BL_FLOAT binNum = partial.mFreq/hzPerBin;
        
        // Take the nearest bin
        binNum = bl_round(binNum);
        
        BL_FLOAT magn = magnsDiff.Get()[(int)binNum];
        if (magn >= 0.0)
            // Amp is big enough, keep the partial
            result.push_back(partial);
    }
    
    *partials = result;
}


// See: https://imagej.net/Auto_Local_Threshold (Phansalkar)
//
// NOTE: did not success to make it work
//
void
PartialTracker2::ThresholdPartialsAmpAuto(const WDL_TypedBuf<BL_FLOAT> &magns,
                                          vector<Partial> *partials)
{
    int winSize = THRESHOLD_PARTIALS_AUTO_WIN_SIZE;
    
    BL_FLOAT hzPerBin = mSampleRate/mBufferSize;
    
    vector<Partial> result;
    for (int i = 0; i < partials->size(); i++)
    {
        const Partial &partial = (*partials)[i];
        
        BL_FLOAT binNum = partial.mFreq/hzPerBin;
        
        // Take the nearest bin
        binNum = bl_round(binNum);
        
        // Compute the mean
        BL_FLOAT mean = 0.0;
        int numValues = 0;
        for (int j = 0; j < winSize; j++)
        {
            int idx = i + j - winSize/2;
            if ((idx < 0) || (idx > magns.GetSize() - 1))
                continue;
            
            BL_FLOAT val = magns.Get()[idx];
            
            mean += val;
            numValues++;
        }
        if (numValues > 0)
        {
            mean /= numValues;
        }
        
        // Compute the standard deviation
        BL_FLOAT stdev = 0.0;
        int numValues2 = 0;
        for (int j = 0; j < winSize; j++)
        {
            int idx = i + j - winSize/2;
            if ((idx < 0) || (idx > magns.GetSize() - 1))
                continue;
            
            BL_FLOAT val = magns.Get()[idx];
            
            stdev += (val - mean)*(val - mean);
            numValues2++;
        }
        if (numValues2 > 0)
        {
            stdev /= numValues2;
            
            stdev = std::sqrt(stdev);
        }
        
#if 0
        // See: https://imagej.net/Auto_Local_Threshold (Phansalkar)
        
        // Phansalkar parameters
        BL_FLOAT k = 0.25; // modifiable
        BL_FLOAT r = 0.5; // modifiable
        
        BL_FLOAT p = 2.0;
        BL_FLOAT q = 10.0;
        
        BL_FLOAT t = mean * (1.0 + p * std::exp(-q * mean) + k * ((stdev / r) - 1.0));
#endif
        
#if 0
        // Sauvola
        BL_FLOAT k = 0.5;
        BL_FLOAT r = 128.0;
        
        BL_FLOAT t = mean*(1.0 + k*(stdev/r - 1.0));
#endif
        
#if 0
        // Mean
        //BL_FLOAT norm = -mThreshold/120.0;
        BL_FLOAT norm = BLUtils::DBToAmpNorm(-mThreshold/120.0, 1e-15, -120.0);
        
        BL_FLOAT t = 1.0;
        if (partial.mAmp > mean - norm)
            t = 0.0;
#endif
        
#if 1 // Original
        BL_FLOAT t = DBToAmp(mThreshold);
#endif
        
        if (partial.mAmp >= t)
            result.push_back(partial);
    }
    
     *partials = result;
}

void
PartialTracker2::SuppressBarbs(vector<Partial> *partials)
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

// Filter
void
PartialTracker2::FilterPartials(vector<Partial> *result)
{
#define INF 1e15
    
    result->clear();
    
    if (mPartials.empty())
        return;
    
    //SuppressNoisyPartials(&mPartials[0]);
    
    // Suppress partials with zero freqs (due to very small magns)
    SuppressBadPartials(&mPartials[0]);
    
    
#if 0 // Deactivated for the moment, because it made loose tracking
      // on the lowest partial ("bell", second bell strike)
    
    // Suppress the barbs on the last detected series
    // (just added by DetectPartials)
    SuppressBarbs(&mPartials[0]);
#endif
    
#if 0 // NOTE: not so good
    //ApplyMinSpacing(&mPartials[0]);
#endif
    
#if APPLY_MIN_SPACING
    // NOTE: better
    ApplyMinSpacingMel(&mPartials[0]);
#endif
    
    if (mPartials.size() < 2)
        return;
    
    const vector<Partial> &prevPartials = mPartials[1];
    vector<Partial> currentPartials = mPartials[0];
    
    // Partials that was not associated at the end
    vector<Partial> remainingPartials;
    
    // NOTE: new version: better (sort by amp, then assoc with threshold)
    AssociatePartials(prevPartials, &currentPartials, &remainingPartials);
    
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

// Makes jumps between bins if not filtered
BL_FLOAT
PartialTracker2::ComputePeakFreqSimple(int peakIndex)
{
    BL_FLOAT result = GetFrequency(peakIndex);
    
    return result;
}

// Better than "Simple" => do not make jups between bins
BL_FLOAT
PartialTracker2::ComputePeakFreqAvg(const WDL_TypedBuf<BL_FLOAT> &magns,
                                   int leftIndex, int rightIndex)
{
    // Pow coeff, to select preferably the high amp values
    // With 2.0, makes smoother freq change
    // With 3.0, make just a little smoother than 2.0
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
    
    if (sumMagns < EPS)
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
PartialTracker2::ComputePeakFreqParabola(const WDL_TypedBuf<BL_FLOAT> &magns,
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
PartialTracker2::ComputePeakFreqAvg2(const WDL_TypedBuf<BL_FLOAT> &magns,
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
    
    if (sumMagns < EPS)
        return 0.0;
    
    BL_FLOAT freq = sumFreqs/sumMagns;
    
    // Take the nearest bin
    BL_FLOAT result = 0.0;
    BL_FLOAT minDiff = INF;
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
PartialTracker2::ComputePeakAmpInterp(const WDL_TypedBuf<BL_FLOAT> &magns,
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
PartialTracker2::ComputePeakAmpInterpFreqObj(const WDL_TypedBuf<BL_FLOAT> &magns,
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
PartialTracker2::GetFrequency(int binIndex)
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
PartialTracker2::FindPartialById(const vector<PartialTracker2::Partial> &partials,
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
PartialTracker2::AssociatePartials(const vector<PartialTracker2::Partial> &prevPartials,
                                   vector<PartialTracker2::Partial> *currentPartials,
                                   vector<PartialTracker2::Partial> *remainingPartials)
{
    // Sort current partials and prev partials by decreasing amplitude
    vector<Partial> currentPartialsSort = *currentPartials;
    sort(currentPartialsSort.begin(), currentPartialsSort.end(), Partial::AmpLess);
    reverse(currentPartialsSort.begin(), currentPartialsSort.end());
    
    vector<Partial> prevPartialsSort = prevPartials;
    sort(prevPartialsSort.begin(), prevPartialsSort.end(), Partial::AmpLess);
    reverse(prevPartialsSort.begin(), prevPartialsSort.end());
 
    // Associate
    
    // Associated partials
    vector<Partial> currentPartialsAssoc;
    for (int i = 0; i < prevPartialsSort.size(); i++)
    {
        const Partial &prevPartial = prevPartialsSort[i];
        
        for (int j = 0; j < currentPartialsSort.size(); j++)
        {
            Partial &currentPartial = currentPartialsSort[j];
            
            if (currentPartial.mId != -1)
                // Already assigned
                continue;
            
#if !PT2_PREDICTIVE
            BL_FLOAT diffFreq = std::fabs(prevPartial.mFreq - currentPartial.mFreq);
#else
            BL_FLOAT diffFreq = std::fabs(prevPartial.mPredictedFreq - currentPartial.mFreq);
#endif
            
            if (diffFreq < MAX_FREQ_DIFF_ASSOC)
            // Associated !
            {
                currentPartial.mId = prevPartial.mId;
                currentPartial.mState = Partial::ALIVE;
                currentPartial.mWasAlive = true;
            
#if PT2_PREDICTIVE
                // mFirstTimePredict ?
                currentPartial.mKf = prevPartial.mKf;
                currentPartial.mPredictedFreq =
                            currentPartial.mKf.updateEstimate(currentPartial.mFreq);
#endif
                
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
    
    sort(remainingPartials->begin(), remainingPartials->end(), Partial::IdLess);
}

void
PartialTracker2::SmoothPartials(const vector<PartialTracker2::Partial> &prevPartials,
                               vector<PartialTracker2::Partial> *currentPartials)
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
