//
//  TransientLib.h
//  Transient
//
//  Created by Apple m'a Tuer on 21/05/17.
//
//

#ifndef __Transient__TransientLib4__
#define __Transient__TransientLib4__

#define TRANSIENT_INF_LOG -60

// From Transientlib2
// Cleaned unused code

//
// See: http://werner.yellowcouch.org/Papers/transients12/index.html
//
// TransientLib4
// Tests
class TransientLib4
{
public:
    //
    // All the above method compute transients and mix them
    //
    
    // Naive implementation
    static void DetectTransients(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioFftBuf,
                                  double threshold, double mix,
                                  WDL_TypedBuf<double> *transients);
    
    // Correct implementation (do smoothing for transient caracterization)
    static void DetectTransientsSmooth(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioFftBuf,
                                       double threshold, double mix,
                                       double precision,
                                       WDL_TypedBuf<double> *outTransients,
                                       WDL_TypedBuf<double> *outSmoothedTransients);
    
    // Better !
    // Avoids hard coded thresholds
    // and othere things that worked bad
    //
    // smoothedTransients is not in/out, for temporal smooth
    //
    static void DetectTransientsSmooth2(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioFftBuf,
                                        double threshold, double mix,
                                        double precision,
                                        WDL_TypedBuf<double> *outTransients,
                                        WDL_TypedBuf<double> *inOutSmoothedTransients);
    
    //
    // The following method computes the transientness in the temporal domain
    //
    
    static void ComputeTransientness(const WDL_TypedBuf<double> &magns,
                                     const WDL_TypedBuf<double> &phases,
                                     const WDL_TypedBuf<double> *prevPhases,
                                     bool freqsToTrans,
                                     bool ampsToTrans,
                                     double smoothFactor,
                                     WDL_TypedBuf<double> *transientness);
    
    static void ComputeTransientnessMix(WDL_TypedBuf<double> *magns,
                                        const WDL_TypedBuf<double> &phases,
                                        const WDL_TypedBuf<double> *prevPhases,
                                        //double threshold,
                                        const WDL_TypedBuf<double> &thresholds,
                                        double mix,
                                        bool freqsToTrans,
                                        bool ampsToTrans,
                                        double smoothFactor,
                                        WDL_TypedBuf<double> *transientness);
    
protected:
    static void GetSmoothedTransients(const WDL_TypedBuf<double> &transients,
                                      WDL_TypedBuf<double> *smoothedTransientsRaw,
                                      WDL_TypedBuf<double> *smoothedTransientsFine,
                                      int rawSmooth, int fineSmooth);
    
    static void GetSmoothedTransientsInterp(const WDL_TypedBuf<double> &transients,
                                            WDL_TypedBuf<double> *smoothedTransients,
                                            double precision);
    
    static void NormalizeVolume(const WDL_TypedBuf<double> &origin,
                                WDL_TypedBuf<double> *result);

    static void NormalizeCurve(WDL_TypedBuf<double> *ioCurve);
    
    // To get a "smooth" threshold
    static double GetThresholdedFactor(double smoothTrans,
                                       double threshold,
                                       double mix);
};

#endif /* defined(__Transient__TransientLib__) */
