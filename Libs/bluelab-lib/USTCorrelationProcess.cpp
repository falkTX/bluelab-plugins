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
//  USTCorrelationProcess.cpp
//  UST
//
//  Created by applematuer on 10/18/19.
//
//

#include <USTProcess.h>

#include <BLUtils.h>

#include "USTCorrelationProcess.h"


#define NUM_SAMPLES 256

#define DECIM_FACTOR 4.0


USTCorrelationProcess::USTCorrelationProcess(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    
    mCorrelation = 0.0;
    
    mWasUpdated = true;
}

USTCorrelationProcess::~USTCorrelationProcess() {}

void
USTCorrelationProcess::Reset(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    
    mCorrelation = 0.0;
    
    mWasUpdated = true;
}

void
USTCorrelationProcess::AddSamples(const WDL_TypedBuf<BL_FLOAT> samples[2])
{
    vector<WDL_TypedBuf<BL_FLOAT> > samplesDecim;
    samplesDecim.push_back(samples[0]);
    samplesDecim.push_back(samples[1]);
    
    USTProcess::DecimateSamplesCorrelation(&samplesDecim,
                                           (BL_FLOAT)DECIM_FACTOR, mSampleRate);
    
    // Add the samples
    for (int i = 0; i < 2; i++)
    {
        mSamples[i].Add(samplesDecim[i].Get(), samplesDecim[i].GetSize());
    }
    
    if (mSamples[0].GetSize() < NUM_SAMPLES)
        // No enough samples
    {
        mWasUpdated = false;
        
        return;
    }
    
    BL_FLOAT minCorr = 1.0;
    
    // Compute correlation until we have consumed enough samples
    while(mSamples[0].GetSize() >= NUM_SAMPLES)
    {
        WDL_TypedBuf<BL_FLOAT> buf[2];
        buf[0].Add(mSamples[0].Get(), NUM_SAMPLES);
        buf[1].Add(mSamples[1].Get(), NUM_SAMPLES);
        
        BL_FLOAT corr = USTProcess::ComputeCorrelation(buf);
        
        if (corr < minCorr)
            minCorr = corr;
        
        // Consume
        for (int i = 0; i < 2; i++)
        {
            BLUtils::ConsumeLeft(&mSamples[i], NUM_SAMPLES);
        }
    }
    
    mCorrelation = minCorr;
    
    mWasUpdated = true;
}

bool
USTCorrelationProcess::WasUpdated()
{
    return mWasUpdated;
}

BL_FLOAT
USTCorrelationProcess::GetCorrelation()
{
    return mCorrelation;
}
