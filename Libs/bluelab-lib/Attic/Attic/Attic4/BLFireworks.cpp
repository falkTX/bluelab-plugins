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
