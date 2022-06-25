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
//  USTCorrelationComputer3.h
//  UST
//
//  Created by applematuer on 1/2/20.
//
//

#ifndef __UST__USTCorrelationComputer3__
#define __UST__USTCorrelationComputer3__

#include <deque>
using namespace std;

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

// // See: https://dsp.stackexchange.com/questions/1671/calculate-phase-angle-between-two-signals-i-e-digital-phase-meter
//
// USTCorrelationComputer2: use atan2, to have correct instant correlation
//


// USTCorrelationComputer3: use real correlation computation
// See: http://www.rs-met.com/documents/tutorials/StereoProcessing.pdf


#define DEFAULT_SMOOTH_TIME_MS 100.0

class CParamSmooth;

class USTCorrelationComputer3
{
public:
    //
    USTCorrelationComputer3(BL_FLOAT sampleRate,
                            BL_FLOAT smoothTimeMs = DEFAULT_SMOOTH_TIME_MS);
    
    virtual ~USTCorrelationComputer3();
    
    void Reset(BL_FLOAT sampleRate);
    
    void Reset();
    
    void Process(const WDL_TypedBuf<BL_FLOAT> samples[2]);
    
    void Process(BL_FLOAT l, BL_FLOAT r);
    
    BL_FLOAT GetCorrelation();
    
    BL_FLOAT GetSmoothWindowMs();
    
protected:
    BL_FLOAT mSampleRate;
    
    BL_FLOAT mCorrelation;
    
    BL_FLOAT mSmoothTimeMs;
    
    // Histories
    long mHistorySize;
    
    deque<BL_FLOAT> mXLXR;
    deque<BL_FLOAT> mXL2;
    deque<BL_FLOAT> mXR2;
};

#endif /* defined(__UST__USTCorrelationComputer3__) */
