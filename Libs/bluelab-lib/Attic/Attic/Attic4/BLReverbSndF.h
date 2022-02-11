//
//  BLReverb.h
//  UST
//
//  Created by applematuer on 1/16/20.
//
//

#ifndef UST_BLReverbSndF_h
#define UST_BLReverbSndF_h

#include <BLReverb.h>

#include "IPlug_include_in_plug_hdr.h"

extern "C" {
#include <reverb.h>
}

// TEST
#define USE_ONLY_EARLY 0 //1

// Use sndfilter
class BLReverbSndF : public BLReverb
{
public:
    BLReverbSndF(BL_FLOAT sampleRate, bool optim);
    
    BLReverbSndF(const BLReverbSndF &other);
    
    virtual ~BLReverbSndF();
    
    BLReverb *Clone() const;
    
    void Reset(BL_FLOAT sampleRate, int blockSize);
    
    // 1 to 4
    void SetOversampFactor(int factor);
    
    // 0 to 1
    void SetEarlyAmount(BL_FLOAT amount);
    // -70 to 10 dB
    void SetEarlyWet(BL_FLOAT wet);
    // -70 to 10 dB
    void SetEarlyDry(BL_FLOAT dry);
    // 0.5 to 2.5
    void SetEarlyFactor(BL_FLOAT factor);
    // -1 to 1
    void SetEarlyWidth(BL_FLOAT width);
    
    // 0 to 1
    void SetWidth(BL_FLOAT width);
    // -70 to 10 dB
    void SetWet(BL_FLOAT wet);
    // LFA amount 0.1 to 0.6
    void SetWander(BL_FLOAT wander);
    // 0 to 0.5
    void SetBassBoost(BL_FLOAT boost);
    // LFO spin 0 to 10
    void SetSpin(BL_FLOAT spin);
    // 200 to 18000Hz
    void SetInputLPF(BL_FLOAT lpf);
    // 50 to 1050Hz
    void SetBassLPF(BL_FLOAT lpf);
    // 200 to 18000Hz
    void SetDampLPF(BL_FLOAT damp);
    
    // 200 to 18000Hz
    void SetOutputLPF(BL_FLOAT lpf);
    // 0.1 to 20 seconds
    void SetRT60(BL_FLOAT rt60);
    // -0.5 to 0.5 seconds
    void SetDelay(BL_FLOAT delay);
    
    
    // Mono
    void Process(const WDL_TypedBuf<BL_FLOAT> &input,
                 WDL_TypedBuf<BL_FLOAT> *outputL,
                 WDL_TypedBuf<BL_FLOAT> *outputR);
    
    // Stereo
    void Process(const WDL_TypedBuf<BL_FLOAT> inputs[2],
                 WDL_TypedBuf<BL_FLOAT> *outputL,
                 WDL_TypedBuf<BL_FLOAT> *outputR);
    
    
    void ApplyPreset(BL_FLOAT preset[]);
    
    void DumpPreset();
    
protected:
    bool mOptim;
    
#if !USE_ONLY_EARLY
    sf_reverb_state_st mRev;
#else
    sf_rv_earlyref_st mRev;
#endif
    
    BL_FLOAT mSampleRate;
    
    //
    int mOversampFactor;
    
    // Early
    BL_FLOAT mEarlyAmount;
    BL_FLOAT mEarlyWet;
    BL_FLOAT mEarlyDry;
    BL_FLOAT mEarlyFactor;
    BL_FLOAT mEarlyWidth;
    
    // Reverb
    BL_FLOAT mWidth;
    BL_FLOAT mWet;
    BL_FLOAT mWander;
    BL_FLOAT mBassBoost;
    BL_FLOAT mSpin;
    BL_FLOAT mInputLPF;
    BL_FLOAT mBassLPF;
    BL_FLOAT mDampLPF;
    
    // Common
    BL_FLOAT mOutputLPF;
    BL_FLOAT mRT60;
    BL_FLOAT mDelay;
};

#endif
