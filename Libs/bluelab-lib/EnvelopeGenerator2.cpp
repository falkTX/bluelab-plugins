//
//  EnvelopeGenerator2.cpp
//  Synthesis
//
//  Created by Martin on 08.04.14.
//
//

#include <BLDebug.h>

#include "EnvelopeGenerator2.h"

//See for idea: https://sergemodularinfo.blogspot.com/p/extended-envelope-generator-adsr.html
#define USE_ATTACK_SHAPE 1

#define USE_ADSR_SHAPE 1

double
EnvelopeGenerator2::NextSample()
{
#if !USE_ADSR_SHAPE
    if (mCurrentStage != ENVELOPE_STAGE_OFF &&
        mCurrentStage != ENVELOPE_STAGE_SUSTAIN)
#else
    if (mCurrentStage != ENVELOPE_STAGE_OFF)
#endif
    {
        if (mCurrentSampleIndex == mNextStageSampleIndex)
        {
            EnvelopeStage newStage =
                    static_cast<EnvelopeStage>((mCurrentStage + 1) % kNumEnvelopeStages);
            EnterStage(newStage);
        }
        
#if !USE_ATTACK_SHAPE && !USE_ADSR_SHAPE
        mCurrentLevel *= mMultiplier;
#else
        // Niko
        if (mCurrentStage == ENVELOPE_STAGE_ATTACK)
        {
            // Apply param shape
#define EPS 1e-10
            double t = 0.0;
            if ((mAttackShape < -EPS) ||
                (mAttackShape > EPS))
                t = pow(mCurrentAttackT, 1.0/mAttackShape);
            
            mCurrentLevel = mAttackStartLevel + t*(mAttackEndLevel - mAttackStartLevel);
           
            // OPTIM: Incremental
            mCurrentAttackT += mAttackTStep;
        }
        
#if USE_ADSR_SHAPE
        if (mCurrentStage == ENVELOPE_STAGE_DECAY)
        {
            // Apply param shape
#define EPS 1e-10
            double t = 0.0;
            if ((mDecayShape < -EPS) ||
                (mDecayShape > EPS))
                //t = pow(mCurrentDecayT, 1.0/mDecayShape);
                t = pow(mCurrentDecayT, mDecayShape);
            
            mCurrentLevel = mDecayStartLevel + t*(mDecayEndLevel - mDecayStartLevel);
            
            // OPTIM: Incremental
            mCurrentDecayT += mDecayTStep;
        }
#endif

#if USE_ADSR_SHAPE
        if (mCurrentStage == ENVELOPE_STAGE_RELEASE)
        {
            // Apply param shape
#define EPS 1e-10
            double t = 0.0;
            if ((mReleaseShape < -EPS) ||
                (mReleaseShape > EPS))
                t = pow(mCurrentReleaseT, mReleaseShape);
            
            mCurrentLevel = mReleaseStartLevel + t*(mReleaseEndLevel - mReleaseStartLevel);
            
            // OPTIM: Incremental
            mCurrentReleaseT += mReleaseTStep;
        }
#endif

#if USE_ADSR_SHAPE
        if (mCurrentStage == ENVELOPE_STAGE_SUSTAIN)
        {
            // Apply param shape
#define EPS 1e-10
            double t = 0.0;
            if ((mSustainShape < -EPS) ||
                (mSustainShape > EPS))
            {
                if (mSustainSlope > 0.0)
                    t = pow(mCurrentSustainT, 1.0/mSustainShape);
                else
                    t = pow(mCurrentSustainT, mSustainShape);
            }
            
            mCurrentLevel = mSustainStartLevel + t*(mSustainEndLevel - mSustainStartLevel);
            
            if (mCurrentLevel < mMinimumLevel)
                mCurrentLevel = mMinimumLevel;
            if (mCurrentLevel > 1.0)
                mCurrentLevel = 1.0;
            
            // OPTIM: Incremental
            mCurrentSustainT += mSustainTStep;
        }
#endif
        
#endif
        
        mCurrentSampleIndex++;
    }
    return mCurrentLevel;
}

void
EnvelopeGenerator2::CalculateMultiplier(double startLevel,
                                       double endLevel,
                                       unsigned long long lengthInSamples)
{
    mMultiplier = 1.0 + (log(endLevel) - log(startLevel)) / (lengthInSamples);
}

void
EnvelopeGenerator2::EnterStage(EnvelopeStage newStage)
{
    mCurrentStage = newStage;
    mCurrentSampleIndex = 0;
    if (mCurrentStage == ENVELOPE_STAGE_OFF ||
        mCurrentStage == ENVELOPE_STAGE_SUSTAIN)
    {
        mNextStageSampleIndex = 0;
    }
    else
    {
        mNextStageSampleIndex = mStageValue[mCurrentStage] * mSampleRate;
    }
    
    switch (newStage)
    {
        case ENVELOPE_STAGE_OFF:
            mCurrentLevel = 0.0;
            mMultiplier = 1.0;
            break;
            
        case ENVELOPE_STAGE_ATTACK:
        {
            mCurrentLevel = mMinimumLevel;
            CalculateMultiplier(mCurrentLevel,
                                //1.0,
                                mAttackGain,
                                mNextStageSampleIndex);
            
#if USE_ATTACK_SHAPE
            mAttackStartLevel = mCurrentLevel;
            mAttackEndLevel = mAttackGain;
            mAttackTStep = 1.0;
            if (mNextStageSampleIndex > 1)
                mAttackTStep = 1.0/(mNextStageSampleIndex - 1);
            mCurrentAttackT = 0.0;
#endif
        }
        break;
            
        case ENVELOPE_STAGE_DECAY:
        {
            //mCurrentLevel = 1.0;
            mCurrentLevel = mAttackGain;
            CalculateMultiplier(mCurrentLevel,
                                fmax(mStageValue[ENVELOPE_STAGE_SUSTAIN], mMinimumLevel),
                                mNextStageSampleIndex);
            
#if USE_ADSR_SHAPE
            mDecayStartLevel = mCurrentLevel;
            mDecayEndLevel = mStageValue[ENVELOPE_STAGE_SUSTAIN];
            mDecayTStep = 1.0;
            if (mNextStageSampleIndex > 1)
                mDecayTStep = 1.0/(mNextStageSampleIndex - 1);
            mCurrentDecayT = 0.0;
#endif
        }
        break;
            
#if !USE_ADSR_SHAPE
        case ENVELOPE_STAGE_SUSTAIN:
            mCurrentLevel = mStageValue[ENVELOPE_STAGE_SUSTAIN];
            mMultiplier = 1.0;
            break;
#else
        case ENVELOPE_STAGE_SUSTAIN:
        {
            mCurrentLevel = mStageValue[ENVELOPE_STAGE_SUSTAIN];
            
            mSustainStartLevel = mCurrentLevel;
            
            double interval = 0.0;
            if (mSustainSlope < 0.0)
            {
                interval = mCurrentLevel - mMinimumLevel;
            }
            else
            {
                if (mSustainSlope > 0.0)
                {
                    // Max level is 1.0
                    interval = 1.0 - mCurrentLevel;
                }
            }
            
            // 1 second
            double nextStageSampleIndex = 1.0 * mSampleRate;
            
            mSustainEndLevel = mCurrentLevel + mSustainSlope*interval;
            mSustainTStep = 1.0;
            //if (mNextStageSampleIndex > 1)
            //    mSustainTStep = 1.0/(mNextStageSampleIndex - 1);
            if (nextStageSampleIndex > 1)
                mSustainTStep = 1.0/(nextStageSampleIndex - 1);
            mCurrentSustainT = 0.0;
        }
        break;
#endif
            
        case ENVELOPE_STAGE_RELEASE:
        {
            // We could go from ATTACK/DECAY to RELEASE,
            // so we're not changing currentLevel here.
            CalculateMultiplier(mCurrentLevel,
                                mMinimumLevel,
                                mNextStageSampleIndex);
            
#if USE_ADSR_SHAPE
            mReleaseStartLevel = mCurrentLevel;
            mReleaseEndLevel = mMinimumLevel;
            mReleaseTStep = 1.0;
            if (mNextStageSampleIndex > 1)
                mReleaseTStep = 1.0/(mNextStageSampleIndex - 1);
            mCurrentDecayT = 0.0;
#endif
        }
        break;
            
        default:
            break;
    }
}

void
EnvelopeGenerator2::SetSampleRate(double newSampleRate)
{
    mSampleRate = newSampleRate;
}

void
EnvelopeGenerator2::SetAttack(double value)
{
    mStageValue[ENVELOPE_STAGE_ATTACK] = value;
}

void
EnvelopeGenerator2::SetDecay(double value)
{
    mStageValue[ENVELOPE_STAGE_DECAY] = value;
}

void
EnvelopeGenerator2::SetSustain(double value)
{
    mStageValue[ENVELOPE_STAGE_SUSTAIN] = value;
}

void
EnvelopeGenerator2::SetRelease(double value)
{
    mStageValue[ENVELOPE_STAGE_RELEASE] = value;
}

void
EnvelopeGenerator2::SetAttackPeak(double attackPeak)
{
    double headRoom = 1.0 - mStageValue[ENVELOPE_STAGE_SUSTAIN];
    if (headRoom < 0.0)
        headRoom = 1.0;
    
    mAttackGain = mStageValue[ENVELOPE_STAGE_SUSTAIN] + attackPeak*headRoom;
}

void
EnvelopeGenerator2::SetAttackShape(double shape)
{
    mAttackShape = shape;
}

void
EnvelopeGenerator2::SetReleaseShape(double shape)
{
    mReleaseShape = shape;
}

void
EnvelopeGenerator2::SetDecayShape(double shape)
{
    mDecayShape = shape;
}

void
EnvelopeGenerator2::SetSustainSlope(double slope)
{
    mSustainSlope = slope;
}

void
EnvelopeGenerator2::SetSustainShape(double shape)
{
    mSustainShape = shape;
}
