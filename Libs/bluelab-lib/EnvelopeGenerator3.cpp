//
//  EnvelopeGenerator3.cpp
//  Synthesis
//
//  Created by Martin on 08.04.14.
//
//

#include <BLDebug.h>

#include "EnvelopeGenerator3.h"

//See for idea: https://sergemodularinfo.blogspot.com/p/extended-envelope-generator-adsr.html
#define USE_ADSR_SHAPE 1

double
EnvelopeGenerator3::NextSample()
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
        
#if !USE_ADSR_SHAPE
        mCurrentLevel *= mMultiplier;
#else
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

        if (mCurrentStage == ENVELOPE_STAGE_SUSTAIN)
        {
            // Apply param shape
#define EPS 1e-10
            double t = 0.0;
            if ((mSustainShape < -EPS) ||
                (mSustainShape > EPS))
                t = pow(mCurrentSustainT, mSustainShape);
            
            mCurrentLevel = mSustainStartLevel + t*(mSustainEndLevel - mSustainStartLevel);
            
            // OPTIM: Incremental
            mCurrentSustainT += mSustainTStep;
            
            if (mCurrentSustainT > 1.0)
                mCurrentSustainT = 1.0;
        }
        
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

        if (mCurrentStage == ENVELOPE_STAGE_END)
        {
            // Apply param shape
#define EPS 1e-10
            double t = 0.0;
            if ((mEndShape < -EPS) ||
                (mEndShape > EPS))
                //t = pow(mCurrentDecayT, 1.0/mDecayShape);
                t = pow(mCurrentEndT, mEndShape);
            
            mCurrentLevel = mEndStartLevel + t*(mEndEndLevel - mEndStartLevel);
            
            // OPTIM: Incremental
            mCurrentEndT += mEndTStep;
        }
#endif
        
        mCurrentSampleIndex++;
    }
    return mCurrentLevel;
}

void
EnvelopeGenerator3::CalculateMultiplier(double startLevel,
                                       double endLevel,
                                       unsigned long long lengthInSamples)
{
    mMultiplier = 1.0 + (log(endLevel) - log(startLevel)) / (lengthInSamples);
}

void
EnvelopeGenerator3::EnterStage(EnvelopeStage newStage)
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
                                mAttackPeak,
                                mNextStageSampleIndex);
            
#if USE_ADSR_SHAPE
            mAttackStartLevel = mCurrentLevel;
            mAttackEndLevel = mAttackPeak;
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
            mCurrentLevel = mAttackPeak;
            CalculateMultiplier(mCurrentLevel,
                                fmax(mStageValue[ENVELOPE_STAGE_SUSTAIN], mMinimumLevel),
                                mNextStageSampleIndex);
            
#if USE_ADSR_SHAPE
            mDecayStartLevel = mCurrentLevel;
            
            mDecayEndLevel = mStageValue[ENVELOPE_STAGE_SUSTAIN];
            
            mDecayEndLevel = ComputeDecayEndLevel();
            
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
            mCurrentLevel = mDecayEndLevel;
            mSustainStartLevel = mDecayEndLevel;
        
            mSustainEndLevel = mStageValue[ENVELOPE_STAGE_SUSTAIN];
        
            // Delay
            double nextStageSampleIndex = mSustainDelay * mSampleRate;
            
            mSustainTStep = 1.0;
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
            mReleaseEndLevel = mEndPeak;
            mReleaseTStep = 1.0;
            if (mNextStageSampleIndex > 1)
                mReleaseTStep = 1.0/(mNextStageSampleIndex - 1);
            mCurrentReleaseT = 0.0;
#endif
        }
        break;
            
        case ENVELOPE_STAGE_END:
        {
            // We could go from ATTACK/DECAY to RELEASE,
            // so we're not changing currentLevel here.
            CalculateMultiplier(mCurrentLevel,
                                mMinimumLevel,
                                mNextStageSampleIndex);
            
#if USE_ADSR_SHAPE
            mEndStartLevel = mEndPeak;
            mEndEndLevel = mMinimumLevel;
            mEndTStep = 1.0;
            
            //double nextStageSampleIndex = mStageValue[ENVELOPE_STAGE_END]*mSampleRate;
            //if (nextStageSampleIndex > 1)
            //    mEndTStep = 1.0/(nextStageSampleIndex - 1);
            if (mNextStageSampleIndex > 1)
                mEndTStep = 1.0/(mNextStageSampleIndex - 1);
            mCurrentEndT = 0.0;
#endif
        }
        break;
            
        default:
            break;
    }
}

void
EnvelopeGenerator3::SetSampleRate(double newSampleRate)
{
    mSampleRate = newSampleRate;
}

void
EnvelopeGenerator3::SetAttack(double value)
{
    mStageValue[ENVELOPE_STAGE_ATTACK] = value;
}

void
EnvelopeGenerator3::SetDecay(double value)
{
    mStageValue[ENVELOPE_STAGE_DECAY] = value;
}

void
EnvelopeGenerator3::SetSustain(double value)
{
    mStageValue[ENVELOPE_STAGE_SUSTAIN] = value;
}

void
EnvelopeGenerator3::SetRelease(double value)
{
    mStageValue[ENVELOPE_STAGE_RELEASE] = value;
}

void
EnvelopeGenerator3::SetEnd(double value)
{
    mStageValue[ENVELOPE_STAGE_END] = value;
}

void
EnvelopeGenerator3::SetAttackPeak(double attackPeak)
{
    //double headRoom = 1.0 - mStageValue[ENVELOPE_STAGE_SUSTAIN];
    
    double decayEndLevel = ComputeDecayEndLevel();
    double headRoom = 1.0 - decayEndLevel;

    // BUG: Richard Todd: "when recalling a preset(reloading a project) => clicks"
    // "and need to touch the sustain value, and re-put it to 0 to fix"
    //
    // This is because kSustain was saved to 0dB (i.e 1.0 i amp),
    // and when reloading project, it was near 1e-6 dB instead of 0dB
    // (due to precision, and parameter normalization)
    // => So here headRoom was < 0, and the bug appeared!
#if 0 // BUG
    if (headRoom < 0.0)
        headRoom = 1.0;
#endif

#if 1 // FIX
    if (headRoom < 0.0)
        headRoom = 0.0;
#endif
    
    //mAttackPeak = mStageValue[ENVELOPE_STAGE_SUSTAIN] + attackPeak*headRoom;
    
    mAttackPeak = decayEndLevel + attackPeak*headRoom;
}

void
EnvelopeGenerator3::SetAttackShape(double shape)
{
    mAttackShape = shape;
}

void
EnvelopeGenerator3::SetDecayPeak(double decayPeak)
{
    mDecayPeak = decayPeak;
}

void
EnvelopeGenerator3::SetDecayShape(double shape)
{
    mDecayShape = shape;
}

void
EnvelopeGenerator3::SetReleaseShape(double shape)
{
    mReleaseShape = shape;
}

void
EnvelopeGenerator3::SetSustainDelay(double delay)
{
    mSustainDelay = delay;
}

void
EnvelopeGenerator3::SetSustainShape(double shape)
{
    mSustainShape = shape;
}

void
EnvelopeGenerator3::SetEndPeak(double peak)
{
    mEndPeak = peak;
}

void
EnvelopeGenerator3::SetEndShape(double shape)
{
    mEndShape = shape;
}

double
EnvelopeGenerator3::ComputeDecayEndLevel()
{
    double endLevel = mStageValue[ENVELOPE_STAGE_SUSTAIN];
    if (mDecayPeak < 0.0)
    {
        endLevel = (1.0 + mDecayPeak)*mStageValue[ENVELOPE_STAGE_SUSTAIN];
    }
    else
    {
        if (mDecayPeak > 0.0)
        {
            endLevel = (1.0 - mDecayPeak)*mStageValue[ENVELOPE_STAGE_SUSTAIN] + mDecayPeak;
        }
    }
    
    return endLevel;
}
