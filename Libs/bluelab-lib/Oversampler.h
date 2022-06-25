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

#ifndef __Denoiser__Oversampler__
#define __Denoiser__Oversampler__

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"
//#include "../../WDL/IPlug/Containers.h"

#define WDL_BESSEL_FILTER_ORDER 8

// Niko: not sure this one is good
#define WDL_BESSEL_DENORMAL_AGGRESSIVE

#include "../../WDL/besselfilter.h"

class Oversampler
{
public:
    Oversampler(int oversampling);
    virtual ~Oversampler();
    
    const BL_FLOAT *Upsample(BL_FLOAT *inBuf, int size);
    const BL_FLOAT *Downsample(BL_FLOAT *inBuf, int size);
    
    BL_FLOAT *GetOutEmptyBuffer();
    
protected:
    WDL_BesselFilterCoeffs mAntiAlias;
    WDL_BesselFilterStage mUpsample, mDownsample;
    
    int mOversampling;
    
    WDL_TypedBuf<BL_FLOAT> mResultBuf;
    WDL_TypedBuf<BL_FLOAT> mOutEmptyBuf;
};

#endif /* defined(__Denoiser__Oversampler__) */
