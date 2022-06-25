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
#define __Denoiser__Oversampler2__

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"
//#include "../../WDL/IPlug/Containers.h"
#include "../../WDL/resample.h"

class Oversampler2
{
public:
    Oversampler2(int oversampling);
    virtual ~Oversampler2();
    
    const BL_FLOAT *Upsample(BL_FLOAT *inBuf, int size);
    const BL_FLOAT *Downsample(BL_FLOAT *inBuf, int size);
    
    int GetOversampling();
    
    BL_FLOAT *GetOutEmptyBuffer();
    
    void Reset(BL_FLOAT sampleRate);
    
protected:
    void Resample(const BL_FLOAT *src, int srcSize, BL_FLOAT srcRate,
                  BL_FLOAT *dst, int dstSize, BL_FLOAT dstRate);
    
    int mOversampling;
    
    WDL_TypedBuf<BL_FLOAT> mResultBuf;
    WDL_TypedBuf<BL_FLOAT> mOutEmptyBuf;
    
    BL_FLOAT mSampleRate;
    WDL_Resampler mResampler;
};

#endif /* defined(__Denoiser__Oversampler2__) */
