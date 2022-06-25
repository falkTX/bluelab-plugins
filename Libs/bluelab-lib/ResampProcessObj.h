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
//  ResampProcessObj.h
//  Saturate
//
//  Created by Apple m'a Tuer on 12/11/17.
//
//

#ifndef __Saturate__ResampProcessObj__
#define __Saturate__ResampProcessObj__

#include <vector>
using namespace std;

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

// WDL
#define OVERSAMPLER_CLASS Oversampler4

// r8brain
//#define OVERSAMPLER_CLASS Oversampler5

class Oversampler4;
class Oversampler5;

class FilterRBJNX;
class FilterIIRLow12dBNX;
class FilterButterworthLPFNX;
class FilterSincConvoLPF;
class FilterFftLowPass;

// Good, but less steep than sinc
#define USE_RBJ_FILTER 0

// Test: Use NIIRFilterLow12dB
// Good, but less steep than sinc
#define USE_IIR_FILTER 0

// NOTE: butterworth has a bug:
// at the early beginning, there is a lot of sound of very
// high amplitude
// NOTE: Butterworth is not so steep, even when chaining 100 filters
#define USE_BUTTERWORTH_FILTER 0

// GOOD: exactly like the result of Sir Clipper
#define USE_SINC_CONVO_FILTER 1

// Ultra-steep, but introduce plugin latency
#define USE_FFT_LOW_PASS_FILTER 0 //1

// From OversampProcessObj3
// Use target sample rate instead of int oversampling factor
class ResampProcessObj
{
public:
    ResampProcessObj(BL_FLOAT targetSampleRate, BL_FLOAT sampleRate,
                     bool filterNyquist);
    
    virtual ~ResampProcessObj();
    
    void Reset(BL_FLOAT sampleRate, int blockSize);
    
    int GetLatency();
    
    void Process(vector<WDL_TypedBuf<BL_FLOAT> > *ioBuffers);
    
protected:
    // Return true if the result data is stored in ioResampBuffer
    //
    // Return false ioBuffer has been directly modified
    // and we don't ned to upsample ioResampBuffer after this call.
    virtual bool ProcessSamplesBuffers(vector<WDL_TypedBuf<BL_FLOAT> > *ioBuffers,
                                       vector<WDL_TypedBuf<BL_FLOAT> > *ioResampBuffers) = 0;
    
    void InitFilters(BL_FLOAT sampleRate);
    
    //
    OVERSAMPLER_CLASS *mForwardResamplers[2];
    OVERSAMPLER_CLASS *mBackwardResamplers[2];
    
#if USE_RBJ_FILTER
    FilterRBJNX *mFilters[2];
#endif
    
#if USE_IIR_FILTER
    FilterIIRLow12dBNX *mFilters[2];
#endif
    
#if USE_BUTTERWORTH_FILTER
    FilterButterworthLPFNX *mFilters[2];
#endif
    
#if USE_SINC_CONVO_FILTER
    FilterSincConvoLPF *mFilters[2];
#endif
    
#if USE_FFT_LOW_PASS_FILTER
    FftLowPassFilter *mFilters[2];
#endif
    
    BL_FLOAT mSampleRate;
    BL_FLOAT mTargetSampleRate;

private:
    // Tmp buffers
    vector<WDL_TypedBuf<BL_FLOAT> > mTmpBuf0;
    vector<WDL_TypedBuf<BL_FLOAT> > mTmpBuf1;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf2;
};

#endif /* defined(__Saturate__ResampProcessObj__) */
