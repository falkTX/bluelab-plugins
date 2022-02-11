//
//  OversamplingObj.cpp
//  Saturate
//
//  Created by Apple m'a Tuer on 12/11/17.
//
//

#include <FilterRBJNX.h>

#include "Oversampler3.h"

#include "OversampProcessObj.h"

//#define NYQUIST_COEFF 0.49 //0.45
#define NYQUIST_COEFF 0.5

OversampProcessObj::OversampProcessObj(int oversampling, BL_FLOAT sampleRate,
                                       bool filterNyquist)
{
    mFilter = NULL;
    
    if (filterNyquist)
        mFilter = new FilterRBJNX(1/*2*//*4*/, FILTER_TYPE_LOWPASS,
                                 sampleRate*oversampling,
                                 sampleRate*NYQUIST_COEFF);
    
    mOversampling = oversampling;
    
    mUpOversampler = new Oversampler3(oversampling, true);
    mUpOversampler->Reset(sampleRate);
    
    mDownOversampler = new Oversampler3(oversampling, false);
    mDownOversampler->Reset(sampleRate);
}

OversampProcessObj::~OversampProcessObj()
{
    delete mUpOversampler;
    delete mDownOversampler;
    
    if (mFilter != NULL)
        delete mFilter;
}

void
OversampProcessObj::Reset(BL_FLOAT sampleRate)
{
    mUpOversampler->Reset(sampleRate);
    mDownOversampler->Reset(sampleRate);
    
    if (mFilter != NULL)
    {
        mFilter->SetSampleRate(sampleRate*mOversampling);
        mFilter->SetCutoffFreq(sampleRate*NYQUIST_COEFF);
    }
}

void
OversampProcessObj::Process(BL_FLOAT *input, BL_FLOAT *output, int nFrames)
{
    if (mOversampling == 1)
    // Nothing to do
    {
        if (mTmpBuf.GetSize() != nFrames)
            mTmpBuf.Resize(nFrames);
        
        BL_FLOAT *copyData = mTmpBuf.Get();
        memcpy(copyData, input, nFrames*sizeof(BL_FLOAT));
        
        ProcessSamplesBuffer(&mTmpBuf);
        
        memcpy(output, copyData, nFrames*sizeof(BL_FLOAT));
        
        return;
    }
    
    // Upsample
    int numUpSamples = nFrames*mUpOversampler->GetOversampling();
    const BL_FLOAT *upSamples = mUpOversampler->Resample(input, nFrames);
    
    // Copy the buffer
    if (mTmpBuf.GetSize() != numUpSamples)
        mTmpBuf.Resize(numUpSamples);
    
    BL_FLOAT *copyData = mTmpBuf.Get();
    memcpy(copyData, upSamples, numUpSamples*sizeof(BL_FLOAT));
    
    ProcessSamplesBuffer(&mTmpBuf);
    
    // Filter Nyquist ?
    if (mFilter != NULL)
    {
        //WDL_TypedBuf<BL_FLOAT> filtBuffer;
        //mFilter->Process(&filtBuffer, mTmpBuf);
        //mTmpBuf = filtBuffer;
        
        mFilter->Process(&mTmpBuf);
        
        copyData = mTmpBuf.Get();
    }
    
    // Downsample
    const BL_FLOAT *downSamples = mDownOversampler->Resample(copyData, nFrames);
    
    memcpy(output, downSamples, nFrames*sizeof(BL_FLOAT));
}
