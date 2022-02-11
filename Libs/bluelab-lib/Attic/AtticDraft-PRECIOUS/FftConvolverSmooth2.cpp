//
//  FftConvolverSmooth.cpp
//  Spatializer
//
//  Created by Pan on 19/12/17.
//
//

#include <FftConvolver3.h>
#include <Debug.h>

#include "FftConvolverSmooth2.h"

#define DEBUG_PRINTF 0

FftConvolverSmooth2::FftConvolverSmooth2(int bufferSize, bool normalize)
{
    mConvolvers[0] = new FftConvolver3(bufferSize, normalize);
    mConvolvers[1] = new FftConvolver3(bufferSize, normalize);
    
    mCurrentConvolverIndex = 0;
    //mResponseJustChanged = false;
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
    //mResponseJustChanged = false;
    mStableState = true;
    mHasJustReset = true;
    
    mNewResponse.Resize(0);
}

void
FftConvolverSmooth2::SetResponse(const WDL_TypedBuf<double> *response)
{
#if DEBUG_PRINTF
    fprintf(stderr, "set response\n");
#endif
    
    // Never found the case of clash => i.e setting a new response
    // when the previous one is not yet ready
    // but we never know...
    
    // This case is managed better in Process()
    // => We keep the newer response !
    //if (mNewResponse.GetSize() > 0)
    //    return;
    
    mNewResponse = *response;
    mFirstNewBufferProcessed = false;
    mStableState = false;

#if 0
// DEBUG
    mConvolvers[0]->SetResponse(response);
    mConvolvers[1]->SetResponse(response);
#endif
    
#if 0
    if (mHasJustReset)
        // Normal behavior
    {
        mConvolvers[mCurrentConvolverIndex]->SetResponse(response);
        
        mHasJustReset = false;
        
        return;
    }
    
    // Will set the response later
    mNewResponse = *response;
    
    // Swap the convolvers only if we are in a stable state
    // (i.e we have computed the buffers enough) 
    if (mFirstNewBufferProcessed)
    {
        int newConvolverId = (mCurrentConvolverIndex + 1) % 2;

#if 0
        mConvolvers[newConvolverId]->SetResponse(response);
#endif
    
        mCurrentConvolverIndex = newConvolverId;
    }
    
    //mResponseJustChanged = true;
    mFirstNewBufferProcessed = false;
    mStableState = false;
#endif
}

bool
FftConvolverSmooth2::Process(double *input, double *output, int nFrames)
{
    // DEBUG
    //return mConvolvers[1]->Process(input, output, nFrames);
    if (mHasJustReset)
    {
#if DEBUG_PRINTF
        fprintf(stderr, "has just reset !\n");
#endif
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
#if DEBUG_PRINTF
        fprintf(stderr, "stable state: simply compute\n");
#endif
        
        bool res = mConvolvers[mCurrentConvolverIndex]->Process(input, output, nFrames);
        
        // Feed the other convolver with the new data
        // NOTE: changes some data actually, but not sure it's efficient...
        int otherIndex = (mCurrentConvolverIndex + 1) % 2;
        mConvolvers[otherIndex]->Process(input, NULL, nFrames);
        
        return res;
    }
   
    // When using it, we have latency
#define COMPUTE_FIRST_BUFFER 1
    
#if COMPUTE_FIRST_BUFFER
    if (!mFirstNewBufferProcessed)
        // Process one more buffer before interpolating
    {
#if DEBUG_PRINTF
        fprintf(stderr, "need to compute first buffer !\n");
        fprintf(stderr, "process old\n");
#endif
        
        // Use prev convolver one more time
        bool res = mConvolvers[mCurrentConvolverIndex]->Process(input, output, nFrames);
        
        // New convolver
        
        int newConvIdx = (mCurrentConvolverIndex + 1) % 2;
        
        // Set the response
        if (mNewResponse.GetSize() > 0)
        {
            mConvolvers[newConvIdx]->SetResponse(&mNewResponse);
            mNewResponse.Resize(0);
            
#if DEBUG_PRINTF
            fprintf(stderr, "apply response to new\n");
#endif
        }
        
        // Feed the other convolver with the new data
        // NOTE: changes some data actually, but not sure it's efficient...
        mConvolvers[newConvIdx]->Process(input, NULL, nFrames);
        
        mFirstNewBufferProcessed = true;
        
#if DEBUG_PRINTF
        fprintf(stderr, "swap convolvers\n");
#endif
        
        //mCurrentConvolverIndex = newConvIdx;
        
        return res;
    }
#else
    // When not using it, we have some crackles
    int newConvIdx0 = (mCurrentConvolverIndex + 1) % 2;
    
    // Set the response
    if (mNewResponse.GetSize() > 0)
    {
        mConvolvers[newConvIdx0]->SetResponse(&mNewResponse);
        mNewResponse.Resize(0);
    }
#endif
    
#if DEBUG_PRINTF
    fprintf(stderr, "do the fade\n");
#endif
    
    // Else we still are not in stable state, but we have all the data to make the fade
    
    // Make a fade between the old convolver and the new convolver
    
    // Get the convolvers indices
    int newConvIdx = (mCurrentConvolverIndex + 1) % 2;
    
    // Prepare two buffers
    WDL_TypedBuf<double> prevOutput;
    prevOutput.Resize(nFrames);
   
    WDL_TypedBuf<double> newOutput;
    newOutput.Resize(nFrames);
    
    double res0 = mConvolvers[mCurrentConvolverIndex]->Process(input, prevOutput.Get(), nFrames);
    double res1 = mConvolvers[newConvIdx]->Process(input, newOutput.Get(), nFrames);
    
    // Make the fade and copy to the output
    
    
    // Fade partially, the rest full (prev or next)
    // This avoids some remaining little clics
#if 0 // Good
#define FADE_START 0.25
#define FADE_END   0.75
#endif
    
#if 0 // Worse
#define FADE_START 0.125
#define FADE_END   0.875
#endif

#if 0 // Good too (middle)
#define FADE_START 0.33
#define FADE_END   0.66
#endif

#if 0 // Seems worse
#define FADE_START 0.45
#define FADE_END   0.55
#endif

#if 1 // Medium (begin) use when using first buffer
#define FADE_START 0.0
#define FADE_END   0.25
#endif

#if 0 //  Medium (end) use when skipping first buffer
#define FADE_START 0.6
#define FADE_END   1.0
#endif

#if 0 //  Medium, bass small crackles (end) use when skipping first buffer
#define FADE_START 0.75
#define FADE_END   1.0
#endif

    
    for (int i = 0; i < nFrames; i++)
    {
        double prevVal = prevOutput.Get()[i];
        double newVal = newOutput.Get()[i];
        
        // Simple
        //double t = ((double)i)/(nFrames - 1);
        
        // Fades only on the part of the frame
        double t = 0.0;
        if ((i >= nFrames*FADE_START) &&
           (i < nFrames*FADE_END))
        {
            t = (i - nFrames*FADE_START)/(nFrames*(FADE_END - FADE_START));
        }
        
        if (i >= nFrames*FADE_END)
            t = 1.0;
        
        //Debug::DumpSingleValue("t.txt", t);
        
        double result = (1.0 - t)*prevVal + t*newVal;
        
        output[i] = result;
    }
    
    // Ok, we have made the fade
    // Now, set the new response to the old convolver too
    // (so it will be up to date next time)
    WDL_TypedBuf<double> newResponse;
    mConvolvers[newConvIdx]->GetResponse(&newResponse);
    mConvolvers[mCurrentConvolverIndex]->SetResponse(&newResponse);
    
   // mResponseJustChanged = false;
    mHasJustReset = false;
    mStableState = true;
    
    // Swap the convolvers
    mCurrentConvolverIndex = newConvIdx;
                  
    return (res0 && res1);
}
