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

// BAD (but not so much...)
// New version, to try to avoid right channel that vibrates a little
#define USE_LINERP_PHASES 0

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

    void Reset(int bufferSize, int overlapping, int oversampling,
               BL_FLOAT sampleRate);
    
    void Capture(const WDL_TypedBuf<BL_FLOAT> &magnsL,
                 const WDL_TypedBuf<BL_FLOAT> &phasesL,
                 const WDL_TypedBuf<BL_FLOAT> &magnsR,
                 const WDL_TypedBuf<BL_FLOAT> &phasesR);
    
    void ApplyPhasesDiff(WDL_TypedBuf<BL_FLOAT> *ioPhasesL,
                         WDL_TypedBuf<BL_FLOAT> *ioPhasesR);
    
protected:
    void ComputeDiff(WDL_TypedBuf<BL_FLOAT> *result,
                     const WDL_TypedBuf<BL_FLOAT> &phasesL,
                     const WDL_TypedBuf<BL_FLOAT> &phasesR);
    
    void DBG_Display(const WDL_TypedBuf<BL_FLOAT> originPhases[2],
                     const WDL_TypedBuf<BL_FLOAT> phases[2],
                     const WDL_TypedBuf<BL_FLOAT> &prevDiff,
                     const WDL_TypedBuf<BL_FLOAT> &diff,
                     const WDL_TypedBuf<BL_FLOAT> &resDiff,
                     const WDL_TypedBuf<BL_FLOAT> &diffDiff);

    // Phases is used for polar samples
    // can be NULL
    void DBG_PolarDiffToCartesian(const WDL_TypedBuf<BL_FLOAT> &magns,
                                  const WDL_TypedBuf<BL_FLOAT> &phaseDiffs,
                                  const WDL_TypedBuf<BL_FLOAT> *phases,
                                  WDL_TypedBuf<BL_FLOAT> *xValues,
                                  WDL_TypedBuf<BL_FLOAT> *yValues);

    
    WDL_TypedBuf<BL_FLOAT> mPrevMagns[2];
    WDL_TypedBuf<BL_FLOAT> mPrevPhases[2];

#if USE_LINERP_PHASES
    // Previous phases are phases before processing (e.g pitch shift)
    // Current phases are for phases after processing
    
    // Freq adjust obj for current phases
    // Left and right channels
    FreqAdjustObj3 mFreqObjsL;
    
    int mBufferSize;
    int mOverlapping;
    int mOversampling;
    BL_FLOAT mSampleRate;
#endif
};

#endif /* defined(__BL_PitchShift__PhasesDiff__) */
