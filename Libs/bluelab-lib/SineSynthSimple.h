//
//  SineSynthSimple.h
//  BL-SASViewer
//
//  Created by applematuer on 2/2/19.
//
//

#ifndef __BL_SASViewer__SineSynthSimple__
#define __BL_SASViewer__SineSynthSimple__

#include <vector>
using namespace std;

#include <PartialTracker4.h>

// SineSynth: from SASFrame
//
// SineSynth2: from SineSynth, for InfraSynthProcess
// - code clean
// - generates sample by sample

#define INFRA_SYNTH_OPTIM3 1

class SineSynthSimple
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
        BL_FLOAT mPhase;
        
#if !INFRA_SYNTH_OPTIM3
        BL_FLOAT mAmpDB;
#else
        BL_FLOAT mAmp;
#endif
    };
    
    SineSynthSimple(BL_FLOAT sampleRate);
    
    virtual ~SineSynthSimple();
    
    void Reset(BL_FLOAT sampleRate);
    
    // Simple
    BL_FLOAT NextSample(vector<Partial> *partials);
    
    // For InfraSynth
    void TriggerSync();
    
    void SetDebug(bool flag);
    
protected:
    BL_FLOAT mSampleRate;
    
    BL_FLOAT mSyncDirection;
    
    bool mDebug;
};

#endif /* defined(__BL_SASViewer__SineSynthSimple__) */
