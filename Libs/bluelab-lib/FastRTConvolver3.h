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
//  FastRTConvolver3.h
//  UST
//
//  Created by applematuer on 12/28/19.
//
//

#ifndef __UST__FastRTConvolver3__
#define __UST__FastRTConvolver3__

#include "../../WDL/fastqueue.h"

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

// From FastRTConvolver
// - manage buffer sizes not power of two
// => makes buffering and a bit latency
//
// FastRTConvolver3: from FastRTConvolver2
// - possibility to use WDL convolver
//

// Use WDL fast convolver instead of ConvolutionManager
//
// NOTE: almost the same performances (seem to be no real gain...)
//
#define USE_WDL_CONVOLVER 1 //0

// Zero latency and fast convolver
class ConvolutionManager;
class WDL_ConvolutionEngine_Div;

class FastRTConvolver3
{
public:
    enum Mode
    {
        NORMAL,
        // Used to get exactly the same delay
        // (for stereo channel for example, if we use convolution
        // on only one channel, the second channel will still be synchronized)
        BYPASS
    };
    
    FastRTConvolver3(int bufferSize, BL_FLOAT sampleRate,
                     const WDL_TypedBuf<BL_FLOAT> &ir,
                     Mode mode = NORMAL);
    
    FastRTConvolver3(const FastRTConvolver3 &other);
    
    virtual ~FastRTConvolver3();
    
    void Reset(BL_FLOAT sampleRate, int blockSize);
    
    void SetBufferSize(int bufferSize);
    
    int GetLatency();
    
    void SetIR(const WDL_TypedBuf<BL_FLOAT> &ir);
    
    void Process(const WDL_TypedBuf<BL_FLOAT> &samples,
                 WDL_TypedBuf<BL_FLOAT> *result);

    // Utils
    static void ResampleImpulse(WDL_TypedBuf<BL_FLOAT> *impulseResponse,
                                BL_FLOAT sampleRate, BL_FLOAT respSampleRate);
    
    // For Spatializer
    static void ResampleImpulse2(WDL_TypedBuf<BL_FLOAT> *impulseResponse,
                                 BL_FLOAT sampleRate, BL_FLOAT respSampleRate,
                                 bool resizeToNextPowerOfTwo = true);
    
    static void ResizeImpulse(WDL_TypedBuf<BL_FLOAT> *impulseResponse);
    
protected:
    void ComputeBufferSize(int blockSize);

    //
    BL_FLOAT mSampleRate;
    
    int mBufferSize;
    WDL_TypedBuf<BL_FLOAT> mCurrentIr;
    
#if !USE_WDL_CONVOLVER
    ConvolutionManager *mConvMan;
#else
    WDL_ConvolutionEngine_Div *mConvEngine;
#endif
    
    // For bufferizing
    //WDL_TypedBuf<BL_FLOAT> mInSamples;
    //WDL_TypedBuf<BL_FLOAT> mOutSamples;
    WDL_TypedFastQueue<BL_FLOAT> mInSamples;
    WDL_TypedFastQueue<BL_FLOAT> mOutSamples;
    
    int mBlockSize;
    
    long mSampleNum;
    
    Mode mMode;

private:
    // Tmp buffers
    WDL_TypedBuf<BL_FLOAT> mTmpBuf0;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf1;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf2;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf3;
};

#endif /* defined(__UST__FastRTConvolver3__) */
