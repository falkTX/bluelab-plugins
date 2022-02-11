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

class FreqAdjustObj3;
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
        
        static double ComputeDistance2(const Partial &partial0,
                                       const Partial &partial1,
                                       double sampleRate);
        
        static bool FreqLess(const Partial &p1, const Partial &p2);
        
        static bool AmpLess(const Partial &p1, const Partial &p2);
        
        static bool IdLess(const Partial &p1, const Partial &p2);
        
#if 0
        // For HarmonicSelect
        bool operator==(const Partial &other);
        
        bool operator!=(const Partial &other);
#endif
        
    public:
        int mPeakIndex;
        int mLeftIndex;
        int mRightIndex;
        
        double mFreq;
        double mAmp;
        double mPhase;
        
        long mId;
        
        enum State mState;
        
        bool mWasAlive;
        long mZombieAge;
        
    protected:
        static unsigned long mCurrentId;
    };
    
    PartialTracker2(int bufferSize, double sampleRate,
                   double overlapping);
    
    virtual ~PartialTracker2();
    
    void Reset();
    
    void SetThreshold(double threshold);
    
    void SetData(const WDL_TypedBuf<double> &magns,
                 const WDL_TypedBuf<double> &phases);
    
    void DetectPartials();
    void ExtractNoiseEnvelope();
    void FilterPartials();
    
    // Return true if partials are available
    bool GetPartials(vector<Partial> *partials);
    
    // get the partials which are alive
    // or were alive and are now zombies
    // (this avoid getting garbage partials that never be associated)
    bool GetAlivePartials(vector<Partial> *partials);
    
    // This is not really an envelope, but the data itself
    void GetNoiseEnvelope(WDL_TypedBuf<double> *noiseEnv);
    void GetHarmonicEnvelope(WDL_TypedBuf<double> *harmoEnv);

    void SetSharpnessExtractNoise(double sharpness);
    
    //
    static void RemoveRealDeadPartials(vector<Partial> *partials);
    
    // For debugging purpose
    static void DBG_DumpPartials(const vector<Partial> &partials,
                                 int maxNumPartials);
    
    static void DBG_DumpPartialsAmp(const vector<Partial> &partials,
                                    int bufferSize, double sampleRate);

    static void DBG_DumpPartialsAmp(const char *fileName,
                                    const vector<Partial> &partials,
                                    int bufferSize, double sampleRate);
    
    static void DBG_DumpPartialsBox(const vector<Partial> &partials,
                                    int bufferSize, double sampleRate);
    
    static void DBG_DumpPartialsBox(const char *fileName,
                                    const vector<Partial> &partials,
                                    int bufferSize, double sampleRate);
    
    // Dump all the partials of a single frame
    void DBG_DumpPartials2(const vector<Partial> &partials);
    
protected:
    void DetectPartials(const WDL_TypedBuf<double> &magns,
                        const WDL_TypedBuf<double> &phases,
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
    
    void GlueTwinPartials(const WDL_TypedBuf<double> &magns,
                          vector<Partial> *partials);
    void GlueTwinPartials(vector<Partial> *partials);
    
    //
    void FilterPartials(vector<Partial> *result);
    
    bool TestDiscardByAmp(const Partial &p0, const Partial &p1);
    
#if 0
    void HarmonicSelect(vector<Partial> *result);
#endif
    
    //
    void CutPartials(const vector<Partial> &partials,
                     WDL_TypedBuf<double> *magns);

    void ExtractNoiseEnvelopeMax();
    
    void ExtractNoiseEnvelopeSelect();
    
    void ExtractNoiseEnvelopeSmooth();
    
    //void SuppressZeroFreqPartialEnv();
    
    // Test
    void ExtractNoiseEnvelopeTrack();
    
    // Peak frequency computation
    double ComputePeakFreqSimple(int peakIndex);

    double ComputePeakFreqAvg(const WDL_TypedBuf<double> &magns,
                              int leftIndex, int rightIndex);

    double ComputePeakFreqParabola(const WDL_TypedBuf<double> &magns,
                                   int peakIndex);


    double ComputePeakFreqAvg2(const WDL_TypedBuf<double> &magns,
                               int leftIndex, int rightIndex);
    
    double ComputePeakAmpAvg(const WDL_TypedBuf<double> &magns,
                             double peakFreq);

    double ComputePeakAmpAvgFreqObj(const WDL_TypedBuf<double> &magns,
                                    double peakFreq);

    double GetFrequency(int binIndex);

    int FindPartialById(const vector<PartialTracker2::Partial> &partials, int idx);
    
    void AssociatePartialsMin(const vector<PartialTracker2::Partial> &prevPartials,
                              vector<PartialTracker2::Partial> *currentPartials,
                              vector<PartialTracker2::Partial> *remainingPartials);
    
    void AssociatePartialsMinAux1(const vector<PartialTracker2::Partial> &prevPartials,
                                  vector<PartialTracker2::Partial> *currentPartials,
                                  vector<PartialTracker2::Partial> *remainingPartials);
    
    void AssociatePartialsMinAux2(const vector<PartialTracker2::Partial> &prevPartials,
                                  vector<PartialTracker2::Partial> *currentPartials,
                                  vector<PartialTracker2::Partial> *remainingPartials);
    
    void AssociatePartialsBiggest(const vector<PartialTracker2::Partial> &prevPartials,
                                  vector<PartialTracker2::Partial> *currentPartials,
                                  vector<PartialTracker2::Partial> *remainingPartials);
    
    void SmoothPartials(const vector<PartialTracker2::Partial> &prevPartials,
                        vector<PartialTracker2::Partial> *currentPartials);

    
    int mBufferSize;
    double mSampleRate;
    int mOverlapping;
    
    double mThreshold;
    
    deque<vector<Partial> > mPartials;
    
    vector<Partial> mResult;
    WDL_TypedBuf<double> mNoiseEnvelope;
    WDL_TypedBuf<double> mHarmonicEnvelope;
    
    FreqAdjustObj3 *mFreqObj;
    WDL_TypedBuf<double> mRealFreqs;
    
    WDL_TypedBuf<double> mPrevMagns;
    
    //
    WDL_TypedBuf<double> mCurrentMagns;
    WDL_TypedBuf<double> mCurrentPhases;
    
    // For noise discard
    //WDL_TypedBuf<double> mPrevPhases;
    
    double mSharpnessExtractNoise;
};

#endif /* defined(__BL_SASViewer__PartialTracker2__) */
