#ifndef QIFFT_H
#define QIFFT_H

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

// See: https://ccrma.stanford.edu/files/papers/stanm118.pdf
// And (p32): http://mural.maynoothuniversity.ie/4523/1/thesis.pdf
// And: https://ccrma.stanford.edu/~jos/parshl/Peak_Detection_Steps_3.html#sec:peakdet
//
// NOTE: for alpha0 and beta0, the fft must have used a window
// (Gaussian or Hann or other)
class QIFFT
{
 public:
    struct Peak
    {
        // True peak idx, in floating point format
        BL_FLOAT mBinIdx;

        // Amp for true peak, in dB
        BL_FLOAT mAmp;

        // Phase for true peak
        BL_FLOAT mPhase;

        // Amp derivative over time
        BL_FLOAT mAlpha0;

        // Freq derivative over time
        BL_FLOAT mBeta0;
    };
    
    // Magns should be in dB!
    static void FindPeak(const WDL_TypedBuf<BL_FLOAT> &magns,
                         const WDL_TypedBuf<BL_FLOAT> &phases,
                         int peakBin, Peak *result);
};

#endif
