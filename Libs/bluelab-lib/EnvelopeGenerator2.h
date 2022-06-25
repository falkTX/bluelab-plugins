/* Copyright (C) 2022 Nicolas Dittlo <deadlab.plugins@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this software; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
 
//
//  EnvelopeGenerator2.h
//  Synthesis
//
//  Created by Martin on 08.04.14.
//
//

#ifndef __Synthesis__EnvelopeGenerator2__
#define __Synthesis__EnvelopeGenerator2__

#include <cmath>

// EnvelopeGenerator2: from EnvelopeGenerator
// - added more shape parameters
class EnvelopeGenerator2 {
public:
    enum EnvelopeStage {
        ENVELOPE_STAGE_OFF = 0,
        ENVELOPE_STAGE_ATTACK,
        ENVELOPE_STAGE_DECAY,
        ENVELOPE_STAGE_SUSTAIN,
        ENVELOPE_STAGE_RELEASE,
        kNumEnvelopeStages
    };
    void EnterStage(EnvelopeStage newStage);
    double NextSample();
    void SetSampleRate(double newSampleRate);
    inline EnvelopeStage GetCurrentStage() const { return mCurrentStage; };
    const double mMinimumLevel;
    
    EnvelopeGenerator2() :
    mMinimumLevel(0.0001),
    mCurrentStage(ENVELOPE_STAGE_OFF),
    mCurrentLevel(mMinimumLevel),
    mMultiplier(1.0),
    mSampleRate(44100.0),
    mCurrentSampleIndex(0),
    mNextStageSampleIndex(0),
    mAttackGain(1.0),
    mAttackShape(1.0),
    mDecayShape(1.0),
    mSustainSlope(0.0),
    mSustainShape(1.0),
    mReleaseShape(1.0)
    {
        mStageValue[ENVELOPE_STAGE_OFF] = 0.0;
        mStageValue[ENVELOPE_STAGE_ATTACK] = 0.01;
        mStageValue[ENVELOPE_STAGE_DECAY] = 0.5;
        mStageValue[ENVELOPE_STAGE_SUSTAIN] = 0.1;
        mStageValue[ENVELOPE_STAGE_RELEASE] = 1.0;
        
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
    };
    
    // Niko
    void SetAttack(double value);
    void SetDecay(double value);
    void SetSustain(double value);
    void SetRelease(double value);
    
    // Niko
    void SetAttackPeak(double attackGain);
    void SetAttackShape(double shape);
    
    // Niko
    void SetReleaseShape(double shape);
    void SetDecayShape(double shape);
    
    void SetSustainSlope(double slope);
    void SetSustainShape(double shape);
    
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
    double mAttackGain;
    double mAttackShape;
    
    // Niko
    double mReleaseShape;
    double mDecayShape;
    double mSustainSlope;
    double mSustainShape;
    
    // Niko
    
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
};

#endif /* defined(__Synthesis__EnvelopeGenerator2__) */
