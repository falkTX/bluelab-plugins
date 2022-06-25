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
 
#ifndef BL_UTILS_PHASES_H
#define BL_UTILS_PHASES_H


class BLUtilsPhases
{
 public:
    // fmod() managing correctly negative numbers
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE fmod_negative(FLOAT_TYPE x, FLOAT_TYPE y);

    // Map to [-pi, pi]
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE princarg(FLOAT_TYPE x);
    
    // Phases
    template <typename FLOAT_TYPE>
    static void FindNextPhase(FLOAT_TYPE *phase, FLOAT_TYPE refPhase);
    
    template <typename FLOAT_TYPE>
    static void UnwrapPhases(WDL_TypedBuf<FLOAT_TYPE> *phases,
                             bool adjustFirstPhase = true);
    
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE MapToPi(FLOAT_TYPE val);

    template <typename FLOAT_TYPE>
    static void MapToPi(WDL_TypedBuf<FLOAT_TYPE> *values);

    // New unwrap
    
    // Real unwrap is "closest", not "next"
    // See: https://ccrma.stanford.edu/~jos/fp/Phase_Unwrapping.html
    // this is the correct mehtod
    template <typename FLOAT_TYPE>
    static void FindClosestPhase(FLOAT_TYPE *phase, FLOAT_TYPE refPhase);

    // Use "closest"
    template <typename FLOAT_TYPE>
    static void UnwrapPhases2(WDL_TypedBuf<FLOAT_TYPE> *phases,
                              bool dbgUnwrap180 = false);

    // This is a debug method, tham makes modulus PI and not TWO_PI
    // Useful for debugging
    template <typename FLOAT_TYPE>
    static void FindClosestPhase180(FLOAT_TYPE *phase, FLOAT_TYPE refPhase);
};

#endif
