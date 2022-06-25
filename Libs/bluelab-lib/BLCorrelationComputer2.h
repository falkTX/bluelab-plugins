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
//  BLCorrelationComputer2.h
//  UST
//
//  Created by applematuer on 1/2/20.
//
//

#ifndef __UST__BLCorrelationComputer2__
#define __UST__BLCorrelationComputer2__

//#include <deque>
//using namespace std;
#include <bl_queue.h>

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

// // See: https://dsp.stackexchange.com/questions/1671/calculate-phase-angle-between-two-signals-i-e-digital-phase-meter
//
// USTCorrelationComputer2: use atan2, to have correct instant correlation
//

// USTCorrelationComputer3: use real correlation computation
// See: http://www.rs-met.com/documents/tutorials/StereoProcessing.pdf

// USTCorrelationComputer4: same as USTCorrelationComputer3, but optimized
// => gives exactly the smae results as USTCorrelationComputer3 !

#define DEFAULT_SMOOTH_TIME_MS 100.0

class CParamSmooth;
class Bufferizer;
class BLCorrelationComputer2
{
public:
    BLCorrelationComputer2(BL_FLOAT sampleRate,
                           BL_FLOAT smoothTimeMs = DEFAULT_SMOOTH_TIME_MS);
    
    virtual ~BLCorrelationComputer2();
    
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
    
    bl_queue<BL_FLOAT> mXLXR;
    bl_queue<BL_FLOAT> mXL2;
    bl_queue<BL_FLOAT> mXR2;

    // Optimization
    BL_FLOAT mSumXLXR;
    BL_FLOAT mSumXL2;
    BL_FLOAT mSumXR2;

    // Use bufferizers, to feed and compute the object with constant buffer size
    Bufferizer *mBufferizers[2];
    bool mGotFirstBuffer;

private:
    WDL_TypedBuf<BL_FLOAT> mTmpBuf0[2];
};

#endif /* defined(__UST__BLCorrelationComputer2__) */
