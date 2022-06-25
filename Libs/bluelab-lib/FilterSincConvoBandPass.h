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
//  FilterSincConvoBandPass.h
//  UST
//
//  Created by applematuer on 8/11/20.
//
//

#ifndef __UST__FilterSincConvoBandPass__
#define __UST__FilterSincConvoBandPass__

#define DEFAULT_FILTER_SIZE 64

#include "IPlug_include_in_plug_hdr.h"

// See: https://tomroelandts.com/articles/how-to-create-simple-band-pass-and-band-reject-filters
// and (simulator): https://fiiir.com/
//
class FastRTConvolver3;
class FilterSincConvoBandPass
{
public:
    FilterSincConvoBandPass();
    
    FilterSincConvoBandPass(const FilterSincConvoBandPass &other);
    
    virtual ~FilterSincConvoBandPass();
    
    void Init(BL_FLOAT fl, BL_FLOAT fh,
              BL_FLOAT sampleRate, int filterSize = DEFAULT_FILTER_SIZE);
    
    void Reset(BL_FLOAT sampleRate, int blockSize);
    
    int GetLatency();
    
    void Process(WDL_TypedBuf<BL_FLOAT> *result,
                 const WDL_TypedBuf<BL_FLOAT> &samples);
    
    void Process(WDL_TypedBuf<BL_FLOAT> *ioSamples);
    
protected:
    void ComputeFilter(BL_FLOAT fc, int filterSize,
                       BL_FLOAT sampleRate,
                       WDL_TypedBuf<BL_FLOAT> *filterData,
                       bool highPass);

    
    WDL_TypedBuf<BL_FLOAT> mFilterData;
    
    FastRTConvolver3 *mConvolver;
    
    BL_FLOAT mFL;
    BL_FLOAT mFH;
    
    int mFilterSize;
};

#endif /* defined(__UST__FilterSincConvoBandPass__) */
