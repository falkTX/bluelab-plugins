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
//  USTPolarLevel.cpp
//  UST
//
//  Created by applematuer on 8/21/19.
//
//

#include <USTProcess.h>
#include <Window.h>

#include <BLUtils.h>
#include <BLUtilsMath.h>

#include <FftProcessObj16.h>
#include <SourcePos.h>

#include "USTPolarLevel.h"


// 128
// Smooth enables
// disable processpolar levels

#define POLAR_LEVELS_NUM_BINS 128.0 //64.0 //256.0 //64.0

#define SMOOTH_WIN_SIZE 10 //5 //15 //5

#define POLAR_LEVEL_SMOOTH_FACTOR 0.8 //0.9 //0.99
#define POLAR_LEVEL_MAX_SMOOTH_FACTOR 0.99


USTPolarLevel::USTPolarLevel(BL_FLOAT sampleRate)
{
    FftProcessObj16::Init();
    
    mSampleRate = sampleRate;
}

USTPolarLevel::~USTPolarLevel() {}

void
USTPolarLevel::Reset()
{
    mPrevPolarsLevels.Resize(0);
    mPrevPolarLevelsMax.Resize(0);
}

void
USTPolarLevel::Reset(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    
    mPrevPolarsLevels.Resize(0);
    mPrevPolarLevelsMax.Resize(0);
}

#if 1
void
USTPolarLevel::ComputePoints(WDL_TypedBuf<BL_FLOAT> samplesIn[2],
                             WDL_TypedBuf<BL_FLOAT> points[2],
                             WDL_TypedBuf<BL_FLOAT> maxPoints[2])
{
    WDL_TypedBuf<BL_FLOAT> levels;
    USTProcess::ComputePolarLevels(samplesIn, POLAR_LEVELS_NUM_BINS,
                                   &levels,
                                   //USTProcess::MAX);
                                   USTProcess::AVG);
    
#if 0
    BLDebug::DumpData("levels.txt", levels);
    
    BLUtils::ComputeDerivative(&levels);
    BLUtils::ComputeAbs(&levels);
    
    BLDebug::DumpData("deriv.txt", levels);
    
    BLUtils::ComputeDerivative(&levels);
    BLUtils::ComputeAbs(&levels);
    
    BLDebug::DumpData("deriv2.txt", levels);
#endif
    
    // TEST
    //BLUtils::AmpToDBNorm(&levels, 1e-15, -120.0);
    //BLUtils::MultValues(&levels, 0.3);
    
    //BLDebug::DumpData("levels1.txt", levels);
    
    // TEST:
    // - good for shape
    // - too low for white noise
    //USTProcess::ProcessPolarLevels(&levels);
    
    //BLUtils::AmpToDBNorm(&levels, 1e-15, -120.0);
    //BLUtils::MultValues(&levels, 0.3);
    
    ProcessLevels(&levels);
    
    // Normal
    WDL_TypedBuf<BL_FLOAT> levels0 = levels;
    
    // Max
    WDL_TypedBuf<BL_FLOAT> levels1 = levels;
    
#if 1
#if 0 // ORIGIN
    // Normal
    USTProcess::ProcessPolarLevels(&levels0, &mPrevPolarsLevels,
                                   false, POLAR_LEVEL_SMOOTH_FACTOR);
#endif
    //BLDebug::DumpData("levels0.txt", levels0);
    //BLDebug::DumpData("prev.txt", mPrevPolarsLevels);
    
    // TEST
    //USTProcess::ProcessPolarLevels(&levels0);
    USTProcess::SmoothPolarLevels(&levels0, &mPrevPolarsLevels,
                                  true, (BL_FLOAT)0.95/*POLAR_LEVEL_SMOOTH_FACTOR*/);
    //BLDebug::DumpData("smooth.txt", levels0);
    
    // Max
    //USTProcess::ProcessPolarLevels(&levels1);
    USTProcess::SmoothPolarLevels(&levels1, &mPrevPolarLevelsMax,
                                   true, (BL_FLOAT)0.995/*POLAR_LEVEL_MAX_SMOOTH_FACTOR*/);
#endif
    
    // Normal
    USTProcess::ComputePolarLevelPoints(levels0, points);
    
    // Max
    USTProcess::ComputePolarLevelPoints(levels1, maxPoints);
}
#endif

#if 0
// TEST (not working)
void
USTPolarLevel::ComputePoints(WDL_TypedBuf<BL_FLOAT> samplesIn[2],
                             WDL_TypedBuf<BL_FLOAT> points[2],
                             WDL_TypedBuf<BL_FLOAT> maxPoints[2])
{
    WDL_TypedBuf<BL_FLOAT> magns[2];
    WDL_TypedBuf<BL_FLOAT> phases[2];
    FftProcessObj16::SamplesToHalfMagnPhases(samplesIn[0], &magns[0], &phases[0]);
    FftProcessObj16::SamplesToHalfMagnPhases(samplesIn[1], &magns[1], &phases[1]);
    
    WDL_TypedBuf<BL_FLOAT> freqs;
    BLUtils::FftFreqs(&freqs, phases[0].GetSize(), mSampleRate);
    
    WDL_TypedBuf<BL_FLOAT> timeDelays;
    BLUtils::ComputeTimeDelays(&timeDelays, phases[0], phases[1], mSampleRate);
    
    WDL_TypedBuf<BL_FLOAT> sourceRs;
    WDL_TypedBuf<BL_FLOAT> sourceThetas;
    
    SourcePos::MagnPhasesToSourcePos(&sourceRs, &sourceThetas,
                                     magns[0],  magns[1],
                                     phases[0], phases[1],
                                     freqs,
                                     timeDelays);
    
    // GOOD with float time delays
    
    // Fill missing values (but only for display) !
    BLUtils::FillMissingValues(&sourceRs, true, UTILS_VALUE_UNDEFINED);
    BLUtils::FillMissingValues(&sourceThetas, true, UTILS_VALUE_UNDEFINED);
    
    // Just in case
    BLUtils::ReplaceValue(&sourceThetas, UTILS_VALUE_UNDEFINED, 0.0);
    
    WDL_TypedBuf<BL_FLOAT> thetasRot = sourceThetas;
    BLUtils::AddValues(&thetasRot, -M_PI/2.0);
    
    WDL_TypedBuf<BL_FLOAT> levels;
    ComputePolarLevelsSourcePos(sourceRs, thetasRot,
                                POLAR_LEVELS_NUM_BINS, &levels);
    
    //BLDebug::DumpData("levels.txt", levels);
    
    // Hard scale factor
    BLUtils::MultValues(&levels, 1.0/50000.0);
    
    // Normal
    WDL_TypedBuf<BL_FLOAT> levels0 = levels;
    
    // Max
    WDL_TypedBuf<BL_FLOAT> levels1 = levels;
    
#if 1
    // Normal
    USTProcess::ProcessPolarLevels(&levels0, &mPrevPolarsLevels,
                                   false, POLAR_LEVEL_SMOOTH_FACTOR);
    
    // Max
    USTProcess::ProcessPolarLevels(&levels1, &mPrevPolarLevelsMax,
                                   true, POLAR_LEVEL_MAX_SMOOTH_FACTOR);
#endif
    
    // Normal
    USTProcess::ComputePolarLevelPoints(levels0, points);
    
    // Max
    USTProcess::ComputePolarLevelPoints(levels1, maxPoints);
}
#endif

// TEST (not working)
void
USTPolarLevel::ComputePolarLevelsSourcePos(const WDL_TypedBuf<BL_FLOAT> &rs,
                                           const WDL_TypedBuf<BL_FLOAT> &thetas,
                                           int numBins,
                                           WDL_TypedBuf<BL_FLOAT> *levels)
{
    // Accumulate levels depending on the angles
    levels->Resize(numBins);
    BLUtils::FillAllZero(levels);
    
    for (int i = 0; i < rs.GetSize(); i++)
    {
        BL_FLOAT r = rs.Get()[i];
        BL_FLOAT theta = thetas.Get()[i];
        
        // Bound to [-Pi, Pi]
        BL_FLOAT angle = fmod(theta, 2.0*M_PI);
        if (angle < 0.0)
            angle += 2.0*M_PI;
        
        angle -= M_PI;
        
#if 1
        if (angle < 0.0)
            angle += M_PI;
#endif
        
        int binNum = (angle/M_PI)*numBins;
        
        if (binNum < 0)
            binNum = 0;
        if (binNum > numBins - 1)
            binNum = numBins - 1;
        
        // Better to take max, will avoid increase in the borders and decrease in the center
        if (r > levels->Get()[binNum])
            levels->Get()[binNum] = r;
    }
}

void
USTPolarLevel::ProcessLevels(WDL_TypedBuf<BL_FLOAT> *ioLevels)
{
    //BLDebug::DumpData("levels.txt", *ioLevels);
    
#if 0
    // Smooth data
    if (mSmoothWin.GetSize() != SMOOTH_WIN_SIZE)
    {
        Window::MakeHanning(SMOOTH_WIN_SIZE, &mSmoothWin);
    }
    
    BLUtils::SmoothDataWin(ioLevels, mSmoothWin);
#endif
    
    //BLDebug::DumpData("smooth.txt", *ioLevels);
    
#if 0
    // Find maxima
    BLUtils::FindMaxima(ioLevels);
#endif
    
//TODO: make rectangles around maxima
//    smooth with max for both smooth factors
    
    //BLDebug::DumpData("maxima0.txt", *ioLevels);
    
    //BLUtils::ApplyPow(ioLevels, 6.0);
    
    //BLDebug::DumpData("levels0.txt", *ioLevels);
    
    // ORIG
    //BLUtils::ThresholdMinRel(ioLevels, 0.5);
    //BLUtils::ThresholdMinRel(ioLevels, 0.75);
    
    //BLDebug::DumpData("levels1.txt", *ioLevels);
}
