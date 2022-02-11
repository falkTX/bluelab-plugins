//
//  SineSynth2.h
//  BL-SASViewer
//
//  Created by applematuer on 2/2/19.
//
//

#ifndef __BL_SASViewer__SineSynth2__
#define __BL_SASViewer__SineSynth2__

#include <vector>
using namespace std;

#include <PartialTracker4.h>

// SineSynth: from SASFrame
//
class WavetableSynth;
class SineSynth2
{
public:
    class Partial
    {
    public:
        Partial();
        
        Partial(const Partial &other);
        
        virtual ~Partial();

    public:
        long mId;
        
        BL_FLOAT mFreq;
        BL_FLOAT mAmpDB;
        BL_FLOAT mPhase;
    };
    
    SineSynth2(int bufferSize, BL_FLOAT sampleRate, int overlapping);
    
    virtual ~SineSynth2();
    
    void Reset(int bufferSize, BL_FLOAT sampleRate, int overlapping);
    
    void SetPartials(const vector<PartialTracker4::Partial> &partials);
    
    void ComputeSamples(WDL_TypedBuf<BL_FLOAT> *samples);
    
    // Debug
    void SetDebug(bool flag);
    
protected:
#if 0
    void ComputeSamplesWin(WDL_TypedBuf<BL_FLOAT> *samples);
    
    void ComputeSamplesPartials(WDL_TypedBuf<BL_FLOAT> *samples);
    
    // Optim
    void ComputeSamplesSAS5(WDL_TypedBuf<BL_FLOAT> *samples);
    
    void ComputeSamplesSASTable(WDL_TypedBuf<BL_FLOAT> *samples);
    void ComputeSamplesSASTable2(WDL_TypedBuf<BL_FLOAT> *samples); // Better interpolation
    void ComputeSamplesSASOverlap(WDL_TypedBuf<BL_FLOAT> *samples);
#endif
    
    // Compute steps
    //

#if 0
    // Versions to interpolate over time
    bool FindPartial(BL_FLOAT freq);

    void GetPartial(PartialTracker4::Partial *result, int index, BL_FLOAT t);
#endif
    
    int FindPrevPartialIdx(int currentPartialIdx);

    int FindPartialFromIdx(int partialIdx);

    
#if 0
    void GetPartial(Partial *result, int index, BL_FLOAT t);
#endif
    
    int mBufferSize;
    BL_FLOAT mSampleRate;
    int mOverlapping;
    
    vector<Partial> mPartials;
    vector<Partial> mPrevPartials;
    
    // For sample synth with table
    WavetableSynth *mTableSynth;
    
    bool mDebug;
};

#endif /* defined(__BL_SASViewer__SineSynth2__) */
