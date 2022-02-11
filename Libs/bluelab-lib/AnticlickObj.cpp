//
//  AnticlickObj.cpp
//  Denoiser
//
//  Created by Apple m'a Tuer on 06/05/17.
//
//

#include <stdlib.h>

#include <Window.h>

#include "AnticlickObj.h"


AnticlickObj::AnticlickObj()
{
    mNeedProcess = false;
    mDirection = OFF_TO_ON;
}

AnticlickObj::~AnticlickObj() {}

void
AnticlickObj::Reset(Direction dir)
{
    mDirection = dir;
    mNeedProcess = true;
}

bool
AnticlickObj::MustProcessOnSignal()
{
    return mNeedProcess;
}

void
AnticlickObj::SetOffSignal(const vector<WDL_TypedBuf<BL_FLOAT> > &bufs)
{
    mOffSignal = bufs;
}

void
AnticlickObj::SetOnSignal(const vector<WDL_TypedBuf<BL_FLOAT> > &bufs)
{
    mOnSignal = bufs;
}

void
AnticlickObj::Process(vector<WDL_TypedBuf<BL_FLOAT> > *bufs)
{
    if (mOffSignal.size() != bufs->size())
        return;
    if (mOnSignal.size() != bufs->size())
        return;
    
    if (mNeedProcess)
    {
        for (int j = 0; j < bufs->size(); j++)
        {
            WDL_TypedBuf<BL_FLOAT> &buf = (*bufs)[j];
            
            const WDL_TypedBuf<BL_FLOAT> &onBuf = mOnSignal[j];
            const WDL_TypedBuf<BL_FLOAT> &offBuf = mOffSignal[j];
        
            if (onBuf.GetSize() != buf.GetSize())
                return;
            if (offBuf.GetSize() != buf.GetSize())
                return;
            
            for (int k = 0; k < buf.GetSize(); k++)
            {
                BL_FLOAT t = 1.0;
                if (buf.GetSize() > 1)
                    t = ((BL_FLOAT)k)/(buf.GetSize() - 1);
                
                if (mDirection == ON_TO_OFF)
                    t = 1.0 - t;
                
                BL_FLOAT val = buf.Get()[k]; // For debugging
                
                BL_FLOAT onVal = onBuf.Get()[k];
                BL_FLOAT offVal = offBuf.Get()[k];
                
                BL_FLOAT result = t*onVal + (1.0 - t)*offVal;
                buf.Get()[k] = result;
            }
        }
    }
    
    mNeedProcess = false;
}
