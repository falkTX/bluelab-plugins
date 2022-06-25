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
//  BufProcessObjSmooth2.h
//  Spatializer
//
//  Created by Pan on 20/11/17.
//
//

#ifndef __Spatializer__BufProcessObjSmooth2__
#define __Spatializer__BufProcessObjSmooth2__

#include "IPlug_include_in_plug_hdr.h"

#include "../../WDL/fft.h"

//#include "../../WDL/IPlug/Containers.h"

class BufProcessObj2
{
public:
    BufProcessObj2() {}
    
    virtual ~BufProcessObj2() {}
    
    virtual void Reset() = 0;
    
    virtual void Flush() = 0;
    
    virtual void SetParameter(void *param) = 0;
    
    virtual void *GetParameter() = 0;
    
    virtual bool Process(BL_FLOAT *input, BL_FLOAT *output, int nFrames) = 0;
    
    virtual void SetBufferSize(int bufferSize) = 0;
};


// FftProcessObjSmooth5: same as FftConvolverSmooth4, but for FftProcessObj
//
// BufProcessObjSmooth: from FftProcessObjSmooth5
// BufProcessObjSmooth2: from BufProcessObjSmooth2, specially designed for FftConvolver-x
class BufProcessObjSmooth2
{
public:
    BufProcessObjSmooth2(BufProcessObj2 *obj0, BufProcessObj2 *obj1, int bufferSize);
    
    virtual ~BufProcessObjSmooth2();
    
    void Reset();
    
    void Flush();
    
    // Parameter behavior depends on the derived type of FftObj
    void SetParameter(void *param);
    
    // Return true if nFrames were provided
    bool Process(BL_FLOAT *input, BL_FLOAT *output, int nFrames);
    
    void SetBufferSize(int bufferSize);
    
protected:
    // 2 object, one current, and the other for managing
    // when new parameters arrive
    BufProcessObj2 *mBufObjs[2];
    
    // We keep the buf size
    // This is when convolver has a big buffer, to not
    // change the parameter until all the buffer is processed
    int mBufSize;
    
    BL_FLOAT mBufComplete;
    
    // 2 because we have 2 process objects
    WDL_TypedBuf<BL_FLOAT> mResultBuf[2];
    
    // Parameter
    void *mParameter;
    bool mParameterChanged;
    
    // Set after Reset was called.
    // So we know everything went back to zero
    bool mHasJustReset;
    
    // Process
    bool mIsProcessing;
    
    // After a parameter changes, we must wait 1 processing
    // before the buffer can be faded
    int mProcessCount;
};

#endif /* defined(__Spatializer__BufProcessObjSmooth2__) */

