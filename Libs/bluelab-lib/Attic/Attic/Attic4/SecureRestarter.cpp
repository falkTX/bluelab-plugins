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
    
    // NEW
    //mPrevNFrames = -1;
}

void
SecureRestarter::Process(BL_FLOAT *buf0, BL_FLOAT *buf1, int nFrames)
{
    if (mFirstTime)
    {
        // Make Hanning if necessary 
        if (mPrevNFrames != nFrames)
        {
            Window::MakeHanning(nFrames, &mHanning);
            
            mPrevNFrames = nFrames;
        }
        
        // Half Hanning (on the first half of the buffers)
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
    
    if (mFirstTime)
    {
        // Make Hanning if necessary
        if (mPrevNFrames != nFrames)
        {
            Window::MakeHanning(nFrames, &mHanning);
            
            mPrevNFrames = nFrames;
        }
        
        for (int j = 0; j < bufs.size(); j++)
        {
            WDL_TypedBuf<BL_FLOAT> &buf = bufs[j];
            
            // Half Hanning (on the first half of the buffers)
            for (int i = 0; i < nFrames/2; i++)
            {
                if (buf.GetSize() == nFrames)
                    buf.Get()[i] *= mHanning.Get()[i];
            }
        }
    }
    
    mFirstTime = false;
}
