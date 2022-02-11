//
//  TransientLib.h
//  Transient
//
//  Created by Apple m'a Tuer on 21/05/17.
//
//

#ifndef __Transient__TransientLib__
#define __Transient__TransientLib__

// See: http://werner.yellowcouch.org/Papers/transients12/index.html
class TransientLib
{
public:
    // Original version
    // Used for BL plugins < 3.0
    static void DetectTransients(const WDL_TypedBuf<WDL_FFT_COMPLEX> *fftBuf,
                                 WDL_TypedBuf<BL_FLOAT> *transients, BL_FLOAT threshold);
    
    // New version
    // Extract the stripped signal (signal minus transients) as well
    static void DetectTransients2(const WDL_TypedBuf<WDL_FFT_COMPLEX> *fftBuf,
                                  BL_FLOAT threshold,
                                  WDL_TypedBuf<BL_FLOAT> *strippedMagns,
                                  WDL_TypedBuf<BL_FLOAT> *transientMagns,
                                  WDL_TypedBuf<BL_FLOAT> *transientIntensityMagns);

};
#endif /* defined(__Transient__TransientLib__) */
