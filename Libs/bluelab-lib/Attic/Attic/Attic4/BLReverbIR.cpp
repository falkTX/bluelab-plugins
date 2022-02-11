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

#include "BLReverbIR.h"

// Default value, will be modified in FastRTConvolver
// depending on host buffer size
#define BUFFER_SIZE 512


BLReverbIR::BLReverbIR(BLReverb *reverb, BL_FLOAT sampleRate,
                       BL_FLOAT IRLengthSeconds)
{
    mSampleRate = sampleRate;
    
    mIRLengthSeconds = IRLengthSeconds;
    
    mReverb = reverb;
    
    int numSamples = mIRLengthSeconds*mSampleRate;
    numSamples = BLUtils::NextPowerOfTwo(numSamples);
    WDL_TypedBuf<BL_FLOAT> irs[2];
    mReverb->GetIRs(irs, numSamples);
    
    for (int i = 0; i < 2; i++)
    {
        mConvolvers[i] = new FastRTConvolver3(BUFFER_SIZE, sampleRate, irs[i]);
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
    numSamples = BLUtils::NextPowerOfTwo(numSamples);
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
