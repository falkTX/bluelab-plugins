//
//  EnvelopeGenerator.cpp
//  Synthesis
//
//  Created by Martin on 08.04.14.
//
//

#include <BLDebug.h>

#include "EnvelopeGenerator.h"

//See for idea: https://sergemodularinfo.blogspot.com/p/extended-envelope-generator-adsr.html
#define USE_ATTACK_SHAPE 1


double
EnvelopeGenerator::NextSample()
{
    if (mCurrentStage != ENVELOPE_STAGE_OFF &&
        mCurrentStage != ENVELOPE_STAGE_SUSTAIN)
    {
        if (mCurrentSampleIndex == mNextStageSampleIndex)
        {
            EnvelopeStage newStage =
                    static_cast<EnvelopeStage>((mCurrentStage + 1) % kNumEnvelopeStages);
            EnterStage(newStage);
        }
        
#if !USE_ATTACK_SHAPE
        mCurrentLevel *= mMultiplier;
#else
        // Niko
        if (mCurrentStage != ENVELOPE_STAGE_ATTACK)
            mCurrentLevel *= mMultiplier;
        else
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
#endif
        
        mCurrentSampleIndex++;
    }
    return mCurrentLevel;
}

void
EnvelopeGenerator::CalculateMultiplier(double startLevel,
                                       double endLevel,
                                       unsigned long long lengthInSamples)
{
    mMultiplier = 1.0 + (log(endLevel) - log(startLevel)) / (lengthInSamples);
}

void
EnvelopeGenerator::EnterStage(EnvelopeStage newStage)
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
            
            mAttackStartLevel = mCurrentLevel;
            mAttackEndLevel = mAttackGain;
            mAttackTStep = 1.0;
            if (mNextStageSampleIndex > 1)
                mAttackTStep = 1.0/(mNextStageSampleIndex - 1);
            mCurrentAttackT = 0.0;
        }
        break;
            
        case ENVELOPE_STAGE_DECAY:
            //mCurrentLevel = 1.0;
            mCurrentLevel = mAttackGain;
            CalculateMultiplier(mCurrentLevel,
                                fmax(mStageValue[ENVELOPE_STAGE_SUSTAIN], mMinimumLevel),
                                mNextStageSampleIndex);
            break;
            
        case ENVELOPE_STAGE_SUSTAIN:
            mCurrentLevel = mStageValue[ENVELOPE_STAGE_SUSTAIN];
            mMultiplier = 1.0;
            break;
            
        case ENVELOPE_STAGE_RELEASE:
            // We could go from ATTACK/DECAY to RELEASE,
            // so we're not changing currentLevel here.
            CalculateMultiplier(mCurrentLevel,
                                mMinimumLevel,
                                mNextStageSampleIndex);
            break;
            
        default:
            break;
    }
}

void
EnvelopeGenerator::SetSampleRate(double newSampleRate)
{
    mSampleRate = newSampleRate;
}

void
EnvelopeGenerator::SetAttack(double value)
{
    mStageValue[ENVELOPE_STAGE_ATTACK] = value;
}

void
EnvelopeGenerator::SetDecay(double value)
{
    mStageValue[ENVELOPE_STAGE_DECAY] = value;
}

void
EnvelopeGenerator::SetSustain(double value)
{
    mStageValue[ENVELOPE_STAGE_SUSTAIN] = value;
}

void
EnvelopeGenerator::SetRelease(double value)
{
    mStageValue[ENVELOPE_STAGE_RELEASE] = value;
}

void
EnvelopeGenerator::SetAttackGain(double attackGain)
{
    double headRoom = 1.0 - mStageValue[ENVELOPE_STAGE_SUSTAIN];
    if (headRoom < 0.0)
        headRoom = 1.0;
    
    mAttackGain = mStageValue[ENVELOPE_STAGE_SUSTAIN] + attackGain*headRoom;
}

void
EnvelopeGenerator::SetAttackShape(double shape)
{
    mAttackShape = shape;
}
