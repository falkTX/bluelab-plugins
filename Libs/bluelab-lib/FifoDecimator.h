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
//  FifoDecimator.h
//  BL-TransientShaper
//
//  Created by Pan on 11/04/18.
//
//

#ifndef __BL_TransientShaper__FifoDecimator__
#define __BL_TransientShaper__FifoDecimator__

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

class FifoDecimator
{
public:
    // If we want to process samples (waveform, set isSamples to true)
    FifoDecimator(long maxSize, BL_FLOAT decimFactor, bool isSamples);
    
    FifoDecimator(bool isSamples);
    
    virtual ~FifoDecimator();
    
    void Reset();
    
    void SetParams(long maxSize, BL_FLOAT decimFactor);
    
    void SetParams(long maxSize, BL_FLOAT decimFactor, bool isSamples);
    
    void AddValues(const WDL_TypedBuf<BL_FLOAT> &samples);
    
    void GetValues(WDL_TypedBuf<BL_FLOAT> *values);
    
protected:
    long mMaxSize;
    BL_FLOAT mDecimFactor;
    bool mIsSamples;
    
    WDL_TypedBuf<BL_FLOAT> mValues;
};

#endif /* defined(__BL_TransientShaper__FifoDecimator__) */
