//
//  SoftFft.h
//  BL-PitchShift
//
//  Created by Pan on 03/04/18.
//
//

#ifndef __BL_PitchShift__SoftFft__
#define __BL_PitchShift__SoftFft__

#include "IPlug_include_in_plug_hdr.h"

#include <BLTypes.h>

class SoftFft
{
public:
    static void Ifft(const WDL_TypedBuf<BL_FLOAT> &magns,
                     const WDL_TypedBuf<BL_FLOAT> &freqs,
                     const WDL_TypedBuf<BL_FLOAT> &phases,
                     BL_FLOAT sampleRate,
                     WDL_TypedBuf<BL_FLOAT> *outSamples);
};

#endif /* defined(__BL_PitchShift__SoftFft__) */
