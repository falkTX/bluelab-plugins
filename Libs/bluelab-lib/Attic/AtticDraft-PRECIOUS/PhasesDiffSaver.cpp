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

#include "PhasesDiffSaver.h"

#define DEBUG_GRAPH 1

#if DEBUG_GRAPH
#include <DebugGraph.h>

#define GRAPH_INPUT_PHASES0 0
#define GRAPH_INPUT_PHASES1 1
#define GRAPH_INPUT_DIFF 2

#define GRAPH_OUTPUT_DIFF 3
#define GRAPH_OUTPUT_PHASES0 4
#define GRAPH_OUTPUT_PHASES1 5
#define GRAPH_OUTPUT_RESULT 6

#define MIN_PHASE_Y -10.0
#define MAX_PHASE_Y 2000.0

#define MIN_PHASE_DIFF_Y -10.0
#define MAX_PHASE_DIFF_Y 10.0

#endif

PhasesDiffSaver::PhasesDiffSaver() {}

PhasesDiffSaver::~PhasesDiffSaver() {}

void
PhasesDiffSaver::PhasesDiff(WDL_TypedBuf<double> *phasesDiff,
                            const double *chan0, const double *chan1,
                            int nFrames)
{
    // TODO: compute half and make symetry !
    
    // Copy buffers, not optimized...
    WDL_TypedBuf<double> chan0Buf;
    Utils::CopyBuf(&chan0Buf, chan0, nFrames);
    
    WDL_TypedBuf<double> chan1Buf;
    Utils::CopyBuf(&chan1Buf, chan1, nFrames);
    
    WDL_TypedBuf<double> magns[2];
    WDL_TypedBuf<double> phases[2];
    FftProcessObj14::SamplesToMagnPhases(chan0Buf, &magns[0], &phases[0]);
    FftProcessObj14::SamplesToMagnPhases(chan1Buf, &magns[1], &phases[1]);
    
    //Utils::TakeHalf(&magns[0]);
    //Utils::TakeHalf(&phases[0]);
    
    //Utils::TakeHalf(&magns[1]);
    //Utils::TakeHalf(&phases[1]);
    
    Utils::Diff(phasesDiff, phases[1], phases[0]);
    //Utils::MapToPi(phasesDiff);
    
#if 1 // For display
    // TEST
    Utils::UnwrapPhases(&phases[0]);
    Utils::UnwrapPhases(&phases[1]);
#endif
    
#if 0 //DEBUG_GRAPH

    DebugGraph::SetCurveValues(phases[0],
                               GRAPH_INPUT_PHASES0,
                               MIN_PHASE_Y, MAX_PHASE_Y, // min and max
                               2.0,
                               255, 0, 0,
                               false, 0.5);
    
    DebugGraph::SetCurveValues(phases[1],
                               GRAPH_INPUT_PHASES1,
                               MIN_PHASE_Y, MAX_PHASE_Y, // min and max
                               2.0,
                               0, 255, 0,
                               false, 0.5);
    
    DebugGraph::SetCurveValues(*phasesDiff,
                               GRAPH_INPUT_DIFF,
                               MIN_PHASE_DIFF_Y, MAX_PHASE_DIFF_Y, // min and max
                               2.0,
                               0, 0, 255,
                               false, 0.5);
#endif
}

void
PhasesDiffSaver::ApplyPhasesDiff(const WDL_TypedBuf<double> &phasesDiff,
                                 const double *chan0, double *chan1, int nFrames)
{
    if (phasesDiff.GetSize() != nFrames)
        return;
    
    // Copy buffers, not optimized...
    WDL_TypedBuf<double> chan0Buf;
    Utils::CopyBuf(&chan0Buf, chan0, phasesDiff.GetSize());
    
    WDL_TypedBuf<double> chan1Buf;
    Utils::CopyBuf(&chan1Buf, chan1, phasesDiff.GetSize());
    
    WDL_TypedBuf<double> magns[2];
    WDL_TypedBuf<double> phases[2];
    
    FftProcessObj14::SamplesToMagnPhases(chan0Buf, &magns[0], &phases[0]);
    FftProcessObj14::SamplesToMagnPhases(chan1Buf, &magns[1], &phases[1]);
    
    //Utils::TakeHalf(&magns[0]);
    //Utils::TakeHalf(&phases[0]);
    
    //Utils::TakeHalf(&magns[1]);
    //Utils::TakeHalf(&phases[1]);
    
    //WDL_TypedBuf<double> origPhaseDiff;
    //Utils::Diff(&origPhaseDiff, phases[1], phases[0]);

    //Utils::SubstractValues(&phases[1], origPhaseDiff);
    
    //WDL_TypedBuf<double> newPhases1 = phases[1];
    
    WDL_TypedBuf<double> newPhases1 = phases[0];
    
    //Utils::SubstractValues(&newPhases1, origPhaseDiff);
    
#if 1
    Utils::ApplyDiff(&newPhases1, phasesDiff);
#endif
    
    FftProcessObj14::/*Half*/MagnPhasesToSamples(magns[1], newPhases1, &chan1Buf);
    
    Utils::CopyBuf(chan1, chan1Buf);
    
#if 1 // For display
    Utils::UnwrapPhases(&phases[0]);
    Utils::UnwrapPhases(&phases[1]);
    Utils::UnwrapPhases(&newPhases1);
#endif
    
#if DEBUG_GRAPH
    DebugGraph::SetCurveValues(phasesDiff,
                               GRAPH_OUTPUT_DIFF,
                               MIN_PHASE_DIFF_Y, MAX_PHASE_DIFF_Y, // min and max
                               1.0,
                               128, 128, 255,
                               false, 0.5);
    
    DebugGraph::SetCurveValues(phases[0],
                               GRAPH_OUTPUT_PHASES0,
                               MIN_PHASE_Y, MAX_PHASE_Y, // min and max
                               1.0,
                               255, 128, 128,
                               false, 0.5);
    
    DebugGraph::SetCurveValues(phases[1],
                               GRAPH_OUTPUT_PHASES1,
                               MIN_PHASE_Y, MAX_PHASE_Y, // min and max
                               1.0,
                               128, 255, 128,
                               false, 0.5);
    
    DebugGraph::SetCurveValues(newPhases1,
                               GRAPH_OUTPUT_RESULT,
                               MIN_PHASE_Y, MAX_PHASE_Y, // min and max
                               1.0,
                               255, 255, 255,
                               false, 0.5);
#endif
}
