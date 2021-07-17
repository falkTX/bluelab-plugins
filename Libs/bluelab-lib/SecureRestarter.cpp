//
//  SecureRestarter.cpp
//  Denoiser
//
//  Created by Apple m'a Tuer on 06/05/17.
//
//

#include <stdlib.h>

#include <Window.h>

#include "SecureRestarter.h"

// Limit the number of samples to fade
#define NUM_SAMPLES_FADE 32

SecureRestarter::SecureRestarter()
{
    mFirstTime = true;
    
    mPrevNumSamplesToFade = -1;
}

SecureRestarter::~SecureRestarter() {}

void
SecureRestarter::Reset()
{
    mFirstTime = true;
    
    // NEW
    //mPrevNFrames = -1;
}

void
SecureRestarter::Process(BL_FLOAT *buf0, BL_FLOAT *buf1, int nFrames)
{
    if (mFirstTime)
    {
        int numSamplesToFade = NUM_SAMPLES_FADE;
        if (nFrames < numSamplesToFade)
            numSamplesToFade = nFrames;
        
        // Make Hanning if necessary 
        if (numSamplesToFade != mPrevNumSamplesToFade)
        {
            Window::MakeHanning(numSamplesToFade, &mHanning);

            mPrevNumSamplesToFade = numSamplesToFade;
        }
        
        // Half Hanning (on the first half of the buffers)
        for (int i = 0; i < numSamplesToFade/2; i++)
        {
            if (buf0 != NULL)
                buf0[i] *= mHanning.Get()[i];
            
            if (buf1 != NULL)
                buf1[i] *= mHanning.Get()[i];
        }
    }
    
    mFirstTime = false;
}

void
SecureRestarter::Process(WDL_TypedBuf<BL_FLOAT> *buf0,
                         WDL_TypedBuf<BL_FLOAT> *buf1)
{
    Process(buf0->Get(), buf1->Get(), buf0->GetSize());
}

void
SecureRestarter::Process(vector<WDL_TypedBuf<BL_FLOAT> > &bufs)
{
    if (bufs.empty())
        return;
    
    const WDL_TypedBuf<BL_FLOAT> &buf0 = bufs[0];
    int nFrames = buf0.GetSize();

    int numSamplesToFade = NUM_SAMPLES_FADE;
    if (nFrames < numSamplesToFade)
        numSamplesToFade = nFrames;
        
    if (mFirstTime)
    {
        // Make Hanning if necessary
        if (mPrevNumSamplesToFade != numSamplesToFade)
        {
            Window::MakeHanning(numSamplesToFade, &mHanning);
            
            mPrevNumSamplesToFade = numSamplesToFade;
        }
        
        for (int j = 0; j < bufs.size(); j++)
        {
            WDL_TypedBuf<BL_FLOAT> &buf = bufs[j];
            
            // Half Hanning (on the first half of the buffers)
            for (int i = 0; i < numSamplesToFade/2; i++)
            {
                if (i < buf.GetSize())
                    buf.Get()[i] *= mHanning.Get()[i];
            }
        }
    }
    
    mFirstTime = false;
}
