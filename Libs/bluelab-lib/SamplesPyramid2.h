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
//  SamplesPyramid2.h
//  BL-Ghost
//
//  Created by applematuer on 9/30/18.
//
//

#ifndef __BL_Ghost__SamplesPyramid2__
#define __BL_Ghost__SamplesPyramid2__

#include <vector>
using namespace std;

#include <BLTypes.h>

#include "../../WDL/fastqueue.h"
#include "IPlug_include_in_plug_hdr.h"

// SamplesPyramid2: for UST
// Try to avoid glitches

// Test for UST
// => BETTER: gritches less
#define FIX_GLITCH 1

// WORKAROUND: limit the maximum pyramid depth
//
// (Tested on long file, and long capture: perfs still ok)
//
// If the level is too high, the waveform gets messy
// (we don't detect well minima and maxima)
#define DEFAULT_MAX_PYRAMID_LEVEL 8

// Make "MipMaps" with samples
// (to optimize when displaying waveform of long sound)
class SamplesPyramid2
{
public:
    SamplesPyramid2(int maxLevel = DEFAULT_MAX_PYRAMID_LEVEL);
    
    virtual ~SamplesPyramid2();
    
    void Reset();
    
    void SetValues(const WDL_TypedBuf<BL_FLOAT> &samples);
    
    void PushValues(const WDL_TypedBuf<BL_FLOAT> &samples);
    
    void PopValues(long numSamples);
    
    void ReplaceValues(long start, const WDL_TypedBuf<BL_FLOAT> &samples);
    
#if !FIX_GLITCH
    void GetValues(long start, long end, long numValues,
                   WDL_TypedBuf<BL_FLOAT> *samples);
#else
    void GetValues(BL_FLOAT start, BL_FLOAT end, long numValues,
                   WDL_TypedBuf<BL_FLOAT> *samples);
#endif
    
protected:
    void ResetTmpBuffers();
    
    //vector<WDL_TypedBuf<BL_FLOAT> > mSamplesPyramid;
    vector<WDL_TypedFastQueue<BL_FLOAT> > mSamplesPyramid;
    
    // Keep a push buffer, to be able to push
    // by bloocks of power of two (otherwise,
    // there are artefacts on the second half).
    //WDL_TypedBuf<BL_FLOAT> mPushBuf;
    WDL_TypedFastQueue<BL_FLOAT> mPushBuf;
    
    long mRemainToPop;

    int mMaxPyramidLevel;
    
private:
    // Tmp buffers
    vector<WDL_TypedBuf<BL_FLOAT> > mTmpBuf0;
    vector<WDL_TypedBuf<BL_FLOAT> > mTmpBuf1;
    vector<WDL_TypedBuf<BL_FLOAT> > mTmpBuf2;
    vector<WDL_TypedBuf<BL_FLOAT> > mTmpBuf3;
    vector<WDL_TypedBuf<BL_FLOAT> > mTmpBuf4;
    vector<WDL_TypedBuf<BL_FLOAT> > mTmpBuf5;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf6;
    vector<WDL_TypedBuf<BL_FLOAT> > mTmpBuf7;
    vector<WDL_TypedBuf<BL_FLOAT> > mTmpBuf8;
};

#endif /* defined(__BL_Ghost__SamplesPyramid2__) */
