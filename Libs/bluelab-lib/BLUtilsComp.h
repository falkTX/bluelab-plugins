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
 
#ifndef BL_UTILS_COMP_H
#define BL_UTILS_COMP_H

#include <cmath>

#include "../../WDL/fft.h"


#define COMP_MAGN(__x__) (std::sqrt((__x__).re * (__x__).re + (__x__).im * (__x__).im))
#define COMP_PHASE(__x__) (std::atan2((__x__).im, (__x__).re))

#define COMP_MAGNF(__x__) (sqrtf((__x__).re * (__x__).re + (__x__).im * (__x__).im))
#define COMP_PHASEF(__x__) (atan2f((__x__).im, (__x__).re))


#define MAGN_PHASE_COMP(__MAGN__, __PHASE__, __COMP__) \
__COMP__.re = __MAGN__*cos(__PHASE__);                 \
__COMP__.im = __MAGN__*sin(__PHASE__);

#define COMP_ADD(__COMP0__, __COMP1__, __RESULT__)     \
__RESULT__.re = __COMP0__.re + __COMP1__.re;           \
__RESULT__.im = __COMP0__.im + __COMP1__.im;

#define COMP_MULT(__COMP0__, __COMP1__, __RESULT__)                      \
__RESULT__.re = __COMP0__.re*__COMP1__.re - __COMP0__.im*__COMP1__.im;  \
__RESULT__.im = __COMP0__.im*__COMP1__.re + __COMP0__.re*__COMP1__.im;

// See: https://courses.lumenlearning.com/ivytech-collegealgebra/chapter/multiply-and-divide-complex-numbers/
#define COMP_DIV(__COMP0__, __COMP1__, __RESULT__)                                  \
{ BL_FLOAT denom = __COMP1__.re*__COMP1__.re + __COMP1__.im*__COMP1__.im;             \
BL_FLOAT denomInv = 1.0/denom;                                                        \
__RESULT__.re = (__COMP0__.re*__COMP1__.re + __COMP0__.im*__COMP1__.im)*denomInv;   \
__RESULT__.im = (__COMP1__.re*__COMP0__.im - __COMP0__.re*__COMP1__.im)*denomInv; }

class BLUtilsComp
{
 public:
    template <typename FLOAT_TYPE>
    static void ComplexToMagn(WDL_TypedBuf<FLOAT_TYPE> *result,
                              const WDL_TypedBuf<WDL_FFT_COMPLEX> &complexBuf);
    
    template <typename FLOAT_TYPE>
    static void ComplexToPhase(WDL_TypedBuf<FLOAT_TYPE> *result,
                               const WDL_TypedBuf<WDL_FFT_COMPLEX> &complexBuf);
    
    template <typename FLOAT_TYPE>
    static void ComplexToMagnPhase(WDL_FFT_COMPLEX comp, FLOAT_TYPE *outMagn, FLOAT_TYPE *outPhase);
    
    template <typename FLOAT_TYPE>
    static void MagnPhaseToComplex(WDL_FFT_COMPLEX *outComp, FLOAT_TYPE magn, FLOAT_TYPE phase);
    
    template <typename FLOAT_TYPE>
    static void ComplexToMagnPhase(WDL_TypedBuf<FLOAT_TYPE> *resultMagn,
                                   WDL_TypedBuf<FLOAT_TYPE> *resultPhase,
                                   const WDL_TypedBuf<WDL_FFT_COMPLEX> &complexBuf);
    
    //static void ComplexToMagnPhase(WDL_TypedBuf<float> *resultMagn,
    //                               WDL_TypedBuf<float> *resultPhase,
    //                               const WDL_TypedBuf<WDL_FFT_COMPLEX> &complexBuf);
    
    template <typename FLOAT_TYPE>
    static void MagnPhaseToComplex(WDL_TypedBuf<WDL_FFT_COMPLEX> *complexBuf,
                                   const WDL_TypedBuf<FLOAT_TYPE> &magns,
                                   const WDL_TypedBuf<FLOAT_TYPE> &phases);
    
    template <typename FLOAT_TYPE>
    static void ComplexToReIm(WDL_TypedBuf<FLOAT_TYPE> *resultRe,
                              WDL_TypedBuf<FLOAT_TYPE> *resultIm,
                              const WDL_TypedBuf<WDL_FFT_COMPLEX> &complexBuf);
    template <typename FLOAT_TYPE>
    static void ReImToComplex(WDL_TypedBuf<WDL_FFT_COMPLEX> *complexBuf,
                              const WDL_TypedBuf<FLOAT_TYPE> &reBuf,
                              const WDL_TypedBuf<FLOAT_TYPE> &imBuf);

    static void ComputeSquareConjugate(WDL_TypedBuf<WDL_FFT_COMPLEX> *buf);

    template <typename FLOAT_TYPE>
    static void ComplexSum(WDL_TypedBuf<FLOAT_TYPE> *ioMagns,
                           WDL_TypedBuf<FLOAT_TYPE> *ioPhases,
                           const WDL_TypedBuf<FLOAT_TYPE> &magns,
                           WDL_TypedBuf<FLOAT_TYPE> &phases);

    static bool IsAllZeroComp(const WDL_TypedBuf<WDL_FFT_COMPLEX> &buffer);
    
    static bool IsAllZeroComp(const WDL_FFT_COMPLEX *buffer, int bufLen);

    template <typename FLOAT_TYPE>
    static void InterpComp(FLOAT_TYPE magn0, FLOAT_TYPE phase0,
                           FLOAT_TYPE magn1, FLOAT_TYPE phase1,
                           FLOAT_TYPE t,
                           FLOAT_TYPE *resMagn, FLOAT_TYPE *resPhase);

    template <typename FLOAT_TYPE>
    static void InterpComp(const WDL_TypedBuf<FLOAT_TYPE> &magns0,
                           const WDL_TypedBuf<FLOAT_TYPE> &phases0,
                           const WDL_TypedBuf<FLOAT_TYPE> &magns1,
                           const WDL_TypedBuf<FLOAT_TYPE> &phases1,
                           FLOAT_TYPE t,
                           WDL_TypedBuf<FLOAT_TYPE> *resMagns,
                           WDL_TypedBuf<FLOAT_TYPE> *resPhases);
};

#endif
