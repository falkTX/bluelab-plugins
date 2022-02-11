//
//  EnvelopeGenerator3.h
//  Synthesis
//
//  Created by Martin on 08.04.14.
//
//

#ifndef __Synthesis__EnvelopeGenerator3__
#define __Synthesis__EnvelopeGenerator3__

#include <cmath>

// EnvelopeGenerator2: from EnvelopeGenerator
// - added more shape parameters
//
// EnvelopeGenerator3: from EnvelopeGenerator2
// - added more parameters ("end", etcc.)
class EnvelopeGenerator3 {
public:
    enum EnvelopeStage {
        ENVELOPE_STAGE_OFF = 0,
        ENVELOPE_STAGE_ATTACK,
        ENVELOPE_STAGE_DECAY,
        ENVELOPE_STAGE_SUSTAIN,
        ENVELOPE_STAGE_RELEASE,
        ENVELOPE_STAGE_END,
        kNumEnvelopeStages
    };
    void EnterStage(EnvelopeStage newStage);
    double NextSample();
    void SetSampleRate(double newSampleRate);
    inline EnvelopeStage GetCurrentStage() const { return mCurrentStage; };
    const double mMinimumLevel;
    
    EnvelopeGenerator3() :
    mMinimumLevel(0.0001),
    mCurrentStage(ENVELOPE_STAGE_OFF),
    mCurrentLevel(mMinimumLevel),
    mMultiplier(1.0),
    mSampleRate(44100.0),
    mCurrentSampleIndex(0),
    mNextStageSampleIndex(0),
    mAttackPeak(1.0),
    mAttackShape(1.0),
    mDecayPeak(0.0),
    mDecayShape(1.0),
    mSustainDelay(1.0),
    mSustainShape(1.0),
    mReleaseShape(1.0),
    mEndPeak(1.0),
    mEndShape(1.0)
    {
        mStageValue[ENVELOPE_STAGE_OFF] = 0.0;
        mStageValue[ENVELOPE_STAGE_ATTACK] = 0.01;
        mStageValue[ENVELOPE_STAGE_DECAY] = 0.5;
        mStageValue[ENVELOPE_STAGE_SUSTAIN] = 0.1;
        mStageValue[ENVELOPE_STAGE_RELEASE] = 1.0;
        mStageValue[ENVELOPE_STAGE_END] = 0.0;
        
        // Niko: advanced attack
        mAttackStartLevel = 0.0;
        mAttackEndLevel = 0.0;
        mAttackTStep = 1.0;
        mCurrentAttackT = 0.0;
        
        // Niko: advanced ADSR
        mDecayStartLevel = 0.0;
        mDecayEndLevel = 0.0;
        mDecayTStep = 1.0;
        mCurrentDecayT = 0.0;
        
        mSustainStartLevel = 0.0;
        mSustainEndLevel = 0.0;
        mSustainTStep = 1.0;
        mCurrentSustainT = 0.0;
        
        mReleaseStartLevel = 0.0;
        mReleaseEndLevel = 0.0;
        mReleaseTStep = 1.0;
        mCurrentReleaseT = 0.0;
        
        mEndStartLevel = 0.0;
        mEndEndLevel = 0.0;
        mEndTStep = 1.0;
        mCurrentEndT = 0.0;
    };
    
    // Niko
    void SetAttack(double value);
    void SetDecay(double value);
    void SetSustain(double value);
    void SetRelease(double value);
    
    void SetAttackPeak(double peak);
    void SetAttackShape(double shape);
    
    void SetDecayPeak(double peak);
    void SetDecayShape(double shape);
    
    void SetSustainDelay(double delay);
    void SetSustainShape(double shape);
    
    void SetReleaseShape(double shape);
    
    void SetEnd(double value);
    void SetEndPeak(double peak);
    void SetEndShape(double shape);
    
private:
    EnvelopeStage mCurrentStage;
    double mCurrentLevel;
    double mMultiplier;
    double mSampleRate;
    double mStageValue[kNumEnvelopeStages];
    void CalculateMultiplier(double startLevel, double endLevel, unsigned long long lengthInSamples);
    unsigned long long mCurrentSampleIndex;
    unsigned long long mNextStageSampleIndex;
    
    // Niko
    double ComputeDecayEndLevel();

    
    double mAttackPeak;
    double mAttackShape;
    
    double mDecayPeak;
    double mDecayShape;
    
    double mReleaseShape;
    
    double mSustainDelay;
    double mSustainShape;
    
    double mEndPeak;
    double mEndShape;
    
    // Attack
    double mAttackStartLevel;
    double mAttackEndLevel;
    double mAttackTStep;
    double mCurrentAttackT;
    
    // Decay
    double mDecayStartLevel;
    double mDecayEndLevel;
    double mDecayTStep;
    double mCurrentDecayT;
    
    // Sustain
    double mSustainStartLevel;
    double mSustainEndLevel;
    double mSustainTStep;
    double mCurrentSustainT;
    
    // Release
    double mReleaseStartLevel;
    double mReleaseEndLevel;
    double mReleaseTStep;
    double mCurrentReleaseT;
    
    // End
    double mEndStartLevel;
    double mEndEndLevel;
    double mEndTStep;
    double mCurrentEndT;
};

#endif /* defined(__Synthesis__EnvelopeGenerator3__) */
