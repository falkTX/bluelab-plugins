//
//  FftProcessObjSmooth5.cpp
//  Spatializer
//
//  Created by Pan on 19/12/17.
//
//

#include "FftProcessObj5.h"
#include <BLUtils.h>

#include "FftProcessObjSmooth5.h"


FftProcessObjSmooth5::FftProcessObjSmooth5(FftProcessObj5 *obj0, FftProcessObj5 *obj1,
                                           int bufferSize)
{
    mFftObjs[0] = obj0;
    mFftObjs[1] = obj1;
    
    mCurrentObjIndex = 0;
    
    mStableState = true;
    mHasJustReset = true;
    mFirstNewBufferProcessed = false;
    
    // Buffering
    mBufSize = bufferSize;
    
    mSamplesBuf.Resize(0);
    
    mResultBuf[0].Resize(0);
    mResultBuf[1].Resize(0);
    
    mResultBufFade.Resize(0);
    
    // Parameter
    mParameterValid = false;

    // Initialize just in case
    mParameter = 0.0;
}

FftProcessObjSmooth5::~FftProcessObjSmooth5()
{
    delete mFftObjs[0];
    delete mFftObjs[1];
}

FftProcessObj5 *
FftProcessObjSmooth5::GetFftObj(int index)
{
    if (index > 1)
        return NULL;
    
    return mFftObjs[index];
}

void
FftProcessObjSmooth5::Reset()
{
    mFftObjs[0]->Reset();
    mFftObjs[1]->Reset();
    
    mCurrentObjIndex = 0;
    
    mStableState = true;
    mHasJustReset = true;
    
    // Buffering
    mSamplesBuf.Resize(0);
    
    mResultBuf[0].Resize(0);
    mResultBuf[1].Resize(0);
    
    mResultBufFade.Resize(0);
}

void
FftProcessObjSmooth5::SetParameter(double param)
{
    // Never found the case of clash => i.e setting a new parameter
    // when the previous one is not yet ready
    // but we never know...
    
    if (mParameterValid)
        return;
    
    // Just register the new parameter, it will be set in Process()
    mParameter = param;
    mParameterValid = true;
    
    mFirstNewBufferProcessed = false;
    mStableState = false;
}

bool
FftProcessObjSmooth5::Process(double *input, double *output, int nFrames)
{
    if (mHasJustReset)
    {
        if (mParameterValid)
        {
            mFftObjs[mCurrentObjIndex]->SetParameter(mParameter);
            mParameterValid = false;
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
FftProcessObjSmooth5::ComputeNewSamples(int nFrames)
{
    if (mSamplesBuf.GetSize() < mBufSize)
        return false;
    
    // We have enough samples to do the processing !
    
    if (mStableState)
        // Normal behavior
    {
        int size0 = mResultBuf[mCurrentObjIndex].GetSize();
        BLUtils::GrowFillZeros(&mResultBuf[mCurrentObjIndex], mBufSize);
        bool res = mFftObjs[mCurrentObjIndex]->
                        Process(mSamplesBuf.Get(),
                                &mResultBuf[mCurrentObjIndex].Get()[size0],
                                mBufSize);
        
        // Feed the other object with the new data
        // NOTE: changes some data actually, but not sure it's efficient...
        int otherIndex = (mCurrentObjIndex + 1) % 2;
        
        mFftObjs[otherIndex]->Process(mSamplesBuf.Get(), NULL, mBufSize);
        
        if (res)
            BLUtils::ConsumeLeft(&mSamplesBuf, mBufSize);
     
        return res;
    }
    
    // When using it, we have latency, but no crackles
#define COMPUTE_FIRST_BUFFER 1
    
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
        mFftObjs[newObjIdx0]->SetParameter(mParameter);
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
    
    double param = mFftObjs[newObjIdx]->GetParameter();
    mFftObjs[mCurrentObjIndex]->SetParameter(param);
    
    mHasJustReset = false;
    mStableState = true;
    
    // Swap the objects
    mCurrentObjIndex = newObjIdx;
    
    return true;
}

bool
FftProcessObjSmooth5::GetOutputSamples(double *output, int nFrames)
{
    // Try to fill with the faded result
    if (mResultBufFade.GetSize() >= nFrames)
    {
        if (output != NULL)
            memcpy(output, mResultBufFade.Get(), nFrames*sizeof(double));
        BLUtils::ConsumeLeft(&mResultBufFade, nFrames);
            
        return true;
    }
    
    // Try to fill with the normal result
    if (mResultBuf[mCurrentObjIndex].GetSize() >= nFrames)
    {
        if (output != NULL)
            memcpy(output, mResultBuf[mCurrentObjIndex].Get(), nFrames*sizeof(double));
            
        int otherIndex = (mCurrentObjIndex + 1) % 2;
            
        BLUtils::ConsumeLeft(&mResultBuf[mCurrentObjIndex], nFrames);
        BLUtils::ConsumeLeft(&mResultBuf[otherIndex], nFrames);
            
        return true;
    }
        
    // Not filled
    return false;
}

bool
FftProcessObjSmooth5::ProcessFirstBuffer()
{
    // Use prev object one more time
    int size0 = mResultBuf[mCurrentObjIndex].GetSize();
    BLUtils::GrowFillZeros(&mResultBuf[mCurrentObjIndex], mBufSize);
    bool res = mFftObjs[mCurrentObjIndex]->
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
            mFftObjs[newObjIdx]->SetParameter(mParameter);
            mParameterValid = false;
        }
    }
    
    // Feed the other object with the new data
    // NOTE: changes some data actually, but not sure it's efficient...
    mFftObjs[newObjIdx]->Process(mSamplesBuf.Get(), NULL, mBufSize);
    
    if (res)
        BLUtils::ConsumeLeft(&mSamplesBuf, mBufSize);
    
    return res;
}

bool
FftProcessObjSmooth5::ProcessMakeFade()
{
    // Else we still are not in stable state, but we have all the data to make the fade
    
    // Make a fade between the old object and the new object
    
    // Get the object indices
    int newObjIdx = (mCurrentObjIndex + 1) % 2;
    
    // Prepare two buffers
    WDL_TypedBuf<double> prevOutput;
    prevOutput.Resize(mBufSize);
    
    WDL_TypedBuf<double> newOutput;
    newOutput.Resize(mBufSize);
    
    double res0 = mFftObjs[mCurrentObjIndex]->
    Process(mSamplesBuf.Get(), prevOutput.Get(), mBufSize);
    
    double res1 = mFftObjs[newObjIdx]->
    Process(mSamplesBuf.Get(), newOutput.Get(), mBufSize);
    
    if (!res0 || !res1)
        // We didn't get enough samples to process in the member object
        return false;
    
    BLUtils::ConsumeLeft(&mSamplesBuf, mBufSize);
    
    // Ok, we have processed, we can continue
    
    int size0 = mResultBufFade.GetSize();
    BLUtils::GrowFillZeros(&mResultBufFade, mBufSize);
    
    // Make the fade and copy to the output
    MakeFade(prevOutput, newOutput, &mResultBufFade.Get()[size0]);
    
    return true;
}

void
FftProcessObjSmooth5::MakeFade(const WDL_TypedBuf<double> &buf0,
                               const WDL_TypedBuf<double> &buf1,
                               double *resultBuf)
{
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
