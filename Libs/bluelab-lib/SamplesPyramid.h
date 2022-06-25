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
//  SamplesPyramid.h
//  BL-Ghost
//
//  Created by applematuer on 9/30/18.
//
//

#ifndef __BL_Ghost__SamplesPyramid__
#define __BL_Ghost__SamplesPyramid__

#include <vector>
using namespace std;

#include "IPlug_include_in_plug_hdr.h"


// Make "MipMaps" with samples
// (to optimize when displaying waveform of long sound)
class SamplesPyramid
{
public:
    SamplesPyramid();
    
    virtual ~SamplesPyramid();
    
    void Reset();
    
    void SetValues(const WDL_TypedBuf<BL_FLOAT> &samples);
    
    void PushValues(const WDL_TypedBuf<BL_FLOAT> &samples);
    
    void PopValues(long numSamples);
    
    void ReplaceValues(long start, const WDL_TypedBuf<BL_FLOAT> &samples);
    
    void GetValues(long start, long end, long numValues,
                   WDL_TypedBuf<BL_FLOAT> *samples);
    
protected:
    vector<WDL_TypedBuf<BL_FLOAT> > mSamplesPyramid;
    
    // Keep a push buffer, to be able to push
    // by blocks of power of two (otherwise,
    // there are artefacts on the second half).
    WDL_TypedBuf<BL_FLOAT> mPushBuf;
    
    long mRemainToPop;
};

#endif /* defined(__BL_Ghost__SamplesPyramid__) */
