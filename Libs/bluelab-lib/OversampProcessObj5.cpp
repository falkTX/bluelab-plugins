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
//  OversampProcessObj5.cpp
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

#include "OversampProcessObj5.h"

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


OversampProcessObj5::OversampProcessObj5(int oversampling, BL_FLOAT sampleRate,
                                         bool filterNyquist)
{
    mFilters[0] = NULL;
    mFilters[1] = NULL;
    
    if (filterNyquist)
    {
        for (int i = 0; i < 2; i++)
        {
#if OVERSAMP_USE_RBJ_FILTER
            mFilters[i] = new FilterRBJNX(NYQUIST_FILTER_ORDER,
                                          FILTER_TYPE_LOWPASS,
                                          sampleRate*oversampling,
                                          sampleRate*NYQUIST_COEFF);
#endif
        
#if USE_IIR_FILTER
            mFilters[i] = new FilterIIRLow12dBNX(NYQUIST_FILTER_ORDER);
            mFilters[i]->Init(sampleRate*NYQUIST_COEFF, sampleRate*oversampling);
#endif
        
#if USE_BUTTERWORTH_FILTER
            mFilters[i] = new FilterButterworthLPFNX(NYQUIST_FILTER_ORDER);
            mFilters[i]->Init(sampleRate*NYQUIST_COEFF, sampleRate*oversampling);
#endif
        
#if USE_SINC_CONVO_FILTER
            mFilters[i] = new FilterSincConvoLPF();
            mFilters[i]->Init(sampleRate*NYQUIST_COEFF, sampleRate*oversampling, SINC_FILTER_SIZE);
#endif
        
#if USE_FFT_LOW_PASS_FILTER
            mFilters[i] = new FilterFftLowPass();
            mFilters[i]->Init(sampleRate*NYQUIST_COEFF, sampleRate*oversampling);
#endif
        }
    }
    
    mOversampling = oversampling;
    
    int dummyBlockSize = 1024;

    for (int i = 0; i < 2; i++)
    {
        mUpOversamplers[i] = new OVERSAMPLER_CLASS(oversampling, true);
        mUpOversamplers[i]->Reset(sampleRate, dummyBlockSize);
        
        mDownOversamplers[i] = new OVERSAMPLER_CLASS(oversampling, false);
        mDownOversamplers[i]->Reset(sampleRate, dummyBlockSize);
    }
}

OversampProcessObj5::~OversampProcessObj5()
{
    for (int i = 0; i < 2; i++)
    {
        delete mUpOversamplers[i];
        delete mDownOversamplers[i];
        
        if (mFilters[i] != NULL)
            delete mFilters[i];
    }
}

void
OversampProcessObj5::Reset(BL_FLOAT sampleRate, int blockSize)
{
    for (int i = 0; i < 2; i++)
    {
        mUpOversamplers[i]->Reset(sampleRate, blockSize);
        mDownOversamplers[i]->Reset(sampleRate, blockSize);
    
        if (mFilters[i] != NULL)
        {
#if OVERSAMP_USE_RBJ_FILTER
            mFilters[i]->SetSampleRate(sampleRate*mOversampling);
            mFilters[i]->SetCutoffFreq(sampleRate*NYQUIST_COEFF);
#endif
        
#if USE_IIR_FILTER
            mFilters[i]->Init(sampleRate*NYQUIST_COEFF, sampleRate*mOversampling);
#endif
        
#if USE_BUTTERWORTH_FILTER
            mFilters[i]->Init(sampleRate*NYQUIST_COEFF, sampleRate*mOversampling);
#endif
        
#if USE_SINC_CONVO_FILTER
            mFilters[i]->Init(sampleRate*NYQUIST_COEFF, sampleRate*mOversampling, SINC_FILTER_SIZE);
#endif
        
#if USE_FFT_LOW_PASS_FILTER
            mFilters[i]->Init(sampleRate*NYQUIST_COEFF, sampleRate*mOversampling);
#endif
        }
    
#if USE_SINC_CONVO_FILTER
        if (mFilters[i] != NULL)
        {
            mFilters[i]->Reset(sampleRate, blockSize);
        }
#endif
    }
}

int
OversampProcessObj5::GetLatency()
{
    int latency = 0;
    
    latency += mUpOversamplers[0]->GetLatency();
    latency += mDownOversamplers[0]->GetLatency();
    
#if USE_SINC_CONVO_FILTER
    latency += mFilters[0]->GetLatency();
#endif

#if USE_FFT_LOW_PASS_FILTER
    latency += mFilters[0]->GetLatency();
#endif

    return latency;
}

void
OversampProcessObj5::Process(vector<WDL_TypedBuf<BL_FLOAT> > &in,
                             vector<WDL_TypedBuf<BL_FLOAT> > *out)
{
    if (in.empty())
        return;
    if (out->size() != in.size())
        return;
    
    if (mOversampling == 1)
        // Nothing to do
    {
        
        ProcessSamplesBuffer(in, out);
        
        return;
    }

    vector<WDL_TypedBuf<BL_FLOAT> > &inCopy = mTmpBuf0;
    inCopy = in;
    for (int i = 0; i < inCopy.size(); i++)
    {
        // Upsample
        mUpOversamplers[i]->Resample(&inCopy[i]);
    }
    
#if 1
    ProcessSamplesBuffer(inCopy, &inCopy);
#endif

    for (int i = 0; i < 2; i++)
    {
#if 1
        // Filter Nyquist ?
        if (mFilters[i] != NULL)
        {
            WDL_TypedBuf<BL_FLOAT> filtBuffer;
            mFilters[i]->Process(&filtBuffer, inCopy[i]);
        
            (*out)[i] = filtBuffer; // New fix
        }
    }
#endif

    for (int i = 0; i < 2; i++)
        mDownOversamplers[i]->Resample(&(*out)[i]);
}
