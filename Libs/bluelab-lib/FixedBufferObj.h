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
//  FixedBuffedObj.h
//  BL-Spatializer
//
//  Created by applematuer on 5/23/19.
//
//

#ifndef __BL_Spatializer__FixedBuffedObj__
#define __BL_Spatializer__FixedBuffedObj__

#include <vector>
using namespace std;

#include "../../WDL/fastqueue.h"

#include "IPlug_include_in_plug_hdr.h"


// Used to force using a fixed buffer size, not nFrames
class FixedBufferObj
{
public:
    FixedBufferObj(int bufferSize);
    
    virtual ~FixedBufferObj();
    
    void Reset();
    
    // NEW
    void Reset(int bufferSize);
    
    void SetInputs(const vector<WDL_TypedBuf<BL_FLOAT> > &buffers);
    bool GetInputs(vector<WDL_TypedBuf<BL_FLOAT> > *buffers);
    
    void ResizeOutputs(vector<WDL_TypedBuf<BL_FLOAT> > *buffers);
                    
    void SetOutputs(const vector<WDL_TypedBuf<BL_FLOAT> > &buffers);
    bool GetOutputs(vector<WDL_TypedBuf<BL_FLOAT> > *buffers, int nFrames);
    
protected:
    int mBufferSize;
    int mCurrentLatency;
    
    //vector<WDL_TypedBuf<BL_FLOAT> > mInputs;
    //vector<WDL_TypedBuf<BL_FLOAT> > mOutputs;
    vector<WDL_TypedFastQueue<BL_FLOAT> > mInputs;
    vector<WDL_TypedFastQueue<BL_FLOAT> > mOutputs;
};

#endif /* defined(__BL_Spatializer__FixedBuffedObj__) */
