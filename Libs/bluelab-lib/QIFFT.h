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
//
// NOTE: this will work for amplitude only if either
// - We use a Gaussian window insteadof Hann
// - Or we use a fft zero padding factor of x2
//
class QIFFT
{
 public:
    struct Peak
    {
        // True peak idx, in floating point format
        BL_FLOAT mBinIdx;

        BL_FLOAT mFreq;
        
        // Amp for true peak, in dB
        BL_FLOAT mAmp;

        // Phase for true peak
        BL_FLOAT mPhase;

        // Amp derivative over time
        BL_FLOAT mAlpha0;

        // Freq derivative over time
        BL_FLOAT mBeta0;
    };

    // NOTE: FindPeak() and FindPeak2() give the same results,
    // modulo some scaling coefficient
    
    // Magns should be in dB!
    //
    // Custom method
    static void FindPeak(const WDL_TypedBuf<BL_FLOAT> &magns,
                         const WDL_TypedBuf<BL_FLOAT> &phases,
                         int bufferSize,
                         int peakBin, Peak *result);

    // Magns should be in dB!
    //
    // Method using all the formulas in appendix A of the paper
    static void FindPeak2(const WDL_TypedBuf<BL_FLOAT> &magns,
                          const WDL_TypedBuf<BL_FLOAT> &phases,
                          int peakBin, Peak *result);
    
 protected:
    // Parabola equation: y(x) = a*(x - c)^2 + b
    // Specific to peak tracking
    static void GetParabolaCoeffs(BL_FLOAT alpha, BL_FLOAT beta, BL_FLOAT gamma,
                                  BL_FLOAT *a, BL_FLOAT *b, BL_FLOAT *c);

    static BL_FLOAT ParabolaFunc(BL_FLOAT x, BL_FLOAT a, BL_FLOAT b, BL_FLOAT c);

    // Parabola equation: y(x) = a*x^2 + b*x + c
    // Generalized (no maximum constraint)
    static void GetParabolaCoeffsGen(BL_FLOAT alpha, BL_FLOAT beta, BL_FLOAT gamma,
                                     BL_FLOAT *a, BL_FLOAT *b, BL_FLOAT *c);

    static BL_FLOAT ParabolaFuncGen(BL_FLOAT x, BL_FLOAT a, BL_FLOAT b, BL_FLOAT c);

    //
    static void DBG_DumpParabola(int peakBin,
                                 BL_FLOAT alpha, BL_FLOAT beta, BL_FLOAT gamma,
                                 BL_FLOAT c,
                                 const WDL_TypedBuf<BL_FLOAT> &magns);

    static void DBG_DumpParabolaGen(int peakBin,
                                    BL_FLOAT a, BL_FLOAT b, BL_FLOAT c,
                                    const WDL_TypedBuf<BL_FLOAT> &phases);
};

#endif
