//
//  Oversampler.h
//  Denoiser
//
//  Created by Apple m'a Tuer on 26/04/17.
//
//

#ifndef __Denoiser__Oversampler2__
#define __Denoiser__Oversampler3__

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"
//#include "../../WDL/IPlug/Containers.h"
#include "../../WDL/resample.h"

// See IPlugResampler
// Continuous resampler
// Does not need OLA
class Oversampler3
{
public:
    // Up or downsample
    Oversampler3(int oversampling, bool upSample);
    
    virtual ~Oversampler3();
    
    const BL_FLOAT *Resample(BL_FLOAT *inBuf, int size);
    
    void Resample(WDL_TypedBuf<BL_FLOAT> *ioBuf);
    
    int GetOversampling();
    
    BL_FLOAT *GetOutEmptyBuffer();
    
    void Reset(BL_FLOAT sampleRate);
    
protected:
    void Resample(const BL_FLOAT *src, int srcSize, BL_FLOAT *dst, int dstSize);
    
    const BL_FLOAT *Resample2(BL_FLOAT *inBuf, int size);
    
    int mOversampling;
    
    WDL_TypedBuf<BL_FLOAT> mResultBuf;
    WDL_TypedBuf<BL_FLOAT> mOutEmptyBuf;
    
    BL_FLOAT mSampleRate;
    WDL_Resampler mResampler;
    
    // Tell if we upsample or downsample
    bool mUpSample;
};

#endif /* defined(__Denoiser__Oversampler3__) */
