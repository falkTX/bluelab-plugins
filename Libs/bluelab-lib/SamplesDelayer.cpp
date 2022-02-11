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
SamplesDelayer::Process(const BL_FLOAT *input, BL_FLOAT *output, int nFrames)
{
    mSamples.Add(input, nFrames);
    
    if (mSamples.GetSize() >= mNFrames + nFrames)
    {
        // Return nFrames
        memcpy(output, mSamples.Get(), nFrames*sizeof(BL_FLOAT));
        
        WDL_TypedBuf<BL_FLOAT> newBuf;
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
