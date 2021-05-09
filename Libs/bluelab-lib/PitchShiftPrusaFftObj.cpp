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

// We don need phases unwrapping!
#include <PhasesUnwrapper.h>

#include "PitchShiftPrusaFftObj.h"

// Exactly like the "Phase Vocoder Done Right" ?
// Or adapted for pitch shifting?
#define REAL_PHASE_VOCODER_PURNA 0 //1

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
    WDL_TypedBuf<WDL_FFT_COMPLEX> &ioBuffer = mTmpBuf6;
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
    //ProcessObj::Reset();
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
    
    Frame frame1;
    frame1.mMagns = *magns;
    frame1.mPhases = *phases;

#if 0 // TEST
    PhasesUnwrapper::UnwrapPhasesFreq(&frame1.mPhases);
#endif
    
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

        return;
    }
    
    // Pre-processing
    //

    // Unwrap phases in time

#if 0 //1 // GOOD
    // TEST
    //
    WDL_TypedBuf<BL_FLOAT> frame1PhasesTUw = frame1.mPhases;
    PhasesUnwrapper::UnwrapPhasesTime(frame0.mPhases, &frame1PhasesTUw);
    BLUtils::ComputeDiff(&frame1.mDTPhases, frame0.mPhases, frame1PhasesTUw);
 #endif

#if 1 //0 //1 // NEW
    PhasesUnwrapper::ComputeUwPhasesDiffTime(&frame1.mDTPhases,
                                             frame0.mPhases, frame1.mPhases,
                                             mSampleRate, mBufferSize,
                                             mOverlapping);
#endif
    
#if 0 //1
    // ORIGIN
    
    PhasesUnwrapper::UnwrapPhasesTime(frame0.mPhases, &frame1.mPhases);
    
    // Phases time derivative
    BLUtils::ComputeDiff(&frame1.mDTPhases, frame0.mPhases, frame1.mPhases);
#endif
    
    // Phases freq derivative

#if 0 //1
    // TEST
    WDL_TypedBuf<BL_FLOAT> frame1PhasesFUw = frame1.mPhases;
    PhasesUnwrapper::UnwrapPhasesFreq(&frame1PhasesFUw);
    BLUtils::ComputeDerivative(frame1PhasesFUw, &frame1.mDFPhases);
#endif

#if 0
    // ORIG
    BLUtils::ComputeDerivative(frame1.mPhases, &frame1.mDTPhases);
#endif
    
#define SIMPLE_DBG_ALGO 1 //0 //1
#if SIMPLE_DBG_ALGO
    for (int i = 0; i < magns->GetSize(); i++)
    {
        //BL_FLOAT p0 = frame0.mPhases.Get()[i];
        //BL_FLOAT p1 = frame1.mPhases.Get()[i];
        //BL_FLOAT dp = p1 - p0;
        
        BL_FLOAT dp = frame1.mDTPhases.Get()[i];
        
        BL_FLOAT ep0 = frame0.mEstimPhases.Get()[i];
    
        BL_FLOAT ep1 = ep0 + dp*mFactor;
        
        frame1.mEstimPhases.Get()[i] = ep1;
    }

    *phases = frame1.mEstimPhases;

    mPrevFrame = frame1;

    return;
#endif
    
    // Compute tolerance
    BL_FLOAT maxMagn0 = BLUtils::ComputeMax(frame0.mMagns);
    BL_FLOAT maxMagn1 = BLUtils::ComputeMax(frame1.mMagns);
    BL_FLOAT maxMagn = MAX(maxMagn0, maxMagn1);

    BL_FLOAT abstol = TOL*maxMagn;
    
    // Set random values for very small magns
    // and insert other values in the heap

    const BL_FLOAT randMaxInv = 1.0/RAND_MAX;
     
    vector<Tuple> tho;
    for (int i = 0; i < frame1.mMagns.GetSize(); i++)
    {
        BL_FLOAT magn1 = frame1.mMagns.Get()[i];
        if (magn1 > abstol)
        {            
            // Add to the heap if greater than threshold            
            Tuple t;
            t.mMagn = magn1;
            t.mBinIdx = i;
            t.mTimeIdx = 1;
            
            tho.push_back(t);
        }
        else
        {
            // Assign random phase value otherwise 
            BL_FLOAT rp = rand()*randMaxInv;
            frame1.mEstimPhases.Get()[i] = rp;
        }
    }

    // Sort tho, for optimization
    sort(tho.begin(), tho.end(), Tuple::IndexSmaller);
         
    vector<Tuple> hp;
    for (int i = 0; i < tho.size(); i++)
    {
        const Tuple &thoT = tho[i];

        Tuple t;
        t.mMagn = frame0.mMagns.Get()[thoT.mBinIdx];
        t.mBinIdx = thoT.mBinIdx;
        t.mTimeIdx = 0;

        hp.push_back(t);
    }
    
    // Create the heap
    make_heap(hp.begin(), hp.end());

#if REAL_PHASE_VOCODER_PURNA
    // For real phase vocoder 
    const BL_FLOAT as = mFactor*2.0;
    const BL_FLOAT bs = mFactor*2.0;
#endif
    
    // Iterate
    while(!tho.empty())
    {
        pop_heap(hp.begin(), hp.end());
        
        Tuple t = hp.back();
        hp.pop_back();
        
        if (t.mTimeIdx == 0)
        {
            int idx = ContainsSorted(tho, t.mBinIdx, 1);
            if (idx >= 0)
            {
#if REAL_PHASE_VOCODER_PURNA
                frame1.mEstimPhases.Get()[t.mBinIdx] =
                    frame0.mEstimPhases.Get()[t.mBinIdx] +
                    as*0.5*(frame0.mDTPhases.Get()[t.mBinIdx] +
                            frame1.mDTPhases.Get()[t.mBinIdx]);
#else // Pitch shifting
                frame1.mEstimPhases.Get()[t.mBinIdx] =
                    frame0.mEstimPhases.Get()[t.mBinIdx] +
                    frame1.mDTPhases.Get()[t.mBinIdx]*mFactor;
#endif
                
                RemoveIdx(&tho, idx);
                
                BL_FLOAT magn = frame1.mMagns.Get()[t.mBinIdx];
                AddHeap(&hp, t.mBinIdx, 1, magn);
            }
        }

        if (t.mTimeIdx == 1)
        {
            int idx0 = ContainsSorted(tho, t.mBinIdx + 1, 1);
            if (idx0 >= 0)
            {
#if REAL_PHASE_VOCODER_PURNA
                frame1.mEstimPhases.Get()[t.mBinIdx + 1] =
                    frame1.mEstimPhases.Get()[t.mBinIdx] +
                    bs*0.5*(frame1.mDFPhases.Get()[t.mBinIdx] +
                            frame1.mDFPhases.Get()[t.mBinIdx + 1]);
#else // Pitch shift
                frame1.mEstimPhases.Get()[t.mBinIdx + 1] =
                    frame1.mEstimPhases.Get()[t.mBinIdx] +
                    frame1.mDFPhases.Get()[t.mBinIdx + 1]*mFactor;
#endif

                RemoveIdx(&tho, idx0);

                BL_FLOAT magn = frame1.mMagns.Get()[t.mBinIdx + 1];
                AddHeap(&hp, t.mBinIdx + 1, 1, magn);
            }

            int idx1 = ContainsSorted(tho, t.mBinIdx - 1, 1);
            if (idx1 >= 0)
            {
#if REAL_PHASE_VOCODER_PURNA
                frame1.mEstimPhases.Get()[t.mBinIdx - 1] =
                    frame1.mEstimPhases.Get()[t.mBinIdx] -
                    bs*0.5*(frame1.mDFPhases.Get()[t.mBinIdx] +
                            frame1.mDFPhases.Get()[t.mBinIdx - 1]);
#else // Pitch shift
                frame1.mEstimPhases.Get()[t.mBinIdx - 1] =
                    frame1.mEstimPhases.Get()[t.mBinIdx] /*-*//*+*/ +
                    frame1.mDFPhases.Get()[t.mBinIdx - 1]*mFactor;
#endif
                
                RemoveIdx(&tho, idx1);

                BL_FLOAT magn = frame1.mMagns.Get()[t.mBinIdx - 1];
                AddHeap(&hp, t.mBinIdx - 1, 1, magn);
            }
        }
    }

    // Result
    //PhasesUnwrapper::UnwrapPhasesFreq(&frame1.mEstimPhases);
    
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
