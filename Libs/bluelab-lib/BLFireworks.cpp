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
//  BLFireworks.cpp
//  UST
//
//  Created by applematuer on 8/21/19.
//
//

#include <Window.h>

#include <BLVectorscopeProcess.h>

#include <BLUtils.h>

#include "BLFireworks.h"


#define POLAR_LEVELS_NUM_BINS 128.0
#define POLAR_LEVEL_MAX_SMOOTH_FACTOR 0.995


BLFireworks::BLFireworks(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
}

BLFireworks::~BLFireworks() {}

void
BLFireworks::Reset()
{
    mPrevPolarLevelsMax.Resize(0);
}

void
BLFireworks::Reset(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    
    mPrevPolarLevelsMax.Resize(0);
}

void
BLFireworks::ComputePoints(WDL_TypedBuf<BL_FLOAT> samplesIn[2],
                             WDL_TypedBuf<BL_FLOAT> points[2],
                             WDL_TypedBuf<BL_FLOAT> maxPoints[2])
{
    WDL_TypedBuf<BL_FLOAT> levels;
    BLVectorscopeProcess::ComputePolarLevels(samplesIn, POLAR_LEVELS_NUM_BINS,
                                   &levels, BLVectorscopeProcess::AVG);
    
    BLUtils::ClipMax(&levels, (BL_FLOAT)1.0);
    
    // Normal
    WDL_TypedBuf<BL_FLOAT> levels0 = levels;
    
    // Max
    WDL_TypedBuf<BL_FLOAT> levels1 = levels;
    
    BLVectorscopeProcess::SmoothPolarLevels(&levels1, &mPrevPolarLevelsMax,
                                  true, POLAR_LEVEL_MAX_SMOOTH_FACTOR);
    
    // Normal
    BLVectorscopeProcess::ComputePolarLevelPoints(levels0, points);
    
    // Max
    BLVectorscopeProcess::ComputePolarLevelPoints(levels1, maxPoints);
}
