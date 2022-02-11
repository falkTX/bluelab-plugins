//
//  AWeighting.h
//  AutoGain
//
//  Created by Pan on 30/01/18.
//
//

#ifndef __AutoGain__AWeighting__
#define __AutoGain__AWeighting__

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

// See: https://en.wikipedia.org/wiki/A-weighting
//
// and: https://community.plm.automation.siemens.com/t5/Testing-Knowledge-Base/What-is-A-weighting/ta-p/357894
//

class AWeighting
{
public:
    // numBins is fftSize/2 !
    // See: http://support.ircam.fr/docs/AudioSculpt/3.0/co/FFT%20Size.html
    //
    static void ComputeAWeights(WDL_TypedBuf<BL_FLOAT> *result, int numBins, BL_FLOAT sampleRate);
    
protected:
    static BL_FLOAT ComputeR(BL_FLOAT frequency);
    
    static BL_FLOAT ComputeA(BL_FLOAT frequency);
};

#endif /* defined(__AutoGain__AWeighting__) */
