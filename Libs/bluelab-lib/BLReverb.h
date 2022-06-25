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
//  BLReverb.h
//  BL-ReverbDepth
//
//  Created by applematuer on 8/31/20.
//
//

#ifndef BL_ReverbDepth_BLReverb_h
#define BL_ReverbDepth_BLReverb_h

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

class BLReverb
{
public:
    virtual ~BLReverb();
    
    virtual BLReverb *Clone() const = 0;
    
    virtual void Reset(BL_FLOAT sampleRate, int blockSize) = 0;
    
    // Mono
    virtual void Process(const WDL_TypedBuf<BL_FLOAT> &input,
                         WDL_TypedBuf<BL_FLOAT> *outputL,
                         WDL_TypedBuf<BL_FLOAT> *outputR) = 0;
    
    // Stereo
    virtual void Process(const WDL_TypedBuf<BL_FLOAT> inputs[2],
                         WDL_TypedBuf<BL_FLOAT> *outputL,
                         WDL_TypedBuf<BL_FLOAT> *outputR) = 0;
    
    //
    void GetIRs(WDL_TypedBuf<BL_FLOAT> irs[2], int size);
};

#endif
