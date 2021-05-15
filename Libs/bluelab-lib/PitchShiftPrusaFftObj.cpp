//
//  PitchShiftPrusaFftObj.cpp
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

#include "PitchShiftPrusaFftObj.h"

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


PitchShiftPrusaFftObj::PitchShiftPrusaFftObj(int bufferSize,
                                             int oversampling,
                                             int freqRes,
                                             BL_FLOAT sampleRate)
: ProcessObj(bufferSize)
{
    ProcessObj::Reset(bufferSize, oversampling, freqRes, sampleRate);
    
    mFactor = 1.0;
}

PitchShiftPrusaFftObj::~PitchShiftPrusaFftObj() {}

void
PitchShiftPrusaFftObj::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer0,
                                        const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer)
{
    WDL_TypedBuf<WDL_FFT_COMPLEX> &ioBuffer = mTmpBuf4;
    BLUtils::TakeHalf(*ioBuffer0, &ioBuffer);
    
    WDL_TypedBuf<BL_FLOAT> &magns = mTmpBuf0;
    WDL_TypedBuf<BL_FLOAT> &phases = mTmpBuf1;
    BLUtilsComp::ComplexToMagnPhase(&magns, &phases, ioBuffer);
    
    //
    // Convert
    //
    Convert(&magns, &phases, mFactor);
        
    BLUtilsComp::MagnPhaseToComplex(&ioBuffer, magns, phases);
    
    BLUtils::SetBuf(ioBuffer0, ioBuffer);
    
    BLUtilsFft::FillSecondFftHalf(ioBuffer0);
}

void
PitchShiftPrusaFftObj::ProcessSamplesPost(WDL_TypedBuf<BL_FLOAT> *ioBuffer)
{    
    // Src buf
    WDL_TypedBuf<BL_FLOAT> &copyBuf = mTmpBuf2;
    copyBuf = *ioBuffer;
    
    // Dst buf
    BL_FLOAT resampSizeF = (1.0/mFactor)*ioBuffer->GetSize();
    int resampSize = bl_round(resampSizeF);
    ioBuffer->Resize(resampSize);

    BLUtilsMath::LinearResample(copyBuf.Get(), copyBuf.GetSize(),
                                ioBuffer->Get(), ioBuffer->GetSize());
}

void
PitchShiftPrusaFftObj::SetFactor(BL_FLOAT factor)
{
    mFactor = factor;
}

void
PitchShiftPrusaFftObj::Reset(int bufferSize, int oversampling,
                             int freqRes, BL_FLOAT sampleRate)
{
    ProcessObj::Reset(bufferSize, oversampling, freqRes, sampleRate);
    
    ResetPitchShift();
}

void
PitchShiftPrusaFftObj::Reset()
{
    ProcessObj::Reset(mBufferSize, mOverlapping, mFreqRes, mSampleRate);
    
    ResetPitchShift();
}

// TODO: optimize with tmp buffers
void
PitchShiftPrusaFftObj::Convert(WDL_TypedBuf<BL_FLOAT> *magns,
                               WDL_TypedBuf<BL_FLOAT> *phases,
                               BL_FLOAT factor)
{
#define TOL 1e-6
    
    const Frame &frame0 = mPrevFrame;
    
    Frame &frame1 = mTmpBuf8;
    frame1.mMagns = *magns;
    frame1.mPhases = *phases;
    
    frame1.mDTPhases.Resize(magns->GetSize());
    BLUtils::FillAllZero(&frame1.mDTPhases);
    frame1.mDFPhases.Resize(magns->GetSize());
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
    
#if 0 //1
    // Naive method, without good unwrapping
    PhasesUnwrapper::UnwrapPhasesTime(frame0.mPhases, &frame1.mPhases);
    BLUtils::ComputeDiff(&frame1.mDTPhases, frame0.mPhases, frame1.mPhases);
#endif

#if 1
    // Good phases time unwrapping
    PhasesUnwrapper::ComputeUwPhasesDiffTime(&frame1.mDTPhases,
                                             frame0.mPhases, frame1.mPhases,
                                             mSampleRate, mBufferSize,
                                             mOverlapping);
#endif
    
    // Phases freq derivative
    
    WDL_TypedBuf<BL_FLOAT> &frame1PhasesFUw = mTmpBuf3;
    frame1PhasesFUw = frame1.mPhases;
    PhasesUnwrapper::UnwrapPhasesFreq(&frame1PhasesFUw);
    BLUtils::ComputeDerivative(frame1PhasesFUw, &frame1.mDFPhases);

    //PropagatePhasesSimple(frame0, &frame1, magns);
    //PropagatePhasesPrusa(frame0, &frame1);
    PropagatePhasesPrusaOptim(frame0, &frame1);
    
    // Result
    //
    
    // No need to update the magns: they have not changed!
    *phases = frame1.mEstimPhases;
    
    // Update prev data
    mPrevFrame = frame1;
}

bool
PitchShiftPrusaFftObj::Contains(const vector<Tuple> &hp, int binIdx, int timeIdx)
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

// NOTE: lower_bound() doesn't like const vectors
int
PitchShiftPrusaFftObj::ContainsSorted(/*const*/ vector<Tuple> &hp,
                                      int binIdx, int timeIdx)
{
    Tuple t;
    t.mBinIdx = binIdx;
    t.mTimeIdx = timeIdx;
    
    vector<Tuple>::iterator it =
        lower_bound(hp.begin(), hp.end(), t, Tuple::IndexSmaller);
    if ((it != hp.end()) && Tuple::IndexEqual(*it, t))
    {
        //int idx = it - hp.begin(); // Should be correct also
        int idx = distance(hp.begin(), it); // More normalized

        return idx;
    }
 
    return -1;
}

void
PitchShiftPrusaFftObj::Remove(vector<Tuple> *hp, int binIdx, int timeIdx)
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
PitchShiftPrusaFftObj::RemoveIdx(vector<Tuple> *hp, int idx)
{
    if (idx >= 0)
        hp->erase(hp->begin() + idx);
}

void
PitchShiftPrusaFftObj::AddHeap(vector<Tuple> *hp, int binIdx,
                               int timeIdx, BL_FLOAT magn)
{
    Tuple t;
    t.mMagn = magn;
    t.mBinIdx = binIdx;
    t.mTimeIdx = timeIdx;

    hp->push_back(t);
    push_heap(hp->begin(), hp->end());
}

void
PitchShiftPrusaFftObj::DBG_Dump(const char *fileName, const vector<Tuple> &hp)
{
    WDL_TypedBuf<BL_FLOAT> magns;
    magns.Resize(hp.size());

    for (int i = 0; i < hp.size(); i++)
    {
        magns.Get()[i] = hp[i].mMagn;
    }

    BLDebug::DumpData(fileName, magns);
}

void
PitchShiftPrusaFftObj::ResetPitchShift()
{
    Frame f;
    mPrevFrame = f;
}

void
PitchShiftPrusaFftObj::PropagatePhasesSimple(const Frame &frame0, Frame *frame1,
                                             WDL_TypedBuf<BL_FLOAT> *magns)
{
    // Naive algo: to do like in standard Phase Vocoder
    for (int i = 0; i < magns->GetSize(); i++)
    {
        BL_FLOAT p0 = frame0.mPhases.Get()[i];
        BL_FLOAT p1 = frame1->mPhases.Get()[i];

        BL_FLOAT ep0 = frame0.mEstimPhases.Get()[i];
        
        BL_FLOAT dp = frame1->mDTPhases.Get()[i];
        
        BL_FLOAT ep1 = ep0 +  dp*mFactor;

        frame1->mEstimPhases.Get()[i] = ep1;
    }

#if 0
    // Process magns like with Smb (and not like in standard Phase Vocoder!)
    // To be used if we don't use phase vocoder time stretch
    BLUtils::FillAllZero(magns);
    for (int i = 0; i < magns->GetSize(); i++)
    {
        int index = i*mFactor;

        if (index >= magns->GetSize())
            continue;
        
        magns->Get()[index] += frame1->mMagns.Get()[i];
    }
#endif
}

void
PitchShiftPrusaFftObj::PropagatePhasesPrusa(const Frame &frame0, Frame *frame1)
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
     
    vector<Tuple> tho;
    // Optim
    // Compute the size of tho that we need
    int thoSize = 0;
    for (int i = 0; i < frame1->mMagns.GetSize(); i++)
    {
        BL_FLOAT magn1 = frame1->mMagns.Get()[i];
        if (magn1 > abstol)
        {
            thoSize++;
        }
    }

    tho.resize(thoSize);
    int thoIdx = 0;
    
    for (int i = 0; i < frame1->mMagns.GetSize(); i++)
    {
        BL_FLOAT magn1 = frame1->mMagns.Get()[i];
        if (magn1 > abstol)
        {            
            // Add to the heap if greater than threshold            
            Tuple t;
            t.mMagn = magn1;
            t.mBinIdx = i;
            t.mTimeIdx = (unsigned char)1;
            
            //tho.push_back(t);
            tho[thoIdx++] = t;
        }
        else
        {
            // Assign random phase value otherwise 
            BL_FLOAT rp = rand()*randMaxInv;
            frame1->mEstimPhases.Get()[i] = rp;
        }
    }

    // Sort tho, for optimization
    sort(tho.begin(), tho.end(), Tuple::IndexSmaller);
         
    vector<Tuple> hp;
    hp.resize(tho.size()); // Optim
    for (int i = 0; i < tho.size(); i++)
    {
        const Tuple &thoT = tho[i];

        Tuple &t = hp[i];
        t.mMagn = frame0.mMagns.Get()[thoT.mBinIdx];
        t.mBinIdx = thoT.mBinIdx;
        t.mTimeIdx = (unsigned char)0;

        //hp.push_back(t);
        //hp[i] = t;
    }
    
    // Create the heap
    make_heap(hp.begin(), hp.end());

#if REAL_PHASE_VOCODER_PRUSA
    // For real phase vocoder 
    const BL_FLOAT as = mFactor*2.0;
    const BL_FLOAT bs = mFactor*2.0;
#endif
    
    // Iterate
    Tuple t; // Optim
    while(!tho.empty())
    {
        pop_heap(hp.begin(), hp.end());
        
        //Tuple t = hp.back();
        t = hp.back();
        hp.pop_back();
        
        if (t.mTimeIdx == (unsigned char)0)
        {
            int idx = ContainsSorted(tho, t.mBinIdx, 1);
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
                    frame1->mDTPhases.Get()[t.mBinIdx]*mFactor;
#endif
                
                RemoveIdx(&tho, idx);
                
                BL_FLOAT magn = frame1->mMagns.Get()[t.mBinIdx];
                AddHeap(&hp, t.mBinIdx, 1, magn);
            }
        }
        
        if (t.mTimeIdx == (unsigned char)1)
        {
            int idx0 = ContainsSorted(tho, t.mBinIdx + 1, 1);
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
                    // NOTE: in Theo Royer, this is "*mFactor"
                    // => this is far better like in Prusa with "/mFactor"
                    frame1->mDFPhases.Get()[t.mBinIdx + 1]/mFactor;
#endif

                RemoveIdx(&tho, idx0);

                BL_FLOAT magn = frame1->mMagns.Get()[t.mBinIdx + 1];
                AddHeap(&hp, t.mBinIdx + 1, 1, magn);
            }

            int idx1 = ContainsSorted(tho, t.mBinIdx - 1, 1);
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
                    // NOTE: in Theo Royer, this is "*mFactor"
                    // => this is far better like in Prusa with "/mFactor"
                    frame1->mDFPhases.Get()[t.mBinIdx - 1]/mFactor;
#endif
                
                RemoveIdx(&tho, idx1);

                BL_FLOAT magn = frame1->mMagns.Get()[t.mBinIdx - 1];
                AddHeap(&hp, t.mBinIdx - 1, 1, magn);
            }
        }
    }
}

void
PitchShiftPrusaFftObj::PropagatePhasesPrusaOptim(const Frame &frame0, Frame *frame1)
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
    vector<Tuple> &tho = mTmpBuf5;
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
    vector<Tuple> &hp0 = mTmpBuf6;
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
    vector<Tuple> &hp1 = mTmpBuf7;
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
    const BL_FLOAT as = mFactor*2.0;
    const BL_FLOAT bs = mFactor*2.0;
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
                    frame1->mDTPhases.Get()[t.mBinIdx]*mFactor;
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
                    // NOTE: in Theo Royer, this is "*mFactor"
                    // => this is far better like in Prusa with "/mFactor"
                    frame1->mDFPhases.Get()[t.mBinIdx + 1]/mFactor;
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
                    // NOTE: in Theo Royer, this is "*mFactor"
                    // => this is far better like in Prusa with "/mFactor"
                    frame1->mDFPhases.Get()[t.mBinIdx - 1]/mFactor;
#endif
                
                thoV.Remove(idx1);

                // For bounds, this is already checked by "thoV.Contains()"
                //BL_FLOAT magn = frame1->mMagns.Get()[t.mBinIdx - 1]; // No need!
                heap1.Insert(t.mBinIdx - 1, 1); //, magn);
            }
        }
    }
}
