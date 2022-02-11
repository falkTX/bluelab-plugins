//
//  Smooth.cpp
//  Transient
//
//  Created by Apple m'a Tuer on 21/05/17.
//
//

#include <Window.h>

#include "Smoother.h"

void
Smoother::Smooth(const WDL_TypedBuf<BL_FLOAT> *iBuf, WDL_TypedBuf<BL_FLOAT> *oBuf, int smoothSize)
{
    WDL_TypedBuf<BL_FLOAT> win;
    Window::MakeHanning(smoothSize, &win);
    
    int bufSize = iBuf->GetSize();
    
    for (int i = 0; i < bufSize; i++)
    {
        BL_FLOAT val = 0.0;
        for (int j = -smoothSize/2; j < smoothSize/2; j++)
        {
            int x = i + j;
            
            if ((x < 0) || (x >= bufSize))
                continue;
            
            val = val + iBuf->Get()[x]*win.Get()[j + smoothSize/2];
        }
        
        oBuf->Get()[i] = val;
    }
}

void
Smoother::Smooth2(const WDL_TypedBuf<BL_FLOAT> *iBuf, WDL_TypedBuf<BL_FLOAT> *oBuf, BL_FLOAT smoothCoeff)
{
    int bufSize = iBuf->GetSize();
    
    // Copy the first value
    oBuf[0] = iBuf[0];
    
    BL_FLOAT prevVal = iBuf->Get()[0];
    for (int i = 1; i < bufSize; i++)
    {
        BL_FLOAT val = iBuf->Get()[i];
        
        BL_FLOAT result = smoothCoeff*prevVal + (1.0 - smoothCoeff)*val;
        
        oBuf->Get()[i] = result;
        
        prevVal = val;
    }
}

void
Smoother::Smooth3(const WDL_TypedBuf<BL_FLOAT> *iBuf, WDL_TypedBuf<BL_FLOAT> *oBuf, int smoothSize)
{
    WDL_TypedBuf<BL_FLOAT> win;
    Window::MakeHanning(smoothSize, &win);
    
    int bufSize = iBuf->GetSize();
    
    for (int i = 0; i < bufSize; i++)
    {
        BL_FLOAT val = 0.0;
        int numVals = 0;
        
        for (int j = -smoothSize/2; j <= smoothSize/2; j++)
        {
            int x = i + j;
            
            if ((x < 0) || (x >= bufSize))
                continue;
            
            val = val + iBuf->Get()[x];
            numVals++;
        }
        
        if (numVals > 0)
            val = val / numVals;
        
        oBuf->Get()[i] = val;
    }
}
