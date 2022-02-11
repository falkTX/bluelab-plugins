//
//  PartialTracker.h
//  BL-SASViewer
//
//  Created by applematuer on 2/2/19.
//
//

#ifndef __BL_SASViewer__PartialTracker__
#define __BL_SASViewer__PartialTracker__

#include "IPlug_include_in_plug_hdr.h"

#include <vector>
#include <deque>
using namespace std;

class FreqAdjustObj3;
class CMASmoother;
class PartialTracker
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
        
        static bool IdLess(const Partial &p1, const Partial &p2);
        
        static bool IsEqual(const Partial &p1, const Partial &p2);
        
        bool operator==(const Partial &other);
        
        bool operator!=(const Partial &other);
        
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
        
    protected:
        static unsigned long mCurrentId;
    };
    
    PartialTracker(int bufferSize, BL_FLOAT sampleRate,
                   BL_FLOAT overlapping);
    
    virtual ~PartialTracker();
    
    void Reset();
    
    void SetThreshold(BL_FLOAT threshold);
    
    void SetData(const WDL_TypedBuf<BL_FLOAT> &magns,
                 const WDL_TypedBuf<BL_FLOAT> &phases);
    
    void DetectPartials();
    void ExtractNoiseEnvelope();
    void FilterPartials();
    
    // Return true if partials are available
    bool GetPartials(vector<Partial> *partials);
    
    // This is not really an envelope, but the data itself
    void GetNoiseEnvelope(WDL_TypedBuf<BL_FLOAT> *noiseEnv);
    void GetHarmonicEnvelope(WDL_TypedBuf<BL_FLOAT> *harmoEnv);

    void SetSharpnessExtractNoise(BL_FLOAT sharpness);
    
    //
    static void RemoveRealDeadPartials(vector<Partial> *partials);
    
    // For debugging purpose
    static void DBG_DumpPartials(const vector<Partial> &partials,
                                 int maxNumPartials);
    
    static void DBG_DumpPartialsAmp(const vector<Partial> &partials,
                                    int bufferSize, BL_FLOAT sampleRate);

    // Dump all the partials of a single frame
    void DBG_DumpPartials2(const vector<Partial> &partials);
    
protected:
    void DetectPartials(const WDL_TypedBuf<BL_FLOAT> &magns,
                        const WDL_TypedBuf<BL_FLOAT> &phases,
                        vector<Partial> *partials);
    
    // TEST
    //void SuppressNoisyPartials(vector<Partial> *partials);
    
    // Suppress partials with zero frequencies
    void SuppressBadPartials(vector<Partial> *partials);

    // Suppress the too small partials
    void SuppressSmallPartials(vector<Partial> *partials);
    
    // Suppress the "barbs" (tiny partials on a side of a bigger partial)
    void SuppressBarbs(vector<Partial> *partials);
    
    // TEST
    void ApplyMinSpacing(vector<Partial> *partials);
    
    // TEST 2
    void ApplyMinSpacingMel(vector<Partial> *partials);
    
    //
    void FilterPartials(vector<Partial> *result);
    
    bool TestDiscardByAmp(const Partial &p0, const Partial &p1);
    
    void HarmonicSelect(vector<Partial> *result);
    
    //
    void CutPartials(const vector<Partial> &partials,
                     WDL_TypedBuf<BL_FLOAT> *magns);

    void ExtractNoiseEnvelopeMax();
    
    void ExtractNoiseEnvelopeSelect();
    
    void ExtractNoiseEnvelopeSmooth();
    
    //void SuppressZeroFreqPartialEnv();
    
    // Test
    void ExtractNoiseEnvelopeTrack();
    
    // Peak frequency computation
    BL_FLOAT ComputePeakFreqSimple(int peakIndex);

    BL_FLOAT ComputePeakFreqAvg(const WDL_TypedBuf<BL_FLOAT> &magns,
                              int leftIndex, int rightIndex);

    BL_FLOAT ComputePeakFreqParabola(const WDL_TypedBuf<BL_FLOAT> &magns,
                                   int peakIndex);


    BL_FLOAT ComputePeakFreqAvg2(const WDL_TypedBuf<BL_FLOAT> &magns,
                               int leftIndex, int rightIndex);
    
    BL_FLOAT ComputePeakAmpAvg(const WDL_TypedBuf<BL_FLOAT> &magns,
                             BL_FLOAT peakFreq);

    BL_FLOAT ComputePeakAmpAvgFreqObj(const WDL_TypedBuf<BL_FLOAT> &magns,
                                    BL_FLOAT peakFreq);

    BL_FLOAT GetFrequency(int binIndex);

    int FindPartialById(const vector<PartialTracker::Partial> &partials, int idx);
    
    void AssociatePartialsMin(const vector<PartialTracker::Partial> &prevPartials,
                              vector<PartialTracker::Partial> *currentPartials,
                              vector<PartialTracker::Partial> *remainingPartials);
    
    void AssociatePartialsMinAux1(const vector<PartialTracker::Partial> &prevPartials,
                                  vector<PartialTracker::Partial> *currentPartials,
                                  vector<PartialTracker::Partial> *remainingPartials);
    
    void AssociatePartialsMinAux2(const vector<PartialTracker::Partial> &prevPartials,
                                  vector<PartialTracker::Partial> *currentPartials,
                                  vector<PartialTracker::Partial> *remainingPartials);
    
    void SmoothPartials(const vector<PartialTracker::Partial> &prevPartials,
                        vector<PartialTracker::Partial> *currentPartials);
    
#if 0
    void AssociatePartialsPermut(const vector<PartialTracker::Partial> &prevPartials,
                                 vector<PartialTracker::Partial> *currentPartials,
                                 vector<PartialTracker::Partial> *remainingPartials);
    
    void AssociatePartialsPermutAux1(const vector<PartialTracker::Partial> &prevPartials,
                                     vector<PartialTracker::Partial> *currentPartials,
                                     vector<PartialTracker::Partial> *remainingPartials);
    void ComputeDistances(const vector<PartialTracker::Partial> &prevPartials,
                          const vector<PartialTracker::Partial> &currentPartials,
                          vector<vector<BL_FLOAT> > *distances,
                          vector<vector<int> > *ids);
#endif
    
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
    
    // For noise discard
    //WDL_TypedBuf<BL_FLOAT> mPrevPhases;
    
    BL_FLOAT mSharpnessExtractNoise;

    CMASmoother *mSmoother;
};

#endif /* defined(__BL_SASViewer__PartialTracker__) */
