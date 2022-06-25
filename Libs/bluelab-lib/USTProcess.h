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
//  USTProcess.h
//  UST
//
//  Created by applematuer on 7/28/19.
//
//

#ifndef __UST__USTProcess__
#define __UST__USTProcess__

#include <vector>
using namespace std;

#include "IPlug_include_in_plug_hdr.h"

#include "../../WDL/fft.h"

//class USTWidthAdjuster4; // old
class USTWidthAdjuster5;
class DelayObj4;
class ParamSmoother;

class USTProcess
{
public:
    enum PolarLevelMode
    {
        MAX,
        AVG
    };
    
    template <typename FLOAT_TYPE>
    static void ComputePolarSamples(const WDL_TypedBuf<FLOAT_TYPE> samples[2],
                                    WDL_TypedBuf<FLOAT_TYPE> polarSamples[2]);
    
    template <typename FLOAT_TYPE>
    static void ComputeLissajous(const WDL_TypedBuf<FLOAT_TYPE> samples[2],
                                 WDL_TypedBuf<FLOAT_TYPE> lissajousSamples[2],
                                 bool fitInSquare);
    
    // Polar level auxiliary
    template <typename FLOAT_TYPE>
    static void ComputePolarLevels(const WDL_TypedBuf<FLOAT_TYPE> samples[2],
                                   int numBins,
                                   WDL_TypedBuf<FLOAT_TYPE> *levels,
                                   enum PolarLevelMode mode);
    
    template <typename FLOAT_TYPE>
    static void SmoothPolarLevels(WDL_TypedBuf<FLOAT_TYPE> *ioLevels,
                                  WDL_TypedBuf<FLOAT_TYPE> *prevLevels,
                                  bool smoothMinMax,
                                  FLOAT_TYPE smoothCoeff);
    
    template <typename FLOAT_TYPE>
    static void ComputePolarLevelPoints(const  WDL_TypedBuf<FLOAT_TYPE> &levels,
                                        WDL_TypedBuf<FLOAT_TYPE> polarLevelSamples[2]);

    //
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE ComputeCorrelation(const WDL_TypedBuf<FLOAT_TYPE> samples[2]);
    
    template <typename FLOAT_TYPE>
    static void ComputeCorrelationMinMax(const WDL_TypedBuf<FLOAT_TYPE> samples[2],
                                         int windowSize,
                                         FLOAT_TYPE *minCorr, FLOAT_TYPE *maxCorr);
    
    // Latest version
    // Replace by USTStereoWidener
#if 0
    template <typename FLOAT_TYPE>
    static void StereoWiden(vector<WDL_TypedBuf<FLOAT_TYPE> * > *ioSamples,
                            FLOAT_TYPE widthFactor);
    
    //static void StereoWiden(vector<WDL_TypedBuf<BL_FLOAT> * > *ioSamples,
    //                        USTWidthAdjuster4 *widthAdjuster);
    
    template <typename FLOAT_TYPE>
    static void StereoWiden(vector<WDL_TypedBuf<FLOAT_TYPE> * > *ioSamples,
                            USTWidthAdjuster5 *widthAdjuster);
    
    template <typename FLOAT_TYPE>
    static void StereoWiden(FLOAT_TYPE *l, FLOAT_TYPE *r, FLOAT_TYPE widthFactor);
#endif
    
    // Correct mehod for balance (no pan law)
    template <typename FLOAT_TYPE>
    static void Balance(vector<WDL_TypedBuf<FLOAT_TYPE> * > *ioSamples,
                        FLOAT_TYPE balance);
    
    template <typename FLOAT_TYPE>
    static void Balance(vector<WDL_TypedBuf<FLOAT_TYPE> * > *ioSamples,
                        ParamSmoother *balanceSmoother);
    
    // Balance with pan low 0dB
    template <typename FLOAT_TYPE>
    static void Balance0(vector<WDL_TypedBuf<FLOAT_TYPE> * > *ioSamples,
                         FLOAT_TYPE balance);
    
    // Balance with pan low -3dB
    template <typename FLOAT_TYPE>
    static void Balance3(vector<WDL_TypedBuf<FLOAT_TYPE> * > *ioSamples,
                         FLOAT_TYPE balance);
    
    // Latest version
    // Replaced by USTPseudoStereoObj
#if 0
    template <typename FLOAT_TYPE>
    static void MonoToStereo(vector<WDL_TypedBuf<FLOAT_TYPE> > *samplesVec,
                             DelayObj4 *delayObj);
#endif
    
    template <typename FLOAT_TYPE>
    static void StereoToMono(vector<WDL_TypedBuf<FLOAT_TYPE> > *samplesVec);
    
    template <typename FLOAT_TYPE>
    static void DecimateSamplesCorrelation(vector<WDL_TypedBuf<FLOAT_TYPE> > *samples,
                                           FLOAT_TYPE decimFactor, FLOAT_TYPE sampleRate);

    // Sound rotation
    template <typename FLOAT_TYPE>
    static WDL_FFT_COMPLEX ComputeComplexRotation(FLOAT_TYPE angle);
    
    template <typename FLOAT_TYPE>
    static void RotateSound(FLOAT_TYPE *left, FLOAT_TYPE *right, WDL_FFT_COMPLEX &rotation);
    
protected:
    // Optim about 10% of UST
#define OPTIM_STEREO_WIDEN 1
    
#if !OPTIM_STEREO_WIDEN
    template <typename FLOAT_TYPE>
    static void StereoWiden(FLOAT_TYPE *left, FLOAT_TYPE *right, FLOAT_TYPE widthFactor);
#else
    template <typename FLOAT_TYPE>
    static WDL_FFT_COMPLEX StereoWidenComputeAngle0();
    
    template <typename FLOAT_TYPE>
    static WDL_FFT_COMPLEX StereoWidenComputeAngle1();
    
    template <typename FLOAT_TYPE>
    static void StereoWiden(FLOAT_TYPE *left, FLOAT_TYPE *right, FLOAT_TYPE widthFactor,
                            const WDL_FFT_COMPLEX &angle0, WDL_FFT_COMPLEX &angle1);
#endif
    
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE ComputeFactor(FLOAT_TYPE normVal, FLOAT_TYPE maxVal);
    
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE ApplyAngleShape(FLOAT_TYPE angle, FLOAT_TYPE shape);
    
    // Helper
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE ComputeCorrelation(const WDL_TypedBuf<FLOAT_TYPE> samples[2],
                                     int index, int num);

};

#endif /* defined(__UST__USTProcess__) */
