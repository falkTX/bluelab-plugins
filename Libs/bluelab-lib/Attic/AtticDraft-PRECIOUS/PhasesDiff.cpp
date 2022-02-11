//
//  PhasesDiffSaver.cpp
//  BL-PitchShift
//
//  Created by Pan on 18/04/18.
//
//

#include "FftProcessObj14.h"
#include "Utils.h"
#include "Debug.h"
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
#define POINT_SIZE 1.0

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

// Debug
#define DEBUG_IGNORE_SMALL_VALUES 1
#if DEBUG_IGNORE_SMALL_VALUES
#define DEBUG_EPS 1e-7
#endif

// Algo
#define USE_PHASES_UNWRAP 1

#if !USE_LINERP_PHASES
PhasesDiff::PhasesDiff(int bufferSize)
#else
PhasesDiff::PhasesDiff(int bufferSize)
: mPrevFreqObjsL(bufferSize, 4, 1, 44100.0),
  mPrevFreqObjsR(bufferSize, 4, 1, 44100.0),
  mFreqObjsL(bufferSize, 4, 1, 44100.0),
  mFreqObjsR(bufferSize, 4, 1, 44100.0)
#endif
{
    mBufferSize = bufferSize;
    
    mOverlapping = 4;
    mOversampling = 1;
    mSampleRate = 44100.0;
}

PhasesDiff::~PhasesDiff() {}

void
PhasesDiff::Reset()
{
#if !USE_LINERP_PHASES
    for (int i = 0; i < 2; i++)
    {
        mPrevMagns[i].Resize(0);
        mPrevPhases[i].Resize(0);
    }
#else
    Reset(mOverlapping, mOversampling, mSampleRate);
#endif
}

#if USE_LINERP_PHASES
void
PhasesDiff::Reset(int overlapping, int oversampling,
                  double sampleRate)
{
    for (int i = 0; i < 2; i++)
    {
        mPrevMagns[i].Resize(0);
        mPrevPhases[i].Resize(0);
    }
    
    mPrevFreqObjsL.Reset(mBufferSize, overlapping, oversampling, sampleRate);
    mPrevFreqObjsR.Reset(mBufferSize, overlapping, oversampling, sampleRate);
    
    mFreqObjsL.Reset(mBufferSize, overlapping, oversampling, sampleRate);
    mFreqObjsR.Reset(mBufferSize, overlapping, oversampling, sampleRate);
}
#endif

void
PhasesDiff::Capture(const WDL_TypedBuf<double> &magnsL,
                    const WDL_TypedBuf<double> &phasesL,
                    const WDL_TypedBuf<double> &magnsR,
                    const WDL_TypedBuf<double> &phasesR)
{
    mPrevMagns[0] = magnsL;
    mPrevMagns[1] = magnsR;
    
    mPrevPhases[0] = phasesL;
    mPrevPhases[1] = phasesR;
    
#if USE_PHASES_UNWRAP
    Utils::UnwrapPhases(&mPrevPhases[0]);
    Utils::UnwrapPhases(&mPrevPhases[1]);
#endif
}

void
PhasesDiff::ApplyPhasesDiff(WDL_TypedBuf<double> *ioPhasesL,
                            WDL_TypedBuf<double> *ioPhasesR)
{
#if DEBUG_IGNORE_SMALL_VALUES
    bool smaller0 = Utils::IsAllSmallerEps(mPrevMagns[0], DEBUG_EPS);
    bool smaller1 = Utils::IsAllSmallerEps(mPrevMagns[1], DEBUG_EPS);
    
    if (smaller0 && smaller1)
        return;
#endif
    
    if (mPrevPhases[0].GetSize() == 0)
        // Not yet set
        return;
    
    WDL_TypedBuf<double> phases[2];
    phases[0] = *ioPhasesL;
    phases[1] = *ioPhasesR;
    
#if USE_PHASES_UNWRAP
    Utils::UnwrapPhases(&phases[0]);
    Utils::UnwrapPhases(&phases[1]);
#endif
    
    // Algo
    WDL_TypedBuf<double> originPhases[2] = { phases[0], phases[1] };

    WDL_TypedBuf<double> diff;
    ComputeDiff(&diff, originPhases[0], originPhases[1]);
    
    WDL_TypedBuf<double> prevDiff;
    ComputeDiff(&prevDiff, mPrevPhases[0], mPrevPhases[1]);
    
    for (int i = 0; i < phases[1].GetSize(); i++)
    {
        double prevDiffI = prevDiff.Get()[i];
        
        double phase1 = phases[1].Get()[i];

        //
        // NOTE a test was done (BAD result): tested to rever phase (just in case)
        // but this destroys the stereo image
        
    
#if !USE_LINERP_PHASES // Original version
        // Make the second channel vibrate slightly (at around 5kHz and above)
        
        double diffI = diff.Get()[i];
        
        phase1 -= diffI;
        phase1 += prevDiffI;
#endif
        
#if USE_LINERP_PHASES // New version: use freq adjust obj
      // to get precise frequencies, and interpolate the phase we should get
        phase1 = GetLinerpPhase(phases[0], phases[1], mPrevPhases, i, prevDiffI);
#endif
     
    
        phases[1].Get()[i] = phase1;
    }
    
    // Result
    *ioPhasesL = phases[0]; // Just in case
    *ioPhasesR = phases[1];
    
#if DEBUG_GRAPH
    // Compute the result diff
    WDL_TypedBuf<double> resDiff;
    ComputeDiff(&resDiff, phases[0], phases[1]);
    
    // Compute the difference between the two differences
    WDL_TypedBuf<double> diffDiff;
    diffDiff.Resize(resDiff.GetSize());
    for (int i = 0; i < resDiff.GetSize(); i++)
    {
        double prevDiff0 = prevDiff.Get()[i];
        double resDiff0 = resDiff.Get()[i];
        
        double diffDiff0 = resDiff0 - prevDiff0;
        
        diffDiff.Get()[i] = diffDiff0;
    }
    
    // For display: Recompute the difference
    WDL_TypedBuf<double> phaseDiffsRes[2];
    Utils::Diff(&phaseDiffsRes[0], phases[0], originPhases[0]);
    Utils::Diff(&phaseDiffsRes[1], phases[1], originPhases[1]);
    
    DBG_Display(originPhases, phases,
                prevDiff, diff, resDiff, diffDiff);
#endif
}

void
PhasesDiff::ComputeDiff(WDL_TypedBuf<double> *result,
                        const WDL_TypedBuf<double> &phasesL,
                        const WDL_TypedBuf<double> &phasesR)
{
    WDL_TypedBuf<double> phasesL0 = phasesL;
    WDL_TypedBuf<double> phasesR0 = phasesR;
    
    Utils::MapToPi(&phasesL0);
    Utils::MapToPi(&phasesR0);
    
    //Utils::UnwrapPhases(&phasesL0);
    //Utils::UnwrapPhases(&phasesR0);
    
    Utils::Diff(result, phasesL0, phasesR0);
    
    //Utils::UnwrapPhases(result);
    Utils::MapToPi(result);
}

double
PhasesDiff::GetLinerpPhase(const WDL_TypedBuf<double> &phasesL,
                           const WDL_TypedBuf<double> &phasesR,
                           const WDL_TypedBuf<double> prevPhases[2],
                           int index, double prevPhaseDiff)
{
    // Prev
    WDL_TypedBuf<double> prevRealFreqsL;
    mPrevFreqObjsL.ComputeRealFrequencies(prevPhases[0], &prevRealFreqsL);
    
    WDL_TypedBuf<double> prevRealFreqsR;
    mPrevFreqObjsR.ComputeRealFrequencies(prevPhases[1], &prevRealFreqsR);
    
    
    // Current //
    WDL_TypedBuf<double> realFreqsL;
    mFreqObjsL.ComputeRealFrequencies(phasesL, &realFreqsL);
    
    WDL_TypedBuf<double> realFreqsR;
    mFreqObjsR.ComputeRealFrequencies(phasesR, &realFreqsR);
    
    // Get interp phases
    double phaseL = phasesL[index];
    
    ComputePhasesDiffLerp(double realFreq,
                                              const WDL_TypedBuf<double> &realFreqsL,
                                              const WDL_TypedBuf<double> &realFreqsR,
                                              const WDL_TypedBuf<double> &phasesL,
                                              const WDL_TypedBuf<double> &phasesR)
}

double
PhasesDiff::ComputePhasesDiffLerp(double realFreq,
                                  const WDL_TypedBuf<double> &realFreqsL,
                                  const WDL_TypedBuf<double> &realFreqsR,
                                  const WDL_TypedBuf<double> &phasesL,
                                  const WDL_TypedBuf<double> &phasesR)
{
    double tL;
    int phaseIdxL = Utils::FindValueIndex(realFreq, realFreqsL, &tL);
    
    double tR;
    int phaseIdxR = Utils::FindValueIndex(realFreq, realFreqsR, &tR);
    
    // Left
    if (phaseIdxL >= phasesL.GetSize() - 1)
        // Give up
        return 0.0;
    
    double pL0 = phasesL.Get()[phaseIdxL];
    double pL1 = phasesL.Get()[phaseIdxL + 1];
    
    double phaseL = Utils::Interp(pL0, pL1, tL);
    
    // Right
    if (phaseIdxR >= phasesR.GetSize() - 1)
        // Give up
        return 0.0;
    
    double pR0 = phasesR.Get()[phaseIdxR];
    double pR1 = phasesR.Get()[phaseIdxR + 1];
    
    double phaseR = Utils::Interp(pR0, pR1, tR);
    
    // Result
    double res = phaseR - phaseL;
    
    return res;
}
                                  
                                  

void
PhasesDiff::DBG_Display(const WDL_TypedBuf<double> originPhases[2],
                        const WDL_TypedBuf<double> phases[2],
                        const WDL_TypedBuf<double> &prevDiff,
                        const WDL_TypedBuf<double> &diff,
                        const WDL_TypedBuf<double> &resDiff,
                        const WDL_TypedBuf<double> &diffDiff)
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
    WDL_TypedBuf<double> prevDiffX;
    WDL_TypedBuf<double> prevDiffY;
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
    WDL_TypedBuf<double> diffX;
    WDL_TypedBuf<double> diffY;
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
    WDL_TypedBuf<double> resDiffX;
    WDL_TypedBuf<double> resDiffY;
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
    WDL_TypedBuf<double> diffDiffX;
    WDL_TypedBuf<double> diffDiffY;
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
PhasesDiff::DBG_PolarDiffToCartesian(const WDL_TypedBuf<double> &magns,
                                     const WDL_TypedBuf<double> &phaseDiffs,
                                     const WDL_TypedBuf<double> *phases,
                                     WDL_TypedBuf<double> *xValues,
                                     WDL_TypedBuf<double> *yValues)
{
#if POLAR_DISPLAY
    PolarViz::PolarToCartesian(magns, phaseDiffs, xValues, yValues);
#endif
    
#if POLAR_SAMPLES_DISPLAY
    PolarViz::PolarSamplesToCartesian(magns, phaseDiffs, *phases, xValues, yValues);
#endif
}
