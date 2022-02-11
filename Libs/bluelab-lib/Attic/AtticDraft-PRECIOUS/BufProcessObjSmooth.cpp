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

#define COMPUTE_FIRST_BUFFER 0

#define DEBUG_GRAPH 1

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

BufProcessObjSmooth::BufProcessObjSmooth(BufProcessObj *obj0, BufProcessObj *obj1,
                                         int bufferSize)
{
    mBufObjs[0] = obj0;
    mBufObjs[1] = obj1;
    
    // Buffering
    mBufSize = bufferSize;
    
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
    
    mCurrentObjIndex = 0;
    
    mStableState = true;
    mHasJustReset = true;
    mFirstNewBufferProcessed = false;
    
    // Buffering
    mSamplesBuf.Resize(0);
    
    mResultBuf[0].Resize(0);
    mResultBuf[1].Resize(0);
    
    mResultBufFade.Resize(0);
    
    // Parameter
    mParameterValid = false;
    
    // Initialize just in case
    mParameter = 0.0;
}

void
BufProcessObjSmooth::SetParameter(double param)
{
    // Never found the case of clash => i.e setting a new parameter
    // when the previous one is not yet ready
    // but we never know...

  // ?????????
    if (mParameterValid)
        return;
    
    // Just register the new parameter, it will be set in Process()
    mParameter = param;
    mParameterValid = true;
    
    mFirstNewBufferProcessed = false;
    mStableState = false;
}

bool
BufProcessObjSmooth::Process(double *input, double *output, int nFrames)
{
#if 1
    double param0 = mBufObjs[mCurrentObjIndex]->GetParameter();
    double param1 = mBufObjs[(mCurrentObjIndex + 1)%2]->GetParameter();
    fprintf(stderr, "parameters: %g %g\n", param0, param1);
#endif
    
    if (mHasJustReset)
    {
        if (mParameterValid)
        {
            mBufObjs[mCurrentObjIndex]->SetParameter(mParameter);
            mParameterValid = false;
            
#if DEBUG_GRAPH
            //TraceGraph(mCurrentObjIndex, 0);
#endif
        }
        
        mHasJustReset = false;
        mStableState = true;
    }
    
    // Try to get the result from a previous computation
    bool res = GetOutputSamples(output, nFrames);
    
    mSamplesBuf.Add(input, nFrames);
    
    // nFames can be greater than mBufSize
    // So loop and compute as much as possible
    bool computedFlag = true;
    while(computedFlag)
    {
        // Compute new result if possible
        computedFlag = ComputeNewSamples(nFrames);
    }
    
    if (!res)
        // We don't have filled the output yes
        // Maybe after computation, we have enough now
    {
        res = GetOutputSamples(output, nFrames);
    }
    
    return res;
}

bool
BufProcessObjSmooth::ComputeNewSamples(int nFrames)
{
#if DEBUG_GRAPH
    TraceGraph(mCurrentObjIndex, 7);
#endif
    
    if (mSamplesBuf.GetSize() < mBufSize)
        return false;
    
    // We have enough samples to do the processing !
    
    if (mStableState)
        // Normal behavior
    {
        // Just in case
        double param = mBufObjs[mCurrentObjIndex]->GetParameter();
        mBufObjs[(mCurrentObjIndex+1)%2]->SetParameter(param);
        
        int size0 = mResultBuf[mCurrentObjIndex].GetSize();
        Utils::GrowFillZeros(&mResultBuf[mCurrentObjIndex], mBufSize);
        bool res = mBufObjs[mCurrentObjIndex]->Process(mSamplesBuf.Get(),
                                                       &mResultBuf[mCurrentObjIndex].Get()[size0],
                                                       mBufSize);
        
        // Feed the other object with the new data
        // NOTE: changes some data actually, but not sure it's efficient...
        int otherIndex = (mCurrentObjIndex + 1) % 2;
        
        mBufObjs[otherIndex]->Process(mSamplesBuf.Get(), NULL, mBufSize);
        
        if (res)
            Utils::ConsumeLeft(&mSamplesBuf, mBufSize);
     
#if DEBUG_GRAPH
        //TraceGraph(mCurrentObjIndex, 1);
#endif
        
        return res;
    }
    
    // When using it, we have latency, but no crackles
#if COMPUTE_FIRST_BUFFER
    
    if (!mFirstNewBufferProcessed)
        // Process one more buffer before interpolating
    {
        bool res = ProcessFirstBuffer();
        
        return res;
    }
        
#else // Do not wait to have computed a previous full buffer
    // In this case, we have fe latency but we some low crackles
    
    int newObjIdx0 = (mCurrentObjIndex + 1) % 2;
    
    // Set the parameter
    if (mParameterValid)
    {
        mBufObjs[newObjIdx0]->SetParameter(mParameter);
        mParameterValid = false;
    }

#endif
    
    // Process and make the fade
    bool res = ProcessMakeFade();
    if (!res)
        return false;
    
    // Ok, we have made the fade
    // Now, set the new parameter to the old object too
    // (so it will be up to date next time)
    
    int newObjIdx = (mCurrentObjIndex + 1) % 2;
    
    double param = mBufObjs[newObjIdx]->GetParameter();
    mBufObjs[mCurrentObjIndex]->SetParameter(param);
    
    mHasJustReset = false;
    mStableState = true;
    
    // Swap the objects
    mCurrentObjIndex = newObjIdx;
    
#if DEBUG_GRAPH
    TraceGraph(mCurrentObjIndex, 6);
#endif
    
    return true;
}

bool
BufProcessObjSmooth::GetOutputSamples(double *output, int nFrames)
{
    // Try to fill with the faded result
    if (mResultBufFade.GetSize() >= nFrames)
    {
        if (output != NULL)
            memcpy(output, mResultBufFade.Get(), nFrames*sizeof(double));
        Utils::ConsumeLeft(&mResultBufFade, nFrames);
        
#if DEBUG_GRAPH
        TraceGraph(mCurrentObjIndex, 2);
#endif
        
        return true;
    }
    
    // Try to fill with the normal result
    if (mResultBuf[mCurrentObjIndex].GetSize() >= nFrames)
    {
        if (output != NULL)
            memcpy(output, mResultBuf[mCurrentObjIndex].Get(), nFrames*sizeof(double));
            
        int otherIndex = (mCurrentObjIndex + 1) % 2;
            
        Utils::ConsumeLeft(&mResultBuf[mCurrentObjIndex], nFrames);
        Utils::ConsumeLeft(&mResultBuf[otherIndex], nFrames);
     
#if DEBUG_GRAPH
        //TraceGraph(mCurrentObjIndex, 3);
#endif
        
        return true;
    }
        
    // Not filled
    return false;
}

bool
BufProcessObjSmooth::ProcessFirstBuffer()
{
#if DEBUG_GRAPH
    //TraceGraph(mCurrentObjIndex, 4);
#endif
    
    // Use prev object one more time
    int size0 = mResultBuf[mCurrentObjIndex].GetSize();
    Utils::GrowFillZeros(&mResultBuf[mCurrentObjIndex], mBufSize);
    bool res = mBufObjs[mCurrentObjIndex]->
                                Process(mSamplesBuf.Get(),
                                        &mResultBuf[mCurrentObjIndex].Get()[size0],
                                        mBufSize);
    
    int newObjIdx = (mCurrentObjIndex + 1) % 2;
    
    if (res)
    {
        // We have actually processed the new buffer
        // (be careful with buffers of 512...)
        mFirstNewBufferProcessed = true;
        
        // New obj
        
        // Set the parameter
        if (mParameterValid)
        {
            mBufObjs[newObjIdx]->SetParameter(mParameter);
            mParameterValid = false;
        }
    }
    
    // Feed the other object with the new data
    // NOTE: changes some data actually, but not sure it's efficient...
    mBufObjs[newObjIdx]->Process(mSamplesBuf.Get(), NULL, mBufSize);
    
    if (res)
        Utils::ConsumeLeft(&mSamplesBuf, mBufSize);
    
    return res;
}

bool
BufProcessObjSmooth::ProcessMakeFade()
{
#if DEBUG_GRAPH
    TraceGraph(mCurrentObjIndex, 5);
#endif

    // Else we still are not in stable state, but we have all the data to make the fade
    
    // Make a fade between the old object and the new object
    
    // Get the object indices
    int newObjIdx = (mCurrentObjIndex + 1) % 2;
    
    // Prepare two buffers
    WDL_TypedBuf<double> prevOutput;
    prevOutput.Resize(mBufSize);
    
    WDL_TypedBuf<double> newOutput;
    newOutput.Resize(mBufSize);
    
    double res0 = mBufObjs[mCurrentObjIndex]->
    Process(mSamplesBuf.Get(), prevOutput.Get(), mBufSize);
    
    double res1 = mBufObjs[newObjIdx]->
    Process(mSamplesBuf.Get(), newOutput.Get(), mBufSize);
    
    if (!res0 || !res1)
        // We didn't get enough samples to process in the member object
        return false;
    
    Utils::ConsumeLeft(&mSamplesBuf, mBufSize);
    
    // Ok, we have processed, we can continue
    
    int size0 = mResultBufFade.GetSize();
    Utils::GrowFillZeros(&mResultBufFade, mBufSize);
    
    // Make the fade and copy to the output
    MakeFade(prevOutput, newOutput, &mResultBufFade.Get()[size0]);
    
    return true;
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
    
#if 0 // Fade the middle (not used)
#define FADE_START 0.33
#define FADE_END   0.66
#endif
    
#if COMPUTE_FIRST_BUFFER
    
    // Make the fade early, to avoid latency
    // We already have a previous buffer, so the data is good
#define FADE_START 0.0
#define FADE_END   0.25
    
#else
    
    // Make the fade later, to avoid fading zone with zero
    // that is in the new buffer if we have not computed
    // aprevious full buffer
#define FADE_START 0.75
#define FADE_END   1.0
    
#endif
    
    int bufSize = buf0.GetSize();
    
    for (int i = 0; i < bufSize; i++)
    {
        double prevVal = buf0.Get()[i];
        double newVal = buf1.Get()[i];
        
        // Simple fade, on all the data
        //double t = ((double)i)/(nFrames - 1);
        
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
    }
}
