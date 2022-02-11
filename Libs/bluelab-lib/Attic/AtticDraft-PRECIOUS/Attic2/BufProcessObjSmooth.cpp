//
//  FftProcessObjSmooth5.cpp
//  Spatializer
//
//  Created by Pan on 19/12/17.
//
//

#include "BufProcessObj.h"
#include "Debug.h"
#include "Utils.h"

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

        double val = ((double)j)/NUM_CURVES;
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

BufProcessObjSmooth::BufProcessObjSmooth(BufProcessObj *obj0, BufProcessObj *obj1)
{
    mBufObjs[0] = obj0;
    mBufObjs[1] = obj1;
    
    Reset();
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
BufProcessObjSmooth::Reset()
{
    mBufObjs[0]->Reset();
    mBufObjs[1]->Reset();
    
    // Parameter
    mParameterChanged = true;
    mBuffersFlushed = true;
    mReverseFade = false;
}

void
BufProcessObjSmooth::SetParameter(double param)
{
    // Just register the new parameter, it will be set in Process()
    mParameter = param;
    mParameterChanged = true;
    mBuffersFlushed = false;
}

bool
BufProcessObjSmooth::Process(double *input, double *output, int nFrames)
{
#if 0 // This makes some sort of scratchs
    // TODO: try to implement something to gain performances
    if (!mParameterChanged)
    {
        bool res = mBufObjs[0]->Process(input, output, nFrames);
        
        return res;
    }
#endif
    
    // Parameter changed

    // Set the parameter
    mBufObjs[1]->SetParameter(mParameter);
    
    // DEBUG
    //mBufObjs[1]->SetParameter(mParameter);
    
    // Compute the two buffers
    mResultBuf[0].Resize(nFrames);
    mResultBuf[1].Resize(nFrames);
    
    bool res0 = mBufObjs[0]->Process(input, mResultBuf[0].Get(), nFrames);
    bool res1 = mBufObjs[1]->Process(input, mResultBuf[1].Get(), nFrames);
    
    bool res = (res0 && res1);
    if (!res)
        return false;
    
    // Make fade
    if (!mReverseFade)
        MakeFade(mResultBuf[0], mResultBuf[1], output);
    else
        MakeFade(mResultBuf[1], mResultBuf[0], output);
    
    mReverseFade = !mReverseFade;
    
    // GET PARAMETER UNUSED ?
    //double param = mBufObjs[mCurrentObjIndex]->GetParameter();
    
    mBufObjs[0]->SetParameter(mParameter);
    
    mParameterChanged = false;
    
    return res;
}

void
BufProcessObjSmooth::MakeFade(const WDL_TypedBuf<double> &buf0,
                              const WDL_TypedBuf<double> &buf1,
                              double *resultBuf)
{
#if 0 // DEBUG
    // No fade
    memcpy(resultBuf, buf0.Get(), buf0.GetSize()*sizeof(double));
    
    return;
#endif
    
    // Fade partially, the rest full (prev or next)
    // This avoids some remaining little clics

#if 0 // Full fade
#define FADE_START 0.0
#define FADE_END   1.0
#endif
    
#if 1 // Fade the middle
#define FADE_START 0.33
#define FADE_END   0.66
#endif
    
#if 0
    // Make the fade later, to avoid fading zone with zero
    // that is in the new buffer if we have not computed
    // aprevious full buffer
#define FADE_START 0.75
#define FADE_END   1.0
    
#endif
    
    int bufSize = buf0.GetSize() - 1;
    
    for (int i = 0; i < buf0.GetSize(); i++)
    {
        double prevVal = buf0.Get()[i];
        double newVal = buf1.Get()[i];
        
        // Simple fade, on all the data
        //double t = ((double)i)/bufSize;
        
        
        // Fades only on the part of the frame
        double t = 0.0;
        if ((i >= bufSize*FADE_START) &&
            (i < bufSize*FADE_END))
        {
            t = (i - bufSize*FADE_START)/(bufSize*(FADE_END - FADE_START));
        }
        
        if (i >= bufSize*FADE_END)
            t = 1.0;
        
        double result = (1.0 - t)*prevVal + t*newVal;
        
        resultBuf[i] = result;
        
        //Debug::DumpSingleValue("t.txt", t);
    }
    
#if 0
    Debug::AppendData("buf0.txt", buf0.Get(), buf0.GetSize());
    Debug::AppendData("buf1.txt", buf1.Get(), buf1.GetSize());
    Debug::AppendData("res.txt", resultBuf, buf0.GetSize());
#endif
}
