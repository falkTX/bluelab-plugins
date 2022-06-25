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
//  FilterIIRLow12dBNX.h
//  UST
//
//  Created by applematuer on 8/10/20.
//
//

#ifndef __UST__FilterIIRLow12dBNX__
#define __UST__FilterIIRLow12dBNX__

#include <vector>
using namespace std;

#include "IPlug_include_in_plug_hdr.h"

class FilterIIRLow12dB;
class FilterIIRLow12dBNX
{
public:
    FilterIIRLow12dBNX(int numFilters);
    
    virtual ~FilterIIRLow12dBNX();
    
    void Init(BL_FLOAT resoFreq, BL_FLOAT sampleRate);
    
    BL_FLOAT Process(BL_FLOAT sample);
    
    void Process(WDL_TypedBuf<BL_FLOAT> *result,
                 const WDL_TypedBuf<BL_FLOAT> &samples);
    
protected:
    vector<FilterIIRLow12dB *> mFilters;
};

#endif /* defined(__UST__FilterIIRLow12dBNX__) */
