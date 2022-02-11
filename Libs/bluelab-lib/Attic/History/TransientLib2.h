//
//  TransientLib.h
//  Transient
//
//  Created by Apple m'a Tuer on 21/05/17.
//
//

#ifndef __Transient__TransientLib2__
#define __Transient__TransientLib2__

// Used for BL plugins >= 3.0
// New version: mix in fft domain

//
// See: http://werner.yellowcouch.org/Papers/transients12/index.html
//
class TransientLib2
{
public:
    // Extract the stripped signal (signal minus transients) as well
    static void DetectTransients(const WDL_TypedBuf<WDL_FFT_COMPLEX> *fftBuf,
                                  BL_FLOAT threshold,
                                  WDL_TypedBuf<BL_FLOAT> *strippedMagns,
                                  WDL_TypedBuf<BL_FLOAT> *transientMagns,
                                  WDL_TypedBuf<BL_FLOAT> *transientIntensityMagns);
    
    // Sort the transient signal, the stripped signal, and the transient intensity
    // Mix it in the fft domain
    static void DetectTransients2(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioFftBuf,
                                  BL_FLOAT threshold,
                                  BL_FLOAT mix,
                                  WDL_TypedBuf<BL_FLOAT> *transientIntensityMagns);
    
    // Same as DetectTransient2, but do not scale (falsely) the resutl by 0.5
    static void DetectTransients4(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioFftBuf,
                                  BL_FLOAT threshold, BL_FLOAT mix,
                                  WDL_TypedBuf<BL_FLOAT> *transIntensityMagns);
    
    static void DetectTransientsSmooth4(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioFftBuf,
                                        BL_FLOAT threshold, BL_FLOAT mix,
                                        BL_FLOAT detectThreshold,
                                        WDL_TypedBuf<BL_FLOAT> *transIntensityMagns);
};
#endif /* defined(__Transient__TransientLib__) */
