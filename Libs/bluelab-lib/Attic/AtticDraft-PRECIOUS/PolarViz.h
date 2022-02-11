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
    static void PolarToCartesian(const WDL_TypedBuf<double> &magns,
                                 const WDL_TypedBuf<double> &phaseDiffs,
                                 WDL_TypedBuf<double> *xValues,
                                 WDL_TypedBuf<double> *yValues);
    
    // NOTE: can be optimized if we have an Fft already computed
    static void PolarSamplesToCartesian(const WDL_TypedBuf<double> &magns,
                                        const WDL_TypedBuf<double> &phaseDiffs,
                                        const WDL_TypedBuf<double> &phases,
                                        WDL_TypedBuf<double> *xValues,
                                        WDL_TypedBuf<double> *yValues);
};

#endif /* defined(__BL_PitchShift__PolarViz__) */
