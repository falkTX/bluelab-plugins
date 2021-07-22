//
//  PhasesEstimPrusa.cpp
//  BL-PitchShift
//
//  Created by Pan on 16/04/18.
//
//

#include <algorithm>
using namespace std;

#include <BLUtils.h>
#include <BLUtilsMath.h>
#include <BLUtilsComp.h>
#include <BLUtilsFft.h>

#include <BLDebug.h>

// We don't need phases unwrapping!
#include <PhasesUnwrapper.h>

#include "PhasesEstimPrusa.h"

// Exactly like the "Phase Vocoder Done Right" ?
// Or adapted for pitch shifting?
#define REAL_PHASE_VOCODER_PRUSA 0 //1

// See:
// http://kth.diva-portal.org/smash/get/diva2:1381398/FULLTEXT01.pdf
// http://ltfat.github.io/notes/ltfatnote050.pdf
// http://dsp-book.narod.ru/Pitch_shifting.pdf
// https://dadorran.wordpress.com/2014/06/02/audio-time-scale-modification-phase-vocoder-implementation-in-matlab/
// https://sethares.engr.wisc.edu/vocoders/phasevocoder.html
// https://sethares.engr.wisc.edu/vocoders/matlabphasevocoder.html
// http://music.informatics.indiana.edu/media/students/kyung/kyung_paper.pdf
// https://github.com/stekyne/PhaseVocoder/blob/master/DSP/PeakShifter.h
// https://github.com/befuture/PhaseVocoder/blob/master/Source/PV.cpp
// http://blogs.zynaptiq.com/bernsee/pitch-shifting-using-the-ft/
// http://www.paulnasca.com/algorithms-created-by-me
// http://hypermammut.sourceforge.net/paulstretch/


PhasesEstimPrusa::PhasesEstimPrusa(int bufferSize,
                                   int oversampling,
                                   int freqRes,
                                   BL_FLOAT sampleRate)
{
    mBufferSize = bufferSize;
    mOverlapping = oversampling;
    mFreqRes = freqRes;
    mSampleRate = sampleRate;
    
    mPitchFactor = 1.0;
}

PhasesEstimPrusa::~PhasesEstimPrusa() {}

void
PhasesEstimPrusa::SetPitchFactor(BL_FLOAT factor)
{
    mPitchFactor = factor;
}

void
PhasesEstimPrusa::Reset(int bufferSize, int oversampling,
                        int freqRes, BL_FLOAT sampleRate)
{
    mBufferSize = bufferSize;
    mOverlapping = oversampling;
    mFreqRes = freqRes;
    mSampleRate = sampleRate;
    
    ResetPitchShift();
}

void
PhasesEstimPrusa::Reset()
{
    ResetPitchShift();
}

void
PhasesEstimPrusa::Process(const WDL_TypedBuf<BL_FLOAT> &magns,
                          WDL_TypedBuf<BL_FLOAT> *phases)
{
#define TOL 1e-6
    
    const Frame &frame0 = mPrevFrame;
    
    Frame &frame1 = mTmpBuf4;
    frame1.mMagns = magns;
    frame1.mPhases = *phases;
    
    frame1.mDTPhases.Resize(magns.GetSize());
    BLUtils::FillAllZero(&frame1.mDTPhases);
    frame1.mDFPhases.Resize(magns.GetSize());
    BLUtils::FillAllZero(&frame1.mDFPhases);
    frame1.mEstimPhases = frame1.mPhases;
    
    // Test if we have prev data 
    if (mPrevFrame.mMagns.GetSize() == 0)
    {
        // Update prev data
        mPrevFrame = frame1;
        
        // For first step,
        // See: http://music.informatics.indiana.edu/media/students/kyung/kyung_paper.pdf
        BLUtils::FillAllZero(&mPrevFrame.mPhases);
    }
    
    // Pre-processing
    //

    // Phases time derivative

    // Good phases time unwrapping
    PhasesUnwrapper::ComputeUwPhasesDiffTime(&frame1.mDTPhases,
                                             frame0.mPhases, frame1.mPhases,
                                             mSampleRate, mBufferSize,
                                             mOverlapping);
    
    // Phases freq derivative
    
    WDL_TypedBuf<BL_FLOAT> &frame1PhasesFUw = mTmpBuf0;
    frame1PhasesFUw = frame1.mPhases;
    PhasesUnwrapper::UnwrapPhasesFreq(&frame1PhasesFUw);
    BLUtils::ComputeDerivative(frame1PhasesFUw, &frame1.mDFPhases);

    PropagatePhasesPrusa(frame0, &frame1);
    
    // Result
    //
    
    // No need to update the magns: they have not changed!
    *phases = frame1.mEstimPhases;
    
    // Update prev data
    mPrevFrame = frame1;
}

bool
PhasesEstimPrusa::Contains(const vector<Tuple> &hp, int binIdx, int timeIdx)
{
    // TODO: optimize this!
    for (int i = 0; i < hp.size(); i++)
    {
        const Tuple &t = hp[i];
        if ((t.mBinIdx == binIdx) && (t.mTimeIdx == timeIdx))
            return true;
    }

    return false;
}

void
PhasesEstimPrusa::Remove(vector<Tuple> *hp, int binIdx, int timeIdx)
{
    int idx = -1;
    for (int i = 0; i < hp->size(); i++)
    {
        const Tuple &t = (*hp)[i];
        if ((t.mBinIdx == binIdx) && (t.mTimeIdx == timeIdx))
        {
            idx = i;
            break;
        }
    }

    if (idx >= 0)
        hp->erase(hp->begin() + idx);
}

void
PhasesEstimPrusa::ResetPitchShift()
{
    Frame f;
    mPrevFrame = f;
}

// Was PropagatePhasesPrusaOptim
void
PhasesEstimPrusa::PropagatePhasesPrusa(const Frame &frame0, Frame *frame1)
{
    // Prusa algo
    
    // Compute tolerance
    BL_FLOAT maxMagn0 = BLUtils::ComputeMax(frame0.mMagns);
    BL_FLOAT maxMagn1 = BLUtils::ComputeMax(frame1->mMagns);
    BL_FLOAT maxMagn = MAX(maxMagn0, maxMagn1);

    BL_FLOAT abstol = TOL*maxMagn;
    
    // Set random values for very small magns
    // and insert other values in the heap    
    const BL_FLOAT randMaxInv = 1.0/RAND_MAX;

    // tho: use n
    vector<Tuple> &tho = mTmpBuf1;
    tho.resize(frame1->mMagns.GetSize());
    
    int frame1MagnsSize = frame1->mMagns.GetSize();
    BL_FLOAT *frame1MagnsBuf = frame1->mMagns.Get();
    
    for (int i = 0; i < frame1MagnsSize; i++)
    {
        // Fill all the values, but invalidate the values
        // which are under the tolerance
        BL_FLOAT magn1 = frame1MagnsBuf[i];
                    
        Tuple &t = tho[i];
        t.mMagn = magn1;
        t.mBinIdx = i;
        t.mTimeIdx = (unsigned char)1;
        t.mIsValid = (unsigned char)1;

        if (magn1 <= abstol)
        {
            // Invalidate if smaller than threshold
            t.mIsValid = (unsigned char)0;
            
            // And assign random phase value
            BL_FLOAT rp = rand()*randMaxInv;
            frame1->mEstimPhases.Get()[i] = rp;
        }
    }
    
    // Biggest magns elements first
    sort(tho.begin(), tho.end(), Tuple::MagnGreater);

    SortedVec thoV(&tho);
        
    // hp0, use n-1
    vector<Tuple> &hp0 = mTmpBuf2;
    hp0.resize(tho.size());
    
    int hp0Size = hp0.size();
    BL_FLOAT *frame0MagnsBuf = frame0.mMagns.Get();
    
    for (int i = 0; i < hp0Size; i++)
    {
        const Tuple &t = tho[i];

        BL_FLOAT magn0 = frame0MagnsBuf[t.mBinIdx];
        
        Tuple &t0 = hp0[i];
        t0.mMagn = magn0;
        t0.mBinIdx = t.mBinIdx;
        t0.mTimeIdx = (unsigned char)0;
        t0.mIsValid = (unsigned char)1;

        // Transmit for tolerance invalidation
        if (t.mIsValid == (unsigned char)0)
            t0.mIsValid = (unsigned char)0;
    }

    // Must also sort hp0, because the magns come from frame0
    sort(hp0.begin(), hp0.end(), Tuple::MagnGreater);
    
    // heap for n - 1
    Heap heap0(&hp0);

    // heap for n
    vector<Tuple> &hp1 = mTmpBuf3;
    hp1.resize(tho.size());

    int hp1Size = hp1.size();
    //BL_FLOAT *frame1MagnsBuf = frame1->mMagns.Get();
    for (int i = 0; i < hp1Size; i++)
    {
        const Tuple &t = tho[i];

        BL_FLOAT magn1 = frame1MagnsBuf[t.mBinIdx];
        
        Tuple &t1 = hp1[i];
        t1.mMagn = magn1;
        t1.mBinIdx = t.mBinIdx;
        t1.mTimeIdx = (unsigned char)1;
        t1.mIsValid = (unsigned char)0; // All empty at the beginning
    }

    // In theory, we should not have to sort (order should be the same as tho)
#if 1
    // Sort also hp1.
    // This is necessary because the order can be a bit differrent than h0,
    // and the magns are not exactly the same,
    // so the sorted orders is slightliy different
    sort(hp1.begin(), hp1.end(), Tuple::MagnGreater);
#endif
    
    Heap heap1(&hp1);
    heap1.Clear(); // "empty" heap

#if REAL_PHASE_VOCODER_PRUSA
    // For real phase vocoder 
    const BL_FLOAT as = mPitchFactor*2.0;
    const BL_FLOAT bs = mPitchFactor*2.0;
#endif
    
    // Iterate
    Tuple t;
    while(!thoV.IsEmpty())
    {
        t = Heap::Pop(&heap0, &heap1);

        // n - 1
        if (t.mTimeIdx == (unsigned char)0)
        {
            int idx = thoV.Contains(t.mBinIdx); // time=1
            
            if (idx >= 0)
            {
#if REAL_PHASE_VOCODER_PRUSA
                frame1->mEstimPhases.Get()[t.mBinIdx] =
                    frame0.mEstimPhases.Get()[t.mBinIdx] +
                    as*0.5*(frame0.mDTPhases.Get()[t.mBinIdx] +
                            frame1->mDTPhases.Get()[t.mBinIdx]);
#else // Pitch shifting
                // Use backward scheme
                // NOTE: can not use Prusa forward or centered scheme for the moment
                // (we don't have frame(n+1), we only have frame(n) and frame(n-1)
                frame1->mEstimPhases.Get()[t.mBinIdx] =
                    frame0.mEstimPhases.Get()[t.mBinIdx] +
                    frame1->mDTPhases.Get()[t.mBinIdx]*mPitchFactor;
#endif
                
                thoV.Remove(idx);
                
                //BL_FLOAT magn = frame1->mMagns.Get()[t.mBinIdx]; // No need!
                heap1.Insert(t.mBinIdx, 1); //, magn);
            }
        }

        // n
        if (t.mTimeIdx == (unsigned char)1)
        {
            // m + 1
            int idx0 = thoV.Contains(t.mBinIdx + 1); // time=1
            if (idx0 >= 0)
            {
#if REAL_PHASE_VOCODER_PRUSA
                frame1->mEstimPhases.Get()[t.mBinIdx + 1] =
                    frame1->mEstimPhases.Get()[t.mBinIdx] +
                    bs*0.5*(frame1->mDFPhases.Get()[t.mBinIdx] +
                            frame1->mDFPhases.Get()[t.mBinIdx + 1]);
#else // Pitch shift
                // Use backward scheme
                // NOTE: can not use Prusa forward or centered scheme for the moment
                // (we don't have frame(n+1), we only have frame(n) and frame(n-1)
                frame1->mEstimPhases.Get()[t.mBinIdx + 1] =
                    frame1->mEstimPhases.Get()[t.mBinIdx] +
                    // NOTE: in Theo Royer, this is "*mPitchFactor"
                    // => this is far better like in Prusa with "/mFactor"
                    frame1->mDFPhases.Get()[t.mBinIdx + 1]/mPitchFactor;
#endif

                thoV.Remove(idx0);

                // For bounds, this is already checked by "thoV.Contains()"
                //BL_FLOAT magn = frame1->mMagns.Get()[t.mBinIdx + 1]; // No need!
                heap1.Insert(t.mBinIdx + 1, 1); //, magn);
            }

            // m - 1
            int idx1 = thoV.Contains(t.mBinIdx - 1); // time=1
            if (idx1 >= 0)
            {
#if REAL_PHASE_VOCODER_PRUSA
                frame1->mEstimPhases.Get()[t.mBinIdx - 1] =
                    frame1->mEstimPhases.Get()[t.mBinIdx] -
                    bs*0.5*(frame1->mDFPhases.Get()[t.mBinIdx] +
                            frame1->mDFPhases.Get()[t.mBinIdx - 1]);
#else // Pitch shift
                // Use backward scheme
                // NOTE: can not use Prusa forward or centered scheme for the moment
                // (we don't have frame(n+1), we only have frame(n) and frame(n-1)
                frame1->mEstimPhases.Get()[t.mBinIdx - 1] =
                    // NOTE: in Theo Royer, this is "+"
                    // => this is better like in Prusa with "-"
                    frame1->mEstimPhases.Get()[t.mBinIdx] -
                    // NOTE: in Theo Royer, this is "*mPitchFactor"
                    // => this is far better like in Prusa with "/mFactor"
                    frame1->mDFPhases.Get()[t.mBinIdx - 1]/mPitchFactor;
#endif
                
                thoV.Remove(idx1);

                // For bounds, this is already checked by "thoV.Contains()"
                //BL_FLOAT magn = frame1->mMagns.Get()[t.mBinIdx - 1]; // No need!
                heap1.Insert(t.mBinIdx - 1, 1); //, magn);
            }
        }
    }
}
