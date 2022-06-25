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
//  Oversampler.h
//  Denoiser
//
//  Created by Apple m'a Tuer on 26/04/17.
//
//

#ifndef __Denoiser__Oversampler2__
#define __Denoiser__Oversampler3__

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"
//#include "../../WDL/IPlug/Containers.h"
#include "../../WDL/resample.h"

// See IPlugResampler
// Continuous resampler
// Does not need OLA
class Oversampler3
{
public:
    // Up or downsample
    Oversampler3(int oversampling, bool upSample);
    
    virtual ~Oversampler3();
    
    const BL_FLOAT *Resample(BL_FLOAT *inBuf, int size);
    
    void Resample(WDL_TypedBuf<BL_FLOAT> *ioBuf);
    
    int GetOversampling();
    
    BL_FLOAT *GetOutEmptyBuffer();
    
    void Reset(BL_FLOAT sampleRate);
    
protected:
    void Resample(const BL_FLOAT *src, int srcSize, BL_FLOAT *dst, int dstSize);
    
    const BL_FLOAT *Resample2(BL_FLOAT *inBuf, int size);
    
    int mOversampling;
    
    WDL_TypedBuf<BL_FLOAT> mResultBuf;
    WDL_TypedBuf<BL_FLOAT> mOutEmptyBuf;
    
    BL_FLOAT mSampleRate;
    WDL_Resampler mResampler;
    
    // Tell if we upsample or downsample
    bool mUpSample;
};

#endif /* defined(__Denoiser__Oversampler3__) */
