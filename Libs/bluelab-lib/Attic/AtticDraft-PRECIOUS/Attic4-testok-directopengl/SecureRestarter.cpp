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


SecureRestarter::SecureRestarter()
{
    mFirstTime = true;
    
    mPrevNFrames = -1;
}

SecureRestarter::~SecureRestarter() {}

void
SecureRestarter::Reset()
{
    mFirstTime = true;
}

void
SecureRestarter::Process(double *buf0, double *buf1, int nFrames)
{
    if (mFirstTime)
    {
        // Make Hanning if necessary 
        if (mPrevNFrames != nFrames)
        {
            Window::MakeHanning(nFrames, &mHanning);
            
            mPrevNFrames = nFrames;
        }
        
        // Half Hanning (on the first half of the buffers
        for (int i = 0; i < nFrames/2; i++)
        {
            if (buf0 != NULL)
                buf0[i] *= mHanning.Get()[i];
            
            if (buf1 != NULL)
                buf1[i] *= mHanning.Get()[i];
        }
    }
    
    mFirstTime = false;
}
