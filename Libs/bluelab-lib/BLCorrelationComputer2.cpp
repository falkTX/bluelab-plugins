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
//  BLCorrelationComputer2.cpp
//  
//
//  Created by applematuer on 1/2/20.
//
//

#include <Bufferizer.h>

#include "BLCorrelationComputer2.h"

// Tested with StereoWidth correlation meter
// May avoid values jittering a little
#define USE_BUFFERIZER 1 //0
#define INPUT_BUFFER_SIZE 512

BLCorrelationComputer2::BLCorrelationComputer2(BL_FLOAT sampleRate,
                                               BL_FLOAT smoothTimeMs)
{
    mSampleRate = sampleRate;
    mSmoothTimeMs = smoothTimeMs;

#if USE_BUFFERIZER
    for (int i = 0; i < 2; i++)
        mBufferizers[i] = new Bufferizer(INPUT_BUFFER_SIZE);
    
    mGotFirstBuffer = false;
#endif
    
    Reset(sampleRate);
}

BLCorrelationComputer2::~BLCorrelationComputer2()
{
#if USE_BUFFERIZER
    for (int i = 0; i < 2; i++)
        delete mBufferizers[i];
#endif
}

void
BLCorrelationComputer2::Reset(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    
    mCorrelation = 0.0;
    
    mHistorySize = mSmoothTimeMs*0.001*mSampleRate;
    
    mXLXR.unfreeze();
    mXL2.unfreeze();
    mXR2.unfreeze();
    
    mXLXR.clear();
    mXL2.clear();
    mXR2.clear();
    
    // Fill the histories with zeros
    mXLXR.resize(mHistorySize);
    mXL2.resize(mHistorySize);
    mXR2.resize(mHistorySize);

    // New
    mXLXR.freeze();
    mXL2.freeze();
    mXR2.freeze();
    
    for (int i = 0; i < mHistorySize; i++)
    {
        mXLXR[i] = 0.0;
        mXL2[i] = 0.0;
        mXR2[i] = 0.0;
    }
    
    mSumXLXR = 0.0;
    mSumXL2 = 0.0;
    mSumXR2 = 0.0;

#if USE_BUFFERIZER
    for (int i = 0; i < 2; i++)
        mBufferizers[i]->Reset();
    
    mGotFirstBuffer = false;
#endif
}

void
BLCorrelationComputer2::Reset()

{
    Reset(mSampleRate);
}

void
BLCorrelationComputer2::Process(const WDL_TypedBuf<BL_FLOAT> samples0[2])
{
#if 0 // origin
    // Fill the history
    for (int i = 0; i < samples[0].GetSize(); i++)
    {
        BL_FLOAT l = samples[0].Get()[i];
        BL_FLOAT r = samples[1].Get()[i];

        BL_FLOAT xLxR = l*r;
        mXLXR.push_back(xLxR);
        
        BL_FLOAT xL2 = l*l;
        mXL2.push_back(xL2);
        
        BL_FLOAT xR2 = r*r;
        mXR2.push_back(xR2);
        
        if (mXLXR.size() >= 2)
        {
            mSumXLXR += mXLXR[mXLXR.size() - 1] - mXLXR[0];
            mSumXL2 += mXL2[mXL2.size() - 1] - mXL2[0];
            mSumXR2 += mXR2[mXR2.size() - 1] - mXR2[0];
        }
        
        while(mXLXR.size() > mHistorySize)
            mXLXR.pop_front();
        while(mXL2.size() > mHistorySize)
            mXL2.pop_front();
        while(mXR2.size() > mHistorySize)
            mXR2.pop_front();
    }
#endif

    // Use bufferizer?
    WDL_TypedBuf<BL_FLOAT> *samples = mTmpBuf0;
    samples[0] = samples0[0];
    samples[1] = samples0[1];

#if USE_BUFFERIZER
    for (int i = 0; i < 2; i++)
        mBufferizers[i]->AddValues(samples0[i]);
    
    // Be sure to totally flush the bufferizer
    // Will avoid indefinitely growing the bufferizer if the block size is greater
    // than the bufferizer capacity.
    bool stopFlush = false;
    // Test if we have gotten samples from the bufferizer
    bool buffered = false;
    while(!stopFlush)
    {
        for (int i = 0; i < 2; i++)
        {
            bool res = mBufferizers[i]->GetBuffer(&samples[i]);
            if (res)
                buffered = true;

            if (!res)
            {
                stopFlush = true;
                break;
            }
        }
    }

    if (!buffered && mGotFirstBuffer)
        // Keep the previous value computed from the bufferizer
        return;
    
    if (buffered)
        mGotFirstBuffer = true;
#endif
    
    // Here, either we have samples from the bufferizer,
    // or we are at the beginning a,d we start computing correlation from
    // the few samples we have
    
    // Fill the history
    for (int i = 0; i < samples[0].GetSize(); i++)
    {
        BL_FLOAT l = samples[0].Get()[i];
        BL_FLOAT r = samples[1].Get()[i];

        BL_FLOAT xLxR = l*r;
        BL_FLOAT xL2 = l*l;
        BL_FLOAT xR2 = r*r;
        
        if (mXLXR.size() >= 2)
        {
            mSumXLXR += xLxR - mXLXR[0];
            mSumXL2 += xL2 - mXL2[0];
            mSumXR2 += xR2 - mXR2[0];
        }

        if (mXLXR.size() == mHistorySize)
            mXLXR.push_pop(xLxR);
        else
            mXLXR.push_back(xLxR);

        if (mXL2.size() == mHistorySize)
            mXL2.push_pop(xL2);
        else
            mXL2.push_back(xL2);

        if (mXR2.size() == mHistorySize)
            mXR2.push_pop(xR2);
        else
            mXR2.push_back(xR2);
    }

    // Just in case
    while(mXLXR.size() > mHistorySize)
        mXLXR.pop_front();
    while(mXL2.size() > mHistorySize)
        mXL2.pop_front();
    while(mXR2.size() > mHistorySize)
        mXR2.pop_front();
        
    // Compute the expectation (aka the averages)
    BL_FLOAT ExLxR = 0.0;
    BL_FLOAT ExL2 = 0.0;
    BL_FLOAT ExR2 = 0.0;
    
    if (mXLXR.size() > 0.0)
    {
        ExLxR = mSumXLXR/mXLXR.size();
        ExL2 = mSumXL2/mXL2.size();
        ExR2 = mSumXR2/mXR2.size();
    }
    
    // Compute the correlation
    BL_FLOAT corr = 0.0;
    if (ExL2*ExR2 > 0.0)
        corr = ExLxR/sqrt(ExL2*ExR2);
    
    mCorrelation = corr;
}

void
BLCorrelationComputer2::Process(BL_FLOAT l, BL_FLOAT r)
{
    // Fill the history
    BL_FLOAT xLxR = l*r;
    if (mXLXR.size() == mHistorySize)
        mXLXR.push_pop(xLxR);
    else
        mXLXR.push_back(xLxR);
        
    BL_FLOAT xL2 = l*l;
    if (mXL2.size() == mHistorySize)
        mXL2.push_pop(xL2);
    else
        mXL2.push_back(xL2);
        
    BL_FLOAT xR2 = r*r;
    if (mXR2.size() == mHistorySize)
        mXR2.push_pop(xR2);
    else
        mXR2.push_back(xR2);
    
    if (mXLXR.size() >= 2)
    {
        mSumXLXR += mXLXR[mXLXR.size() - 1] - mXLXR[0];
        mSumXL2 += mXL2[mXL2.size() - 1] - mXL2[0];
        mSumXR2 += mXR2[mXR2.size() - 1] - mXR2[0];
    }
    
    while(mXLXR.size() > mHistorySize)
        mXLXR.pop_front();
    while(mXL2.size() > mHistorySize)
        mXL2.pop_front();
    while(mXR2.size() > mHistorySize)
        mXR2.pop_front();
    
    // Compute the expectation (aka the averages)
    BL_FLOAT ExLxR = 0.0;
    BL_FLOAT ExL2 = 0.0;
    BL_FLOAT ExR2 = 0.0;
    
    if (mXLXR.size() > 0.0)
    {        
        // Assume that the 3 sizes are the same
        BL_FLOAT sizeInv = 1.0/mXLXR.size();
        ExLxR = mSumXLXR*sizeInv;
        ExL2 = mSumXL2*sizeInv;
        ExR2 = mSumXR2*sizeInv;
    }
    
    // Compute the correlation
    BL_FLOAT corr = 0.0;
    if (ExL2*ExR2 > 0.0)
        corr = ExLxR/sqrt(ExL2*ExR2);
    
    mCorrelation = corr;
}

BL_FLOAT
BLCorrelationComputer2::GetCorrelation()
{
    return mCorrelation;
}

BL_FLOAT
BLCorrelationComputer2::GetSmoothWindowMs()
{
    return mSmoothTimeMs;
}
