//
//  PolarViz.h
//  BL-PitchShift
//
//  Created by Pan on 22/04/18.
//
//

#ifndef __BL_PitchShift__PolarViz__
#define __BL_PitchShift__PolarViz__

#include "IPlug_include_in_plug_hdr.h"

class PolarViz
{
public:
    // NOTE: be careful, the phases are in fact difference of phases !
    // Use a threshold to suppress the tiny values
    static void PolarToCartesian(const WDL_TypedBuf<BL_FLOAT> &magns,
                                 const WDL_TypedBuf<BL_FLOAT> &phaseDiffs,
                                 WDL_TypedBuf<BL_FLOAT> *xValues,
                                 WDL_TypedBuf<BL_FLOAT> *yValues,
                                 BL_FLOAT thresholdDB = 0.0);
    
    // Simple version
    // Consider radius an angle
    // No matter of dB scale or rotation
    static void PolarToCartesian2(const WDL_TypedBuf<BL_FLOAT> &Rs,
                                  const WDL_TypedBuf<BL_FLOAT> &thetas,
                                  WDL_TypedBuf<BL_FLOAT> *xValues,
                                  WDL_TypedBuf<BL_FLOAT> *yValues);

    
    // NOTE: maybe the following method is stupid, and similar to the simpler
    // methdod just above
    static void PolarSamplesToCartesian(const WDL_TypedBuf<BL_FLOAT> &magns,
                                        const WDL_TypedBuf<BL_FLOAT> &phaseDiffs,
                                        const WDL_TypedBuf<BL_FLOAT> &phases,
                                        WDL_TypedBuf<BL_FLOAT> *xValues,
                                        WDL_TypedBuf<BL_FLOAT> *yValues);
    
    // NOTE: maybe the following method is stupid, and similar to the simpler
    // methdod just above
    
    // Optimized version, if we already have the samples
    static void PolarSamplesToCartesian2(const WDL_TypedBuf<BL_FLOAT> &samples,
                                         const WDL_TypedBuf<BL_FLOAT> &phaseDiffs,
                                         const WDL_TypedBuf<BL_FLOAT> &phases,
                                         WDL_TypedBuf<BL_FLOAT> *xValues,
                                         WDL_TypedBuf<BL_FLOAT> *yValues);
};

#endif /* defined(__BL_PitchShift__PolarViz__) */
