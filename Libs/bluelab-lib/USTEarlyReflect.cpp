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
//  USTEarlyReflect.cpp
//  UST
//
//  Created by applematuer on 12/3/19.
//
//

#include <revmodel.hpp>

#include "USTEarlyReflect.h"


USTEarlyReflect::USTEarlyReflect(BL_FLOAT sampleRate)
{
    mRevModel = new revmodel();
    
    mRevModel->init(sampleRate);
    InitRevModel();
}

USTEarlyReflect::~USTEarlyReflect()
{
    delete mRevModel;
}

void
USTEarlyReflect::Reset(BL_FLOAT sampleRate)
{
    // Re-create, otherwise we have nan with 88200Hz sample rate
    //delete mRevModel;
    //mRevModel = new revmodel();
    
    mRevModel->init(sampleRate);
    InitRevModel();
}

void
USTEarlyReflect::Process(const WDL_TypedBuf<BL_FLOAT> &input,
                         WDL_TypedBuf<BL_FLOAT> *outputL,
                         WDL_TypedBuf<BL_FLOAT> *outputR,
                         BL_FLOAT revGain)
{
    outputL->Resize(input.GetSize());
    outputR->Resize(input.GetSize());
    
    for (int i = 0; i < input.GetSize(); i++)
    {
        BL_FLOAT in = input.Get()[i];
        BL_FLOAT outL;
        BL_FLOAT outR;
        
        mRevModel->process(in, outL, outR);
        
        outputL->Get()[i] = outL*revGain;
        outputR->Get()[i] = outR*revGain;
    }
}

void
USTEarlyReflect::InitRevModel()
{
    //mRevModel->setroomsize(0.5);
    mRevModel->setroomsize(0.1);
    //mRevModel->setdamp(0.5);
    mRevModel->setdamp(2.0);
    mRevModel->setwet(0.3);
    mRevModel->setdry(0.0);
    mRevModel->setwidth(1.0);
    mRevModel->setmode(0.0);
}
