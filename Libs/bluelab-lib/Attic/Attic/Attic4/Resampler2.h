//
//  Oversampler.h
//  Denoiser
//
//  Created by Apple m'a Tuer on 26/04/17.
//
//

#ifndef __Denoiser__Resampler2__
#define __Denoiser__Resampler2__

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"
#include "../../WDL/resample.h"

// From Oversampler3
// Handles BL_FLOAT instead of floats, opposite to Resampler
// Warning: be careful for Nyquist theorem (this may not be managed here...)
class Resampler2
{
public:
    // Up or downsample
    Resampler2(BL_FLOAT inSampleRate, BL_FLOAT outSampleRate);
    
    virtual ~Resampler2();
    
    void Resample(const WDL_TypedBuf<BL_FLOAT> *input, WDL_TypedBuf<BL_FLOAT> *output);

    // For Spatializer
    //
    // Do not set all to zero in case we don't have enough out samples
    void Resample2(const WDL_TypedBuf<BL_FLOAT> *input, WDL_TypedBuf<BL_FLOAT> *output);
    
    void Reset(BL_FLOAT inSampleRate, BL_FLOAT outSampleRate);
    
    static void Resample(const WDL_TypedBuf<BL_FLOAT> *input,
                         WDL_TypedBuf<BL_FLOAT> *output,
                         BL_FLOAT inSampleRate, BL_FLOAT outSampleRate);
    
    static void Resample2(const WDL_TypedBuf<BL_FLOAT> *input,
                          WDL_TypedBuf<BL_FLOAT> *output,
                          BL_FLOAT inSampleRate, BL_FLOAT outSampleRate);
    
#if 0 // Buggy, not tested
    static void Resample(const WDL_TypedBuf<BL_FLOAT> *input,
                         WDL_TypedBuf<BL_FLOAT> *output,
                         int srcSize, int dstSize);
#endif
    
protected:
    void Resample(const BL_FLOAT *src, int srcSize, BL_FLOAT *dst, int dstSize);
    
    // For Spatializer
    void Resample2(const BL_FLOAT *src, int srcSize, BL_FLOAT *dst, int dstSize);
    
    // Resampler (is continuous, no need OLA)
    WDL_Resampler mResampler;
    
    BL_FLOAT mInSampleRate;
    BL_FLOAT mOutSampleRate;
};

#endif /* defined(__Denoiser__Resampler2__) */
