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
//  BLStereoWidener.h
//  BL
//
//  Created by applematuer on 1/3/20.
//
//

#ifndef __BL__BLStereoWidener__
#define __BL__BLStereoWidener__

#include <vector>
using namespace std;

#include "IPlug_include_in_plug_hdr.h"

#include "../../WDL/fft.h"

// BLStereoWidener : from USTStereoWidener

class BLWidthAdjuster;
class ParamSmoother2;
class BLStereoWidener
{
public:
    BLStereoWidener(BL_FLOAT sampleRate);
    
    virtual ~BLStereoWidener();

    void Reset(BL_FLOAT sampleRate);
        
    void StereoWiden(BL_FLOAT *l, BL_FLOAT *r, BL_FLOAT widthFactor) const;
    
    void StereoWiden(vector<WDL_TypedBuf<BL_FLOAT> * > *ioSamples,
                     BL_FLOAT widthFactor) const;
    
    void StereoWiden(vector<WDL_TypedBuf<BL_FLOAT> * > *ioSamples,
                     BLWidthAdjuster *widthAdjuster) const;

protected:
    BL_FLOAT ComputeFactor(BL_FLOAT normVal, BL_FLOAT maxVal) const;
    
    void StereoWiden(BL_FLOAT *left, BL_FLOAT *right, BL_FLOAT widthFactor,
                     const WDL_FFT_COMPLEX &angle0,
                     const WDL_FFT_COMPLEX &angle1) const;
    
    static WDL_FFT_COMPLEX ComputeAngle0();
    static WDL_FFT_COMPLEX ComputeAngle1();

    //
    WDL_FFT_COMPLEX mAngle0;
    WDL_FFT_COMPLEX mAngle1;

    ParamSmoother2 *mStereoWidthSmoother;
};

#endif /* defined(__BL__BLStereoWidener__) */
