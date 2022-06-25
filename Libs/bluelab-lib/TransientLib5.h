/* Copyright (C) 2022 Nicolas Dittlo <deadlab.plugins@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this software; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
 
//
//  TransientLib.h
//  Transient
//
//  Created by Apple m'a Tuer on 21/05/17.
//
//

#ifndef __Transient__TransientLib5__
#define __Transient__TransientLib5__

#define TRANSIENT_INF_LOG -60

// From Transientlib2
// Cleaned unused code

//
// See: http://werner.yellowcouch.org/Papers/transients12/index.html
//
// TransientLib4
//
// TransientLib5: use object, no static anymore
class CMA2Smoother;
class TransientLib5
{
public:
    TransientLib5();
    virtual ~TransientLib5();
    
    //
    // All the above method compute transients and mix them
    //
    
    // Naive implementation
    void DetectTransients(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioFftBuf,
                          BL_FLOAT threshold, BL_FLOAT mix,
                          WDL_TypedBuf<BL_FLOAT> *transients);
    
    // Correct implementation (do smoothing for transient caracterization)
    void DetectTransientsSmooth(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioFftBuf,
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
    void DetectTransientsSmooth2(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioFftBuf,
                                 BL_FLOAT threshold, BL_FLOAT mix,
                                 BL_FLOAT precision,
                                 WDL_TypedBuf<BL_FLOAT> *outTransients,
                                 WDL_TypedBuf<BL_FLOAT> *inOutSmoothedTransients);
    
    //
    // The following method computes the transientness in the temporal domain
    //
    void ComputeTransientness(const WDL_TypedBuf<BL_FLOAT> &magns,
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
    void ComputeTransientness2(const WDL_TypedBuf<BL_FLOAT> &magns,
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
    void ComputeTransientness3(const WDL_TypedBuf<BL_FLOAT> &magns,
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
    void ComputeTransientness4(const WDL_TypedBuf<BL_FLOAT> &magns,
                               const WDL_TypedBuf<BL_FLOAT> &phases,
                               const WDL_TypedBuf<BL_FLOAT> *prevPhases,
                               BL_FLOAT freqAmpRatio,
                               BL_FLOAT smoothFactor,
                               BL_FLOAT sampleRate,
                               WDL_TypedBuf<BL_FLOAT> *smoothWin,
                               WDL_TypedBuf<BL_FLOAT> *transientness);
    
    // Changed computation of weights for "p"
    void ComputeTransientness5(const WDL_TypedBuf<BL_FLOAT> &magns,
                               const WDL_TypedBuf<BL_FLOAT> &phases,
                               const WDL_TypedBuf<BL_FLOAT> *prevPhases,
                               BL_FLOAT freqAmpRatio,
                               BL_FLOAT smoothFactor,
                               BL_FLOAT sampleRate,
                               WDL_TypedBuf<BL_FLOAT> *smoothWin,
                               WDL_TypedBuf<BL_FLOAT> *transientness);
    
    // Final version, manages well 88200Hz, with buffer size 88200
    void ComputeTransientness6(const WDL_TypedBuf<BL_FLOAT> &magns,
                               const WDL_TypedBuf<BL_FLOAT> &phases,
                               const WDL_TypedBuf<BL_FLOAT> *prevPhases,
                               BL_FLOAT freqAmpRatio,
                               BL_FLOAT smoothFactor,
                               BL_FLOAT sampleRate,
                               WDL_TypedBuf<BL_FLOAT> *transientness);
    
    void ComputeTransientnessMix(WDL_TypedBuf<BL_FLOAT> *magns,
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
    void ComputeTransientnessMix2(WDL_TypedBuf<BL_FLOAT> *magns,
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
    void ProcessMix(WDL_TypedBuf<BL_FLOAT> *newMagns,
                    const WDL_TypedBuf<BL_FLOAT> &magns,
                    const WDL_TypedBuf<BL_FLOAT> &transientness,
                    const vector< vector< int > > &transToMagn,
                    const WDL_TypedBuf<BL_FLOAT> &thresholds,
                    BL_FLOAT mix);

    void TransientnessToFreqDomain(WDL_TypedBuf<BL_FLOAT> *transientness,
                                   vector< vector< int > > *transToMagn,
                                   const WDL_TypedBuf<int> &sampleIds);
    
    void GetSmoothedTransients(const WDL_TypedBuf<BL_FLOAT> &transients,
                               WDL_TypedBuf<BL_FLOAT> *smoothedTransientsRaw,
                               WDL_TypedBuf<BL_FLOAT> *smoothedTransientsFine,
                               int rawSmooth, int fineSmooth);
    
    void GetSmoothedTransientsInterp(const WDL_TypedBuf<BL_FLOAT> &transients,
                                     WDL_TypedBuf<BL_FLOAT> *smoothedTransients,
                                     BL_FLOAT precision);
    
    void NormalizeVolume(const WDL_TypedBuf<BL_FLOAT> &origin,
                         WDL_TypedBuf<BL_FLOAT> *result);

    void NormalizeCurve(WDL_TypedBuf<BL_FLOAT> *ioCurve);
    
    // To get a "smooth" threshold
    BL_FLOAT GetThresholdedFactor(BL_FLOAT smoothTrans,
                                  BL_FLOAT threshold,
                                  BL_FLOAT mix);
    
    // Use central moving average
    void SmoothTransients(WDL_TypedBuf<BL_FLOAT> *transients,
                          BL_FLOAT smoothFactor);

    // Use convolution with a windows to smooth
    // (does not shift the maximum)
    //
    // GOOD: good result, does not shift
    // BAD: very computationally expensive
    void SmoothTransients2(WDL_TypedBuf<BL_FLOAT> *transients,
                           BL_FLOAT smoothFactor,
                           WDL_TypedBuf<BL_FLOAT> *smoothWin);
    
    // Use pyramid smooth
    //static void SmoothTransients3(WDL_TypedBuf<BL_FLOAT> *transients,
    //                              BL_FLOAT smoothFactor);
    
    void SmoothTransients3(WDL_TypedBuf<BL_FLOAT> *transients, int level);
                                  
    // Use global avg
    // Must be blended by synthesis window
    void SmoothTransients4(WDL_TypedBuf<BL_FLOAT> *transients);
    
    // Not used (less accurate)
    void SmoothTransientsAdvanced(WDL_TypedBuf<BL_FLOAT> *transients,
                                  BL_FLOAT smoothFactor);

    //
    CMA2Smoother *mSmoother0;
    CMA2Smoother *mSmoother1;
    
private:
    // Tmp buffers
    WDL_TypedBuf<BL_FLOAT> mTmpBuf0;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf1;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf2;
    vector< vector< int > > mTmpBuf3;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf4;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf5;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf6;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf7;
    vector< vector< int > > mTmpBuf8;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf9;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf10;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf11;
    WDL_TypedBuf<int> mTmpBuf12;
    WDL_TypedBuf<int> mTmpBuf13;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf14;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf15;
    WDL_TypedBuf<int> mTmpBuf16;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf17;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf18;
    WDL_TypedBuf<int> mTmpBuf19;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf20;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf21;
    WDL_TypedBuf<int> mTmpBuf22;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf23;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf24;
    WDL_TypedBuf<int> mTmpBuf25;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf26;
    WDL_TypedBuf<int> mTmpBuf27;
    vector< vector< int > > mTmpBuf28;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf29;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf30;
    WDL_TypedBuf<int> mTmpBuf31;
    vector< vector< int > > mTmpBuf32;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf33;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf34;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf35;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf36;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf37;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf38;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf39;
    WDL_TypedBuf<int> mTmpBuf40;
};

#endif /* defined(__Transient__TransientLib__) */
