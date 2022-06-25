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
//  InstantCompressor.h
//  UST
//
//  Created by applematuer on 2/29/20.
//
//

#ifndef __UST__InstantCompressor__
#define __UST__InstantCompressor__

#include <BLTypes.h>

// See: https://www.musicdsp.org/en/latest/Effects/169-compressor.html

// Apply compression, instantly, on already computed rms avg
class InstantCompressor
{
public:
    InstantCompressor(BL_FLOAT sampleRate);
    
    virtual ~InstantCompressor();
    
    void  Reset(BL_FLOAT sampleRate);
    
    // Set all parameters at a time
    void SetParameters(BL_FLOAT threshold, BL_FLOAT slope,
                       BL_FLOAT  tatt, BL_FLOAT  trel);
    
    // set parameters one by one
    void SetThreshold(BL_FLOAT threshold);
    void SetSlope(BL_FLOAT slope);
    void SetAttack(BL_FLOAT tatt);
    void SetRelease(BL_FLOAT trel);
    
    // Niko
    // Knee in percent
    void SetKnee(BL_FLOAT knee);
    
    void Process(BL_FLOAT *rmsAmp);
    
    BL_FLOAT GetGain();
    
    // Debug
    void DBG_DumpCompressionCurve();
    
protected:
    BL_FLOAT mSampleRate;
    
    // threshold (percents)
    BL_FLOAT mThreshold;

    // slope angle (percents)
    BL_FLOAT mSlope;
    
    // attack time  (ms)
    BL_FLOAT mAttack;
    
    // release time (ms)
    BL_FLOAT mRelease;
    
    // envelope
    BL_FLOAT  mEnv;
    
    // Current compression gain
    BL_FLOAT mGain;
    
    // Niko
    // Knee in percent
    BL_FLOAT mKnee;
    
    // For optimization
    BL_FLOAT mAttack0;
    BL_FLOAT mRelease0;
};

#endif /* defined(__UST__InstantCompressor__) */
