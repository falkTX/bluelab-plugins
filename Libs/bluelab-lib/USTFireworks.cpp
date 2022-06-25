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
//  USTFireworks.cpp
//  UST
//
//  Created by applematuer on 8/21/19.
//
//

#include <USTProcess.h>
#include <Window.h>
#include <BLUtils.h>

#include "USTFireworks.h"


#define POLAR_LEVELS_NUM_BINS 128.0

#define POLAR_LEVEL_MAX_SMOOTH_FACTOR 0.995


USTFireworks::USTFireworks(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
}

USTFireworks::~USTFireworks() {}

void
USTFireworks::Reset()
{
    mPrevPolarLevelsMax.Resize(0);
}

void
USTFireworks::Reset(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    
    mPrevPolarLevelsMax.Resize(0);
}

void
USTFireworks::ComputePoints(WDL_TypedBuf<BL_GUI_FLOAT> samplesIn[2],
                            WDL_TypedBuf<BL_GUI_FLOAT> points[2],
                            WDL_TypedBuf<BL_GUI_FLOAT> maxPoints[2])
{
    //WDL_TypedBuf<BL_FLOAT> polarSamples[2];
    //USTProcess::ComputePolarSamples(samplesIn, points);
    
    WDL_TypedBuf<BL_GUI_FLOAT> levels;
    USTProcess::ComputePolarLevels(samplesIn, POLAR_LEVELS_NUM_BINS,
                                   &levels,
                                   //USTProcess::MAX);
                                   USTProcess::AVG);
    
    BLUtils::ClipMax(&levels, (BL_GUI_FLOAT)1.0);
    
    // Normal
    WDL_TypedBuf<BL_GUI_FLOAT> levels0 = levels;
    
    // Max
    WDL_TypedBuf<BL_GUI_FLOAT> levels1 = levels;
    
    USTProcess::SmoothPolarLevels(&levels1, &mPrevPolarLevelsMax,
                                  true, (BL_GUI_FLOAT)POLAR_LEVEL_MAX_SMOOTH_FACTOR);
    
    // Normal
    USTProcess::ComputePolarLevelPoints(levels0, points);
    
    // Max
    USTProcess::ComputePolarLevelPoints(levels1, maxPoints);
}
