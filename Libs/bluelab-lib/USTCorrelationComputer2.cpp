//
//  USTCorrelationComputer2.cpp
//  UST
//
//  Created by applematuer on 1/2/20.
//
//
#include <cmath>

#include "USTCorrelationComputer2.h"

#include <CParamSmooth.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846264
#endif

#define EPS 1e-16

// BUG: when in mono, and calling Process(BL_FLOAT l, BL_FLOAT r),
// the correlation was not 1
#define FIX_DISTANCE_PONDERATION 1

// FIX: atan2 returns [-pi, pi], and we need [-pi/2, pi/2]
#define FIX_ATAN2_RETURN 1
#define RT_SMOOTH 1

// Avoid computing correlation when the signal is silent
// (otherwise, made negative correlations in silent parts)
#define FIX_IGNORE_SILENCE 1


USTCorrelationComputer2::USTCorrelationComputer2(BL_FLOAT sampleRate,
                                                 BL_FLOAT smoothTimeMs,
                                                 Mode mode)
{
    mSampleRate = sampleRate;
    mSmoothTimeMs = smoothTimeMs;
    mMode = mode;
    
    mParamSmooth = new CParamSmooth(smoothTimeMs, sampleRate);
    
    Reset(sampleRate);
    
    mWasJustReset = true;
}

USTCorrelationComputer2::~USTCorrelationComputer2()
{
    delete mParamSmooth;
}

void
USTCorrelationComputer2::Reset(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    
    mCorrelation = 0.0;
    mInstantCorrelation = 0.0;
    
    mParamSmooth->Reset(sampleRate);
    
    mWasJustReset = true;
}

void
USTCorrelationComputer2::Reset()

{
    Reset(mSampleRate);
    
    mWasJustReset = true;
}

#if !FIX_ATAN2_RETURN
//TODO: optimize this !
void
USTCorrelationComputer2::Process(const WDL_TypedBuf<BL_FLOAT> samples[2])
{
    BL_FLOAT result = 0.0;
    BL_FLOAT sumDist = 0.0;
    
    for (int i = 0; i < samples[0].GetSize(); i++)
    {
        BL_FLOAT l = samples[0].Get()[i];
        BL_FLOAT r = samples[1].Get()[i];
        
#if FIX_IGNORE_SILENCE
        // Ignore silence
        if ((std::fabs(l) < EPS) && (std::fabs(r) < EPS))
            continue;
#endif
        
        BL_FLOAT angle = std::atan2(r, l);
        BL_FLOAT dist = std::sqrt(l*l + r*r);
        
        // Adjust
        angle = -angle;
        angle -= M_PI/4.0;
        
        // Flip vertically, to keep only one half-circle
        if (angle < 0.0)
            angle = -angle;
        
        // Flip horizontally to keep only one quarter slice
        if (angle > M_PI/2.0)
            angle = M_PI - angle;
        
        // Normalize into [0, 1]
        BL_FLOAT corr = angle/(M_PI/2.0);
        
        // Set into [-1, 1]
        corr = (corr * 2.0) - 1.0;
        
        // Ponderate with distance;
        corr *= dist;
        
        result += corr;
        sumDist += dist;
    }
    
    if (sumDist > 0.0)
        result /= sumDist;
    
    if (mMode == KEEP_ONLY_NEGATIVE)
    {
        if (result > 0.0)
            result = 0.0;
    }
    
    mInstantCorrelation = result;
    
    if (mWasJustReset)
    {
        mWasJustReset = false;
        
        mParamSmooth->Reset(mSampleRate, result);
    }
    
    mCorrelation = mParamSmooth->Process(result);
}
#endif

#if FIX_ATAN2_RETURN // Not tested
//TODO: optimize this !
void
USTCorrelationComputer2::Process(const WDL_TypedBuf<BL_FLOAT> samples[2],
                                 bool ponderateDist)
{    
    BL_FLOAT result = 0.0;
    BL_FLOAT sumDist = 0.0;
    
    for (int i = 0; i < samples[0].GetSize(); i++)
    {
        BL_FLOAT l = samples[0].Get()[i];
        BL_FLOAT r = samples[1].Get()[i];
        
#if FIX_IGNORE_SILENCE
        // Ignore silence
        if ((std::fabs(l) < EPS) && (std::fabs(r) < EPS))
            continue;
#endif

        BL_FLOAT angle = std::atan2(r, l);
        BL_FLOAT dist = 0.0;
        if (ponderateDist)
	  dist = std::sqrt(l*l + r*r);
        
        // Check and fix [-pi/2, pi/2], because atan 2 can
        // return values outside
        if (angle < -M_PI/2.0)
        {
            angle = -M_PI/2.0 - angle;
            
            if (angle < -M_PI/2.0)
                angle += 2.0*M_PI;
        }
        
        if (angle > M_PI/2.0)
        {
            angle = M_PI/2.0 - angle;
            
            if (angle < -M_PI/2.0)
                angle += 2.0*M_PI;
        }
        
        // Adjust to [-pi/4, 3pi/4]
        angle += M_PI/4.0;
        
        // Flip vertically, to keep only one half-circle [0, pi]
        if (angle < 0.0)
            angle += M_PI;
        
        // Flip horizontally to keep only one quarter slice [0, pi/2]
        if (angle > M_PI/2.0)
            angle = M_PI - angle;
        
        // Normalize into [0, 1]
        BL_FLOAT corr = angle/(M_PI/2.0);
        
        // Set into [-1, 1]
        corr = (corr * 2.0) - 1.0;
        
        if (ponderateDist)
        {
            // Ponderate with distance;
            corr *= dist;
        
            sumDist += dist;
        }
        
#if !RT_SMOOTH
        result += corr;
#else
        if (mMode == KEEP_ONLY_NEGATIVE)
        {
            if (corr > 0.0)
                corr = 0.0;
        }
        
        if (mWasJustReset)
        {
            mWasJustReset = false;
            
            mParamSmooth->Reset(mSampleRate, corr);
        }
        
        mInstantCorrelation = corr;
        
        mCorrelation = mParamSmooth->Process(corr);
#endif
    }
    
#if !RT_SMOOTH
    if (ponderateDist)
    {
        if (sumDist > 0.0)
            result /= sumDist;
    }
    
    if (mMode == KEEP_ONLY_NEGATIVE)
    {
        if (result > 0.0)
            result = 0.0;
    }
    
    mInstantCorrelation = result;
    
    if (mWasJustReset)
    {
        mWasJustReset = false;
        
        mParamSmooth->Reset(mSampleRate, result);
    }
    
    mCorrelation = mParamSmooth->Process(result);
#endif
}
#endif

#if !FIX_ATAN2_RETURN
//TODO: optimize this !
void
qUSTCorrelationComputer2::Process(BL_FLOAT l, BL_FLOAT r)
{
#if FIX_IGNORE_SILENCE
    // Ignore silence
  if ((std::fabs(l) < EPS) && (std::fabs(r) < EPS))
        return;
#endif
    
    BL_FLOAT angle = std::atan2(r, l);
    
#if !FIX_DISTANCE_PONDERATION
    BL_FLOAT dist = std::sqrt(l*l + r*r);
#endif
    
    // Adjust
    angle = -angle;
    angle -= M_PI/4.0;
    
    // Flip vertically, to keep only one half-circle
    if (angle < 0.0)
        angle = -angle;
    
    // Flip horizontally to keep only one quarter slice
    if (angle > M_PI/2.0)
        angle = M_PI - angle;
    
    // Normalize into [0, 1]
    BL_FLOAT corr = angle/(M_PI/2.0);
    
    // Set into [-1, 1]
    corr = (corr * 2.0) - 1.0;
    
#if !FIX_DISTANCE_PONDERATION
    // Ponderate with distance;
    corr *= dist;
#endif
    
    if (mMode == KEEP_ONLY_NEGATIVE)
    {
        if (corr > 0.0)
            corr = 0.0;
    }

    mInstantCorrelation = corr;
    
    if (mWasJustReset)
    {
        mWasJustReset = false;
        
        mParamSmooth->Reset(mSampleRate, corr);
    }
    
    mCorrelation = mParamSmooth->Process(corr);
}
#endif

#if FIX_ATAN2_RETURN
//TODO: optimize this !
void
USTCorrelationComputer2::Process(BL_FLOAT l, BL_FLOAT r)
{
#if FIX_IGNORE_SILENCE
    // Ignore silence
  if ((std::fabs(l) < EPS) && (std::fabs(r) < EPS))
        return;
#endif

    BL_FLOAT angle = std::atan2(r, l);
    
#if !FIX_DISTANCE_PONDERATION
    BL_FLOAT dist = std::sqrt(l*l + r*r);
#endif
    
    // Check and fix [-pi/2, pi/2], because atan 2 can
    // return values outside
    if (angle < -M_PI/2.0)
    {
        angle = -M_PI/2.0 - angle;
        
        if (angle < -M_PI/2.0)
            angle += 2.0*M_PI;
    }
    
    if (angle > M_PI/2.0)
    {
        angle = M_PI/2.0 - angle;
        
        if (angle < -M_PI/2.0)
            angle += 2.0*M_PI;
    }
    
    // Adjust to [-pi/4, 3pi/4]
    angle += M_PI/4.0;
    
    // Flip vertically, to keep only one half-circle [0, pi]
    if (angle < 0.0)
        angle += M_PI;
    
    // Flip horizontally to keep only one quarter slice [0, pi/2]
    if (angle > M_PI/2.0)
        angle = M_PI - angle;
    
    // Normalize into [0, 1]
    BL_FLOAT corr = angle/(M_PI/2.0);
    
    // Set into [-1, 1]
    corr = (corr * 2.0) - 1.0;
    
#if !FIX_DISTANCE_PONDERATION
    // Ponderate with distance;
    corr *= dist;
#endif
    
    if (mMode == KEEP_ONLY_NEGATIVE)
    {
        if (corr > 0.0)
            corr = 0.0;
    }
    
    mInstantCorrelation = corr;
    
    if (mWasJustReset)
    {
        mWasJustReset = false;
        
        mParamSmooth->Reset(mSampleRate, corr);
    }
    
    mCorrelation = mParamSmooth->Process(corr);
}
#endif

BL_FLOAT
USTCorrelationComputer2::GetCorrelation()
{
    return mCorrelation;
}

BL_FLOAT
USTCorrelationComputer2::GetInstantCorrelation()
{
    return mInstantCorrelation;
}

BL_FLOAT
USTCorrelationComputer2::GetSmoothWindowMs()
{
    return mSmoothTimeMs;
}
