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
//  USTCorrelationProcess.h
//  UST
//
//  Created by applematuer on 10/18/19.
//
//

#ifndef __UST__USTCorrelationProcess__
#define __UST__USTCorrelationProcess__

#include "IPlug_include_in_plug_hdr.h"

// Bufferized
class USTCorrelationProcess
{
public:
    USTCorrelationProcess(BL_FLOAT sampleRate);
    
    virtual ~USTCorrelationProcess();
    
    void Reset(BL_FLOAT sampleRate);
    
    void AddSamples(const WDL_TypedBuf<BL_FLOAT> samples[2]);

    bool WasUpdated();
    
    BL_FLOAT GetCorrelation();

protected:
    BL_FLOAT mSampleRate;
    
    WDL_TypedBuf<BL_FLOAT> mSamples[2];
    
    BL_FLOAT mCorrelation;
    
    bool mWasUpdated;
};

#endif /* defined(__UST__USTCorrelationProcess__) */
