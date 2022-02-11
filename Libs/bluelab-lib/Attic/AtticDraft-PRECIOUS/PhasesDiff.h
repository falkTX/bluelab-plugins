//
//  PhasesDiffSaver.h
//  BL-PitchShift
//
//  Created by Pan on 18/04/18.
//
//

#ifndef __BL_PitchShift__PhasesDiff__
#define __BL_PitchShift__PhasesDiff__

#include <FreqAdjustObj3.h>

#include "IPlug_include_in_plug_hdr.h"

#define USE_LINERP_PHASES 1

// Used to save the stereo phases difference before processing
// and restore them after processing
// (to avoid "phasing effect)
class PhasesDiff
{
public:
    // bufferSize: for USE_LINERP_PHASES
    PhasesDiff(int bufferSize);
    
    virtual ~PhasesDiff();

    void Reset();

#if USE_LINERP_PHASES
    void Reset(int overlapping, int oversampling,
               double sampleRate);
#endif
    
    void Capture(const WDL_TypedBuf<double> &magnsL,
                 const WDL_TypedBuf<double> &phasesL,
                 const WDL_TypedBuf<double> &magnsR,
                 const WDL_TypedBuf<double> &phasesR);
    
    void ApplyPhasesDiff(WDL_TypedBuf<double> *ioPhasesL,
                         WDL_TypedBuf<double> *ioPhasesR);
    
protected:
    void ComputeDiff(WDL_TypedBuf<double> *result,
                     const WDL_TypedBuf<double> &phasesL,
                     const WDL_TypedBuf<double> &phasesR);
    
    double GetLinerpPhase(const WDL_TypedBuf<double> &phasesL,
                          const WDL_TypedBuf<double> &phasesR,
                          const WDL_TypedBuf<double> prevPhases[2],
                          int index, double prevPhaseDiff);
    
    double ComputePhasesDiffLerp(double realFreq,
                                 const WDL_TypedBuf<double> &realFreqsL,
                                 const WDL_TypedBuf<double> &realFreqsR,
                                 const WDL_TypedBuf<double> &phasesL,
                                 const WDL_TypedBuf<double> &phasesR);
    
    void DBG_Display(const WDL_TypedBuf<double> originPhases[2],
                     const WDL_TypedBuf<double> phases[2],
                     const WDL_TypedBuf<double> &prevDiff,
                     const WDL_TypedBuf<double> &diff,
                     const WDL_TypedBuf<double> &resDiff,
                     const WDL_TypedBuf<double> &diffDiff);

    
    // Phases is used for polar samples
    // can be NULL
    void DBG_PolarDiffToCartesian(const WDL_TypedBuf<double> &magns,
                                  const WDL_TypedBuf<double> &phaseDiffs,
                                  const WDL_TypedBuf<double> *phases,
                                  WDL_TypedBuf<double> *xValues,
                                  WDL_TypedBuf<double> *yValues);

    
    WDL_TypedBuf<double> mPrevMagns[2];
    WDL_TypedBuf<double> mPrevPhases[2];
    
#if USE_LINERP_PHASES
    // Previous phases are phases before processing (e.g pitch shift)
    // Current phases are for phases after processing
    
    // Freq adjust obj for previous phases
    // Left and right channels
    FreqAdjustObj3 mPrevFreqObjsL;
    FreqAdjustObj3 mPrevFreqObjsR;
    
    // Freq adjust obj for current phases
    // Left and right channels
    FreqAdjustObj3 mFreqObjsL;
    FreqAdjustObj3 mFreqObjsR;
    
    int mBufferSize;
    int mOverlapping;
    int mOversampling;
    double mSampleRate;
#endif
};

#endif /* defined(__BL_PitchShift__PhasesDiff__) */
