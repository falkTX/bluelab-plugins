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
//  PhasesDiffSaver.cpp
//  BL-PitchShift
//
//  Created by Pan on 18/04/18.
//
//

#include <FftProcessObj16.h>

#include <BLUtils.h>
#include <BLUtilsPhases.h>

#include "PolarViz.h"

#include "PhasesDiff.h"

#define DEBUG_GRAPH 0

#if DEBUG_GRAPH
#include <DebugGraph.h>

//
// Curves
//

// Inputs
#define GRAPH_INPUT_PHASES0 0
#define GRAPH_INPUT_PHASES1 1

// Outputs
#define GRAPH_OUTPUT_PHASES0 2
#define GRAPH_OUTPUT_PHASES1 3

// Results
#define GRAPH_RESULT_PHASES0 4
#define GRAPH_RESULT_PHASES1 5

// Diffs lines
#define GRAPH_DIFF 6
#define GRAPH_PREV_DIFF 7
#define GRAPH_RES_DIFF 8
#define GRAPH_DIFF_DIFF 9

// Diffs polar
#define GRAPH_DIFF_POLAR 10
#define GRAPH_PREV_DIFF_POLAR 11
#define GRAPH_RES_DIFF_POLAR 12
#define GRAPH_DIFF_DIFF_POLAR 13


// Bounds
#define MIN_PHASE_Y -10.0
#define MAX_PHASE_Y 8000.0

#define MIN_PHASE_DIFF_Y -10.0
#define MAX_PHASE_DIFF_Y 10.0

#define MIN_PHASE_DIFF_DIFF_Y -1e-11
#define MAX_PHASE_DIFF_DIFF_Y 1e-11

// Bounds polar
#define MIN_PHASE_POLAR_X -1.0
#define MAX_PHASE_POLAR_X 1.0

#define MIN_PHASE_POLAR_Y -1.0
#define MAX_PHASE_POLAR_Y 1.0

// Draw style

// Lines
#define INPUT_LINE_WIDTH 1.0
#define OUTPUT_LINE_WIDTH 1.0
#define RESULT_LINE_WIDTH 1.0

// Points
#define POINTS_FILL 1
#define POINTS_ALPHA 0.5
#define POINT_SIZE 1.0 // 2.0

// Colors
#define INPUT_INTENSITY 0
#define OUTPUT_INTENSITY 128
#define RESULT_INTENSITY 0

// DISPLAY: What to display
#define DISPLAY_INPUTS 0
#define DISPLAY_OUTPUTS 0
#define DISPLAY_RESULTS 0

#define DISPLAY_DIFF 1
#define DISPLAY_PREV_DIFF 1
#define DISPLAY_RES_DIFF 1
#define DISPLAY_DIFF_DIFF 1

// Draw type
#define DISPLAY_LINES 0
#define DISPLAY_POINTS 1

#define POLAR_DISPLAY 1
#define POLAR_SAMPLES_DISPLAY 0

#endif // DEBUG_GRAPH

// DEBUG (and just for debugging)
#define DEBUG_IGNORE_SMALL_VALUES 0
#if DEBUG_IGNORE_SMALL_VALUES
#define DEBUG_EPS 1e-7
#endif

// Algo
#define USE_PHASES_UNWRAP 1

#if !USE_LINERP_PHASES
PhasesDiff::PhasesDiff(int bufferSize)
#else
PhasesDiff::PhasesDiff(int bufferSize)
: mFreqObjsL(bufferSize, 4, 1, 44100.0)
#endif
{
#if USE_LINERP_PHASES
    mBufferSize = bufferSize;
    mOverlapping = 4;
    mOversampling = 1;
    mSampleRate = 44100.0;
#endif
}

PhasesDiff::~PhasesDiff() {}

void
PhasesDiff::Reset()
{
#if USE_LINERP_PHASES
    Reset(mBufferSize, mOverlapping, mOversampling, mSampleRate);
#else
    for (int i = 0; i < 2; i++)
    {
        mPrevMagns[i].Resize(0);
        mPrevPhases[i].Resize(0);
    }
#endif
}

void
PhasesDiff::Reset(int bufferSize, int overlapping, int oversampling,
                  BL_FLOAT sampleRate)
{
    for (int i = 0; i < 2; i++)
    {
        mPrevMagns[i].Resize(0);
        mPrevPhases[i].Resize(0);
    }
  
#if USE_LINERP_PHASES
    mFreqObjsL.Reset(bufferSize, overlapping, oversampling, sampleRate);
#endif
}

void
PhasesDiff::Capture(const WDL_TypedBuf<BL_FLOAT> &magnsL,
                    const WDL_TypedBuf<BL_FLOAT> &phasesL,
                    const WDL_TypedBuf<BL_FLOAT> &magnsR,
                    const WDL_TypedBuf<BL_FLOAT> &phasesR)
{
    mPrevMagns[0] = magnsL;
    mPrevMagns[1] = magnsR;
    
    mPrevPhases[0] = phasesL;
    mPrevPhases[1] = phasesR;
    
#if USE_PHASES_UNWRAP
    BLUtilsPhases::UnwrapPhases(&mPrevPhases[0]);
    BLUtilsPhases::UnwrapPhases(&mPrevPhases[1]);
#endif
}

void
PhasesDiff::ApplyPhasesDiff(WDL_TypedBuf<BL_FLOAT> *ioPhasesL,
                            WDL_TypedBuf<BL_FLOAT> *ioPhasesR)
{
#if DEBUG_IGNORE_SMALL_VALUES
    bool smaller0 = BLUtils::IsAllSmallerEps(mPrevMagns[0], DEBUG_EPS);
    bool smaller1 = BLUtils::IsAllSmallerEps(mPrevMagns[1], DEBUG_EPS);
    
    if (smaller0 && smaller1)
        return;
#endif
    
    if (mPrevPhases[0].GetSize() == 0)
        // Not yet set
        return;
    
    WDL_TypedBuf<BL_FLOAT> phasesL = *ioPhasesL;
    WDL_TypedBuf<BL_FLOAT> phasesR = *ioPhasesR;
    
#if USE_PHASES_UNWRAP
    BLUtilsPhases::UnwrapPhases(&phasesL);
    BLUtilsPhases::UnwrapPhases(&phasesR);
#endif
    
    // Algo
    WDL_TypedBuf<BL_FLOAT> originPhases[2] = { phasesL, phasesR };

    WDL_TypedBuf<BL_FLOAT> diff;
    ComputeDiff(&diff, originPhases[0], originPhases[1]);
    
    WDL_TypedBuf<BL_FLOAT> prevDiff;
    ComputeDiff(&prevDiff, mPrevPhases[0], mPrevPhases[1]);
    
#if USE_LINERP_PHASES
    // Real frequencies
    
    // Current
    WDL_TypedBuf<BL_FLOAT> theoricalFreqs;
    BLUtils::FftFreqs(&theoricalFreqs, ioPhasesL->GetSize(), mSampleRate);
    
    WDL_TypedBuf<BL_FLOAT> realFreqsL;
    mFreqObjsL.ComputeRealFrequencies(phasesL, &realFreqsL);
#endif
    
    for (int i = 0; i < phasesR.GetSize(); i++)
    {
#if !USE_LINERP_PHASES // Simple method (but efficient)
      BL_FLOAT diffI = diff.Get()[i];
      BL_FLOAT prevDiffI = prevDiff.Get()[i];
      
      // Left
      BL_FLOAT phaseL = phasesL.Get()[i];
      phaseL += diffI/2.0;
      phaseL -= prevDiffI/2.0;
      phasesL.Get()[i] = phaseL;
        
      // Right
      BL_FLOAT phaseR = phasesR.Get()[i];
      phaseR -= diffI/2.0;
      phaseR += prevDiffI/2.0;
      phasesR.Get()[i] = phaseR;
    
#else // USE_LINERP_PHASES
      //
      // To try to get precise frequencies
        
      // Find the correspondance between the new frequency bins (pitched for example),
      // and find the corresponding bin of the original signal
      //
      // NOTE: consider the processing is the same on the two stereo channels
      // (for more, something additional must be implemented)
            
      // Take for example the left channel
      // Find the bin indice in the origina signal
      BL_FLOAT realFreqL = realFreqsL.Get()[i];
      
      int prevIndexL = prevIndexL = BLUtils::FindValueIndex(realFreqL, theoricalFreqs, NULL);
      // Exactly the same as "theorical"
      //int prevIndexL = prevIndexL = BLUtils::FindValueIndex(realFreqL, prevRealFreqsL, NULL);
        
      if (prevIndexL < 0)
        prevIndexL = 0;
      if (prevIndexL > prevDiff.GetSize() - 1)
        prevIndexL = prevDiff.GetSize() - 1;
            
      // NOTE: Exactly the same behaviour as above (but with a "better" index)
      BL_FLOAT prevDiffI = prevDiff.Get()[prevIndexL];
      BL_FLOAT diffI = diff.Get()[i];
        
      // Left
      BL_FLOAT phaseL = phasesL.Get()[i];
      phaseL += diffI/2.0;
      phaseL -= prevDiffI/2.0;
      phasesL.Get()[i] = phaseL;
            
      // Right
      BL_FLOAT phaseR = phasesR.Get()[i];
      phaseR -= diffI/2.0;
      phaseR += prevDiffI/2.0;
      phasesR.Get()[i] = phaseR;
#endif
    }
    
    // Result
    *ioPhasesL = phasesL;
    *ioPhasesR = phasesR;
    
#if DEBUG_GRAPH
    // Compute the result diff
    WDL_TypedBuf<BL_FLOAT> resDiff;
    ComputeDiff(&resDiff, phasesL, phasesR);
    
    // Compute the difference between the two differences
    WDL_TypedBuf<BL_FLOAT> diffDiff;
    diffDiff.Resize(resDiff.GetSize());
    for (int i = 0; i < resDiff.GetSize(); i++)
    {
        BL_FLOAT prevDiff0 = prevDiff.Get()[i];
        BL_FLOAT resDiff0 = resDiff.Get()[i];
        
        BL_FLOAT diffDiff0 = resDiff0 - prevDiff0;
        
        diffDiff.Get()[i] = diffDiff0;
    }
    
    // For display: Recompute the difference
    WDL_TypedBuf<BL_FLOAT> phaseDiffsRes[2];
    BLUtils::Diff(&phaseDiffsRes[0], phasesL, originPhases[0]);
    BLUtils::Diff(&phaseDiffsRes[1], phasesR, originPhases[1]);
    
    WDL_TypedBuf<BL_FLOAT> phases[2] = { phasesL, phasesR };
    
    DBG_Display(originPhases, phases,
                prevDiff, diff, resDiff, diffDiff);    
#endif
}

void
PhasesDiff::ComputeDiff(WDL_TypedBuf<BL_FLOAT> *result,
                        const WDL_TypedBuf<BL_FLOAT> &phasesL,
                        const WDL_TypedBuf<BL_FLOAT> &phasesR)
{
    WDL_TypedBuf<BL_FLOAT> phasesL0 = phasesL;
    WDL_TypedBuf<BL_FLOAT> phasesR0 = phasesR;
    
    // ?
    BLUtilsPhases::MapToPi(&phasesL0);
    BLUtilsPhases::MapToPi(&phasesR0);
    
    BLUtils::Diff(result, phasesL0, phasesR0);
    BLUtilsPhases::MapToPi(result);
}

void
PhasesDiff::DBG_Display(const WDL_TypedBuf<BL_FLOAT> originPhases[2],
                        const WDL_TypedBuf<BL_FLOAT> phases[2],
                        const WDL_TypedBuf<BL_FLOAT> &prevDiff,
                        const WDL_TypedBuf<BL_FLOAT> &diff,
                        const WDL_TypedBuf<BL_FLOAT> &resDiff,
                        const WDL_TypedBuf<BL_FLOAT> &diffDiff)
{
#if DEBUG_GRAPH
    
    // Lines
#if DISPLAY_LINES
    
#if DISPLAY_INPUTS
    DebugGraph::SetCurveValues(mPrevPhases[0],
                               GRAPH_INPUT_PHASES0,
                               MIN_PHASE_Y, MAX_PHASE_Y,
                               INPUT_LINE_WIDTH,
                               255, INPUT_INTENSITY, INPUT_INTENSITY,
                               false, 0.5);
    
    DebugGraph::SetCurveValues(mPrevPhases[1],
                               GRAPH_INPUT_PHASES1,
                               MIN_PHASE_Y, MAX_PHASE_Y,
                               INPUT_LINE_WIDTH,
                               255, INPUT_INTENSITY, INPUT_INTENSITY,
                               false, 0.5);
#endif
    
    // Phases
#if DISPLAY_OUTPUTS
    DebugGraph::SetCurveValues(originPhases[0],
                               GRAPH_OUTPUT_PHASES0,
                               MIN_PHASE_Y, MAX_PHASE_Y,
                               OUTPUT_LINE_WIDTH,
                               OUTPUT_INTENSITY, OUTPUT_INTENSITY, 255,
                               false, 0.5);
    
    DebugGraph::SetCurveValues(originPhases[1],
                               GRAPH_OUTPUT_PHASES1,
                               MIN_PHASE_Y, MAX_PHASE_Y,
                               OUTPUT_LINE_WIDTH,
                               OUTPUT_INTENSITY, OUTPUT_INTENSITY, 255,
                               false, 0.5);
#endif
    
#if DISPLAY_RESULTS
    DebugGraph::SetCurveValues(phases[0],
                               GRAPH_RESULT_PHASES0,
                               MIN_PHASE_Y, MAX_PHASE_Y,
                               RESULT_LINE_WIDTH,
                               RESULT_INTENSITY, 255, RESULT_INTENSITY,
                               false, 0.5);
    
    DebugGraph::SetCurveValues(phases[1],
                               GRAPH_RESULT_PHASES1,
                               MIN_PHASE_Y, MAX_PHASE_Y,
                               RESULT_LINE_WIDTH,
                               RESULT_INTENSITY, 255, RESULT_INTENSITY,
                               false, 0.5);
#endif
    
    // Diffs
#if DISPLAY_PREV_DIFF
    DebugGraph::SetCurveValues(prevDiff,
                               GRAPH_PREV_DIFF,
                               MIN_PHASE_DIFF_Y, MAX_PHASE_DIFF_Y,
                               INPUT_LINE_WIDTH,
                               255, INPUT_INTENSITY, INPUT_INTENSITY,
                               false, 0.5);
#endif
    
#if DISPLAY_DIFF
    DebugGraph::SetCurveValues(diff,
                               GRAPH_DIFF,
                               MIN_PHASE_DIFF_Y, MAX_PHASE_DIFF_Y,
                               OUTPUT_LINE_WIDTH,
                               INPUT_INTENSITY, INPUT_INTENSITY, 255,
                               false, 0.5);
#endif

#if DISPLAY_RES_DIFF
    DebugGraph::SetCurveValues(resDiff,
                               GRAPH_RES_DIFF,
                               MIN_PHASE_DIFF_Y, MAX_PHASE_DIFF_Y,
                               OUTPUT_LINE_WIDTH,
                               OUTPUT_INTENSITY, OUTPUT_INTENSITY, 255,
                               false, 0.5);
#endif

#if DISPLAY_DIFF_DIFF
    DebugGraph::SetCurveValues(diffDiff,
                               GRAPH_DIFF_DIFF,
                               MIN_PHASE_DIFF_DIFF_Y, MAX_PHASE_DIFF_DIFF_Y,
                               OUTPUT_LINE_WIDTH,
                               255, 255, 255,
                               false, 0.5);
#endif
    
#endif
    
    // Points
#if DISPLAY_POINTS
    
#if DISPLAY_PREV_DIFF
    WDL_TypedBuf<BL_FLOAT> prevDiffX;
    WDL_TypedBuf<BL_FLOAT> prevDiffY;
    DBG_PolarDiffToCartesian(mPrevMagns[0], prevDiff, &originPhases[0],
                             &prevDiffX, &prevDiffY);
    
    DebugGraph::SetPointValues(prevDiffX, prevDiffY,
                               GRAPH_PREV_DIFF_POLAR,
                               MIN_PHASE_POLAR_X, MAX_PHASE_POLAR_X,
                               MIN_PHASE_POLAR_Y, MAX_PHASE_POLAR_Y,
                               POINT_SIZE,
                               INPUT_INTENSITY, INPUT_INTENSITY, 255,
                               POINTS_FILL, POINTS_ALPHA);
#endif

#if DISPLAY_DIFF
    WDL_TypedBuf<BL_FLOAT> diffX;
    WDL_TypedBuf<BL_FLOAT> diffY;
    DBG_PolarDiffToCartesian(mPrevMagns[0], diff, &originPhases[0],
                             &diffX, &diffY);
    
    DebugGraph::SetPointValues(diffX, diffY,
                               GRAPH_DIFF_POLAR,
                               MIN_PHASE_POLAR_X, MAX_PHASE_POLAR_X,
                               MIN_PHASE_POLAR_Y, MAX_PHASE_POLAR_Y,
                               POINT_SIZE,
                               255, INPUT_INTENSITY, INPUT_INTENSITY,
                               POINTS_FILL, POINTS_ALPHA);
#endif

#if DISPLAY_RES_DIFF
    WDL_TypedBuf<BL_FLOAT> resDiffX;
    WDL_TypedBuf<BL_FLOAT> resDiffY;
    DBG_PolarDiffToCartesian(mPrevMagns[0], resDiff, &originPhases[0],
                             &resDiffX, &resDiffY);
    
    DebugGraph::SetPointValues(resDiffX, resDiffY,
                               GRAPH_RES_DIFF_POLAR,
                               MIN_PHASE_POLAR_X, MAX_PHASE_POLAR_X,
                               MIN_PHASE_POLAR_Y, MAX_PHASE_POLAR_Y,
                               POINT_SIZE,
                               OUTPUT_INTENSITY, OUTPUT_INTENSITY, 255,
                               POINTS_FILL, POINTS_ALPHA);
#endif

#if DISPLAY_DIFF_DIFF
    WDL_TypedBuf<BL_FLOAT> diffDiffX;
    WDL_TypedBuf<BL_FLOAT> diffDiffY;
    DBG_PolarDiffToCartesian(mPrevMagns[0], diffDiff, &originPhases[0],
                             &diffDiffX, &diffDiffY);
    
    DebugGraph::SetPointValues(diffDiffX, diffDiffY,
                               GRAPH_DIFF_DIFF_POLAR,
                               MIN_PHASE_POLAR_X, MAX_PHASE_POLAR_X,
                               MIN_PHASE_POLAR_Y, MAX_PHASE_POLAR_Y,
                               POINT_SIZE,
                               255, 255, 255,
                               POINTS_FILL, POINTS_ALPHA);
#endif
    
#endif
    
#endif
}

void
PhasesDiff::DBG_PolarDiffToCartesian(const WDL_TypedBuf<BL_FLOAT> &magns,
                                     const WDL_TypedBuf<BL_FLOAT> &phaseDiffs,
                                     const WDL_TypedBuf<BL_FLOAT> *phases,
                                     WDL_TypedBuf<BL_FLOAT> *xValues,
                                     WDL_TypedBuf<BL_FLOAT> *yValues)
{
#if POLAR_DISPLAY
    PolarViz::PolarToCartesian(magns, phaseDiffs, xValues, yValues);
#endif
    
#if POLAR_SAMPLES_DISPLAY
    PolarViz::PolarSamplesToCartesian(magns, phaseDiffs, *phases, xValues, yValues);
#endif
}
