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
//  FilterFreqResp.h
//  UST
//
//  Created by applematuer on 7/29/19.
//
//

#ifndef __UST__FilterFreqResp__
#define __UST__FilterFreqResp__

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

class FilterLR4Crossover;

class FilterFreqResp
{
public:
    FilterFreqResp();
    
    virtual ~FilterFreqResp();
    
    // For other filters, implement other methods
    void GetFreqResp(WDL_TypedBuf<BL_FLOAT> *freqRespLo,
                     WDL_TypedBuf<BL_FLOAT> *freqRespHi,
                     int numSamples,
                     FilterLR4Crossover *filter);
    
    void GenImpulse(WDL_TypedBuf<BL_FLOAT> *impulse);
    
    void GetFreqResp(const WDL_TypedBuf<BL_FLOAT> &filteredImpulse,
                     WDL_TypedBuf<BL_FLOAT> *freqResp);
    
    // For float GUI
#if (BL_GUI_TYPE_FLOAT!=BL_TYPE_FLOAT)
    void GetFreqResp(const WDL_TypedBuf<BL_FLOAT> &filteredImpulse,
                     WDL_TypedBuf<BL_GUI_FLOAT> *freqResp);
#endif
};

#endif /* defined(__UST__FilterFreqResp__) */
