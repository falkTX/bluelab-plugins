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
//  FftProcessObjSmooth5.cpp
//  Spatializer
//
//  Created by Pan on 19/12/17.
//
//

#include "BufProcessObj.h"

#include <BLUtils.h>
#include <BLUtilsFade.h>

#include "BufProcessObjSmooth2.h"

#define USE_SWAP_HACK 1

#define DEBUG_DUMP 0

#define DEBUG_GRAPH 0

#if DEBUG_GRAPH
#include <DebugGraph.h>
#endif

// These are the best fade parameters
// With other parameters, either there are some lighter bars
// in the spectrogram (clics)
// or there are some small dark bars (no audible, but not perfect)

#if 0 // BAD: makes rumble
// ORIG (until version 5.0.5)
#define FADE_START 0.25
#define FADE_END 0.5
#endif

#if 1 // BETTER
// NEW: seems better, avoids "smooth crackles"
// Seems to blend better the different result
// (almost no vertical bar in the spectrogram)
#define FADE_START 0.0
#define FADE_END 1.0
#endif

// FIX: when playing after a reset, the first move of the knob makes a click
#define JUST_RESET_HACK 1


#if DEBUG_GRAPH
#define NUM_CURVES 8
#define NUM_POINTS 1
static void
TraceGraph(int objectNum, int curveNum)
{
    for (int j = 0; j < NUM_CURVES; j++)
    {
        int color[3];
        color[0] = (j*1236895 + 473433) % 255;
        color[1] = (j*196895 + 46888867786) % 255;
        color[2] = (j*12356765 + 57745645645) % 255;

        BL_FLOAT val = ((BL_FLOAT)j)/NUM_CURVES;
        val += objectNum/(NUM_CURVES*2.0);
        
        if (j != curveNum)
            val -= objectNum/(NUM_CURVES*2.0);
        
        for (int i = 0; i < NUM_POINTS; i++)
        {
            DebugGraph::PushCurveValue(val,
                                       j, 
                                       -1.2, 1.2,
                                       2.0,
                                       color[0], color[1], color[2]);
        }
    }
}
#endif

BufProcessObjSmooth2::BufProcessObjSmooth2(BufProcessObj2 *obj0, BufProcessObj2 *obj1,
                                           int bufferSize)
{
    mBufObjs[0] = obj0;
    mBufObjs[1] = obj1;
    
    mBufSize = bufferSize;
    
    mParameter = NULL;
    
    Reset();
}

BufProcessObjSmooth2::~BufProcessObjSmooth2()
{
    delete mBufObjs[0];
    delete mBufObjs[1];
}

void
BufProcessObjSmooth2::Reset()
{
    mBufObjs[0]->Reset();
    mBufObjs[1]->Reset();
    
    mHasJustReset = true;
    
    mParameterChanged = false;
    
    mIsProcessing = false;
    mProcessCount = 0;
    
    mBufComplete = -1.0;
}

// Not well tested
void
BufProcessObjSmooth2::Flush()
{
    mBufObjs[0]->Flush();
    mBufObjs[1]->Flush();
 
#if 1
    mHasJustReset = true;
    
    mParameterChanged = false;
    
    mIsProcessing = false;
    mProcessCount = 0;
    
    mBufComplete = -1.0;
#endif
}

void
BufProcessObjSmooth2::SetParameter(void *param)
{
    // Just register the new parameter, it will be set in Process()
    mParameter = param;
    
#if !JUST_RESET_HACK
    if (mHasJustReset)
#else
    if (false)
#endif
    {
        mBufObjs[0]->SetParameter(mParameter);
        mBufObjs[1]->SetParameter(mParameter);
        
        mHasJustReset = false;
    }
    else
        mParameterChanged = true;
}

#if DEBUG_DUMP
#define DEBUG_VALUE_BUF 1.5
#define DEBUG_VALUE_PARAM0 2.0
#define DEBUG_VALUE_PARAM1 2.5
#endif

bool
BufProcessObjSmooth2::Process(BL_FLOAT *input, BL_FLOAT *output, int nFrames)
{
    // Must wait, make the fade in one direction
    // wait again, then make the fade in the other direction
    //
    // (buf0, param0) => (buf1, param1) => (buf0, param1)
    //
    
    if (mParameterChanged && (mProcessCount == 0))
        // Ready to start processing
    {
        mIsProcessing = true;
        mParameterChanged = false;
    }
    
    if (!mIsProcessing)
    {
        bool res = mBufObjs[0]->Process(input, output, nFrames);
    
#if DEBUG_DUMP
        WDL_TypedBuf<BL_FLOAT> zeros;
        zeros.Resize(nFrames);
        BLUtils::FillAllZero(&zeros);
        
        BLDebug::AppendData("buf-old.txt", zeros.Get(), zeros.GetSize());
        BLDebug::AppendValue("buf-old.txt", DEBUG_VALUE_BUF);
        
        BLDebug::AppendData("buf-new.txt", zeros.Get(), zeros.GetSize());
        BLDebug::AppendValue("buf-new.txt", DEBUG_VALUE_BUF);
        
        BLDebug::AppendData("res.txt", output, nFrames);
        BLDebug::AppendValue("res.txt", DEBUG_VALUE_BUF);
        
        BLDebug::AppendData("t.txt", zeros.Get(), zeros.GetSize());
        BLDebug::AppendValue("t.txt", DEBUG_VALUE_BUF);
#endif
        
        return res;
    }
    
    //
    // Processing
    //
    
    // Set the parameter
    if ((mProcessCount == 0) && (mBufComplete < 0.0))
        // First parameter step
    {
        mBufObjs[1]->SetParameter(mParameter);
        
        mBufComplete = 0.0;
        
#if DEBUG_DUMP
        BLDebug::AppendValue("buf-old.txt", DEBUG_VALUE_PARAM0);
        BLDebug::AppendValue("buf-new.txt", DEBUG_VALUE_PARAM0);
        BLDebug::AppendValue("res.txt", DEBUG_VALUE_PARAM0);
        BLDebug::AppendValue("t.txt", DEBUG_VALUE_PARAM0);
#endif
    }
    
    if ((mProcessCount == 2) && (mBufComplete < 0.0))
        // Second parameter step
    {
        // Set the parameter with a process shift of 1
        void *param = mBufObjs[1]->GetParameter();
        mBufObjs[0]->SetParameter(param);
        
        mBufComplete = 0.0;
        
#if DEBUG_DUMP
        BLDebug::AppendValue("buf-old.txt", DEBUG_VALUE_PARAM1);
        BLDebug::AppendValue("buf-new.txt", DEBUG_VALUE_PARAM1);
        BLDebug::AppendValue("res.txt", DEBUG_VALUE_PARAM1);
        BLDebug::AppendValue("t.txt", DEBUG_VALUE_PARAM1);
#endif
    }
    
    // Compute the two buffers
    mResultBuf[0].Resize(nFrames);
    mResultBuf[1].Resize(nFrames);
    
    bool res0 = mBufObjs[0]->Process(input, mResultBuf[0].Get(), nFrames);
    bool res1 = mBufObjs[1]->Process(input, mResultBuf[1].Get(), nFrames);
    
    bool res = (res0 && res1);
    if (!res)
        return false;
    
#if DEBUG_DUMP
    WDL_TypedBuf<BL_FLOAT> zeros;
    zeros.Resize(nFrames);
    BLUtils::FillAllZero(&zeros);
    
    WDL_TypedBuf<BL_FLOAT> ones;
    ones.Resize(nFrames);
    BLUtils::FillAllValue(&ones, 1.0);
    
    WDL_TypedBuf<BL_FLOAT> t;
    t.Resize(nFrames);
#endif

    if (mProcessCount == 0)
        // Wait one buffer after having changed the first parameter
    {
        memcpy(output, mResultBuf[0].Get(), nFrames*sizeof(BL_FLOAT));
        
#if DEBUG_DUMP
        t = zeros;
#endif
    }
    else if (mProcessCount == 1)
    {
        // Fade from mBufObjs[0], parameter 0
        // to mBufObjs[1], parameter1
        BL_FLOAT t0 = FADE_START;
        BL_FLOAT t1 = FADE_END;
        
        BL_FLOAT v0 = 0.0;
        BL_FLOAT v1 = 1.0;
        
        BLUtilsFade::Fade(mResultBuf[0], mResultBuf[1], output, t0, t1, v0, v1);

        mBufComplete = -1.0;
        
#if DEBUG_DUMP
        BLUtils::Fade(zeros, ones, t.Get(), t0, t1, v0, v1);
#endif
        
#if USE_SWAP_HACK
        // Here, mBufObjs[1] is ready.
        // So we can switch and use it as the current one
        // Instead of waiting and fading to mBufObjs[0]
        
        BufProcessObj2 *tmp = mBufObjs[0];
        mBufObjs[0] = mBufObjs[1];
        mBufObjs[1] = tmp;
        
        // And we have finished !
        mIsProcessing = false;
        mProcessCount = 0;
        
        return res;
#endif
        
    }
    else if (mProcessCount == 2)
    // Wait one buffer after having changed the second parameter
    {
        memcpy(output, mResultBuf[1].Get(), nFrames*sizeof(BL_FLOAT));
        
#if DEBUG_DUMP
        t = ones;
#endif
    }
    else if (mProcessCount == 3)
    {
        // Fade from mBufObjs[1], parameter1
        // to mBufObjs[0], parameter 1
        BL_FLOAT t0 = FADE_START;
        BL_FLOAT t1 = FADE_END;
        
        BL_FLOAT v0 = 1.0;
        BL_FLOAT v1 = 0.0;
        
        BLUtilsFade::Fade(mResultBuf[1], mResultBuf[0], output, t0, t1, v0, v1);
        
#if DEBUG_DUMP
        BLUtilsFade::Fade(zeros, ones, t.Get(), t0, t1, v0, v1);
#endif
        
        mBufComplete = -1.0;
    }
    
    
#if DEBUG_DUMP
    BLDebug::AppendData("buf-old.txt", mResultBuf[1].Get(), mResultBuf[1].GetSize());
    BLDebug::AppendValue("buf-old.txt", DEBUG_VALUE_BUF);
    
    BLDebug::AppendData("buf-new.txt", mResultBuf[0].Get(), mResultBuf[0].GetSize());
    BLDebug::AppendValue("buf-new.txt", DEBUG_VALUE_BUF);
    
    BLDebug::AppendData("res.txt", output, nFrames);
    BLDebug::AppendValue("res.txt", DEBUG_VALUE_BUF);
    
    BLDebug::AppendData("t.txt", t.Get(), t.GetSize());
    BLDebug::AppendValue("t.txt", DEBUG_VALUE_BUF);
#endif
    
    // Mechanisme used to wait more time after having changed a parameter,
    // if the convolver buffer size is greater than nFrames
    if (mBufComplete >= 0.0)
    {
        BL_FLOAT step = ((BL_FLOAT)nFrames)/mBufSize;
        
        // Hack
        // Tested with nFrames = 1024 and BUFFER_SIZE = 2048
        // This works, no clic (but makes some latency)
        if (step < 1.0)
            step *= 0.5;
        
        mBufComplete += step;
    }
    
    if ((mBufComplete < 0.0) || (mBufComplete >= 1.0))
        mProcessCount++;
    
    if (mProcessCount > 3)
        // We have finished
    {
        mIsProcessing = false;
        mProcessCount = 0;
    }
    
    return res;
}

void
BufProcessObjSmooth2::SetBufferSize(int bufferSize)
{
    mBufSize = bufferSize;
    
    // NEW
    mBufObjs[0]->SetBufferSize(bufferSize);
    mBufObjs[1]->SetBufferSize(bufferSize);
    
    Reset();
}
