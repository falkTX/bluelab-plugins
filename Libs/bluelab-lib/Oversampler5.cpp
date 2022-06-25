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
//  Oversampler5.cpp
//  UST
//
//  Created by applematuer on 8/12/20.
//
//

#include <CDSPResampler.h>

#include <BLUtils.h>

#include "Oversampler5.h"

Oversampler5::Oversampler5(int oversampling, bool upSample)
{
    mOversampling = oversampling;
    
    mUpSample = upSample;
    
    mSampleRate = 0;
    
    mResampler = NULL;
}

Oversampler5::~Oversampler5()
{
    if (mResampler != NULL)
        delete mResampler;
}

int
Oversampler5::GetOversampling()
{
    return mOversampling;
}

BL_FLOAT *
Oversampler5::GetOutEmptyBuffer()
{
    return mOutEmptyBuf.Get();
}

int
Oversampler5::GetLatency()
{
    // NOTE: getLatency() and getLatencyFrac() don't work
    // must use getInLenBeforeOutStart()
    
    //int latency = mResampler->getLatency();
    //BL_FLOAT latencyFrac = mResampler->getLatencyFrac();
    
    if (mResampler == NULL)
        return 0;
    
    int latency = mResampler->getInLenBeforeOutStart();
    
    if (!mUpSample)
        latency /= mOversampling;
    
    return latency;
}

void
Oversampler5::Reset(BL_FLOAT sampleRate, int blockSize)
{
    mSampleRate = sampleRate;
    
    if (mResampler != NULL)
        delete mResampler;
    
    BL_FLOAT overSampleRate = sampleRate*mOversampling;
    
    // Default
    // Latency = 3584smp
    BL_FLOAT ReqTransBand = 2.0;
    
    // See: https://github.com/avaneev/r8brain-free-src/issues/6
    // Latency = 1775smp
    // Cuts a little more high frequencies (really sharp cut)
    //BL_FLOAT ReqTransBand = 5.0;
    //BL_FLOAT ReqTransBand = 50.0;
    
    // Default
    //BL_FLOAT ReqAtten = 206.91;
    
    if (mUpSample)
    {
        mResampler = new r8b::CDSPResampler24(sampleRate, overSampleRate,
                                              blockSize, ReqTransBand);
    }
    else
    {
        mResampler = new r8b::CDSPResampler24(overSampleRate, sampleRate,
                                              blockSize*mOversampling, ReqTransBand);
    }
}

void
Oversampler5::Resample(WDL_TypedBuf<BL_FLOAT> *ioBuf)
{
    // Current buf
    WDL_TypedBuf<BL_FLOAT> srcBuf = *ioBuf;

    long long int outNumSamples = mUpSample ? srcBuf.GetSize()*mOversampling : srcBuf.GetSize()/mOversampling;
    ioBuf->Resize((int)outNumSamples);
    BLUtils::FillAllZero(ioBuf);
    
    if (mResampler == NULL)
        return;
    
#if !BL_TYPE_FLOAT // double
    BL_FLOAT *opp;
    int writeCount = mResampler->process(srcBuf.Get(), srcBuf.GetSize(), opp);
#else
    // Convert to double
    WDL_TypedBuf<double> srcBuf0;
    srcBuf0.Resize(srcBuf.GetSize());
    for (int i = 0; i < srcBuf.GetSize(); i++)
        srcBuf0.Get()[i] = srcBuf.Get()[i];
    
    // Process
    double *opp0;
    int writeCount = mResampler->process(srcBuf0.Get(), srcBuf0.GetSize(), opp0);
    
    // Convert to float
    WDL_TypedBuf<BL_FLOAT> oppBuf;
    oppBuf.Resize(writeCount);
    for (int i = 0; i < oppBuf.GetSize(); i++)
        oppBuf.Get()[i] = opp0[i];
    
    BL_FLOAT *opp = oppBuf.Get();
#endif
    
    if(writeCount > outNumSamples)
        writeCount = (int)outNumSamples;
	
	if (writeCount > 0)
    {
        int startPos = ioBuf->GetSize() - writeCount;
        
        memcpy(&ioBuf->Get()[startPos], opp, writeCount*sizeof(BL_FLOAT));
    }
}
