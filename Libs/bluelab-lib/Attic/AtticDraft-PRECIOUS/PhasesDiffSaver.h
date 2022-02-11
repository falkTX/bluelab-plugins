//
//  PhasesDiffSaver.h
//  BL-PitchShift
//
//  Created by Pan on 18/04/18.
//
//

#ifndef __BL_PitchShift__PhasesDiffSaver__
#define __BL_PitchShift__PhasesDiffSaver__

#include "IPlug_include_in_plug_hdr.h"

// Used to save the stereo phases difference before processing
// and restore them after processing
// (to avoid "phasing effect)
class PhasesDiffSaver
{
public:
    PhasesDiffSaver();
    
    virtual ~PhasesDiffSaver();
    
    void PhasesDiff(WDL_TypedBuf<double> *phasesDiff,
                    const double *chan0, const double *chan1,
                    int nFrames);
    
    void ApplyPhasesDiff(const WDL_TypedBuf<double> &phasesDiff,
                         const double *chan0, double *chan1, int nFrames);
};

#endif /* defined(__BL_PitchShift__PhasesDiffSaver__) */
