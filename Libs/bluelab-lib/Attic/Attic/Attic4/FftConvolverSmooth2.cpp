//
//  FftConvolverSmooth2.cpp
//  Spatializer
//
//  Created by Pan on 19/12/17.
//
//

#include <FftConvolver3.h>

#include "FftConvolverSmooth2.h"


FftConvolverSmooth2::FftConvolverSmooth2(int bufferSize, bool normalize)
{
    mConvolvers[0] = new FftConvolver3(bufferSize, normalize);
    mConvolvers[1] = new FftConvolver3(bufferSize, normalize);
    
    mCurrentConvolverIndex = 0;
    
    mStableState = true;
    mHasJustReset = true;
    mFirstNewBufferProcessed = false;
}

FftConvolverSmooth2::~FftConvolverSmooth2()
{
    delete mConvolvers[0];
    delete mConvolvers[1];
}

void
FftConvolverSmooth2::Reset()
{
    mConvolvers[0]->Reset();
    mConvolvers[1]->Reset();
    
    mCurrentConvolverIndex = 0;
    
    mStableState = true;
    mHasJustReset = true;
    
    mNewResponse.Resize(0);
}

void
FftConvolverSmooth2::SetResponse(const WDL_TypedBuf<BL_FLOAT> *response)
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
FftConvolverSmooth2::Process(BL_FLOAT *input, BL_FLOAT *output, int nFrames)
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
    
    if (mStableState)
       // Normal behavior
    {        
        bool res = mConvolvers[mCurrentConvolverIndex]->Process(input, output, nFrames);
        
        // Feed the other convolver with the new data
        // NOTE: changes some data actually, but not sure it's efficient...
        int otherIndex = (mCurrentConvolverIndex + 1) % 2;
        mConvolvers[otherIndex]->Process(input, NULL, nFrames);
        
        return res;
    }
   
    // When using it, we have latency, but no crackles
#define COMPUTE_FIRST_BUFFER 1
    
#if COMPUTE_FIRST_BUFFER
    if (!mFirstNewBufferProcessed)
        // Process one more buffer before interpolating
    {        
        // Use prev convolver one more time
        bool res = mConvolvers[mCurrentConvolverIndex]->Process(input, output, nFrames);
        
        // New convolver
        
        int newConvIdx = (mCurrentConvolverIndex + 1) % 2;
        
        // Set the response
        if (mNewResponse.GetSize() > 0)
        {
            mConvolvers[newConvIdx]->SetResponse(&mNewResponse);
            mNewResponse.Resize(0);
        }
        
        // Feed the other convolver with the new data
        // NOTE: changes some data actually, but not sure it's efficient...
        mConvolvers[newConvIdx]->Process(input, NULL, nFrames);
        
        mFirstNewBufferProcessed = true;
        
        //mCurrentConvolverIndex = newConvIdx;
        
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
    
    // Else we still are not in stable state, but we have all the data to make the fade
    
    // Make a fade between the old convolver and the new convolver
    
    // Get the convolvers indices
    int newConvIdx = (mCurrentConvolverIndex + 1) % 2;
    
    // Prepare two buffers
    WDL_TypedBuf<BL_FLOAT> prevOutput;
    prevOutput.Resize(nFrames);
   
    WDL_TypedBuf<BL_FLOAT> newOutput;
    newOutput.Resize(nFrames);
    
    BL_FLOAT res0 = mConvolvers[mCurrentConvolverIndex]->Process(input, prevOutput.Get(), nFrames);
    BL_FLOAT res1 = mConvolvers[newConvIdx]->Process(input, newOutput.Get(), nFrames);
    
    // Make the fade and copy to the output
    
    
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

    
    for (int i = 0; i < nFrames; i++)
    {
        BL_FLOAT prevVal = prevOutput.Get()[i];
        BL_FLOAT newVal = newOutput.Get()[i];
        
        // Simple fade, on all the data
        //BL_FLOAT t = ((BL_FLOAT)i)/(nFrames - 1);
        
        // Fades only on the part of the frame
        BL_FLOAT t = 0.0;
        if ((i >= nFrames*FADE_START) &&
           (i < nFrames*FADE_END))
        {
            t = (i - nFrames*FADE_START)/(nFrames*(FADE_END - FADE_START));
        }
        
        if (i >= nFrames*FADE_END)
            t = 1.0;
        
        BL_FLOAT result = (1.0 - t)*prevVal + t*newVal;
        
        output[i] = result;
    }
    
    // Ok, we have made the fade
    // Now, set the new response to the old convolver too
    // (so it will be up to date next time)
    WDL_TypedBuf<BL_FLOAT> newResponse;
    mConvolvers[newConvIdx]->GetResponse(&newResponse);
    mConvolvers[mCurrentConvolverIndex]->SetResponse(&newResponse);
    
   // mResponseJustChanged = false;
    mHasJustReset = false;
    mStableState = true;
    
    // Swap the convolvers
    mCurrentConvolverIndex = newConvIdx;
                  
    return (res0 && res1);
}
