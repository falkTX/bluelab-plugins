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
//  FftConvolverSmooth4.h
//  Spatializer
//
//  Created by Pan on 20/11/17.
//
//

#ifndef __Spatializer__BufProcessObjSmooth__
#define __Spatializer__BufProcessObjSmooth__

#include "IPlug_include_in_plug_hdr.h"

#include "../../WDL/fft.h"

//#include "../../WDL/IPlug/Containers.h"

class BufProcessObj;


// FftProcessObjSmooth5: same as FftConvolverSmooth4, but for FftProcessObj
//
// BufProcessObjSmooth: from FftProcessObjSmooth5
class BufProcessObjSmooth
{
public:
    BufProcessObjSmooth(BufProcessObj *obj0, BufProcessObj *obj1,
                        int bufSize, int oversampling, int freqRes);
    
    virtual ~BufProcessObjSmooth();
    
    BufProcessObj *GetBufObj(int index);
    
    void Reset(int oversampling, int freqRes);
    
    // Parameter behavior depends on the derived type of FftObj
    void SetParameter(void *param);
    
    // Return true if nFrames were provided
    bool Process(BL_FLOAT *input, BL_FLOAT *output, int nFrames);
    
protected:
    static void MakeFade(const WDL_TypedBuf<BL_FLOAT> &buf0,
                         const WDL_TypedBuf<BL_FLOAT> &buf1,
                         BL_FLOAT *resultBuf);
    
    BufProcessObj *mBufObjs[2];
    
    // 2 because we have 2 process objects
    WDL_TypedBuf<BL_FLOAT> mResultBuf[2];
    
    // Parameter
    void *mParameter;
    bool mParameterChanged;
    
    bool mReverseFade;
    
    // Set after Reset was called.
    // So we know everything went back to zero
    bool mHasJustReset;
    
    //
    // Stuff for flushing the buffers before switching
    // back to only one obj processor
    //
    
    int mFlushBufferCount;
    
    // Bufs size and oversampling are used to compute a size
    // to finish to flush the buffers
    // when the parameter doesn't change anymore
    int mBufSize;
    
    int mOversampling;
    
    // Not used
    int mFreqRes;
};

#endif /* defined(__Spatializer__BufProcessObjSmooth__) */

