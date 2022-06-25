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
//  EarlyReflect.h
//  UST
//
//  Created by applematuer on 1/16/20.
//
//

#ifndef UST_EarlyReflect_h
#define UST_EarlyReflect_h

#include "IPlug_include_in_plug_hdr.h"

extern "C" {
#include <reverb.h>
}

class EarlyReflect
{
public:
    EarlyReflect(BL_FLOAT sampleRate, BL_FLOAT factor = 1.0, BL_FLOAT width = 0.0);
    
    virtual ~EarlyReflect();
    
    void Reset(BL_FLOAT sampleRate);
    
    void Process(const WDL_TypedBuf<BL_FLOAT> &input,
                 WDL_TypedBuf<BL_FLOAT> *outputL,
                 WDL_TypedBuf<BL_FLOAT> *outputR);
    
protected:
    sf_rv_earlyref_st mRev;
    
    BL_FLOAT mSampleRate;
    BL_FLOAT mFactor;
    BL_FLOAT mWidth;
};

#endif
