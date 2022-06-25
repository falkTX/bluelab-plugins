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
//  Oversampler5.h
//  UST
//
//  Created by applematuer on 8/12/20.
//
//

#ifndef __UST__Oversampler5__
#define __UST__Oversampler5__


// From Oversampler4
// - use r8brain-free-src-version-4.6
// (try to avoid filtering high frequencies)
// - very good quality, but gives a latency of ~3584 samples by default
// (can be reduced to 1775 if we set the band from 2 to 5)
namespace r8b {
class CDSPResampler24;
}

class Oversampler5
{
public:
    // Up or downsample
    Oversampler5(int oversampling, bool upSample);
    
    virtual ~Oversampler5();
    
    void Reset(BL_FLOAT sampleRate, int blockSize);
    
    int GetLatency();
    
    void Resample(WDL_TypedBuf<BL_FLOAT> *ioBuf);
    
    int GetOversampling();
    
    BL_FLOAT *GetOutEmptyBuffer();
    
protected:
    int mOversampling;
    
    WDL_TypedBuf<BL_FLOAT> mResultBuf;
    WDL_TypedBuf<BL_FLOAT> mOutEmptyBuf;
    
    BL_FLOAT mSampleRate;
    
    r8b::CDSPResampler24 *mResampler;
    
    // Tell if we upsample or downsample
    bool mUpSample;
};

#endif /* defined(__UST__Oversampler5__) */
