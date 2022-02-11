//
//  InfraSynthProcess.h
//  BL-Air
//
//  Created by Pan on 20/04/18.
//
//

#ifndef __BL_Infra__InfraSynthProcess__
#define __BL_Infra__InfraSynthProcess__

#include <vector>
using namespace std;

#include <SineSynthSimple.h>

#define INFRA_PROCESS_PROFILE 0

#if INFRA_PROCESS_PROFILE
#include <BlaTimer.h>
#endif


class FilterIIRLow12dB;


// InfraSynthProcess: from InfraProcess2
//
class InfraSynthProcess
{
public:
    InfraSynthProcess(BL_FLOAT sampleRate);
    
    virtual ~InfraSynthProcess();
    
    void Reset();
    
    void Reset(BL_FLOAT sampleRate);
    
    BL_FLOAT NextSample(vector<SineSynthSimple::Partial> *phantomPartials,
                        vector<SineSynthSimple::Partial> *subPartials,
                        BL_FLOAT *phantomSamp = NULL, BL_FLOAT *subSamp = NULL);
    
    BL_FLOAT AmplifyOriginSample(BL_FLOAT sample);
    
    
    void SetPhantomFreq(BL_FLOAT phantomFreq);
    void SetPhantomMix(BL_FLOAT phantomMix);
    
    void SetSubOrder(int subOrder);
    void SetSubMix(BL_FLOAT subMix);
    
    void SetDebug(bool flag);
    
    void GeneratePhantomPartials(const SineSynthSimple::Partial &partial,
                                 vector<SineSynthSimple::Partial> *newPartials,
                                 bool fixedPhantomFreq = true);

    void GenerateSubPartials(const SineSynthSimple::Partial &partial,
                             vector<SineSynthSimple::Partial> *newPartials);

    // For InfraSynth and sync
    //
    
    // Method 1: synchronize oscillators in real time
    // NOTE: does not work well.
    void TriggerSync();
    
    // Method 2: add a phase to each partial at creation, to sync them
    void EnableStartSync(bool flag);
    
protected:
    BL_FLOAT mSampleRate;
    
    SineSynthSimple *mPhantomSynth;
    SineSynthSimple *mSubSynth;
    
    BL_FLOAT mPhantomFreq;
    BL_FLOAT mPhantomMix;
    int mSubOrder;
    BL_FLOAT mSubMix;
    
    // For ramps (progressively change the parameter)
    BL_FLOAT mPrevPhantomMix;
    BL_FLOAT mPrevSubMix;
        
    // Low pass filter when increasing the original signal
    // but only the low frequencies
    FilterIIRLow12dB *mLowFilter;
    
    // Low pass filter to fix
    // FIX: when generating sub octave, there are high frequencies appearing
    FilterIIRLow12dB *mSubLowFilter;
    
#if INFRA_PROCESS_PROFILE
    BlaTimer mTimer;
    long mCount;
#endif
    
    bool mEnableStartSync;
    
    bool mDebug;
};

#endif /* defined(__BL_Infra__InfraSynthProcess__) */
