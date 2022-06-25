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
//  FftConvolverSmooth.cpp
//  Spatializer
//
//  Created by Pan on 19/12/17.
//
//

#include <FftConvolver3.h>

#include "FftConvolverSmooth.h"

FftConvolverSmooth::FftConvolverSmooth(int bufferSize, bool normalize)
{
    mConvolvers[0] = new FftConvolver3(bufferSize, normalize);
    mConvolvers[1] = new FftConvolver3(bufferSize, normalize);
    
    mCurrentConvolverIndex = 0;
    mResponseJustChanged = false;
    mHasJustReset = true;
}

FftConvolverSmooth::~FftConvolverSmooth()
{
    delete mConvolvers[0];
    delete mConvolvers[1];
}

void
FftConvolverSmooth::Reset()
{
    mConvolvers[0]->Reset();
    mConvolvers[1]->Reset();
    
    mCurrentConvolverIndex = 0;
    mResponseJustChanged = false;
    mHasJustReset = true;
}

void
FftConvolverSmooth::SetResponse(const WDL_TypedBuf<BL_FLOAT> *response)
{
    if (mHasJustReset)
        // Normal behavior
    {
        mConvolvers[mCurrentConvolverIndex]->SetResponse(response);
        
        mHasJustReset = false;
        
        return;
    }
    
    int newConvolverId = (mCurrentConvolverIndex + 1) % 2;
    mConvolvers[newConvolverId]->SetResponse(response);
    
    mCurrentConvolverIndex = newConvolverId;
    
    mResponseJustChanged = true;
}

bool
FftConvolverSmooth::Process(BL_FLOAT *input, BL_FLOAT *output, int nFrames)
{
    // DEBUG
    //return mConvolvers[1]->Process(input, output, nFrames);
    
    if (!mResponseJustChanged)
       // Normal behavior
    {
        bool res = mConvolvers[mCurrentConvolverIndex]->Process(input, output, nFrames);
        
        // Feed the other convolver with the new data
        // NOTE: changes some data actually, but not sure it's efficient...
        int otherIndex = (mCurrentConvolverIndex + 1) % 2;
        mConvolvers[otherIndex]->Process(input, NULL, nFrames);
        
        return res;
    }
    
    // Make a fade between the old convolver and the new convolver
    
    // Get the convolvers indices
    int prevConvIdx = (mCurrentConvolverIndex + 1) % 2;
    int newConvIdx = mCurrentConvolverIndex;
    
    // Prepare two buffers
    WDL_TypedBuf<BL_FLOAT> prevOutput;
    prevOutput.Resize(nFrames);
   
    WDL_TypedBuf<BL_FLOAT> newOutput;
    newOutput.Resize(nFrames);
    
    BL_FLOAT res0 = mConvolvers[prevConvIdx]->Process(input, prevOutput.Get(), nFrames);
    BL_FLOAT res1 = mConvolvers[newConvIdx]->Process(input, newOutput.Get(), nFrames);
    
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

#if 1 // Good too
#define FADE_START 0.33
#define FADE_END   0.66
#endif

#if 0 // Seems worse
#define FADE_START 0.45
#define FADE_END   0.55
#endif

    
    for (int i = 0; i < nFrames; i++)
    {
        BL_FLOAT prevVal = prevOutput.Get()[i];
        BL_FLOAT newVal = newOutput.Get()[i];
        
        // Simple
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
        
        //BLDebug::DumpSingleValue("t.txt", t);
        
        BL_FLOAT result = (1.0 - t)*prevVal + t*newVal;
        
        output[i] = result;
    }
    
    // Ok, we have made the fade
    // Now, set the new response to the old convolver too
    // (so it will be up to date next time)
    WDL_TypedBuf<BL_FLOAT> newResponse;
    mConvolvers[newConvIdx]->GetResponse(&newResponse);
    mConvolvers[prevConvIdx]->SetResponse(&newResponse);
    
    mResponseJustChanged = false;
    mHasJustReset = false;
    
    return (res0 && res1);
}
