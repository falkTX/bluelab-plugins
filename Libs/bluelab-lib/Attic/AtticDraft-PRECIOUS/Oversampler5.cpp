//
//  Oversampler5.cpp
//  UST
//
//  Created by applematuer on 8/12/20.
//
//

#include <CDSPResampler.h>

#include <Utils.h>

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

double *
Oversampler5::GetOutEmptyBuffer()
{
    return mOutEmptyBuf.Get();
}

int
Oversampler5::GetLatency()
{
    // 0
    int latency = mResampler->getLatency();
    
    //double latencyFrac = mResampler->getLatencyFrac();
    
    return latency;
}

void
Oversampler5::Reset(double sampleRate, int blockSize)
{
    mSampleRate = sampleRate;
    
    if (mResampler != NULL)
        delete mResampler;
    
    double overSampleRate = sampleRate*mOversampling;
    if (mUpSample)
    {
        mResampler = new r8b::CDSPResampler24(sampleRate, overSampleRate, blockSize);
    }
    else
    {
        mResampler = new r8b::CDSPResampler24(overSampleRate, sampleRate, blockSize*mOversampling);
    }
}

void
Oversampler5::Resample(WDL_TypedBuf<double> *ioBuf)
{
    // Current buf
    WDL_TypedBuf<double> srcBuf = *ioBuf;

    long long int ol = mUpSample ? srcBuf.GetSize()*mOversampling : srcBuf.GetSize()/mOversampling;
    ioBuf->Resize(ol);
    
    int outPos = 0;
    while(ol > 0)
    {
        double *opp;
		int writeCount = mResampler->process(srcBuf.Get(), srcBuf.GetSize(), opp);
        
        if(writeCount > ol)
			writeCount = (int)ol;
		
        memcpy(&ioBuf->Get()[outPos], opp, writeCount*sizeof(double));
        outPos += writeCount;
        
		ol -= writeCount;
    }
}
