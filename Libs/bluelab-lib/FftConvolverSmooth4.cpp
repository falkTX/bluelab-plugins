//
//  FftConvolverSmooth2.cpp
//  Spatializer
//
//  Created by Pan on 19/12/17.
//
//

#include "FftConvolver4.h"
#include <BLUtils.h>

#include "FftConvolverSmooth4.h"


FftConvolverSmooth4::FftConvolverSmooth4(int bufferSize, bool normalize)
{
    mConvolvers[0] = new FftConvolver4(bufferSize, normalize);
    mConvolvers[1] = new FftConvolver4(bufferSize, normalize);
    
    mCurrentConvolverIndex = 0;
    
    mStableState = true;
    mHasJustReset = true;
    mFirstNewBufferProcessed = false;
    
    // Buffering
    mBufSize = bufferSize;
    
    mSamplesBuf.Resize(0);
    
    mResultBuf[0].Resize(0);
    mResultBuf[1].Resize(0);
    
    mResultBufFade.Resize(0);
}

FftConvolverSmooth4::~FftConvolverSmooth4()
{
    delete mConvolvers[0];
    delete mConvolvers[1];
}

void
FftConvolverSmooth4::Reset()
{
    mConvolvers[0]->Reset();
    mConvolvers[1]->Reset();
    
    mCurrentConvolverIndex = 0;
    
    mStableState = true;
    mHasJustReset = true;
    
    mNewResponse.Resize(0);
    
    // Buffering
    mSamplesBuf.Resize(0);
    
    mResultBuf[0].Resize(0);
    mResultBuf[1].Resize(0);
    
    mResultBufFade.Resize(0);
}

void
FftConvolverSmooth4::SetResponse(const WDL_TypedBuf<BL_FLOAT> *response)
{
    // Never found the case of clash => i.e setting a new response
    // when the previous one is not yet ready
    // but we never know...
    
    // This case is managed better in Process()
    // => We keep the newer response !
    //if (mNewResponse.GetSize() > 0)
    //    return;
    
    // Just register the new response, it will be set in Process()
    mNewResponse = *response;
    
    mFirstNewBufferProcessed = false;
    mStableState = false;
}

bool
FftConvolverSmooth4::Process(BL_FLOAT *input, BL_FLOAT *output, int nFrames)
{
    if (mHasJustReset)
    {
        if (mNewResponse.GetSize() > 0)
        {
            mConvolvers[mCurrentConvolverIndex]->SetResponse(&mNewResponse);
            mNewResponse.Resize(0);
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
FftConvolverSmooth4::ComputeNewSamples(int nFrames)
{
    if (mSamplesBuf.GetSize() < mBufSize)
        return false;
    
    // We have enough samples to do the processing !
    
    if (mStableState)
        // Normal behavior
    {
        int size0 = mResultBuf[mCurrentConvolverIndex].GetSize();
        BLUtils::GrowFillZeros(&mResultBuf[mCurrentConvolverIndex], mBufSize);
        bool res = mConvolvers[mCurrentConvolverIndex]->
        Process(mSamplesBuf.Get(),
                &mResultBuf[mCurrentConvolverIndex].Get()[size0],
                mBufSize);
        
        // Feed the other convolver with the new data
        // NOTE: changes some data actually, but not sure it's efficient...
        int otherIndex = (mCurrentConvolverIndex + 1) % 2;
        
        mConvolvers[otherIndex]->Process(mSamplesBuf.Get(), NULL, mBufSize);
        
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
    
    int newConvIdx0 = (mCurrentConvolverIndex + 1) % 2;
    
    // Set the response
    if (mNewResponse.GetSize() > 0)
    {
        mConvolvers[newConvIdx0]->SetResponse(&mNewResponse);
        mNewResponse.Resize(0);
    }

#endif
    
    // Process and make the fade
    bool res = ProcessMakeFade();
    if (!res)
        return false;
    
    // Ok, we have made the fade
    // Now, set the new response to the old convolver too
    // (so it will be up to date next time)
    
    int newConvIdx = (mCurrentConvolverIndex + 1) % 2;
    
    WDL_TypedBuf<BL_FLOAT> newResponse;
    mConvolvers[newConvIdx]->GetResponse(&newResponse);
    mConvolvers[mCurrentConvolverIndex]->SetResponse(&newResponse);
    
    // mResponseJustChanged = false;
    mHasJustReset = false;
    mStableState = true;
    
    // Swap the convolvers
    mCurrentConvolverIndex = newConvIdx;
    
    return true;
}

bool
FftConvolverSmooth4::GetOutputSamples(BL_FLOAT *output, int nFrames)
{
    // Try to fill with the faded result
    if (mResultBufFade.GetSize() >= nFrames)
    {
        if (output != NULL)
            memcpy(output, mResultBufFade.Get(), nFrames*sizeof(BL_FLOAT));
        BLUtils::ConsumeLeft(&mResultBufFade, nFrames);
            
        return true;
    }
    
    // Try to fill with the normal result
    if (mResultBuf[mCurrentConvolverIndex].GetSize() >= nFrames)
    {
        if (output != NULL)
            memcpy(output, mResultBuf[mCurrentConvolverIndex].Get(), nFrames*sizeof(BL_FLOAT));
            
        int otherIndex = (mCurrentConvolverIndex + 1) % 2;
            
        BLUtils::ConsumeLeft(&mResultBuf[mCurrentConvolverIndex], nFrames);
        BLUtils::ConsumeLeft(&mResultBuf[otherIndex], nFrames);
            
        return true;
    }
        
    // Not filled
    return false;
}

bool
FftConvolverSmooth4::ProcessFirstBuffer()
{
    // Use prev convolver one more time
    int size0 = mResultBuf[mCurrentConvolverIndex].GetSize();
    BLUtils::GrowFillZeros(&mResultBuf[mCurrentConvolverIndex], mBufSize);
    bool res = mConvolvers[mCurrentConvolverIndex]->
                                Process(mSamplesBuf.Get(),
                                        &mResultBuf[mCurrentConvolverIndex].Get()[size0],
                                        mBufSize);
    
    int newConvIdx = (mCurrentConvolverIndex + 1) % 2;
    
    if (res)
    {
        // We have actually processed the new buffer
        // (be careful with buffers of 512...)
        mFirstNewBufferProcessed = true;
        
        // New convolver
        
        // Set the response
        if (mNewResponse.GetSize() > 0)
        {
            mConvolvers[newConvIdx]->SetResponse(&mNewResponse);
            mNewResponse.Resize(0);
        }
    }
    
    // Feed the other convolver with the new data
    // NOTE: changes some data actually, but not sure it's efficient...
    mConvolvers[newConvIdx]->Process(mSamplesBuf.Get(), NULL, mBufSize);
    
    if (res)
        BLUtils::ConsumeLeft(&mSamplesBuf, mBufSize);
    
    //mCurrentConvolverIndex = newConvIdx;
    
    return res;
}

bool
FftConvolverSmooth4::ProcessMakeFade()
{
    // Else we still are not in stable state, but we have all the data to make the fade
    
    // Make a fade between the old convolver and the new convolver
    
    // Get the convolvers indices
    int newConvIdx = (mCurrentConvolverIndex + 1) % 2;
    
    // Prepare two buffers
    WDL_TypedBuf<BL_FLOAT> prevOutput;
    prevOutput.Resize(mBufSize);
    
    WDL_TypedBuf<BL_FLOAT> newOutput;
    newOutput.Resize(mBufSize);
    
    BL_FLOAT res0 = mConvolvers[mCurrentConvolverIndex]->
    Process(mSamplesBuf.Get(), prevOutput.Get(), mBufSize);
    
    BL_FLOAT res1 = mConvolvers[newConvIdx]->
    Process(mSamplesBuf.Get(), newOutput.Get(), mBufSize);
    
    if (!res0 || !res1)
        // We didn't get enough samples to process in the memeber convolvers
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
FftConvolverSmooth4::MakeFade(const WDL_TypedBuf<BL_FLOAT> &buf0,
                              const WDL_TypedBuf<BL_FLOAT> &buf1,
                              BL_FLOAT *resultBuf)
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
        BL_FLOAT prevVal = buf0.Get()[i];
        BL_FLOAT newVal = buf1.Get()[i];
        
        // Simple fade, on all the data
        //BL_FLOAT t = ((BL_FLOAT)i)/(nFrames - 1);
        
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
