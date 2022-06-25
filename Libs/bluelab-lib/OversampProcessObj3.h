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
//  OversampProcessObj3.h
//  Saturate
//
//  Created by Apple m'a Tuer on 12/11/17.
//
//

#ifndef __Saturate__OversampProcessObj3__
#define __Saturate__OversampProcessObj3__

#include "IPlug_include_in_plug_hdr.h"
//#include "../../WDL/IPlug/Containers.h"

//#define OVERSAMPLER_CLASS Oversampler4
#define OVERSAMPLER_CLASS Oversampler5

class Oversampler4;
class Oversampler5;

class FilterRBJNX;
class FilterIIRLow12dBNX;
class FilterButterworthLPFNX;
class FilterSincConvoLPF;
class FilterFftLowPass;

// Good, but less steep than sinc
#define OVERSAMP_USE_RBJ_FILTER 0

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


// See: https://gist.github.com/kbob/045978eb044be88fe568

// OversampProcessObj: dos not take care of Nyquist when downsampling
//
// OversampProcessObj2: use Decimator to take care of Nyquist when downsampling
// GOOD ! : avoid filtering too much high frequencies
//
// OversampProcessObj3: from OversampProcessObj2
// - use Oversampler4 (for fix click)
// - finally, adapted the code from OversampProcessObj (which loks better than OversampProcessObj2)

// NOTE: a problem was detected during UST Clipper (with Oversampler4)
// When using on a pure sine wave, clipper gain 18dB
// => there are very light vertical bars on the spectrogram, every ~2048 sample
// (otherwise it looks to work well)
//
// NOTE: This class is the best solution: using fixed Oversampler4,
// it gives very good results!
class OversampProcessObj3
{
public:
    OversampProcessObj3(int oversampling, BL_FLOAT sampleRate,
                        bool filterNyquist);
    
    virtual ~OversampProcessObj3();
    
    // Must be called at least once
    //void Reset(BL_FLOAT sampleRate);
    
    void Reset(BL_FLOAT sampleRate, int blockSize);
    
    int GetLatency();
    
    void Process(WDL_TypedBuf<BL_FLOAT> *ioBuffer);
    
protected:
    virtual void ProcessSamplesBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer) = 0;
    
    OVERSAMPLER_CLASS *mUpOversampler;
    OVERSAMPLER_CLASS *mDownOversampler;
    
#if OVERSAMP_USE_RBJ_FILTER
    FilterRBJNX *mFilter;
#endif
    
#if USE_IIR_FILTER
    FilterIIRLow12dBNX *mFilter;
#endif
    
#if USE_BUTTERWORTH_FILTER
    FilterButterworthLPFNX *mFilter;
#endif
    
#if USE_SINC_CONVO_FILTER
    FilterSincConvoLPF *mFilter;
#endif
    
#if USE_FFT_LOW_PASS_FILTER
    FftLowPassFilter *mFilter;
#endif
    
    int mOversampling;
};

#endif /* defined(__Saturate__OversampProcessObj3__) */
