//
//  OversampProcessObj3.cpp
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

#include "OversampProcessObj3.h"

// Origin
#define NYQUIST_COEFF 0.5

// Debug
//#define NYQUIST_COEFF 0.45

// Origin
#define NYQUIST_FILTER_ORDER 1

// Test, to try to filter as much as SirClipper
// (not working)
//#define NYQUIST_FILTER_ORDER 4

// TEST
//#define NYQUIST_FILTER_ORDER 32

// TEST2
//#define NYQUIST_FILTER_ORDER 100

// For Sinc and convolution
//

//#define SINC_FILTER_SIZE 16, 64, 128
//#define SINC_FILTER_SIZE 64 // ORIGIN, less steep
#define SINC_FILTER_SIZE 256 // GOOD, steepness like Sir Clipper
//#define SINC_FILTER_SIZE 1024, 4096


OversampProcessObj3::OversampProcessObj3(int oversampling, BL_FLOAT sampleRate,
                                         bool filterNyquist)
{
    mFilter = NULL;
    
    if (filterNyquist)
    {
#if OVERSAMP_USE_RBJ_FILTER
        mFilter = new FilterRBJNX(NYQUIST_FILTER_ORDER,
                                 FILTER_TYPE_LOWPASS,
                                 sampleRate*oversampling,
                                 sampleRate*NYQUIST_COEFF);
#endif
        
#if USE_IIR_FILTER
        mFilter = new FilterIIRLow12dBNX(NYQUIST_FILTER_ORDER);
        mFilter->Init(sampleRate*NYQUIST_COEFF, sampleRate*oversampling);
#endif
        
#if USE_BUTTERWORTH_FILTER
        mFilter = new FilterButterworthLPFNX(NYQUIST_FILTER_ORDER);
        mFilter->Init(sampleRate*NYQUIST_COEFF, sampleRate*oversampling);
#endif
        
#if USE_SINC_CONVO_FILTER
        mFilter = new FilterSincConvoLPF();
        mFilter->Init(sampleRate*NYQUIST_COEFF, sampleRate*oversampling, SINC_FILTER_SIZE);
#endif
        
#if USE_FFT_LOW_PASS_FILTER
        mFilter = new FilterFftLowPass();
        mFilter->Init(sampleRate*NYQUIST_COEFF, sampleRate*oversampling);
#endif
    }
    
    mOversampling = oversampling;
    
    int dummyBlockSize = 1024;
    
    mUpOversampler = new OVERSAMPLER_CLASS(oversampling, true);
    mUpOversampler->Reset(sampleRate, dummyBlockSize);
    
    mDownOversampler = new OVERSAMPLER_CLASS(oversampling, false);
    mDownOversampler->Reset(sampleRate, dummyBlockSize);
}

OversampProcessObj3::~OversampProcessObj3()
{
    delete mUpOversampler;
    delete mDownOversampler;
    
    if (mFilter != NULL)
        delete mFilter;
}

void
OversampProcessObj3::Reset(BL_FLOAT sampleRate, int blockSize)
{
    mUpOversampler->Reset(sampleRate, blockSize);
    mDownOversampler->Reset(sampleRate, blockSize);
    
    if (mFilter != NULL)
    {
#if OVERSAMP_USE_RBJ_FILTER
        mFilter->SetSampleRate(sampleRate*mOversampling);
        mFilter->SetCutoffFreq(sampleRate*NYQUIST_COEFF);
#endif
        
#if USE_IIR_FILTER
        mFilter->Init(sampleRate*NYQUIST_COEFF, sampleRate*mOversampling);
#endif
        
#if USE_BUTTERWORTH_FILTER
        mFilter->Init(sampleRate*NYQUIST_COEFF, sampleRate*mOversampling);
#endif
        
#if USE_SINC_CONVO_FILTER
        mFilter->Init(sampleRate*NYQUIST_COEFF, sampleRate*mOversampling, SINC_FILTER_SIZE);
#endif
        
#if USE_FFT_LOW_PASS_FILTER
        mFilter->Init(sampleRate*NYQUIST_COEFF, sampleRate*mOversampling);
#endif
    }
    
#if USE_SINC_CONVO_FILTER
    if (mFilter != NULL)
    {
        mFilter->Reset(sampleRate, blockSize);
    }
#endif
}

int
OversampProcessObj3::GetLatency()
{
    int latency = 0;
    
    latency += mUpOversampler->GetLatency();
    latency += mDownOversampler->GetLatency();
    
#if USE_SINC_CONVO_FILTER
    latency += mFilter->GetLatency();
#endif

#if USE_FFT_LOW_PASS_FILTER
    latency += mFilter->GetLatency();
#endif

    return latency;
}

void
OversampProcessObj3::Process(WDL_TypedBuf<BL_FLOAT> *ioBuffer)
{
    if (mOversampling == 1)
        // Nothing to do
    {
        
        ProcessSamplesBuffer(ioBuffer);
        
        return;
    }
    
    // Upsample
    mUpOversampler->Resample(ioBuffer);
    
#if 1
    ProcessSamplesBuffer(ioBuffer);
#endif
    
#if 1
    // Filter Nyquist ?
    if (mFilter != NULL)
    {
        WDL_TypedBuf<BL_FLOAT> filtBuffer;
        mFilter->Process(&filtBuffer, *ioBuffer);
        
        *ioBuffer = filtBuffer; // New fix
    }
#endif
    
    mDownOversampler->Resample(ioBuffer);
}
