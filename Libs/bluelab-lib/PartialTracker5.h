//
//  PartialTracker5.h
//  BL-SASViewer
//
//  Created by applematuer on 2/2/19.
//
//

#ifndef __BL_SASViewer__PartialTracker5__
#define __BL_SASViewer__PartialTracker5__

#include <vector>
#include <deque>
using namespace std;

#include <bl_queue.h>

#include <SimpleKalmanFilter.h>
#include <Scale.h>

#include "IPlug_include_in_plug_hdr.h"

#include "../../WDL/fft.h"

// PartialTracker2
// - from PartialTracker
//
// For association, sort by biggest amplitudes,
// and use a freqency threshold instead of taking the nearest
//
// Compute predicted next ppartial and associate using prediction
//
//
// PartialTracker3:
// - amps in Db, and magns scaled to Mel
//
// PartialTracker4:
// - fix partial amp computation
// (was sometimes really under the correct value for Infra, tested on
// "Bass-065_keep-the-bass-dubby"

// PartialTracker5: improvement of trakcng quality, for new SASViewer
// NOTE: previously test FreqAdjustObj. this was not efficient (wobbling)
class FreqAdjustObj3;
class PartialTracker5
{
public:
    // class Partial
    class Partial
    {
    public:
        enum State
        {
            ALIVE,
            ZOMBIE,
            DEAD
        };
        
        Partial();
        
        Partial(const Partial &other);
        
        virtual ~Partial();
        
        void GenNewId();
        
        //
        static bool FreqLess(const Partial &p1, const Partial &p2);
        
        static bool AmpLess(const Partial &p1, const Partial &p2);
        
        static bool IdLess(const Partial &p1, const Partial &p2);
        
        static bool CookieLess(const Partial &p1, const Partial &p2);
        
    public:
        int mPeakIndex;
        int mLeftIndex;
        int mRightIndex;
        
        // When detecting and filtering, mFreq and mAmp are "scaled and normalized"
        // After processing, we can compute the real frequencies in Hz and amp in dB.
        BL_FLOAT mFreq;
        union{
            // Inside PartialTracker5
            BL_FLOAT mAmp;
            
            // After, outside PartialTracker5, if external classes need amp in dB
            // Need to call DenormPartials() then PartialsAmpToAmpDB()
            BL_FLOAT mAmpDB;
        };
        BL_FLOAT mPhase;
        
        BL_FLOAT mPeakHeight;
        
        long mId;
        
        enum State mState;
        
        bool mWasAlive;
        long mZombieAge;
        
        long mAge;
        
        // All-purpose field
        BL_FLOAT mCookie;
        
        SimpleKalmanFilter mKf;
        BL_FLOAT mPredictedFreq;
        
    protected:
        static unsigned long mCurrentId;
    };
    
    PartialTracker5(int bufferSize, BL_FLOAT sampleRate,
                    BL_FLOAT overlapping);
    
    virtual ~PartialTracker5();
    
    void Reset();
    
    void Reset(int bufferSize, BL_FLOAT sampleRate);
    
    BL_FLOAT GetMinAmpDB();
    
    void SetThreshold(BL_FLOAT threshold);
    
    // Magn/phase
    void SetData(const WDL_TypedBuf<BL_FLOAT> &magns,
                 const WDL_TypedBuf<BL_FLOAT> &phases);
    
    void GetPreProcessedMagns(WDL_TypedBuf<BL_FLOAT> *magns);
    
    //
    void DetectPartials();
    void ExtractNoiseEnvelope();
    void FilterPartials();
    
    void GetPartials(vector<Partial> *partials);
    
    void ClearResult();
    
    // This is not really an envelope, but the data itself
    // (with holes filled)
    void GetNoiseEnvelope(WDL_TypedBuf<BL_FLOAT> *noiseEnv);
    void GetHarmonicEnvelope(WDL_TypedBuf<BL_FLOAT> *harmoEnv);
    
    // Maximum frequency we try to detect (limit for BL-Infra for example)
    void SetMaxDetectFreq(BL_FLOAT maxFreq);
    
    // Preprocess time smoth
    void SetTimeSmoothCoeff(BL_FLOAT coeff);
    
    void DBG_SetDbgParam(BL_FLOAT param);
    
    void SetTimeSmoothNoiseCoeff(BL_FLOAT coeff);
    
    // For processing result warping for example
    void PreProcessDataX(WDL_TypedBuf<BL_FLOAT> *data);
    
    // For processing result color for example, just before display
    void PreProcessDataXY(WDL_TypedBuf<BL_FLOAT> *data);
    
    // Unwrap phases for interpolatin in Mel
    void PreProcessUnwrapPhases(WDL_TypedBuf<BL_FLOAT> *magns,
                                WDL_TypedBuf<BL_FLOAT> *phases);
    
    void DenormPartials(vector<PartialTracker5::Partial> *partials);
    void DenormData(WDL_TypedBuf<BL_FLOAT> *data);
    
    void PartialsAmpToAmpDB(vector<PartialTracker5::Partial> *partials);
    
protected:
    // Pre process
    //
    void PreProcess(WDL_TypedBuf<BL_FLOAT> *magns,
                    WDL_TypedBuf<BL_FLOAT> *phases);
    
    // Apply time smooth (removes the noise and make more neat peaks), very good!
    // NOTE: Smooth only magns. Test on complex, and that was baD.
    void PreProcessTimeSmooth(WDL_TypedBuf<BL_FLOAT> *magns);
    
    // Apply A-Weighting, so the peaks at highest frequencies will not be small
    void PreProcessAWeighting(WDL_TypedBuf<BL_FLOAT> *magns, bool reverse = false);
    
    BL_FLOAT ProcessAWeighting(int binNum, int numBins,
                               BL_FLOAT magn, bool reverse);

    
    // Get the partials which are alive
    // (this avoid getting garbage partials that would never be associated)
    bool GetAlivePartials(vector<Partial> *partials);
    
    void RemoveRealDeadPartials(vector<Partial> *partials);
    
    // Detection
    //
    void DetectPartials(const WDL_TypedBuf<BL_FLOAT> &magns,
                        const WDL_TypedBuf<BL_FLOAT> &phases,
                        vector<Partial> *partials);
    
    // Peak frequency computation
    //
    BL_FLOAT ComputePeakIndexAvg(const WDL_TypedBuf<BL_FLOAT> &magns,
                                 int leftIndex, int rightIndex);
    BL_FLOAT ComputePeakIndexAvgSimple(const WDL_TypedBuf<BL_FLOAT> &magns,
                                       int leftIndex, int rightIndex);
    
    BL_FLOAT ComputePeakIndexParabola(const WDL_TypedBuf<BL_FLOAT> &magns,
                                      int peakIndex);
    
    // Advanced method
    BL_FLOAT ComputePeakIndexHalfProminenceAvg(const WDL_TypedBuf<BL_FLOAT> &magns,
                                               int peakIndex,
                                               int leftIndex, int rightIndex);
    
    // Peak amp
    //
    BL_FLOAT ComputePeakAmpInterp(const WDL_TypedBuf<BL_FLOAT> &magns,
                                  BL_FLOAT peakFreq);
    
    void ComputePeakMagnPhaseInterp(const WDL_TypedBuf<BL_FLOAT> &magns,
                                    const WDL_TypedBuf<BL_FLOAT> &unwrappedPhases,
                                    BL_FLOAT peakFreq,
                                    BL_FLOAT *peakAmp, BL_FLOAT *peakPhase);
    
    
    // Avoid the partial foot to leak on the left and right
    // with very small amplitudes
    void NarrowPartialFoot(const WDL_TypedBuf<BL_FLOAT> &magns,
                           int peakIndex,
                           int *leftIndex, int *rightIndex);
    
    void NarrowPartialFoot(const WDL_TypedBuf<BL_FLOAT> &magns,
                           vector<Partial> *partials);
    
    // Glue the barbs to the main partial
    // Return true if some barbs have been glued
    bool GluePartialBarbs(const WDL_TypedBuf<BL_FLOAT> &magns,
                          vector<Partial> *partials);
    
    // Suppress the "barbs" (tiny partials on a side of a bigger partial)
    void SuppressBarbs(vector<Partial> *partials);
    
    // Discard partials which are almost flat
    // (compare height of the partial, and width in the middle
    bool DiscardFlatPartial(const WDL_TypedBuf<BL_FLOAT> &magns,
                            int peakIndex, int leftIndex, int rightIndex);
    
    void DiscardFlatPartials(const WDL_TypedBuf<BL_FLOAT> &magns,
                             vector<Partial> *partials);
    
    bool DiscardInvalidPeaks(const WDL_TypedBuf<BL_FLOAT> &magns,
                             int peakIndex, int leftIndex, int rightIndex);

    
    // Suppress partials with zero frequencies
    void SuppressZeroFreqPartials(vector<Partial> *partials);
    
    void ThresholdPartialsPeakHeight(vector<Partial> *partials);
    
    void TimeSmoothNoise(WDL_TypedBuf<BL_FLOAT> *noise);
    
    // Peaks
    //
    
    // Fixed
    BL_FLOAT ComputePeakProminence(const WDL_TypedBuf<BL_FLOAT> &magns,
                                   int peakIndex, int leftIndex, int rightIndex);

    // Inverse of prominence
    BL_FLOAT ComputePeakHeight(const WDL_TypedBuf<BL_FLOAT> &magns,
                               int peakIndex, int leftIndex, int rightIndex);
    
    BL_FLOAT ComputePeakHeightDb(const WDL_TypedBuf<BL_FLOAT> &magns,
                               int peakIndex, int leftIndex, int rightIndex,
                               const Partial &partial);
    
    BL_FLOAT ComputePeakHigherFoot(const WDL_TypedBuf<BL_FLOAT> &magns,
                                 int leftIndex, int rightIndex);

    BL_FLOAT ComputePeakLowerFoot(const WDL_TypedBuf<BL_FLOAT> &magns,
                                  int leftIndex, int rightIndex);

    // Compute for all peaks
    void ComputePeaksHeights(const WDL_TypedBuf<BL_FLOAT> &magns,
                             vector<Partial> *partials);

    
    // Filter
    //
    void FilterPartials(vector<Partial> *result);
    
    bool TestDiscardByAmp(const Partial &p0, const Partial &p1);
    
  
    // Partials cut
    //
    void CutPartials(const vector<Partial> &partials,
                     WDL_TypedBuf<BL_FLOAT> *magns);

    void CutPartialsMinEnv(WDL_TypedBuf<BL_FLOAT> *magns);
    
    void KeepOnlyPartials(const vector<Partial> &partials,
                          WDL_TypedBuf<BL_FLOAT> *magns);

    
    // Extract noise envelope
    //
    
    void ExtractNoiseEnvelopeMax();
    
    void ExtractNoiseEnvelopeTrack();
    
    // Good
    void ExtractNoiseEnvelopeSimple();
    
    
    void ProcessMusicalNoise(WDL_TypedBuf<BL_FLOAT> *noise);

    // TEST
    void ThresholdNoiseIsles(WDL_TypedBuf<BL_FLOAT> *noise);
    
    void ZeroToNextNoiseMinimum(WDL_TypedBuf<BL_FLOAT> *noise);

    void SmoothNoiseEnvelope(WDL_TypedBuf<BL_FLOAT> *noise);
    
    void SmoothNoiseEnvelopeTime(WDL_TypedBuf<BL_FLOAT> *noise);
    

    int FindPartialById(const vector<PartialTracker5::Partial> &partials, int idx);
    
    // Associate partials
    //
    
    // Simple method, based on frequencies only
    void AssociatePartials(const vector<PartialTracker5::Partial> &prevPartials,
                           vector<PartialTracker5::Partial> *currentPartials,
                           vector<PartialTracker5::Partial> *remainingPartials);
    
    // See: https://www.dsprelated.com/freebooks/sasp/PARSHL_Program.html#app:parshlapp
    // "Peak Matching (Step 5)"
    // Use fight/winner/loser
    void AssociatePartialsPARSHL(const vector<PartialTracker5::Partial> &prevPartials,
                                 vector<PartialTracker5::Partial> *currentPartials,
                                 vector<PartialTracker5::Partial> *remainingPartials);

    // Adaptive threshold, depending on bin num;
    BL_FLOAT GetThreshold(int binNum);
    BL_FLOAT GetDeltaFreqCoeff(int binNum);

    // Optim: pre-compute a weights
    void ComputeAWeights(int numBins, BL_FLOAT sampleRate);
        
    // Debug
    void DBG_DumpPartials(const char *fileName,
                          const vector<Partial> &partials,
                          int bufferSize);

    int DenormBinIndex(int idx);
    
    //
    
    int mBufferSize;
    BL_FLOAT mSampleRate;
    int mOverlapping;
    
    BL_FLOAT mThreshold;
    
    //
    WDL_TypedBuf<BL_FLOAT> mCurrentMagns;
    WDL_TypedBuf<BL_FLOAT> mCurrentPhases;
    
    deque<vector<Partial> > mPartials;
    
    vector<Partial> mResult;
    WDL_TypedBuf<BL_FLOAT> mNoiseEnvelope;
    WDL_TypedBuf<BL_FLOAT> mHarmonicEnvelope;
    
    WDL_TypedBuf<BL_FLOAT> mSmoothWinNoise;
    
    // For SmoothNoiseEnvelopeTime()
    WDL_TypedBuf<BL_FLOAT> mPrevNoiseEnvelope;
    
    // For ComputeMusicalNoise()
    bl_queue<WDL_TypedBuf<BL_FLOAT> > mPrevNoiseMasks;
    
    //
    BL_FLOAT mMaxDetectFreq;
    
    // Debug
    BL_FLOAT mDbgParam;
    
    // For Pre-Process
    BL_FLOAT mTimeSmoothCoeff;
    // Smooth only magns (tried smooth complex, but that was bad)
    WDL_TypedBuf<BL_FLOAT> mTimeSmoothPrevMagns;
    
    // Scales
    Scale *mScale;
    Scale::Type mXScale;
    Scale::Type mYScale;
    
    Scale::Type mXScaleInv;
    Scale::Type mYScaleInv;
    
    // Time smooth noise
    BL_FLOAT mTimeSmoothNoiseCoeff;
    WDL_TypedBuf<BL_FLOAT> mTimeSmoothPrevNoise;

    // Optim
    WDL_TypedBuf<BL_FLOAT> mAWeights;
    
private:
    // Tmp buffers
    WDL_TypedBuf<BL_FLOAT> mTmpBuf0;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf1;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf2;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf3;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf4;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf5;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf6;
    
    void ReserveTmpBufs();

    WDL_TypedBuf<BL_FLOAT> mTmpBuf7;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf8;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf9;
    
    vector<Partial> mTmpPartials0;
    vector<Partial> mTmpPartials1;
    vector<Partial> mTmpPartials2;
    vector<Partial> mTmpPartials3;
    vector<Partial> mTmpPartials4;
    vector<Partial> mTmpPartials5;
    vector<Partial> mTmpPartials6;
    vector<Partial> mTmpPartials7;
    vector<Partial> mTmpPartials8;
    vector<Partial> mTmpPartials9;
    vector<Partial> mTmpPartials10;
    vector<Partial> mTmpPartials11;
    vector<Partial> mTmpPartials12;
    vector<Partial> mTmpPartials13;
    vector<Partial> mTmpPartials14;
    vector<Partial> mTmpPartials15;
    vector<Partial> mTmpPartials16;
    vector<Partial> mTmpPartials17;
    vector<Partial> mTmpPartials18;
};

#endif /* defined(__BL_SASViewer__PartialTracker5__) */
