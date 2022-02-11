//
//  OversampProcessObj2.cpp
//  Saturate
//
//  Created by Apple m'a Tuer on 12/11/17.
//
//

#include <Oversampler3.h>
#include <Decimator.h>

#include "OversampProcessObj2.h"

// Good around 0.45
// Too close to 0.5 => perfs drop out + big latency 
#define PASS_COEFF 0.475

//#define PASS_COEFF 0.25 // TEST


OversampProcessObj2::OversampProcessObj2(int oversampling, BL_FLOAT sampleRate)
{
    mOversampling = oversampling;
    
    mUpOversampler = new Oversampler3(oversampling, true);
    mUpOversampler->Reset(sampleRate);
    
    mDecimator = new Decimator();
    
    // Must use low value, otherwise perf drops due to big kernel
    // in Decimator
    BL_FLOAT passFrequency = sampleRate*PASS_COEFF;
    
    mDecimator->initialize(sampleRate,
                           passFrequency,
                           oversampling);
}

OversampProcessObj2::~OversampProcessObj2()
{
    delete mUpOversampler;
    delete mDecimator;
}

void
OversampProcessObj2::Reset(BL_FLOAT sampleRate)
{
    mUpOversampler->Reset(sampleRate);
    
    // Must use low value, otherwise perf drops due to big kernel
    // in Decimator
    BL_FLOAT passFrequency = sampleRate*PASS_COEFF;
    
    mDecimator->initialize(sampleRate,
                           passFrequency,
                           mOversampling);
}

void
OversampProcessObj2::Process(BL_FLOAT *input, BL_FLOAT *output, int nFrames)
{
    // Upsample
    int numUpSamples = nFrames*mUpOversampler->GetOversampling();
    const BL_FLOAT *upSamples = mUpOversampler->Resample(input, nFrames);
    
    // Copy the buffer
    if (mTmpBuf.GetSize() != numUpSamples)
        mTmpBuf.Resize(numUpSamples);
    
    BL_FLOAT *copyData = mTmpBuf.Get();
    memcpy(copyData, upSamples, numUpSamples*sizeof(BL_FLOAT));
    
    ProcessSamplesBuffer(&mTmpBuf);
    
    // Downsample (with taking care of Nyquist)
    mDecimator->decimate(mTmpBuf.Get(), output, nFrames);
}
