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
 
#ifndef BL_UTILS_FFT_H
#define BL_UTILS_FFT_H

#include "IPlug_include_in_plug_hdr.h"

#include "../../WDL/fft.h"

class BLUtilsFft
{
 public:
    template <typename FLOAT_TYPE>
    static void NormalizeFftValues(WDL_TypedBuf<FLOAT_TYPE> *magns);
    
    // Num bins is the number of bins of the full fft
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE FftBinToFreq(int binNum, int numBins, FLOAT_TYPE sampleRate);
    
    // fixed version for stereo (phase correction)
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE FftBinToFreq2(int binNum, int numBins, FLOAT_TYPE sampleRate);
    
    template <typename FLOAT_TYPE>
    static int FreqToFftBin(FLOAT_TYPE freq, int numBins,
                            FLOAT_TYPE sampleRate, FLOAT_TYPE *t = NULL);
    
    template <typename FLOAT_TYPE>
    static void FftFreqs(WDL_TypedBuf<FLOAT_TYPE> *freqs, int numBins,
                         FLOAT_TYPE sampleRate);
    
    template <typename FLOAT_TYPE>
    static void MinMaxFftBinFreq(FLOAT_TYPE *minFreq, FLOAT_TYPE *maxFreq,
                                 int numBins, FLOAT_TYPE sampleRate);

    //
    static void FillSecondFftHalf(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer);
    
    // Magns only
    template <typename FLOAT_TYPE>
    static void FillSecondFftHalf(WDL_TypedBuf<FLOAT_TYPE> *ioMagns);

    static void FillSecondFftHalf(const WDL_TypedBuf<WDL_FFT_COMPLEX> &inHalfBuf,
                                  WDL_TypedBuf<WDL_FFT_COMPLEX> *outBuf);
    
    // Magns only
    template <typename FLOAT_TYPE>
    static void FillSecondFftHalf(const WDL_TypedBuf<FLOAT_TYPE> &inHalfMagns,
                                  WDL_TypedBuf<FLOAT_TYPE> *outMagns);

    //
    template <typename FLOAT_TYPE>
    static void FftIdsToSamplesIds(const WDL_TypedBuf<FLOAT_TYPE> &phases,
                                   WDL_TypedBuf<int> *samplesIds);
#define USE_SIMD_OPTIM 1
    template <typename FLOAT_TYPE>
    static void FftIdsToSamplesIdsSym(const WDL_TypedBuf<FLOAT_TYPE> &phases,
                                      WDL_TypedBuf<int> *samplesIds);
    
    // More precise.
    // Canbe used to compute accurate time delay
    template <typename FLOAT_TYPE>
    static void FftIdsToSamplesIdsFloat(const WDL_TypedBuf<FLOAT_TYPE> &phases,
                                        WDL_TypedBuf<FLOAT_TYPE> *samplesIds);
    
    // NOT TESTED
    template <typename FLOAT_TYPE>
    static void SamplesIdsToFftIds(const WDL_TypedBuf<FLOAT_TYPE> &phases,
                                   WDL_TypedBuf<int> *fftIds);
};

#endif
