//
//  SineSynth3.h
//  BL-SASViewer
//
//  Created by applematuer on 2/2/19.
//
//

#ifndef __BL_SASViewer__SineSynth3__
#define __BL_SASViewer__SineSynth3__

#include <vector>
using namespace std;

#include <PartialTracker5.h>

// SineSynth: from SASFrame
//
class WavetableSynth;
class SineSynth3
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
    
    SineSynth3(int bufferSize, BL_FLOAT sampleRate, int overlapping);
    
    virtual ~SineSynth3();
    
    void Reset(int bufferSize, BL_FLOAT sampleRate, int overlapping);
    
    void SetPartials(const vector<PartialTracker5::Partial> &partials);
    
    void ComputeSamples(WDL_TypedBuf<BL_FLOAT> *samples);
    
    // Debug
    void SetDebug(bool flag);
    
protected:    
    // Compute steps
    //
    
    int FindPrevPartialIdx(int currentPartialIdx);

    int FindPartialFromIdx(int partialIdx);

    //
    
    int mBufferSize;
    BL_FLOAT mSampleRate;
    int mOverlapping;
    
    vector<Partial> mPartials;
    vector<Partial> mPrevPartials;
    
    // For sample synth with table
    WavetableSynth *mTableSynth;
    
    bool mDebug;
};

#endif /* defined(__BL_SASViewer__SineSynth3__) */
