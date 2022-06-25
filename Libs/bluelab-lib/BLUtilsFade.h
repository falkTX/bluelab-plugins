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
 
#ifndef BL_UTILS_FADE_H
#define BL_UTILS_FADE_H

#include "IPlug_include_in_plug_hdr.h"

class BLUtilsFade
{
 public:
    template <typename FLOAT_TYPE>
    static void Fade(const WDL_TypedBuf<FLOAT_TYPE> &buf0,
                     const WDL_TypedBuf<FLOAT_TYPE> &buf1,
                     FLOAT_TYPE *resultBuf, FLOAT_TYPE fadeStart, FLOAT_TYPE fadeEnd);
    
    template <typename FLOAT_TYPE>
    static void Fade(WDL_TypedBuf<FLOAT_TYPE> *buf,
                     FLOAT_TYPE fadeStart, FLOAT_TYPE fadeEnd, bool fadeIn);
    
    template <typename FLOAT_TYPE>
    static void Fade(FLOAT_TYPE *buf, int bufSize,
                     FLOAT_TYPE fadeStart, FLOAT_TYPE fadeEnd, bool fadeIn);
    
    template <typename FLOAT_TYPE>
    static void Fade(const WDL_TypedBuf<FLOAT_TYPE> &buf0,
                     const WDL_TypedBuf<FLOAT_TYPE> &buf1,
                     FLOAT_TYPE *resultBuf,
                     FLOAT_TYPE fadeStart, FLOAT_TYPE fadeEnd,
                     FLOAT_TYPE startT, FLOAT_TYPE endT);
    
    template <typename FLOAT_TYPE>
    static void Fade(WDL_TypedBuf<FLOAT_TYPE> *ioBuf0,
                     const WDL_TypedBuf<FLOAT_TYPE> &buf1,
                     FLOAT_TYPE fadeStart, FLOAT_TYPE fadeEnd,
                     bool fadeIn,
                     FLOAT_TYPE startPos = 0.0, FLOAT_TYPE endPos = 1.0);
    
    //
    template <typename FLOAT_TYPE>
    static void FadeOut(WDL_TypedBuf<FLOAT_TYPE> *ioBuf,
                        int startSampleId, int endSampleId);
    
    // Cross fade the beginning and the end
    template <typename FLOAT_TYPE>
    static void Fade(WDL_TypedBuf<FLOAT_TYPE> *ioBuf0,
                           const WDL_TypedBuf<FLOAT_TYPE> &buf1,
                           FLOAT_TYPE fadeStart, FLOAT_TYPE fadeEnd);
    
    template <typename FLOAT_TYPE>
    static void Fade(FLOAT_TYPE *ioBuf0Data,
                           const FLOAT_TYPE *buf1Data,
                           int bufSize,
                           FLOAT_TYPE fadeStart, FLOAT_TYPE fadeEnd);

    // Fade 2 (works well, and uses sigmoid)
    //
    template <typename FLOAT_TYPE>
    static void Fade2(FLOAT_TYPE *ioBuf0Data, const FLOAT_TYPE *buf1Data, int bufSize,
                      FLOAT_TYPE fadeStartPos, FLOAT_TYPE fadeEndPos,
                      FLOAT_TYPE startT, FLOAT_TYPE endT,
                      FLOAT_TYPE sigmoA = (FLOAT_TYPE)0.5);

    // Helper function
    // Fade the left part
    template <typename FLOAT_TYPE>
    static void Fade2Left(FLOAT_TYPE *ioBuf0Data,
                          const FLOAT_TYPE *buf1Data, int bufSize,
                          FLOAT_TYPE fadeStartPos, FLOAT_TYPE fadeEndPos,
                          FLOAT_TYPE startT, FLOAT_TYPE endT,
                          FLOAT_TYPE sigmoA = (FLOAT_TYPE)0.5);

    // Helper function
    // Fde the right part
    template <typename FLOAT_TYPE>
    static void Fade2Right(FLOAT_TYPE *ioBuf0Data,
                           const FLOAT_TYPE *buf1Data, int bufSize,
                           FLOAT_TYPE fadeStartPos, FLOAT_TYPE fadeEndPos,
                           FLOAT_TYPE startT, FLOAT_TYPE endT,
                           FLOAT_TYPE sigmoA = (FLOAT_TYPE)0.5);

    // Halper function: do Fade2Left() and Fade2Right()
    template <typename FLOAT_TYPE>
    static void Fade2Double(FLOAT_TYPE *ioBuf0Data,
                            const FLOAT_TYPE *buf1Data, int bufSize,
                            FLOAT_TYPE fadeStartPos, FLOAT_TYPE fadeEndPos,
                            FLOAT_TYPE startT, FLOAT_TYPE endT,
                            FLOAT_TYPE sigmaA = (FLOAT_TYPE)0.5);

};

#endif
