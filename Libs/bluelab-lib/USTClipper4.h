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
//  USTClipper4.h
//  UST
//
//  Created by applematuer on 7/30/19.
//
//

#ifndef __UST__USTClipper4__
#define __UST__USTClipper4__

#ifdef IGRAPHICS_NANOVG

#include <vector>
using namespace std;

#include "IPlug_include_in_plug_hdr.h"

class GraphControl12;
class USTClipperDisplay3;
class USTClipperDisplay4;
class ClipperOverObj4;
class DelayObj4;

// From USTClipper3
// - try to have the same quality as SIR Clipper
class USTClipper4
{
public:
    USTClipper4(BL_FLOAT sampleRate);
    
    virtual ~USTClipper4();
    
    void SetGraph(GraphControl12 *graph);
    
    //void Reset(BL_FLOAT sampleRate);
    
    void Reset(BL_FLOAT sampleRate, int blockSize);
    
    void SetEnabled(bool flag);
    
    void SetClipValue(BL_FLOAT clipValue);
    
    void SetZoom(BL_FLOAT zoom);
    
    int GetLatency();
    
    void Process(vector<WDL_TypedBuf<BL_FLOAT> > *ioSamples);
    
protected:
    friend class ClipperOverObj4;
    static void ComputeClipping(BL_FLOAT inSample, BL_FLOAT *outSample, BL_FLOAT clipValue);
    // Knee ratio is n [0, 1]
    static void ComputeClippingKnee(BL_FLOAT inSample, BL_FLOAT *outSample,
                                    BL_FLOAT clipValue, BL_FLOAT kneeRatio);

    void DBG_TestClippingKnee(BL_FLOAT clipValue);
    
    //
    USTClipperDisplay4 *mClipperDisplay;
    
    BL_FLOAT mClipValue;
    
    ClipperOverObj4 *mClipObjs[2];
    
    bool mIsEnabled;
    BL_FLOAT mSampleRate;
    
    int mCurrentBlockSize;
    
    DelayObj4 *mInputDelay;
};

#endif // IGRAPHICS_NANOVG

#endif /* defined(__UST__USTClipper4__) */
