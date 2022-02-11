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
                                  BL_FLOAT threshold, BL_FLOAT mix,
                                  WDL_TypedBuf<BL_FLOAT> *transients);
    
    // Correct implementation (do smoothing for transient caracterization)
    static void DetectTransientsSmooth(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioFftBuf,
                                       BL_FLOAT threshold, BL_FLOAT mix,
                                       BL_FLOAT precision,
                                       WDL_TypedBuf<BL_FLOAT> *outTransients,
                                       WDL_TypedBuf<BL_FLOAT> *outSmoothedTransients);
    
    // Better !
    // Avoids hard coded thresholds
    // and othere things that worked bad
    //
    // smoothedTransients is not in/out, for temporal smooth
    //
    static void DetectTransientsSmooth2(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioFftBuf,
                                        BL_FLOAT threshold, BL_FLOAT mix,
                                        BL_FLOAT precision,
                                        WDL_TypedBuf<BL_FLOAT> *outTransients,
                                        WDL_TypedBuf<BL_FLOAT> *inOutSmoothedTransients);
    
    //
    // The following method computes the transientness in the temporal domain
    //
    
    static void ComputeTransientness(const WDL_TypedBuf<BL_FLOAT> &magns,
                                     const WDL_TypedBuf<BL_FLOAT> &phases,
                                     const WDL_TypedBuf<BL_FLOAT> *prevPhases,
                                     bool freqsToTrans,
                                     bool ampsToTrans,
                                     BL_FLOAT smoothFactor,
                                     WDL_TypedBuf<BL_FLOAT> *transientness);
    
    // modulable freq/trans detection
    // parameter [0 - 1]: freqsAmpsToTransRatio
    // - at 0 we detect only freqs,
    // - at 1 we detect only amps
    static void ComputeTransientness2(const WDL_TypedBuf<BL_FLOAT> &magns,
                                      const WDL_TypedBuf<BL_FLOAT> &phases,
                                      const WDL_TypedBuf<BL_FLOAT> *prevPhases,
                                      BL_FLOAT freqAmpRatio,
                                      BL_FLOAT smoothFactor,
                                      WDL_TypedBuf<BL_FLOAT> *transientness);
    
    // NEW
    //
    // More correct version: can increase only the "s" part
    // (previously, we could increase either both, either more "p" than "s")
    // And the volume is not increased compared to bypass
    //
    // BAD: doesn't work well with 88200Hz sample rate
    // The maximum is shifted when increasing smoothing
    // (and more shifted with 88200Hz ?)
    static void ComputeTransientness3(const WDL_TypedBuf<BL_FLOAT> &magns,
                                      const WDL_TypedBuf<BL_FLOAT> &phases,
                                      const WDL_TypedBuf<BL_FLOAT> *prevPhases,
                                      BL_FLOAT freqAmpRatio,
                                      BL_FLOAT smoothFactor,
                                      WDL_TypedBuf<BL_FLOAT> *transientness);
    
    // Same as above, but fix for 88200Hz
    // (smoothing that does not shift the maximum)
    //
    // SmoothWin is just a pointer, to be kept by the calling code (unused)
    //
    static void ComputeTransientness4(const WDL_TypedBuf<BL_FLOAT> &magns,
                                      const WDL_TypedBuf<BL_FLOAT> &phases,
                                      const WDL_TypedBuf<BL_FLOAT> *prevPhases,
                                      BL_FLOAT freqAmpRatio,
                                      BL_FLOAT smoothFactor,
                                      BL_FLOAT sampleRate,
                                      WDL_TypedBuf<BL_FLOAT> *smoothWin,
                                      WDL_TypedBuf<BL_FLOAT> *transientness);
    
    // Changed computation of weights for "p"
    static void ComputeTransientness5(const WDL_TypedBuf<BL_FLOAT> &magns,
                                      const WDL_TypedBuf<BL_FLOAT> &phases,
                                      const WDL_TypedBuf<BL_FLOAT> *prevPhases,
                                      BL_FLOAT freqAmpRatio,
                                      BL_FLOAT smoothFactor,
                                      BL_FLOAT sampleRate,
                                      WDL_TypedBuf<BL_FLOAT> *smoothWin,
                                      WDL_TypedBuf<BL_FLOAT> *transientness);
    
    // Final version, manages well 88200Hz, with buffer size 88200
    static void ComputeTransientness6(const WDL_TypedBuf<BL_FLOAT> &magns,
                                      const WDL_TypedBuf<BL_FLOAT> &phases,
                                      const WDL_TypedBuf<BL_FLOAT> *prevPhases,
                                      BL_FLOAT freqAmpRatio,
                                      BL_FLOAT smoothFactor,
                                      BL_FLOAT sampleRate,
                                      WDL_TypedBuf<BL_FLOAT> *transientness);
    
    static void ComputeTransientnessMix(WDL_TypedBuf<BL_FLOAT> *magns,
                                        const WDL_TypedBuf<BL_FLOAT> &phases,
                                        const WDL_TypedBuf<BL_FLOAT> *prevPhases,
                                        //BL_FLOAT threshold,
                                        const WDL_TypedBuf<BL_FLOAT> &thresholds,
                                        BL_FLOAT mix,
                                        bool freqsToTrans,
                                        bool ampsToTrans,
                                        BL_FLOAT smoothFactor,
                                        WDL_TypedBuf<BL_FLOAT> *transientness,
                                        const WDL_TypedBuf<BL_FLOAT> *window = NULL);
    
    // modulable freq/trans detection
    // parameter [0 - 1]: freqsAmpsToTransRatio
    // - at 0 we detect only freqs,
    // - at 1 we detect only amps
    static void ComputeTransientnessMix2(WDL_TypedBuf<BL_FLOAT> *magns,
                                         const WDL_TypedBuf<BL_FLOAT> &phases,
                                         const WDL_TypedBuf<BL_FLOAT> *prevPhases,
                                         //BL_FLOAT threshold,
                                         const WDL_TypedBuf<BL_FLOAT> &thresholds,
                                         BL_FLOAT mix,
                                         BL_FLOAT freqAmpRatio,
                                         BL_FLOAT smoothFactor,
                                         WDL_TypedBuf<BL_FLOAT> *transientness,
                                         const WDL_TypedBuf<BL_FLOAT> *window = NULL);
    
protected:
    // Helper method that does the mix
    static void ProcessMix(WDL_TypedBuf<BL_FLOAT> *newMagns,
                           const WDL_TypedBuf<BL_FLOAT> &magns,
                           const WDL_TypedBuf<BL_FLOAT> &transientness,
                           const vector< vector< int > > &transToMagn,
                           const WDL_TypedBuf<BL_FLOAT> &thresholds,
                           BL_FLOAT mix);

    static void TransientnessToFreqDomain(WDL_TypedBuf<BL_FLOAT> *transientness,
                                          vector< vector< int > > *transToMagn,
                                          const WDL_TypedBuf<int> &sampleIds);
    
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
    
    // To get a "smooth" threshold
    static BL_FLOAT GetThresholdedFactor(BL_FLOAT smoothTrans,
                                       BL_FLOAT threshold,
                                       BL_FLOAT mix);
    
    // Use central moving average
    static void SmoothTransients(WDL_TypedBuf<BL_FLOAT> *transients,
                                 BL_FLOAT smoothFactor);

    // Use convolution with a windows to smooth
    // (does not shift the maximum)
    //
    // GOOD: good result, does not shift
    // BAD: very computationally expensive
    static void SmoothTransients2(WDL_TypedBuf<BL_FLOAT> *transients,
                                  BL_FLOAT smoothFactor,
                                  WDL_TypedBuf<BL_FLOAT> *smoothWin);
    
    // Use pyramid smooth
    //static void SmoothTransients3(WDL_TypedBuf<BL_FLOAT> *transients,
    //                              BL_FLOAT smoothFactor);
    
    static void SmoothTransients3(WDL_TypedBuf<BL_FLOAT> *transients, int level);
                                  
    // Use global avg
    // Must be blended by synthesis window
    static void SmoothTransients4(WDL_TypedBuf<BL_FLOAT> *transients);
    
    // Not used (less accurate)
    static void SmoothTransientsAdvanced(WDL_TypedBuf<BL_FLOAT> *transients,
                                         BL_FLOAT smoothFactor);
    
};

#endif /* defined(__Transient__TransientLib__) */
