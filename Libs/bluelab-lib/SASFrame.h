//
//  SASFrame.h
//  BL-SASViewer
//
//  Created by applematuer on 2/2/19.
//
//

#ifndef __BL_SASViewer__SASFrame__
#define __BL_SASViewer__SASFrame__

#include <vector>
using namespace std;

#include <PartialTracker3.h>

class PartialTWMEstimate;

class SASFrame
{
public:
    class SASPartial
    {
    public:
        SASPartial();
        
        SASPartial(const SASPartial &other);
        
        virtual ~SASPartial();
        
        static bool AmpLess(const SASPartial &p1, const SASPartial &p2);

    public:
        BL_FLOAT mFreq;
        BL_FLOAT mAmpDB;
        BL_FLOAT mPhase;
    };
    
    SASFrame(int bufferSize, BL_FLOAT sampleRate, int overlapping);
    
    virtual ~SASFrame();
    
    void Reset();
    
    void SetPartials(const vector<PartialTracker3::Partial> &partials);
    
    BL_FLOAT GetAmplitudeDB();
    
    BL_FLOAT GetFrequency();
    
    void GetColor(WDL_TypedBuf<BL_FLOAT> *color);
    
    void GetNormWarping(WDL_TypedBuf<BL_FLOAT> *warping);
    
    void ComputeSamples(WDL_TypedBuf<BL_FLOAT> *samples);
    
    void SetPitch(BL_FLOAT pitch);
    
protected:
    void ComputeSamplesSAS(WDL_TypedBuf<BL_FLOAT> *samples);
    
    void DBG_ComputeSamples(WDL_TypedBuf<BL_FLOAT> *samples);
    
    void Compute();
    
    // Compute steps
    //
    
    void ComputeAmplitude();
    
    void ComputeFrequency();
    
    void SelectLowFreqPartials(vector<PartialTracker3::Partial> *partials);
    void SelectHighAmpPartials(vector<PartialTracker3::Partial> *partials);

    void ComputeColor();
    void ComputeColorAux();
    
    void ComputeNormWarping();
    void ComputeNormWarpingAux();
    
    
    BL_FLOAT ApplyNormWarping(BL_FLOAT freq);

    BL_FLOAT ApplyColor(BL_FLOAT freq);

    bool FindPartial(BL_FLOAT freq);

    void GetPartial(PartialTracker3::Partial *result, int index, BL_FLOAT t);
    
    int FindPrevPartialIdx(int currentPartialIdx);

    void GetSASPartial(SASPartial *result, int index, BL_FLOAT t);
    
    // Estimate the fundamental frequency
    BL_FLOAT TWMEstimate(const vector<PartialTracker3::Partial> &partials);
    BL_FLOAT ComputeTWMError(const vector<PartialTracker3::Partial> &partials,
                           BL_FLOAT testFreq);
    BL_FLOAT ComputeErrorK(const PartialTracker3::Partial &partial,
                         BL_FLOAT harmo, BL_FLOAT Amax);
    BL_FLOAT ComputeErrorN(const PartialTracker3::Partial &partial,
                         BL_FLOAT harmo, BL_FLOAT Amax);
    BL_FLOAT GetNearestHarmonic(BL_FLOAT freq, BL_FLOAT refFreq);
    
    
    // Tracked partials
    vector<PartialTracker3::Partial> mPartials;
    vector<PartialTracker3::Partial> mPrevPartials;
    
    BL_FLOAT mAmplitudeDB;
    
    BL_FLOAT mFrequency;
    
    int mBufferSize;
    BL_FLOAT mSampleRate;
    int mOverlapping;
    
    WDL_TypedBuf<BL_FLOAT> mColor;
    
    WDL_TypedBuf<BL_FLOAT> mNormWarping;
    
    vector<SASPartial> mSASPartials;
    vector<SASPartial> mPrevSASPartials;
    
    BL_FLOAT mPitch;
    
    BL_FLOAT mPrevFrequency;
    int mPrevNumPartials;
    
    PartialTWMEstimate *mFreqEstimate;
};

#endif /* defined(__BL_SASViewer__SASFrame__) */
