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
//  WavetableSynth.h
//  BL-SASViewer
//
//  Created by applematuer on 3/1/19.
//
//

#ifndef __BL_SASViewer__WavetableSynth__
#define __BL_SASViewer__WavetableSynth__

#include <vector>
using namespace std;

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

class WavetableSynth
{
public:
    WavetableSynth(int bufferSize,
                   int overlapping,
                   BL_FLOAT sampleRate,
                   int precision,
                   BL_FLOAT minFreq);
    
    virtual ~WavetableSynth();
    
    void Reset(BL_FLOAT sampleRate);
    
    // Get a whole buffer
    void GetSamplesNearest(WDL_TypedBuf<BL_FLOAT> *buffer,
                           BL_FLOAT freq,
                           BL_FLOAT amp = 1.0);
    
    // Linear: makes wobble with pure frequencies
    // (which are not aligned to an already synthetized table)
    void GetSamplesLinear(WDL_TypedBuf<BL_FLOAT> *buffer,
                          BL_FLOAT freq,
                          BL_FLOAT amp = 1.0);
    
    // Get one sample
    BL_FLOAT GetSampleNearest(int idx, BL_FLOAT freq, BL_FLOAT amp = 1.0);
    
    // Linear: makes wobble with pure frequencies
    // (which are not aligned to an already synthetized table)
    BL_FLOAT GetSampleLinear(int idx, BL_FLOAT freq, BL_FLOAT amp = 1.0);
    
    void NextBuffer();
    
protected:
    class Table
    {
    public:
        Table() { mCurrentPos = 0.0; mFrequency = 0.0; }
        virtual ~Table() {}
        
        //
        WDL_TypedBuf<BL_FLOAT> mBuffer;
        //long mCurrentPos;
        BL_FLOAT mCurrentPos;
        
        BL_FLOAT mFrequency;
    };
    
    void ComputeTables();
    
    int mBufferSize;
    int mOverlapping;
    BL_FLOAT mSampleRate;
    int mPrecision;
    BL_FLOAT mMinFreq;
    
    vector<Table> mTables;
};

#endif /* defined(__BL_SASViewer__WavetableSynth__) */
