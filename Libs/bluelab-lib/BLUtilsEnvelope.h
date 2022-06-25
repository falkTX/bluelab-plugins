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
 
#ifndef BL_UTILS_ENVELOPE_H
#define BL_UTILS_ENVELOPE_H

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

class CMA2Smoother;
class BLUtilsEnvelope
{
 public:
    // Envelopes
    template <typename FLOAT_TYPE>
    static void ComputeEnvelope(const WDL_TypedBuf<FLOAT_TYPE> &samples,
                                WDL_TypedBuf<FLOAT_TYPE> *envelope,
                                bool extendBoundsValues);
    
    //template <typename FLOAT_TYPE>
    static void ComputeEnvelopeSmooth(CMA2Smoother *smoother,
                                      const WDL_TypedBuf<BL_FLOAT> &samples,
                                      WDL_TypedBuf<BL_FLOAT> *envelope,
                                      BL_FLOAT smoothCoeff,
                                      bool extendBoundsValues);
    
    //template <typename FLOAT_TYPE>
    static void ComputeEnvelopeSmooth2(CMA2Smoother *smoother,
                                       const WDL_TypedBuf<BL_FLOAT> &samples,
                                       WDL_TypedBuf<BL_FLOAT> *envelope,
                                       BL_FLOAT smoothCoeff);
    
    template <typename FLOAT_TYPE>
    static void ZeroBoundEnvelope(WDL_TypedBuf<FLOAT_TYPE> *envelope);
    
    // Correct the samples from a source envelop to fit into a distination envelope
    template <typename FLOAT_TYPE>
    static void CorrectEnvelope(WDL_TypedBuf<FLOAT_TYPE> *samples,
                                const WDL_TypedBuf<FLOAT_TYPE> &envelope0,
                                const WDL_TypedBuf<FLOAT_TYPE> &envelope1);
    
    // Compute the shift in sample necessary to fit the two envelopes
    // (base on max of each envelope).
    //
    // precision is used to shift by larger steps only
    // (in case of origin shifts multiple of BUFFER_SIZE)
    template <typename FLOAT_TYPE>
    static int GetEnvelopeShift(const WDL_TypedBuf<FLOAT_TYPE> &envelope0,
                                const WDL_TypedBuf<FLOAT_TYPE> &envelope1,
                                int precision = 1);

    template <typename FLOAT_TYPE>
    static void ShiftSamples(const WDL_TypedBuf<FLOAT_TYPE> *ioSamples,
                             int shiftSize);
};

#endif
