//
//  PartialTracker4.h
//  BL-SASViewer
//
//  Created by applematuer on 2/2/19.
//
//

#ifndef __BL_SASViewer__PartialTracker4__
#define __BL_SASViewer__PartialTracker4__

#include "IPlug_include_in_plug_hdr.h"

#include <vector>
#include <deque>
using namespace std;

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

// With predictive, we have almost the same
// results than without
#define PREDICTIVE 1 //0 // 1 // TEST SUNDAY: activate predict

#if PREDICTIVE
#include <SimpleKalmanFilter.h>

// "How much do we expect to our measurement vary"
//
// 200Hz
#define KF_E_MEA 200.0 //50.0 //200.0 // TEST SUNDAY, was 200.0
#define KF_E_EST KF_E_MEA

// "usually a small number between 0.001 and 1"
//
// If too low: predicted values move too slowly
// If too high: predicted values go straight
//
// 0.01: "oohoo" => fails when "EEAAooaa"
#define KF_Q 5.0 //2.0 //10.0 //10.0 //5.0 //1.0 //0.01 // TEST SUNDAY, was 1.0
#endif

// Buffer size and sample rate were not updated during Reset()
// FIX: Infra: avoid computing wrong max frequency (500Hz),
// when using higher sample rates
#define FIX_INFRA_SAMPLERATE 1


class FreqAdjustObj3;
class PartialTracker4
{
public:
    // Struct Partial
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
        
        BL_FLOAT mFreq;
        BL_FLOAT mAmpDB;
        BL_FLOAT mPhase;
        
        long mId;
        
        enum State mState;
        
        bool mWasAlive;
        long mZombieAge;
        
        long mAge;
        
        // All-purpose field
        BL_FLOAT mCookie;
        
#if PREDICTIVE
        SimpleKalmanFilter mKf;
        
        BL_FLOAT mPredictedFreq;
#endif
        
    protected:
        static unsigned long mCurrentId;
    };
    
    PartialTracker4(int bufferSize, BL_FLOAT sampleRate,
                    BL_FLOAT overlapping);
    
    virtual ~PartialTracker4();
    
    void Reset();
    
#if FIX_INFRA_SAMPLERATE
    void Reset(int bufferSize, BL_FLOAT sampleRate);
#endif
    
    void SetThreshold(BL_FLOAT threshold);
    
    void SetData(const WDL_TypedBuf<BL_FLOAT> &magns,
                 const WDL_TypedBuf<BL_FLOAT> &phases);
    
    void DetectPartials();
    void ExtractNoiseEnvelope();
    void FilterPartials();
    
    void GetPartials(vector<Partial> *partials);
    
    void ClearResult();
    
    // This is not really an envelope, but the data itself
    // (with holes filled)
    void GetNoiseEnvelope(WDL_TypedBuf<BL_FLOAT> *noiseEnv);
    void GetHarmonicEnvelope(WDL_TypedBuf<BL_FLOAT> *harmoEnv);
    
    // OPTIM PROF Infra
    void SetMaxDetectFreq(BL_FLOAT maxFreq);
    
    // Debug
    static void DBG_DumpPartials(const vector<Partial> &partials,
                                 int maxNumPartials);
    
    static void DBG_DumpPartials2(const char *fileName,
                                  const vector<Partial> &partials,
                                  int bufferSize, BL_FLOAT sampleRate);
    
    static void DBG_DumpPartials2Mel(const char *fileName,
                                     const vector<Partial> &partials,
                                     int bufferSize, BL_FLOAT sampleRate);
    
protected:
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
    
    BL_FLOAT ComputePeakIndexParabola(const WDL_TypedBuf<BL_FLOAT> &magns,
                                    int peakIndex);
    
    // Peak amp
    //
    BL_FLOAT ComputePeakAmpInterp(const WDL_TypedBuf<BL_FLOAT> &magns,
                                BL_FLOAT peakFreq);
    
    BL_FLOAT ComputePeakAmpInterpFreqObj(const WDL_TypedBuf<BL_FLOAT> &magns,
                                       BL_FLOAT peakFreq);
    
    BL_FLOAT ComputePeakPhaseInterp(const WDL_TypedBuf<BL_FLOAT> &phases,
                                  BL_FLOAT peakFreq);
    
    // Extend the foot that is closer to the peak,
    // to get a symetric partial box (around the peak)
    // (avoids one foot blocked by a barb)
    void SymetrisePartialFoot(int peakIndex,
                              int *leftIndex, int *rightIndex);
    
    // Avoid the partial foot to leak on the left and right
    // with very small amplitudes
    void NarrowPartialFoot(const WDL_TypedBuf<BL_FLOAT> &magns,
                           int peakIndex,
                           int *leftIndex, int *rightIndex);
    
    void NarrowPartialFoot(const WDL_TypedBuf<BL_FLOAT> &magns,
                           vector<Partial> *partials);
    
    // Glue the barbs to the main partial
    // Return true if some barbs have been glued
    bool GluePartialBarbs(const WDL_TypedBuf<BL_FLOAT> &magns, vector<Partial> *partials);
    
    // Suppress the "barbs" (tiny partials on a side of a bigger partial)
    void SuppressBarbs(vector<Partial> *partials);
    
    // Still used ?
    void GlueTwinPartials(const WDL_TypedBuf<BL_FLOAT> &magns,
                          vector<Partial> *partials);
    
    void GlueTwinPartials(vector<Partial> *partials);
    
    // Discard partials which are almost flat
    // (compare height of the partial, and width in the middle
    bool DiscardFlatPartial(const WDL_TypedBuf<BL_FLOAT> &magns,
                            int peakIndex, int leftIndex, int rightIndex);
    
    void DiscardFlatPartials(const WDL_TypedBuf<BL_FLOAT> &magns,
                             vector<Partial> *partials);
    
    // Suppress partials with zero frequencies
    void SuppressZeroFreqPartials(vector<Partial> *partials);

    // Better than not mel
    void ApplyMinSpacingMel(vector<Partial> *partials);
    
    // Threshold
    //
    
    // Suppress the too small partials
    void ThresholdPartialsAmp(vector<Partial> *partials);
    
    // Suppress the too small partials
    void ThresholdPartialsPeakProminence(const WDL_TypedBuf<BL_FLOAT> &magns,
                                         vector<Partial> *partials);
    
    void ThresholdPartialsPeakHeight(const WDL_TypedBuf<BL_FLOAT> &magns,
                                     vector<Partial> *partials);
    
    void ThresholdPartialsPeakHeightDb(const WDL_TypedBuf<BL_FLOAT> &magns,
                                       vector<Partial> *partials);
    
    void ThresholdPartialsPeakHeightPink(const WDL_TypedBuf<BL_FLOAT> &magns,
                                         vector<Partial> *partials);
    
    // Should be independent of partials environment
    void ThresholdPartialsAmpSmooth(const WDL_TypedBuf<BL_FLOAT> &magns,
                                    vector<Partial> *partials);
    
    // Should be independent of partials environment
    void ThresholdPartialsAmpAuto(const WDL_TypedBuf<BL_FLOAT> &magns,
                                  vector<Partial> *partials);
    
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

    //
    BL_FLOAT GetFrequency(int binIndex);

    int FindPartialById(const vector<PartialTracker4::Partial> &partials, int idx);
    
    // Associate partials
    //
    void AssociatePartials(const vector<PartialTracker4::Partial> &prevPartials,
                           vector<PartialTracker4::Partial> *currentPartials,
                           vector<PartialTracker4::Partial> *remainingPartials);
    
    // Still used ?
    void SmoothPartials(const vector<PartialTracker4::Partial> &prevPartials,
                        vector<PartialTracker4::Partial> *currentPartials);

    
    // Debug
    //
    static void DBG_DumpPartials(const char *fileName,
                                 const vector<Partial> &partials,
                                 int maxNumPartials);
    
    static void DBG_DumpPartialsAmp(const vector<Partial> &partials,
                                    int bufferSize, BL_FLOAT sampleRate);
    
    static void DBG_DumpPartialsAmp(const char *fileName,
                                    const vector<Partial> &partials,
                                    int bufferSize, BL_FLOAT sampleRate);
    
    static void DBG_DumpPartialsBox(const vector<Partial> &partials,
                                    int bufferSize, BL_FLOAT sampleRate);
    
    static void DBG_DumpPartialsBox(const char *fileName,
                                    const vector<Partial> &partials,
                                    int bufferSize, BL_FLOAT sampleRate);
    
    // Dump all the partials of a single frame
    void DBG_DumpPartials2(const vector<Partial> &partials);
    
    void DBG_DumpPartials2(const char *fileName,
                           const vector<Partial> &partials);
    
    // Same, with mel scale
    void DBG_DumpPartials2Mel(const vector<Partial> &partials);
    
    void DBG_DumpPartials2Mel(const char *fileName,
                              const vector<Partial> &partials);
    
    // Same, with partial height
    void DBG_DumpPartials2MelHeight(const vector<Partial> &partials,
                                    const WDL_TypedBuf<BL_FLOAT> &magns);
    
    void DBG_DumpPartials2MelHeight(const char *fileName,
                                    const vector<Partial> &partials,
                                    const WDL_TypedBuf<BL_FLOAT> &magns);
    
    
    int mBufferSize;
    BL_FLOAT mSampleRate;
    int mOverlapping;
    
    BL_FLOAT mThreshold;
    
    deque<vector<Partial> > mPartials;
    
    vector<Partial> mResult;
    WDL_TypedBuf<BL_FLOAT> mNoiseEnvelope;
    WDL_TypedBuf<BL_FLOAT> mHarmonicEnvelope;
    
    FreqAdjustObj3 *mFreqObj;
    WDL_TypedBuf<BL_FLOAT> mRealFreqs;
    
    WDL_TypedBuf<BL_FLOAT> mPrevMagns;
    
    //
    WDL_TypedBuf<BL_FLOAT> mCurrentMagns;
    WDL_TypedBuf<BL_FLOAT> mCurrentPhases;
    
    // Data smooth
    //
    WDL_TypedBuf<BL_FLOAT> mSmoothWinDetect;
    WDL_TypedBuf<BL_FLOAT> mCurrentSmoothMagns;
    
    WDL_TypedBuf<BL_FLOAT> mSmoothWinThreshold;
    
    WDL_TypedBuf<BL_FLOAT> mSmoothWinNoise;
    
    // For SmoothNoiseEnvelopeTime()
    WDL_TypedBuf<BL_FLOAT> mPrevNoiseEnvelope;
    
    // For ComputeMusicalNoise()
    deque<WDL_TypedBuf<BL_FLOAT> > mPrevNoiseMasks;
    
    //
    BL_FLOAT mMaxDetectFreq;
};

#endif /* defined(__BL_SASViewer__PartialTracker4__) */
