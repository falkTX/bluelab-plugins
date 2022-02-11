//
//  Oversampler.h
//  Denoiser
//
//  Created by Apple m'a Tuer on 26/04/17.
//
//

#ifndef __Denoiser__Oversampler__
#define __Denoiser__Oversampler__

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"
//#include "../../WDL/IPlug/Containers.h"

#define WDL_BESSEL_FILTER_ORDER 8

// Niko: not sure this one is good
#define WDL_BESSEL_DENORMAL_AGGRESSIVE

#include "../../WDL/besselfilter.h"

class Oversampler
{
public:
    Oversampler(int oversampling);
    virtual ~Oversampler();
    
    const BL_FLOAT *Upsample(BL_FLOAT *inBuf, int size);
    const BL_FLOAT *Downsample(BL_FLOAT *inBuf, int size);
    
    BL_FLOAT *GetOutEmptyBuffer();
    
protected:
    WDL_BesselFilterCoeffs mAntiAlias;
    WDL_BesselFilterStage mUpsample, mDownsample;
    
    int mOversampling;
    
    WDL_TypedBuf<BL_FLOAT> mResultBuf;
    WDL_TypedBuf<BL_FLOAT> mOutEmptyBuf;
};

#endif /* defined(__Denoiser__Oversampler__) */
