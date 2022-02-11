//
//  TransientLib.h
//  Transient
//
//  Created by Apple m'a Tuer on 21/05/17.
//
//

#ifndef __Transient__TransientLib3__
#define __Transient__TransientLib3__

#define TRANSIENT_INF_LOG -60

// From Transientlib2
// Cleaned unused code

//
// See: http://werner.yellowcouch.org/Papers/transients12/index.html
//
class TransientLib3
{
public:
    // Naive implementation
    static void DetectTransients(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioFftBuf,
                                  BL_FLOAT threshold, BL_FLOAT mix,
                                  WDL_TypedBuf<BL_FLOAT> *transients);
    
    // Correct implementation (do smoothing for transient caracterization)
    static void DetectTransientsSmooth(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioFftBuf,
                                       BL_FLOAT threshold, BL_FLOAT mix,
                                       BL_FLOAT precision,
                                       WDL_TypedBuf<BL_FLOAT> *outTransients,
                                       WDL_TypedBuf<BL_FLOAT> *outSmoothedTransients);
    
protected:
    static void GetSmoothedTransients(const WDL_TypedBuf<BL_FLOAT> &transients,
                                      WDL_TypedBuf<BL_FLOAT> *smoothedTransientsRaw,
                                      WDL_TypedBuf<BL_FLOAT> *smoothedTransientsFine,
                                      int rawSmooth, int fineSmooth);
    
    static void GetSmoothedTransientsInterp(const WDL_TypedBuf<BL_FLOAT> &transients,
                                            WDL_TypedBuf<BL_FLOAT> *smoothedTransients,
                                            BL_FLOAT precision);
    
    static void NormalizeVolume(const WDL_TypedBuf<BL_FLOAT> &origin,
                                WDL_TypedBuf<BL_FLOAT> *result);

    static void NormalizeCurve(WDL_TypedBuf<BL_FLOAT> *ioCurve);
};

#endif /* defined(__Transient__TransientLib__) */
