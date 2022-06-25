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

#ifndef __Spatializer__FftConvolverSmooth4__
#define __Spatializer__FftConvolverSmooth4__

#include "IPlug_include_in_plug_hdr.h"

#include "../../WDL/fft.h"

//#include "../../WDL/IPlug/Containers.h"

class FftConvolver4;

// FftConvolver composite
// Uses two FftConvolvers, and make a fade just after the response
// has been changed
// This avoids clicks in the signal when changin the reseponse continuously and rapidly
//
// FftConvolverSmooth2: from FftConvolverSmooth
// Wait for having processed one buffer before making interpolation
// Because when a new impulse response is set to a convolver, there is a blank at the beginning
// of the first new buffer.
// So we wait, to interpolate without the blank
//
// And skip responses if they arrive too rapidly (not tested)
//
// FftConvolverSmooth3: use FftConvolver4 instead of FftConvolver3
//
// FftConvolverSmooth4: buffersize when nFrame is too small (e.g 512)
//
class FftConvolverSmooth4
{
public:
    FftConvolverSmooth4(int bufferSize, bool normalize = true);
    
    virtual ~FftConvolverSmooth4();
    
    void Reset();
    
    // Set the response to convolve
    void SetResponse(const WDL_TypedBuf<BL_FLOAT> *response);
    
    // Return true if nFrames were provided
    bool Process(BL_FLOAT *input, BL_FLOAT *output, int nFrames);
    
protected:
    bool ComputeNewSamples(int nFrames);
    
    bool GetOutputSamples(BL_FLOAT *output, int nFrames);
    
    bool ProcessFirstBuffer();
    
    bool ProcessMakeFade();
    
    static void MakeFade(const WDL_TypedBuf<BL_FLOAT> &buf0,
                         const WDL_TypedBuf<BL_FLOAT> &buf1,
                         BL_FLOAT *resultBuf);
    
    FftConvolver4 *mConvolvers[2];
    int mCurrentConvolverIndex;
    
    bool mStableState;
    bool mHasJustReset;
    bool mFirstNewBufferProcessed;
    
    WDL_TypedBuf<BL_FLOAT> mNewResponse;
    
    // For buffering
    int mBufSize;
    
    WDL_TypedBuf<BL_FLOAT> mSamplesBuf;
    
    // 2 because we have 2 convolvers
    WDL_TypedBuf<BL_FLOAT> mResultBuf[2];
    
    WDL_TypedBuf<BL_FLOAT> mResultBufFade;
};

#endif /* defined(__Spatializer__FftConvolverSmooth4__) */

