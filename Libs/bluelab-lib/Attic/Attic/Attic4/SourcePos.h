//
//  SourcePos.h
//  BL-StereoWidth
//
//  Created by applematuer on 10/18/18.
//
//

#ifndef __BL_StereoWidth__SourcePos__
#define __BL_StereoWidth__SourcePos__

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

class SourcePos
{
public:
    static BL_FLOAT GetDefaultMicSeparation();
    
    static void MagnPhasesToSourcePos(WDL_TypedBuf<BL_FLOAT> *outSourceRs,
                                      WDL_TypedBuf<BL_FLOAT> *outSourceThetas,
                                      const WDL_TypedBuf<BL_FLOAT> &magnsL,
                                      const WDL_TypedBuf<BL_FLOAT> &magnsR,
                                      const WDL_TypedBuf<BL_FLOAT> &phasesL,
                                      const WDL_TypedBuf<BL_FLOAT> &phasesR,
                                      const WDL_TypedBuf<BL_FLOAT> &freqs,
                                      const WDL_TypedBuf<BL_FLOAT> &timeDelays);
    
    static void SourcePosToMagnPhases(const WDL_TypedBuf<BL_FLOAT> &sourceRs,
                                      const WDL_TypedBuf<BL_FLOAT> &sourceThetas,
                                      BL_FLOAT micsDistance,
                                      BL_FLOAT widthFactor,
                                      WDL_TypedBuf<BL_FLOAT> *ioMagnsL,
                                      WDL_TypedBuf<BL_FLOAT> *ioMagnsR,
                                      WDL_TypedBuf<BL_FLOAT> *ioPhasesL,
                                      WDL_TypedBuf<BL_FLOAT> *ioPhasesR,
                                      const WDL_TypedBuf<BL_FLOAT> &freqs,
                                      const WDL_TypedBuf<BL_FLOAT> &timeDelays);
    
    // Return true is the distance an agle have been actually computed
    static bool MagnPhasesToSourcePos(BL_FLOAT *sourceR, BL_FLOAT *sourceTheta,
                                      BL_FLOAT magnsL, BL_FLOAT magnsR,
                                      BL_FLOAT phasesL, BL_FLOAT phasesR,
                                      BL_FLOAT freq,
                                      BL_FLOAT timeDelay);
    
    // Uses second order equation
    // Well checked for identity transform !
    // v2
    static bool SourcePosToMagnPhases(BL_FLOAT sourceR, BL_FLOAT sourceTheta,
                                      BL_FLOAT d,
                                      BL_FLOAT *ioMagnL, BL_FLOAT *ioMagnR,
                                      BL_FLOAT *ioPhaseL, BL_FLOAT *ioPhaseR,
                                      BL_FLOAT freq,
                                      BL_FLOAT timeDelay);
    
protected:
    static void ModifyPhase(BL_FLOAT *phaseL, BL_FLOAT *phaseR, BL_FLOAT factor);
};

#endif /* defined(__BL_StereoWidth__SourcePos__) */
