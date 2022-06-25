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
//  USTEarlyReflect.h
//  UST
//
//  Created by applematuer on 12/3/19.
//
//

#ifndef __UST__USTEarlyReflect__
#define __UST__USTEarlyReflect__

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

class revmodel;

class USTEarlyReflect
{
public:
    USTEarlyReflect(BL_FLOAT sampleRate);
    
    virtual ~USTEarlyReflect();
    
    void Reset(BL_FLOAT sampleRate);
    
    void Process(const WDL_TypedBuf<BL_FLOAT> &input,
                 WDL_TypedBuf<BL_FLOAT> *outputL,
                 WDL_TypedBuf<BL_FLOAT> *outputR,
                 BL_FLOAT revGain);
    
protected:
    void InitRevModel();
    
    revmodel *mRevModel;
};

#endif /* defined(__UST__USTEarlyReflect__) */
