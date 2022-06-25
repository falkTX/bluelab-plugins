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
