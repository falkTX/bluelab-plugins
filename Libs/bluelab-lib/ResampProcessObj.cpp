//
//  ResampProcessObj.cpp
//  Saturate
//
//  Created by Apple m'a Tuer on 12/11/17.
//
//

#include <Oversampler4.h>
#include <Oversampler5.h>

#include <FilterRBJNX.h>
#include <FilterIIRLow12dBNX.h>
#include <FilterButterworthLPFNX.h>
#include <FilterSincConvoLPF.h>
#include <FilterFftLowPass.h>

#include <BLUtils.h>
#include <BLUtilsMath.h>

#include "ResampProcessObj.h"


#define NYQUIST_COEFF 0.5 // 0.45

#define NYQUIST_FILTER_ORDER 1

#define SINC_FILTER_SIZE 64 // ORIGIN, less steep
//#define SINC_FILTER_SIZE 256 // GOOD, steepness like Sir Clipper


ResampProcessObj::ResampProcessObj(BL_FLOAT targetSampleRate,
                                   BL_FLOAT sampleRate,
                                   bool filterNyquist)
{
    mSampleRate = sampleRate;
    mTargetSampleRate = targetSampleRate;
    
    for (int i = 0; i < 2; i++)
    {
        mFilters[i] = NULL;
    
        if (filterNyquist)
        {
#if USE_RBJ_FILTER
            mFilters[i] = new FilterRBJNX(NYQUIST_FILTER_ORDER,
                                          FILTER_TYPE_LOWPASS);
#endif
        
#if USE_IIR_FILTER
            mFilters[i] = new FilterIIRLow12dBNX(NYQUIST_FILTER_ORDER);
#endif
        
#if USE_BUTTERWORTH_FILTER
            mFilters[i] = new FilterButterworthLPFNX(NYQUIST_FILTER_ORDER);
#endif
        
#if USE_SINC_CONVO_FILTER
            mFilters[i] = new FilterSincConvoLPF();
#endif
        
#if USE_FFT_LOW_PASS_FILTER
            mFilters[i] = new FilterFftLowPass();
#endif
        }
    }
    
    InitFilters(sampleRate);
    
    //
    int dummyBlockSize = 1024;
    
    BL_FLOAT oversampling = mTargetSampleRate/sampleRate;
    if (oversampling < 1.0)
        oversampling = 1.0/oversampling;
    
    bool upsampleForward = (mTargetSampleRate > sampleRate);
    
    for (int i = 0; i < 2; i++)
    {
        mForwardResamplers[i] = new OVERSAMPLER_CLASS(oversampling, upsampleForward);
        mForwardResamplers[i]->Reset(sampleRate, dummyBlockSize);
    
        mBackwardResamplers[i] = new OVERSAMPLER_CLASS(oversampling, !upsampleForward);
        mBackwardResamplers[i]->Reset(targetSampleRate, dummyBlockSize);
    }
}

ResampProcessObj::~ResampProcessObj()
{
    for (int i = 0; i < 2; i++)
    {
        if (mForwardResamplers[i] != NULL)
            delete mForwardResamplers[i];
        
        if (mBackwardResamplers[i] != NULL)
            delete mBackwardResamplers[i];
    }
    
    for (int i = 0; i < 2; i++)
    {
        if (mFilters[i] != NULL)
            delete mFilters[i];
    }
}

void
ResampProcessObj::Reset(BL_FLOAT sampleRate, int blockSize)
{
    mSampleRate = sampleRate;
    
    for (int i = 0; i < 2; i++)
    {
        mForwardResamplers[i]->Reset(sampleRate, blockSize);
        mBackwardResamplers[i]->Reset(mTargetSampleRate, blockSize);
    }
    
    InitFilters(sampleRate);
    
#if USE_SINC_CONVO_FILTER
    for (int i = 0; i < 2; i++)
    {
        if (mFilters[i] != NULL)
        {
            mFilters[i]->Reset(sampleRate, blockSize);
        }
    }
#endif
}

int
ResampProcessObj::GetLatency()
{
    int latency = 0;
    
    latency += mForwardResamplers[0]->GetLatency();
    latency += mBackwardResamplers[0]->GetLatency();
    
#if USE_SINC_CONVO_FILTER
    latency += mFilters[0]->GetLatency();
#endif

#if USE_FFT_LOW_PASS_FILTER
    latency += mFilters[0]->GetLatency();
#endif

    return latency;
}

void
ResampProcessObj::Process(vector<WDL_TypedBuf<BL_FLOAT> > *ioBuffers)
{
    if (std::fabs(mSampleRate - mTargetSampleRate) < BL_EPS)
    // Nothing to do
    {
        vector<WDL_TypedBuf<BL_FLOAT> > copyBuffers = *ioBuffers;
        ProcessSamplesBuffers(ioBuffers, &copyBuffers);
        
        return;
    }
    
    vector<WDL_TypedBuf<BL_FLOAT> > resampBuffers = *ioBuffers;
    for (int i = 0; i < ioBuffers->size(); i++)
    {
        // Upsample
        mForwardResamplers[i]->Resample(&resampBuffers[i]);
    }
    
    bool resultIsResampBuffer = ProcessSamplesBuffers(ioBuffers, &resampBuffers);
    
    if (resultIsResampBuffer)
    {
        for (int i = 0; i < ioBuffers->size(); i++)
        {
            // Filter Nyquist ?
            if (mFilters[i] != NULL)
            {
                WDL_TypedBuf<BL_FLOAT> filtBuffer;
                mFilters[i]->Process(&filtBuffer, resampBuffers[i]);
        
                resampBuffers[i] = filtBuffer;
            }
    
            mBackwardResamplers[i]->Resample(&resampBuffers[i]);
        }
        
        *ioBuffers = resampBuffers;
    }
}

void
ResampProcessObj::InitFilters(BL_FLOAT sampleRate)
{
    // Default: upsample first
    // We will use the filters when downsampling
    BL_FLOAT filterSampleRate = mTargetSampleRate;
    BL_FLOAT nyquistFreq = sampleRate*NYQUIST_COEFF;
    
    if (mTargetSampleRate < sampleRate)
        // Downsample first
    {
        filterSampleRate = sampleRate;
        nyquistFreq = mTargetSampleRate*NYQUIST_COEFF;
    }
    
    for (int i = 0; i < 2; i++)
    {
        if (mFilters[i] == NULL)
            break;
        
#if USE_RBJ_FILTER
        mFilters[i]->SetSampleRate(filterSampleRate)
        mFilters[i]->SetCutoffFrequency(nyquistFreq);
#endif
    
#if USE_IIR_FILTER
        mFilters[i]->Init(nyquistFreq, filterSampleRate);
#endif
    
#if USE_BUTTERWORTH_FILTER
        mFilters[i]->Init(nyquistFreq, filterSampleRate);
#endif
    
#if USE_SINC_CONVO_FILTER
        mFilters[i]->Init(nyquistFreq, filterSampleRate, SINC_FILTER_SIZE);
#endif
    
#if USE_FFT_LOW_PASS_FILTER
        mFilters[i]->Init(nyquistFreq, filterSampleRate);
#endif
    }
}
