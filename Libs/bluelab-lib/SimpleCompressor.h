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
//  SimpleCompressor.h
//  UST
//
//  Created by applematuer on 2/29/20.
//
//

#ifndef __UST__SimpleCompressor__
#define __UST__SimpleCompressor__

#include "IPlug_include_in_plug_hdr.h"

// sndfilter
extern "C" {
#include <compressor.h>
}

#define SIMPLE_COMPRESSOR_CHUNK_SIZE 32

class SimpleCompressor
{
public:
    SimpleCompressor(BL_FLOAT sampleRate);
    
    virtual ~SimpleCompressor();
    
    void  Reset(BL_FLOAT sampleRate);
    
    void SetParameters(BL_FLOAT pregain, BL_FLOAT threshold, BL_FLOAT knee,
                       BL_FLOAT ratio, BL_FLOAT attack, BL_FLOAT release);
    
#if 0
    void Process(BL_FLOAT *l, BL_FLOAT *r);
#endif
    
    // The buffers must be multiples of 32
    void Process(WDL_TypedBuf<BL_FLOAT> *l, WDL_TypedBuf<BL_FLOAT> *r);
    
protected:
    void Init();
    
    BL_FLOAT mSampleRate;
    
    // dB, amount to boost the signal before applying compression [0 to 100]
    BL_FLOAT mPregain;
    
     // dB, level where compression kicks in [-100 to 0]
    BL_FLOAT mThreshold;
    
    // dB, width of the knee [0 to 40]
    BL_FLOAT mKnee;
    
    // unitless, amount to inversely scale the output when applying comp [1 to 20]
    BL_FLOAT mRatio;
    
    // seconds, length of the attack phase [0 to 1]
    BL_FLOAT mAttack;
    
    // seconds, length of the release phase [0 to 1]
    BL_FLOAT mRelease;
    
    //
    sf_compressor_state_st *mState;
};

#endif /* defined(__UST__SimpleCompressor__) */
