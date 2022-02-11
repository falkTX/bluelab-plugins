//
//  Oversampler.h
//  Denoiser
//
//  Created by Apple m'a Tuer on 26/04/17.
//
//

#ifndef __Denoiser__Oversampler2__
#define __Denoiser__Oversampler2__

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"
//#include "../../WDL/IPlug/Containers.h"
#include "../../WDL/resample.h"

class Oversampler2
{
public:
    Oversampler2(int oversampling);
    virtual ~Oversampler2();
    
    const BL_FLOAT *Upsample(BL_FLOAT *inBuf, int size);
    const BL_FLOAT *Downsample(BL_FLOAT *inBuf, int size);
    
    int GetOversampling();
    
    BL_FLOAT *GetOutEmptyBuffer();
    
    void Reset(BL_FLOAT sampleRate);
    
protected:
    void Resample(const BL_FLOAT *src, int srcSize, BL_FLOAT srcRate,
                  BL_FLOAT *dst, int dstSize, BL_FLOAT dstRate);
    
    int mOversampling;
    
    WDL_TypedBuf<BL_FLOAT> mResultBuf;
    WDL_TypedBuf<BL_FLOAT> mOutEmptyBuf;
    
    BL_FLOAT mSampleRate;
    WDL_Resampler mResampler;
};

#endif /* defined(__Denoiser__Oversampler2__) */
