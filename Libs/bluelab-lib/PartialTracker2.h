//
//  PartialTracker2.h
//  BL-SASViewer
//
//  Created by applematuer on 2/2/19.
//
//

#ifndef __BL_SASViewer__PartialTracker2__
#define __BL_SASViewer__PartialTracker2__

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

// With predictive, we have almost the same
// results than without
#define PT2_PREDICTIVE 0

#if PT2_PREDICTIVE
#include <SimpleKalmanFilter.h>

// 200Hz
#define KF_E_MEA 200.0
#define KF_E_EST KF_E_MEA

// 0.01: "oohoo" => fails when "EEAAooaa"
#define KF_Q 1.0 //0.01
#endif

class FreqAdjustObj3;
class CMASmoother;
class PartialTracker2
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
        
        static BL_FLOAT ComputeDistance2(const Partial &partial0,
                                       const Partial &partial1,
                                       BL_FLOAT sampleRate);
        
        static bool FreqLess(const Partial &p1, const Partial &p2);
        
        static bool AmpLess(const Partial &p1, const Partial &p2);
        
        static bool IdLess(const Partial &p1, const Partial &p2);
        
    public:
        int mPeakIndex;
        int mLeftIndex;
        int mRightIndex;
        
        BL_FLOAT mFreq;
        BL_FLOAT mAmp;
        BL_FLOAT mPhase;
        
        long mId;
        
        enum State mState;
        
        bool mWasAlive;
        long mZombieAge;
        
#if PT2_PREDICTIVE
        SimpleKalmanFilter mKf;
        
        BL_FLOAT mPredictedFreq;
#endif
        
    protected:
        static unsigned long mCurrentId;
    };
    
    PartialTracker2(int bufferSize, BL_FLOAT sampleRate,
                    BL_FLOAT overlapping);
    
    virtual ~PartialTracker2();
    
    void Reset();
    
    void SetThreshold(BL_FLOAT threshold);
    
    void SetData(const WDL_TypedBuf<BL_FLOAT> &magns,
                 const WDL_TypedBuf<BL_FLOAT> &phases);
    
    void DetectPartials();
    void ExtractNoiseEnvelope();
    void FilterPartials();
    
    bool GetPartials(vector<Partial> *partials);
    
    // Get the partials which are alive
    // (this avoid getting garbage partials that would never be associated)
    bool GetAlivePartials(vector<Partial> *partials);
    
    // This is not really an envelope, but the data itself
    void GetNoiseEnvelope(WDL_TypedBuf<BL_FLOAT> *noiseEnv);
    void GetHarmonicEnvelope(WDL_TypedBuf<BL_FLOAT> *harmoEnv);
    
    // TEST
    void GetNoiseEnvelopeSamplesTest(WDL_TypedBuf<BL_FLOAT> *noiseEnv);
    
    //
    static void RemoveRealDeadPartials(vector<Partial> *partials);
    
    //
    // For debugging purpose
    static void DBG_DumpPartials(const vector<Partial> &partials,
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
    
protected:
    // Detection
    void DetectPartials(const WDL_TypedBuf<BL_FLOAT> &magns,
                        const WDL_TypedBuf<BL_FLOAT> &phases,
                        vector<Partial> *partials);
    
    // Extend the foot that is closer to the peak,
    // to get a symetric partial box (around the peak)
    // (avoids one foot blocked by a barb)
    void SymetrisePartialFoot(int peakIndex,
                              int *leftIndex, int *rightIndex);
    
    // Glue the barbs to the main partial
    // Return true if some barbs have been glued
    bool GluePartialBarbs(const WDL_TypedBuf<BL_FLOAT> &magns, vector<Partial> *partials);
    
    // Avoid the partial foot to leak on the left and right
    // with very small amplitudes
    void NarrowPartialFoot(const WDL_TypedBuf<BL_FLOAT> &magns,
                           int peakIndex,
                           int *leftIndex, int *rightIndex);

    void NarrowPartialFoot(const WDL_TypedBuf<BL_FLOAT> &magns,
                           vector<Partial> *partials);
                           
    // Discard partials which are almost flat
    // (compare height of the partial, and width in the middle
    bool DiscardFlatPartial(const WDL_TypedBuf<BL_FLOAT> &magns,
                            int peakIndex, int leftIndex, int rightIndex);
    
    void DiscardFlatPartials(const WDL_TypedBuf<BL_FLOAT> &magns,
                             vector<Partial> *partials);
    
    //void DetectPartialsSmooth(const WDL_TypedBuf<BL_FLOAT> &magns,
    //                          const WDL_TypedBuf<BL_FLOAT> &phases,
    //                          const WDL_TypedBuf<BL_FLOAT> &smoothMagns,
    //                          vector<Partial> *outPartials);
    
    // Suppress partials with zero frequencies
    void SuppressBadPartials(vector<Partial> *partials);

    // Threshold
    //
    
    // Suppress the too small partials
    void ThresholdPartialsAmp(vector<Partial> *partials);
    
    // Suppress the too small partials
    void ThresholdPartialsAmpProminence(const WDL_TypedBuf<BL_FLOAT> &magns,
                                        vector<Partial> *partials);
    
    // Buggy
    BL_FLOAT ComputeProminence(const WDL_TypedBuf<BL_FLOAT> &magns,
                             int peakIndex, int leftIndex, int rightIndex);

    // Fixed
    BL_FLOAT ComputeProminence2(const WDL_TypedBuf<BL_FLOAT> &magns,
                              int peakIndex, int leftIndex, int rightIndex);
    
    BL_FLOAT ComputeHigherFoot(const WDL_TypedBuf<BL_FLOAT> &magns,
                             int leftIndex, int rightIndex);

    BL_FLOAT ComputeLowerFoot(const WDL_TypedBuf<BL_FLOAT> &magns,
                            int leftIndex, int rightIndex);

    
    // Should be independent of partials environment
    void ThresholdPartialsAmpSmooth(const WDL_TypedBuf<BL_FLOAT> &magns,
                                    vector<Partial> *partials);
    
    // Should be independent of partials environment
    void ThresholdPartialsAmpAuto(const WDL_TypedBuf<BL_FLOAT> &magns,
                                  vector<Partial> *partials);
    
    // Suppress the "barbs" (tiny partials on a side of a bigger partial)
    void SuppressBarbs(vector<Partial> *partials);
    
    // Better than not mel
    void ApplyMinSpacingMel(vector<Partial> *partials);
    
    // Still used ?
    void GlueTwinPartials(const WDL_TypedBuf<BL_FLOAT> &magns,
                          vector<Partial> *partials);
    
    void GlueTwinPartials(vector<Partial> *partials);
    
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
    
    //void ExtractNoiseEnvelopeSelect();
    
    void ExtractNoiseEnvelopeSmooth();
    
    void ExtractNoiseEnvelopeTrack();
    
    void ExtractNoiseEnvelopeTest();
    
    
    void ZeroToNextNoiseMinimum(WDL_TypedBuf<BL_FLOAT> *noise);

    void SmoothNoiseEnvelope(WDL_TypedBuf<BL_FLOAT> *noise);
    
    
    // Peak frequency computation
    //
    BL_FLOAT ComputePeakFreqSimple(int peakIndex);

    BL_FLOAT ComputePeakFreqAvg(const WDL_TypedBuf<BL_FLOAT> &magns,
                              int leftIndex, int rightIndex);

    BL_FLOAT ComputePeakFreqParabola(const WDL_TypedBuf<BL_FLOAT> &magns,
                                   int peakIndex);


    BL_FLOAT ComputePeakFreqAvg2(const WDL_TypedBuf<BL_FLOAT> &magns,
                               int leftIndex, int rightIndex);
 
    // Peak amp
    //
    BL_FLOAT ComputePeakAmpInterp(const WDL_TypedBuf<BL_FLOAT> &magns,
                                BL_FLOAT peakFreq);

    BL_FLOAT ComputePeakAmpInterpFreqObj(const WDL_TypedBuf<BL_FLOAT> &magns,
                                       BL_FLOAT peakFreq);

    //
    BL_FLOAT GetFrequency(int binIndex);

    int FindPartialById(const vector<PartialTracker2::Partial> &partials, int idx);
    
    // Associate partials
    //
    void AssociatePartials(const vector<PartialTracker2::Partial> &prevPartials,
                           vector<PartialTracker2::Partial> *currentPartials,
                           vector<PartialTracker2::Partial> *remainingPartials);
    
    // Still used ?
    void SmoothPartials(const vector<PartialTracker2::Partial> &prevPartials,
                        vector<PartialTracker2::Partial> *currentPartials);

    //
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

    CMASmoother *mSmoother;
};

#endif /* defined(__BL_SASViewer__PartialTracker2__) */
