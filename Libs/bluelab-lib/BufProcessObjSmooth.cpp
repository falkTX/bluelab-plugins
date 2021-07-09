//
//  FftProcessObjSmooth5.cpp
//  Spatializer
//
//  Created by Pan on 19/12/17.
//
//

#include "BufProcessObj.h"
#include <BLUtils.h>

#include "BufProcessObjSmooth.h"

#define DEBUG_GRAPH 0

#if DEBUG_GRAPH
#include <DebugGraph.h>
#endif

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

BufProcessObjSmooth::BufProcessObjSmooth(BufProcessObj *obj0, BufProcessObj *obj1,
                                         int bufSize, int oversampling, int freqRes)
{
    mBufObjs[0] = obj0;
    mBufObjs[1] = obj1;
    
    mBufSize = bufSize;
    
    mParameter = NULL;

    mFlushBufferCount = 0;
    
    Reset(oversampling, freqRes);
}

BufProcessObjSmooth::~BufProcessObjSmooth()
{
    delete mBufObjs[0];
    delete mBufObjs[1];
}

BufProcessObj *
BufProcessObjSmooth::GetBufObj(int index)
{
    if (index > 1)
        return NULL;
    
    return mBufObjs[index];
}

void
BufProcessObjSmooth::Reset(int oversampling, int freqRes)
{
    if (oversampling > 0)
        mOversampling = oversampling;
    
    if (freqRes > 0)
        mFreqRes = freqRes;
    
    mBufObjs[0]->Reset(oversampling, freqRes);
    mBufObjs[1]->Reset(oversampling, freqRes);
    
    // Parameter
    mParameterChanged = true;
    mReverseFade = false;
    
    mHasJustReset = true;
}

void
BufProcessObjSmooth::SetParameter(void *param)
{
    // Just register the new parameter, it will be set in Process()
    mParameter = param;
    
    if (mHasJustReset)
    {
        mBufObjs[0]->SetParameter(mParameter);
        mBufObjs[1]->SetParameter(mParameter);
        
        mHasJustReset = false;
    }
    else
    {
        mParameterChanged = true;
        
        mFlushBufferCount = 0;
    }
}

bool
BufProcessObjSmooth::Process(BL_FLOAT *input, BL_FLOAT *output, int nFrames)
{
    // With that activated, the buffer1 makes a click when it starts
#if 0
    // Necessary number of iterations, to be sure to make a good last fade
    // and switch to a single process obj
    int maxFlushIterations = (int)((((BL_FLOAT)mBufSize)/nFrames)*mOversampling);
    
    if (!mParameterChanged && // Param is stable
        (mFlushBufferCount++ >= maxFlushIterations) && // We have flushed the buffers
        !mReverseFade) // and we have made the last fade in the right order
    {
        bool res = mBufObjs[0]->Process(input, output, nFrames);
        
        return res;
    }
#endif
    
    // Parameter changed

    // Set the parameter
    mBufObjs[1]->SetParameter(mParameter);
    
    // Compute the two buffers
    mResultBuf[0].Resize(nFrames);
    mResultBuf[1].Resize(nFrames);
    
    bool res0 = mBufObjs[0]->Process(input, mResultBuf[0].Get(), nFrames);
    bool res1 = mBufObjs[1]->Process(input, mResultBuf[1].Get(), nFrames);
    
    bool res = (res0 && res1);
    if (!res)
        return false;
    
#if 1
    // Make fade
    if (!mReverseFade)
        MakeFade(mResultBuf[0], mResultBuf[1], output);
    else
        MakeFade(mResultBuf[1], mResultBuf[0], output);
#endif
    
#if 0 // Debug
    BLUtils::Mix(output, mResultBuf[0].Get(), mResultBuf[1].Get(), nFrames, 0.5);
#endif
    
    mReverseFade = !mReverseFade;
    
    mBufObjs[0]->SetParameter(mParameter);
    mParameterChanged = false;
    
    return res;
}

void
BufProcessObjSmooth::MakeFade(const WDL_TypedBuf<BL_FLOAT> &buf0,
                              const WDL_TypedBuf<BL_FLOAT> &buf1,
                              BL_FLOAT *resultBuf)
{    
    // Fade partially, the rest full (prev or next)
    // This avoids some remaining little clics

#if 1 // Fade the middle
#define FADE_START 0.33
#define FADE_END   0.66
#endif
    
#if 0 // Full fade // NOT SURE, IT MAY BE BEST
#define FADE_START 0.0
#define FADE_END   1.0
#endif
    
#if 0 // Fade the begining
#define FADE_START 0.0
#define FADE_END   0.25
#endif
    
#if 0 // Fade the end
#define FADE_START 0.75
#define FADE_END   1.0
#endif
    
    int bufSize = buf0.GetSize() - 1;
    
    for (int i = 0; i < buf0.GetSize(); i++)
    {
        BL_FLOAT prevVal = buf0.Get()[i];
        BL_FLOAT newVal = buf1.Get()[i];
        
        // Fades only on the part of the frame
        BL_FLOAT t = 0.0;
        if ((i >= bufSize*FADE_START) &&
            (i < bufSize*FADE_END))
        {
            t = (i - bufSize*FADE_START)/(bufSize*(FADE_END - FADE_START));
        }
        
        if (i >= bufSize*FADE_END)
            t = 1.0;
        
        BL_FLOAT result = (1.0 - t)*prevVal + t*newVal;
        
        resultBuf[i] = result;
    }
}
