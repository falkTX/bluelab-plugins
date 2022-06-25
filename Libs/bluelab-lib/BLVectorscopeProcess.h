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
//  BLVectroscopeProcess.h
//  UST
//
//  Created by applematuer on 7/28/19.
//
//

#ifndef __UST__BLVectroscopeProcess__
#define __UST__BLVectroscopeProcess__

#include <vector>
using namespace std;

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

#include "../../WDL/fft.h"

class USTWidthAdjuster5;

// BLVectorscopeUSTProcess: from BLVectroscopeProcess
//
class BLVectorscopeProcess
{
public:
    enum PolarLevelMode
    {
        MAX,
        AVG
    };
    
    static void ComputePolarSamples(const WDL_TypedBuf<BL_FLOAT> samples[2],
                                    WDL_TypedBuf<BL_FLOAT> polarSamples[2]);
    
    static void ComputeLissajous(const WDL_TypedBuf<BL_FLOAT> samples[2],
                                 WDL_TypedBuf<BL_FLOAT> lissajousSamples[2],
                                 bool fitInSquare);
    
    // Polar level auxiliary
    static void ComputePolarLevels(const WDL_TypedBuf<BL_FLOAT> samples[2],
                                   int numBins,
                                   WDL_TypedBuf<BL_FLOAT> *levels,
                                   enum PolarLevelMode mode);
    
    static void SmoothPolarLevels(WDL_TypedBuf<BL_FLOAT> *ioLevels,
                                  WDL_TypedBuf<BL_FLOAT> *prevLevels,
                                  bool smoothMinMax,
                                  BL_FLOAT smoothCoeff);
    
    static void ComputePolarLevelPoints(const  WDL_TypedBuf<BL_FLOAT> &levels,
                                        WDL_TypedBuf<BL_FLOAT> polarLevelSamples[2]);
};

#endif /* defined(__UST__BLVectroscopeProcess__) */
