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

#include <Utils.h>
#include <Debug.h>

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
#define MAX_AMP_DIFF_RATIO 2.0 //1.5 //2.0 //1.0 //4.0 //2.0 //10.0

// Compute simple, avg, or parabola ?
// (Avg seems a little more smooth than parabola)
#define COMPUTE_PEAKS_SIMPLE 0 //0 //1
#define COMPUTE_PEAKS_AVG 1 //0 //1 //0 //1
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

#define DEBUG_COEFF_EXTRACT_HARMO 1.0 //4.0

// Minimum spacing between partials in bins
#define MIN_SPACING_BINS 0 //8 //16

// 0.01 => gives good result (avoids "tss" detected as partial)
// 0.005 => avoids missing partials in heigh frequencies
//#define MIN_SPACING_NORM_MEL 0.01
//#define MIN_SPACING_NORM_MEL 0.005
#define MIN_SPACING_NORM_MEL 0.001 // in test...

// 100 Hz
#define MAX_FREQ_DIFF_ASSOC 100.0


// NOTE: See: https://www.dsprelated.com/freebooks/sasp/Spectral_Modeling_Synthesis.html
//
// and: https://www.dsprelated.com/freebooks/sasp/PARSHL_Program.html#app:parshlapp
//

unsigned long PartialTracker2::Partial::mCurrentId = 0;



PartialTracker2::Partial::Partial()
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
    
    
PartialTracker2::Partial::Partial(const Partial &other)
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
    
PartialTracker2::Partial::~Partial() {}

void
PartialTracker2::Partial::GenNewId()
{
    mId = mCurrentId++;
}
    
double
PartialTracker2::Partial::ComputeDistance2(const Partial &partial0,
                                          const Partial &partial1,
                                          double sampleRate)
{
#define AMP_COEFF 0.0 //0.25
    double diffBin = fabs(partial0.mFreq - partial1.mFreq)/sampleRate;
        
    double diffAmp = fabs(partial0.mAmp - partial1.mAmp)/partial0.mAmp;
    
    diffAmp *= AMP_COEFF;
    
    double dist2 = diffBin*diffBin + diffAmp*diffAmp;
    
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

#if HARMONIC_SELECT
// For HarmonicSelect
bool
PartialTracker2::Partial::operator==(const Partial &other)
{
// Keep it commented !!
// (otherwise it will overwrite the global EPS, and
// make mistakes with dB
//#define EPS 1e-8
    
    if ((fabs(mFreq - other.mFreq) < EPS) &&
        (fabs(mAmp - other.mAmp) < EPS))
        return true;
        
    return false;
}

bool
PartialTracker2::Partial::operator!=(const Partial &other)
{
    return !(*this == other);
}
#endif

PartialTracker2::PartialTracker2(int bufferSize, double sampleRate,
                                 double overlapping)
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
}

PartialTracker2::~PartialTracker2()
{
    if (mFreqObj != NULL)
        delete mFreqObj;
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
PartialTracker2::SetThreshold(double threshold)
{
    mThreshold = threshold;
}

void
PartialTracker2::SetData(const WDL_TypedBuf<double> &magns,
                        const WDL_TypedBuf<double> &phases)
{
    mCurrentMagns = magns;
    mCurrentPhases = phases;
}

void
PartialTracker2::DetectPartials()
{
    WDL_TypedBuf<double> magns0 = mCurrentMagns;
    
    if (mFreqObj != NULL)
    {
        // See: http://blogs.zynaptiq.com/bernsee/pitch-shifting-using-the-ft/
        mFreqObj->ComputeRealFrequencies(mCurrentPhases, &mRealFreqs);
    }
    
    vector<Partial> partials;
    DetectPartials(magns0, mCurrentPhases, &partials);

    // TEST: remove small partials before gluing the twins
    // => this avoids gluing many small partials together
    SuppressSmallPartials(&partials);
    
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
    
    // TEST
    mResult = partials;
}

void
PartialTracker2::ExtractNoiseEnvelope()
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
    WDL_TypedBuf<double> dummyPhases;
    Utils::ResizeFillZeros(&dummyPhases, mNoiseEnvelope.GetSize());
    for (int i = 0; i < NUM_ITER_EXTRACT_NOISE - 1; i++)
    {
        vector<Partial> partials;
        DetectPartials(mNoiseEnvelope, dummyPhases, &partials);
        
        CutPartials(partials, &mNoiseEnvelope);
    }
}

void
PartialTracker2::ExtractNoiseEnvelopeSelect()
{
    // If not partial detected, noise envelope is simply the input magns
    mNoiseEnvelope = mCurrentMagns;
    
    // Iterate
    WDL_TypedBuf<double> dummyPhases;
    Utils::ResizeFillZeros(&dummyPhases, mNoiseEnvelope.GetSize());
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
            double amp = partial.mAmp;
            
            double sharpness = amp/dIdx;
            
            if (sharpness > mSharpnessExtractNoise)
                selectedPartials.push_back(partial);
        }
        
        CutPartials(selectedPartials, &mNoiseEnvelope);
    }
    
    WDL_TypedBuf<double> noiseEnv = mNoiseEnvelope;
    Utils::MultValues(&noiseEnv, DEBUG_COEFF_EXTRACT_HARMO);
    
    // Compute harmonic envelope
    // (origin signal less noise)
    mHarmonicEnvelope = mCurrentMagns;
    Utils::SubstractValues(&mHarmonicEnvelope, noiseEnv);
    
    Utils::ClipMin(&mHarmonicEnvelope, 0.0);
}

void
PartialTracker2::ExtractNoiseEnvelopeTrack()
{
    static int count = 0;
    count++;
    fprintf(stderr, "count: %d\n", count);
    // max 152
    
    //if (count >= 50)
    //    return;
    
    if (count >= 50)
    //if (count >= 49)
    {
        int dummy = 0;
    }
    
    // If not partial detected, noise envelope is simply the input magns
    mNoiseEnvelope = mCurrentMagns;
    
    Debug::DumpData("magns.txt", mCurrentMagns);
    
    vector<Partial> partials;
    //GetPartials(&partials);
    
    // NOTE: Must get the alive partials only,
    // otherwise we would get additional "garbage" partials,
    // that would corrupt the partial rectangle
    // and then compute incorrect noise peaks
    GetAlivePartials(&partials);
    
    DBG_DumpPartialsAmp("partials0.txt", partials, mBufferSize, mSampleRate);
    
    DBG_DumpPartialsBox("partials-box0.txt", partials, mBufferSize, mSampleRate);
    
    // "tibetan bell": avoids keeping a partial at 2KHz in the noise envelope
    // => makes a better noise envelope !
#if 1
    //GlueTwinPartials(&partials);
    GlueTwinPartials(mCurrentMagns, &partials);
#endif
    
    DBG_DumpPartialsAmp("partials1.txt", partials, mBufferSize, mSampleRate);
    
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
        double amp = partial.mAmp;
        
        double sharpness = amp/dIdx;
            
        if (sharpness > mSharpnessExtractNoise)
            selectedPartials.push_back(partial);
#endif
        
        selectedPartials.push_back(partial);
    }
    
    CutPartials(selectedPartials, &mNoiseEnvelope);
    
    Debug::DumpData("noise.txt", mNoiseEnvelope);
    
    WDL_TypedBuf<double> noiseEnv = mNoiseEnvelope;
    Utils::MultValues(&noiseEnv, DEBUG_COEFF_EXTRACT_HARMO);
    
    // Compute harmonic envelope
    // (origin signal less noise)
    mHarmonicEnvelope = mCurrentMagns;
    Utils::SubstractValues(&mHarmonicEnvelope, noiseEnv);
    
    Utils::ClipMin(&mHarmonicEnvelope, 0.0);
}

#if 0
// Zero the noise and harmonic envelopes if the
// first partial is not correct (no minimum on the left)
void
PartialTracker2::SuppressZeroFreqPartialEnv()
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
PartialTracker2::ExtractNoiseEnvelopeSmooth()
{
#define CMA_WINDOW_SIZE 40
    
    // If not partial detected, noise envelope is simply the input magns
    mNoiseEnvelope = mCurrentMagns;
    
    WDL_TypedBuf<double> smoothEnv;
    smoothEnv.Resize(mNoiseEnvelope.GetSize());
    CMASmoother::ProcessOne(mNoiseEnvelope.Get(), smoothEnv.Get(),
                            mNoiseEnvelope.GetSize(), CMA_WINDOW_SIZE);
    
    mNoiseEnvelope = smoothEnv;
}

void
PartialTracker2::CutPartials(const vector<Partial> &partials,
                            WDL_TypedBuf<double> *magns)
{
    // Take each partial, and "cut" it from the full data
    for (int i = 0; i < partials.size(); i++)
    {
        const Partial &partial = partials[i];
        
        //if ((partial.mPeakIndex == 282) ||
        //    (partial.mPeakIndex == 283))
        //{
        //    int dummy = 0;
        //}

        
        if (partial.mLeftIndex == 277) // -1
        {
            int dummy = 0;
        }
            
        int minIdx = partial.mLeftIndex;
        if (minIdx >= magns->GetSize())
            continue;
        
        int maxIdx = partial.mRightIndex;
        if (maxIdx >= magns->GetSize())
            continue;
        
        double minVal = magns->Get()[minIdx];
        double maxVal = magns->Get()[maxIdx];
        
        // TODO: check this
        int numIdx = maxIdx - minIdx + 1;
        for (int j = 0; j < numIdx; j++)
        {
            double t = 0.0;
            if (numIdx > 1)
                t = ((double)j)/(numIdx - 1);
            
            double val = (1.0 - t)*minVal + t*maxVal;
            if (minIdx + j >= magns->GetSize())
                continue;
            
            magns->Get()[minIdx + j] = val;
        }
    }
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
    
#if HARMONIC_SELECT
    HarmonicSelect(&mResult);
#endif
}

bool
PartialTracker2::GetPartials(vector<Partial> *partials)
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

bool
PartialTracker2::GetAlivePartials(vector<Partial> *partials)
{
    if (mPartials.empty())
        return false;
    
    partials->clear();
    
    for (int i = 0; i < mPartials[0].size(); i++)
    {
        const Partial &p = mPartials[0][i];
        //if (p.mWasAlive)
        if (p.mState == Partial::ALIVE)
        {
            partials->push_back(p);
        }
    }
    
    return true;
}

void
PartialTracker2::GetNoiseEnvelope(WDL_TypedBuf<double> *noiseEnv)
{
    *noiseEnv = mNoiseEnvelope;
}

void
PartialTracker2::GetHarmonicEnvelope(WDL_TypedBuf<double> *harmoEnv)
{
    *harmoEnv = mHarmonicEnvelope;
}

void
PartialTracker2::SetSharpnessExtractNoise(double sharpness)
{
    mSharpnessExtractNoise = sharpness;
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
            Debug::AppendValue(fileName, p.mFreq);
        }
        else
            // Append zeros to stay synchro
        {
            Debug::AppendValue(fileName, 0.0);
        }
    }
}

void
PartialTracker2::DBG_DumpPartialsAmp(const vector<Partial> &partials,
                                     int bufferSize, double sampleRate)
{
    DBG_DumpPartialsAmp("partials.txt", partials, bufferSize, sampleRate);
}

void
PartialTracker2::DBG_DumpPartialsAmp(const char *fileName,
                                     const vector<Partial> &partials,
                                     int bufferSize, double sampleRate)
{
#define NUM_MAGNS 1024
    
    double hzPerBin = sampleRate/bufferSize;
    
    WDL_TypedBuf<double> result;
    result.Resize(NUM_MAGNS);
    Utils::FillAllZero(&result);
    
    for (int i = 0; i < partials.size(); i++)
    {
        const PartialTracker2::Partial &p = partials[i];
        
        double amp = p.mAmp;
        
        double binNum = p.mFreq/hzPerBin;
        
        // Take the nearest bin
        binNum = round(binNum);
        
        // Debug
        //if ((int)binNum == 279)
        //{
        //    int dummy = 0;
        //}
            
        //int binNum = p.mPeakIndex;
        
        if (binNum < result.GetSize())
        {
            result.Get()[(int)binNum] = amp;
        }
    }
    
    Debug::DumpData(fileName, result);
}

void
PartialTracker2::DBG_DumpPartialsBox(const vector<Partial> &partials,
                                     int bufferSize, double sampleRate)
{
    DBG_DumpPartialsBox("partials-box.txt", partials,
                        bufferSize, sampleRate);
}

void
PartialTracker2::DBG_DumpPartialsBox(const char *fileName,
                                     const vector<Partial> &partials,
                                     int bufferSize, double sampleRate)
{
#define NUM_MAGNS 1024
    
    double hzPerBin = sampleRate/bufferSize;
    
    WDL_TypedBuf<double> result;
    result.Resize(NUM_MAGNS);
    Utils::FillAllZero(&result);
    
    for (int i = 0; i < partials.size(); i++)
    {
        const PartialTracker2::Partial &p = partials[i];
        
        double amp = p.mAmp;
        
        for (int j = p.mLeftIndex; j <= p.mRightIndex; j++)
        {
            result.Get()[j] = amp;
        }
    }
    
    Debug::DumpData(fileName, result);
}

// Same method as above (implemented twice...)
// (but better code)
void
PartialTracker2::DBG_DumpPartials2(const vector<Partial> &partials)
{
    double hzPerBin = mSampleRate/mBufferSize;
    
    WDL_TypedBuf<double> buffer;
    Utils::ResizeFillZeros(&buffer, mBufferSize/2);
    
    for (int i = 0; i < partials.size(); i++)
    {
        const PartialTracker2::Partial &p = partials[i];
        
        double binNum = p.mFreq/hzPerBin;
        double amp = p.mAmp;
        
        buffer.Get()[(int)binNum] = amp;
    }
    
    Debug::DumpData("partials.txt", buffer);
}

void
PartialTracker2::DetectPartials(const WDL_TypedBuf<double> &magns,
                                const WDL_TypedBuf<double> &phases,
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
    
    double prevVal0 = 0.0;
    double prevVal1 = 0.0;
    while(currentIndex < magns.GetSize())
    {
        double currentVal = magns.Get()[currentIndex];
        
        if ((prevVal1 > prevVal0) && (prevVal1 > currentVal))
            // Maximum found
        {
            if (currentIndex - 1 >= 0)
            {
                // Take the left and right "feets" of the partial,
                // then the middle.
                // (in order to be more precise)
            
                // Left
                int leftIndex = currentIndex - 1; //- 2;
                if (leftIndex > 0)
                {
                    double prevLeftVal = magns.Get()[leftIndex];
                    while(leftIndex > 0)
                    {
                        leftIndex--;
                    
                        double leftVal = magns.Get()[leftIndex];
                    
                        // Stop if we reach 0 or if it goes up again
                        if ((leftVal < EPS_PARTIAL) || (leftVal >= prevLeftVal))
                        {
                            if (leftVal >= prevLeftVal)
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
                int rightIndex = currentIndex - 1; //
                if (rightIndex < magns.GetSize())
                {
                    double prevRightVal = magns.Get()[rightIndex];
                    while(rightIndex < magns.GetSize() - 1)
                    {
                        rightIndex++;
                    
                        double rightVal = magns.Get()[rightIndex];
                    
                        // Stop if we reach 0 or if it goes up again
                        if ((rightVal < EPS_PARTIAL) || (rightVal >= prevRightVal))
                        {
                            if (rightVal >= prevRightVal)
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
            
                double peakAmp = magns.Get()[peakIndex];
     
                // Discard partial ?
                bool discard = false;
                
#if 0 // Moved this code at the beging of filter
      // For the noise envelope to get also the small partials
                // Too small amp ?
                double peakAmpDB = Utils::AmpToDBNorm(peakAmp, EPS_DB, mThreshold);
                if (peakAmpDB < MIN_AMP)
                    // Amp is not big enough, discard the partial
                    discard = true;
#endif
                
#if 0 // Not working well
                // Is a barb on the side of a real partial ?
                double leftAmp = magns.Get()[leftIndex];
                double rightAmp = magns.Get()[rightIndex];
                double maxSideAmp = (leftAmp > rightAmp) ? leftAmp : rightAmp;
            
                if (fabs(leftAmp - rightAmp)*BARB_COEFF > (peakAmp - maxSideAmp))
                    discard = true;
#endif
                
                // Create new partial
                Partial p;
            
                p.mPeakIndex = peakIndex;
                p.mLeftIndex = leftIndex;
                p.mRightIndex = rightIndex;
                
#if COMPUTE_PEAKS_SIMPLE // Makes some jumps between frequencies
      // But resynthetize well
                double peakFreq = ComputePeakFreqSimple(peakIndex);
#endif
#if COMPUTE_PEAKS_AVG
      // Make smooth partials change
      // But makes wobble in the sound volume
                double peakFreq = ComputePeakFreqAvg(magns, leftIndex, rightIndex);
#endif
#if COMPUTE_PEAKS_PARABOLA
                // Make smooth partials change
                // But makes wobble in the sound volume
                double peakFreq = ComputePeakFreqParabola(magns, peakIndex);
#endif
#if COMPUTE_PEAKS_AVG2
                double peakFreq = ComputePeakFreqAvg2(magns, leftIndex, rightIndex);
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

// Does not work very well
// (the Mel version works better)
void
PartialTracker2::ApplyMinSpacing(vector<Partial> *partials)
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
            
            if (fabs(peakIndex0 - peakIndex1) < MIN_SPACING_BINS)
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
PartialTracker2::ApplyMinSpacingMel(vector<Partial> *partials)
{
    double hzPerBin = mSampleRate/mBufferSize;
    
    vector<Partial> result;
    for (int i = 0; i < partials->size(); i++)
    {
        const Partial &partial0 = (*partials)[i];
        double peakFreq0 = partial0.mFreq;
        double peakFreqMel0 = Utils::FreqToMelNorm(peakFreq0, hzPerBin, mBufferSize);
        
        bool discard = false;
        for (int j = 0; j < partials->size(); j++)
        {
            if (j == i)
                continue;
            
            const Partial &partial1 = (*partials)[j];
            double peakFreq1 = partial1.mFreq;
            double peakFreqMel1 = Utils::FreqToMelNorm(peakFreq1, hzPerBin, mBufferSize);
            
            if (fabs(peakFreqMel0 - peakFreqMel1) < MIN_SPACING_NORM_MEL)
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
PartialTracker2::GlueTwinPartials(const WDL_TypedBuf<double> &magns,
                                  vector<Partial> *partials)
{
    vector<Partial> result;
    
    double hzPerBin = mSampleRate/mBufferSize;
    
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
            
            double peakFreq = ComputePeakFreqAvg(magns, leftIndex, rightIndex);
            
            // For peak amp, take max amp
            double maxAmp = 0.0;
            for (int k = 0; k < twinPartials.size(); k++)
            {
                double amp = twinPartials[k].mAmp;
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
            result.push_back(currentPartial);
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
    
    double hzPerBin = mSampleRate/mBufferSize;
    
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
            
            double peakFreq = peakIndex*hzPerBin;
            
            // For peak amp, take max amp
            double maxAmp = 0.0;
            for (int k = 0; k < twinPartials.size(); k++)
            {
                double amp = twinPartials[k].mAmp;
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
        
        double peakFreq = partial.mFreq;
        
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
PartialTracker2::SuppressSmallPartials(vector<Partial> *partials)
{
    vector<Partial> result;
    for (int i = 0; i < partials->size(); i++)
    {
        const Partial &partial = (*partials)[i];
    
        double peakAmp = partial.mAmp;
        
        // Too small amp ?
        bool discard = false;
        double peakAmpDB = Utils::AmpToDBNorm(peakAmp, EPS_DB, mThreshold);
        if (peakAmpDB < MIN_AMP)
            // Amp is not big enough, discard the partial
            discard = true;
        
        if (!discard)
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

// Make a tracking for smoothing
void
PartialTracker2::FilterPartials(vector<Partial> *result)
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
    
    // Set to 1 if don't glue twin partials
#if 0
    // Check small magns
    SuppressSmallPartials(&mPartials[0]);
#endif
    
#if 0 // Deactivated for the moment, because it made loose tracking
      // on the lowest partial ("bell", second bell strike)
    
    // Suppress the barbs on the last detected series
    // (just added by DetectPartials)
    SuppressBarbs(&mPartials[0]);
#endif
    
    // TEST: not so good
    //ApplyMinSpacing(&mPartials[0]);
    
    // TEST 2
    ApplyMinSpacingMel(&mPartials[0]);
    
    if (mPartials.size() < 2)
        return;
    
    const vector<Partial> &prevPartials = mPartials[1];
    vector<Partial> currentPartials = mPartials[0];
    
    // Partials that was not associated at the end
    vector<Partial> remainingPartials;
    
    //AssociatePartialsMin(prevPartials, &currentPartials,
    //                     &remainingPartials);
    
    AssociatePartialsBiggest(prevPartials, &currentPartials,
                             &remainingPartials);
    
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

// Discard partial association if the amps ratio is too big
// (this may avoid associated a new partial with a barb on the side of
// the previous partial)
bool
PartialTracker2::TestDiscardByAmp(const Partial &p0, const Partial &p1)
{
    bool result = false;
    double ampRatio = 1.0;
    
    if (p0.mAmp > EPS)
        ampRatio = p1.mAmp/p0.mAmp;
    
    if ((ampRatio > MAX_AMP_DIFF_RATIO) ||
        (ampRatio < 1.0/MAX_AMP_DIFF_RATIO))
        result = true;
    
    return result;;
}

#if HARMONIC_SELECT
// Remove the detected partials that are between two
// valid partials (almost harmonic)
void
PartialTracker2::HarmonicSelect(vector<Partial> *result)
{
#define INF 1e8
    
    if (result->empty())
        return;
    
    vector<Partial> newPartials;
    
    double freq0 = (*result)[0].mFreq;
    newPartials.push_back((*result)[0]);
    
    double freq = freq0;
    while((freq < mSampleRate/2.0) && !result->empty())
    {
        // Find the nearest partial
        double minDiffFreq = INF;
        double minIdx = -1;
        for (int i = 0; i < result->size(); i++)
        {
            const Partial &p = (*result)[i];
            
            double diffFreq = fabs(p.mFreq - freq); // TODO: ComputeDist2 ?
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
    vector<Partial>::iterator last = unique(newPartials.begin(), newPartials.end());
    newPartials.erase(last, newPartials.end());
    
    *result = newPartials;
}
#endif

// Makes jumps between bins if not filtered
double
PartialTracker2::ComputePeakFreqSimple(int peakIndex)
{
    double result = GetFrequency(peakIndex);
    
    return result;
}

// Better than "Simple" => do not make jups between bins
double
PartialTracker2::ComputePeakFreqAvg(const WDL_TypedBuf<double> &magns,
                                   int leftIndex, int rightIndex)
{
    // Pow coeff, to select preferably the high amp values
    // With 2.0, makes smoother freq change
    // With 3.0, make just a littel smoother than 2.0
#define COEFF 3.0 //2.0
    
    double sumFreqs = 0.0;
    double sumMagns = 0.0;
    
    for (int i = leftIndex; i <= rightIndex; i++)
    {
        double freq = GetFrequency(i);
        
        double magn = magns.Get()[i];
        
        magn = pow(magn, COEFF);
        
        sumFreqs += freq*magn;
        sumMagns += magn;
    }
    
    if (sumMagns < EPS)
        return 0.0;
    
    double result = sumFreqs/sumMagns;
    
    return result;
}

// Parabola peak center detection
// Works well (but I prefer my method) 
//
// See: http://eprints.maynoothuniversity.ie/4523/1/thesis.pdf (p32)
//
// and: https://ccrma.stanford.edu/~jos/parshl/Peak_Detection_Steps_3.html#sec:peakdet
//
double
PartialTracker2::ComputePeakFreqParabola(const WDL_TypedBuf<double> &magns,
                                        int peakIndex)
{
    double hzPerBin = mSampleRate/mBufferSize;
    
    if ((peakIndex - 1 < 0) || (peakIndex >= magns.GetSize()))
    {
        double result = peakIndex*hzPerBin;
        
        return result;
    }
    
    double alpha = magns.Get()[peakIndex - 1];
    alpha = AmpToDB(alpha);
    
    double beta = magns.Get()[peakIndex];
    beta = AmpToDB(beta);
    
    double gamma = magns.Get()[peakIndex + 1];
    gamma = AmpToDB(gamma);
    
    // Center
    double c = 0.5*((alpha - gamma)/(alpha - 2.0*beta + gamma));
    
    double binNum = peakIndex + c;
                    
    double result = binNum*hzPerBin;
    
    return result;
}

// Better than "Simple" => do not make jups between bins
double
PartialTracker2::ComputePeakFreqAvg2(const WDL_TypedBuf<double> &magns,
                                   int leftIndex, int rightIndex)
{
    // Pow coeff, to select preferably the high amp values
    // With 2.0, makes smoother freq change
    // With 3.0, make just a littel smoother than 2.0
#define COEFF 3.0 //2.0
    
    double sumFreqs = 0.0;
    double sumMagns = 0.0;
    
    for (int i = leftIndex; i <= rightIndex; i++)
    {
        double freq = GetFrequency(i);
        
        double magn = magns.Get()[i];
        
        magn = pow(magn, COEFF);
        
        sumFreqs += freq*magn;
        sumMagns += magn;
    }
    
    if (sumMagns < EPS)
        return 0.0;
    
    double freq = sumFreqs/sumMagns;
    
    // Take the nearest bin
    double result = 0.0;
    double minDiff = INF;
    for (int i = 0; i < magns.GetSize(); i++)
    {
        double freq0 = GetFrequency(i);
        
        double diff0 = fabs(freq - freq0);
        if (diff0 < minDiff)
        {
            minDiff = diff0;
            result = freq0;
        }
    }
    return result;
}

// Simple method
double
PartialTracker2::ComputePeakAmpAvg(const WDL_TypedBuf<double> &magns,
                                  double peakFreq)
{
    double hzPerBin = mSampleRate/mBufferSize;
    
    double bin = peakFreq/hzPerBin;
    
    int prevBin = (int)bin;
    int nextBin = (int)bin + 1;
    
    if (nextBin >= magns.GetSize())
    {
        double peakAmp = magns.Get()[prevBin];
        
        return peakAmp;
    }
    
    double prevAmp = magns.Get()[prevBin];
    double nextAmp = magns.Get()[nextBin];
    
    double t = bin - prevBin;
    
    double peakAmp = (1.0 - t)*prevAmp + t*nextAmp;
    
    return peakAmp;
}

double
PartialTracker2::ComputePeakAmpAvgFreqObj(const WDL_TypedBuf<double> &magns,
                                         double peakFreq)
{
    int idx0 = 0;
    for (int i = 0; i < mRealFreqs.GetSize(); i++)
    {
        double realFreq = mRealFreqs.Get()[i];
        
        if (realFreq > peakFreq)
        {
            idx0 = i - 1;
            if (idx0 < 0)
                idx0 = 0;
            
            break;
        }
    }
    
    double freq0 = mRealFreqs.Get()[idx0];
    
    int idx1 = idx0 + 1;
    if (idx1 >= mRealFreqs.GetSize())
    {
        double result = magns.Get()[idx0];
        
        return result;
    }
    double freq1 = mRealFreqs.Get()[idx1];
    
    double t = (peakFreq - freq0)/(freq1 - freq0);
    
    double prevAmp = magns.Get()[idx0];
    double nextAmp = magns.Get()[idx1];
    
    double peakAmp = (1.0 - t)*prevAmp + t*nextAmp;
    
    return peakAmp;
}

double
PartialTracker2::GetFrequency(int binIndex)
{
    if ((mFreqObj == NULL) || (binIndex >= mRealFreqs.GetSize()))
    {
        double hzPerBin = mSampleRate/mBufferSize;
        double result = binIndex*hzPerBin;
        
        return result;
    }
    else
    {
        double result = mRealFreqs.Get()[binIndex];
        
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
PartialTracker2::AssociatePartialsMin(const vector<PartialTracker2::Partial> &prevPartials,
                                     vector<PartialTracker2::Partial> *currentPartials,
                                     vector<PartialTracker2::Partial> *remainingPartials)
{
    if (prevPartials.size() <= currentPartials->size())
        AssociatePartialsMinAux1(prevPartials, currentPartials, remainingPartials);
    else
        AssociatePartialsMinAux2(prevPartials, currentPartials, remainingPartials);
}

void
PartialTracker2::AssociatePartialsMinAux1(const vector<PartialTracker2::Partial> &prevPartials,
                                         vector<PartialTracker2::Partial> *currentPartials,
                                         vector<PartialTracker2::Partial> *remainingPartials)
{
    // Associate partials
    vector<Partial> currentPartialsAssoc;
    for (int i = 0; i < prevPartials.size(); i++)
    {
        const Partial &prevPartial = prevPartials[i];
        
        // Find the nearest partial (using frequency)
        double minFreqDiff = INF;
        int minPartialIdx = -1;
        for (int j = 0; j < currentPartials->size(); j++)
        {
            Partial &currentPartial = (*currentPartials)[j];
            if (currentPartial.mId != -1)
                continue;
            
            bool discardByAmp = TestDiscardByAmp(prevPartial, currentPartial);
            double freqDiff = fabs(currentPartial.mFreq - prevPartial.mFreq);
            
            if (!discardByAmp &&
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
PartialTracker2::AssociatePartialsMinAux2(const vector<PartialTracker2::Partial> &prevPartials,
                                         vector<PartialTracker2::Partial> *currentPartials,
                                         vector<PartialTracker2::Partial> *remainingPartials)
{
    // Associate partials
    vector<Partial> currentPartialsAssoc;
    
    for (int i = 0; i < currentPartials->size(); i++)
    {
        Partial &currentPartial = (*currentPartials)[i];
        if (currentPartial.mId != -1)
            continue;
        
        // Find the nearest partial (using frequency)
        double minFreqDiff = INF;
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
            
            bool discardByAmp = TestDiscardByAmp(prevPartial, currentPartial);
            double freqDiff = fabs(currentPartial.mFreq - prevPartial.mFreq);
            
            if (!discardByAmp &&
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

void
PartialTracker2::AssociatePartialsBiggest(const vector<PartialTracker2::Partial> &prevPartials,
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
            
            double diffFreq = fabs(prevPartial.mFreq - currentPartial.mFreq);
            if (diffFreq < MAX_FREQ_DIFF_ASSOC)
            {
                currentPartial.mId = prevPartial.mId;
                currentPartial.mState = Partial::ALIVE;
                currentPartial.mWasAlive = true;
                
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
            double freq0 = prevPartial.mFreq;
            double freq1 = currentPartial.mFreq;
            
            double newFreq = FILTER_SMOOTH_COEFF*freq0 + (1.0 - FILTER_SMOOTH_COEFF)*freq1;
            
            currentPartial.mFreq = newFreq;
            
            // Smooth amp
            double amp0 = prevPartial.mAmp;
            double amp1 = currentPartial.mAmp;
            
            double newAmp = FILTER_SMOOTH_COEFF*amp0 + (1.0 - FILTER_SMOOTH_COEFF)*amp1;
            
            currentPartial.mAmp = newAmp;
        }
    }
}
