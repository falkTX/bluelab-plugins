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
//  BLCorrelationComputer.cpp
//  UST
//
//  Created by applematuer on 1/2/20.
//
//


#include "BLCorrelationComputer.h"

#include <CParamSmooth.h>

#ifndef M_PI
#define M_PI 3.1415926535897932384
#endif

// BUG: when in mono, and calling Process(double l, double r),
// the correlation was not 1
#define FIX_DISTANCE_PONDERATION 1


BLCorrelationComputer::BLCorrelationComputer(BL_FLOAT sampleRate,
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

BLCorrelationComputer::~BLCorrelationComputer()
{
    delete mParamSmooth;
}

void
BLCorrelationComputer::Reset(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    
    mCorrelation = 0.0;
    mInstantCorrelation = 0.0;
    
    mParamSmooth->Reset(sampleRate);
    
    mWasJustReset = true;
}

void
BLCorrelationComputer::Reset()

{
    Reset(mSampleRate);
    
    mWasJustReset = true;
}

// FIX_ATAN2_RETURN
//TODO: optimize this !
void
BLCorrelationComputer::Process(const WDL_TypedBuf<BL_FLOAT> samples[2],
                                 bool ponderateDist)
{
#define RT_SMOOTH 1

#if !RT_SMOOTH
    BL_FLOAT result = 0.0;
    BL_FLOAT sumDist = 0.0;
#endif
    
    for (int i = 0; i < samples[0].GetSize(); i++)
    {
        BL_FLOAT l = samples[0].Get()[i];
        BL_FLOAT r = samples[1].Get()[i];
        
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

#if !RT_SMOOTH
            sumDist += dist;
#endif
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

// FIX_ATAN2_RETURN
//TODO: optimize this !
void
BLCorrelationComputer::Process(BL_FLOAT l, BL_FLOAT r)
{
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

BL_FLOAT
BLCorrelationComputer::GetCorrelation()
{
    return mCorrelation;
}

BL_FLOAT
BLCorrelationComputer::GetInstantCorrelation()
{
    return mInstantCorrelation;
}

BL_FLOAT
BLCorrelationComputer::GetSmoothWindowMs()
{
    return mSmoothTimeMs;
}
