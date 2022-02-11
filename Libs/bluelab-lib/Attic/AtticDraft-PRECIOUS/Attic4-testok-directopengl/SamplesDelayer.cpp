//
//  SamplesDelayer.cpp
//  Transient
//
//  Created by Apple m'a Tuer on 24/05/17.
//
//

#include "SamplesDelayer.h"

SamplesDelayer::SamplesDelayer(int nFrames)
{
    mNFrames = nFrames;
}

SamplesDelayer::~SamplesDelayer() {}

bool
SamplesDelayer::Process(const double *input, double *output, int nFrames)
{
    mSamples.Add(input, nFrames);
    
    if (mSamples.GetSize() >= mNFrames + nFrames)
    {
        // Return nFrames
        memcpy(output, mSamples.Get(), nFrames*sizeof(double));
        
        WDL_TypedBuf<double> newBuf;
        newBuf.Add(&mSamples.Get()[nFrames], mSamples.GetSize() - nFrames);
        
        mSamples = newBuf;
        
        return true;
    }
    
    return false;
}

void
SamplesDelayer::Reset()
{
    mSamples.Resize(0);
}
