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
//  OversamplingObj.h
//  Saturate
//
//  Created by Apple m'a Tuer on 12/11/17.
//
//

#ifndef __Saturate__OversampProcessObj__
#define __Saturate__OversampProcessObj__

//#include "../../WDL/IPlug/Containers.h"
#include "IPlug_include_in_plug_hdr.h"

class Oversampler3;

class FilterRBJNX;

class OversampProcessObj
{
public:
    OversampProcessObj(int oversampling, BL_FLOAT sampleRate,
                       bool filterNyquist = false);
    
    virtual ~OversampProcessObj();
    
    // Must be called at least once
    void Reset(BL_FLOAT sampleRate);
    
    void Process(BL_FLOAT *input, BL_FLOAT *output, int nFrames);
    
protected:
    virtual void ProcessSamplesBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer) = 0;
    
    Oversampler3 *mUpOversampler;
    Oversampler3 *mDownOversampler;
    
    WDL_TypedBuf<BL_FLOAT> mTmpBuf;
    
    int mOversampling;
    
    FilterRBJNX *mFilter;
};

#endif /* defined(__Saturate__OversampProcessObj__) */
