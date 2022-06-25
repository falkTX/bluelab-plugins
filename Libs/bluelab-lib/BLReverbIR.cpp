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
//  BLReverbIR.cpp
//  BL-Reverb
//
//  Created by applematuer on 1/17/20.
//
//

#include <BLReverb.h>
#include <FastRTConvolver3.h>

#include <BLUtils.h>
#include <BLUtilsMath.h>

#include "BLReverbIR.h"

// Default value, will be modified in FastRTConvolver
// depending on host buffer size
#define REVERB_IR_BUFFER_SIZE 512


BLReverbIR::BLReverbIR(BLReverb *reverb, BL_FLOAT sampleRate,
                       BL_FLOAT IRLengthSeconds)
{
    mSampleRate = sampleRate;
    
    mIRLengthSeconds = IRLengthSeconds;
    
    mReverb = reverb;
    
    int numSamples = mIRLengthSeconds*mSampleRate;
    numSamples = BLUtilsMath::NextPowerOfTwo(numSamples);
    WDL_TypedBuf<BL_FLOAT> irs[2];
    mReverb->GetIRs(irs, numSamples);
    
    for (int i = 0; i < 2; i++)
    {
        mConvolvers[i] = new FastRTConvolver3(REVERB_IR_BUFFER_SIZE,
                                              sampleRate, irs[i]);
    }
}

BLReverbIR::~BLReverbIR()
{
    for (int i = 0; i < 2; i++)
    {
        delete mConvolvers[i];
    }
}

void
BLReverbIR::Reset(BL_FLOAT sampleRate, int blockSize)
{
    mSampleRate = sampleRate;
    
    mReverb->Reset(sampleRate, blockSize);
    
    for (int i = 0; i < 2; i++)
    {
        mConvolvers[i]->Reset(sampleRate, blockSize);
    }
    
    UpdateIRs();
}

int
BLReverbIR::GetLatency()
{
    int latency = mConvolvers[0]->GetLatency();
    
    return latency;
}

void
BLReverbIR::UpdateIRs()
{
    int numSamples = mIRLengthSeconds*mSampleRate;
    numSamples = BLUtilsMath::NextPowerOfTwo(numSamples);
    WDL_TypedBuf<BL_FLOAT> irs[2];
    mReverb->GetIRs(irs, numSamples);
    
    for (int i = 0; i < 2; i++)
    {
        mConvolvers[i]->SetIR(irs[i]);
    }
}

void
BLReverbIR::Process(const WDL_TypedBuf<BL_FLOAT> &sampsIn,
                    WDL_TypedBuf<BL_FLOAT> *sampsOut0,
                    WDL_TypedBuf<BL_FLOAT> *sampsOut1)
{
    sampsOut0->Resize(sampsIn.GetSize());
    mConvolvers[0]->Process(sampsIn, sampsOut0);
    
    sampsOut1->Resize(sampsIn.GetSize());
    mConvolvers[1]->Process(sampsIn, sampsOut1);
}
