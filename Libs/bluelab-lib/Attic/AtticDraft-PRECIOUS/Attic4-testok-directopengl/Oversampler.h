//
//  Oversampler.h
//  Denoiser
//
//  Created by Apple m'a Tuer on 26/04/17.
//
//

#ifndef __Denoiser__Oversampler__
#define __Denoiser__Oversampler__

#include "../../WDL/IPlug/Containers.h"

#define WDL_BESSEL_FILTER_ORDER 8

// Niko: not sure this one is good
#define WDL_BESSEL_DENORMAL_AGGRESSIVE

#include "../../WDL/besselfilter.h"

class Oversampler
{
public:
    Oversampler(int oversampling);
    virtual ~Oversampler();
    
    const double *Upsample(double *inBuf, int size);
    const double *Downsample(double *inBuf, int size);
    
    double *GetOutEmptyBuffer();
    
protected:
    WDL_BesselFilterCoeffs mAntiAlias;
    WDL_BesselFilterStage mUpsample, mDownsample;
    
    int mOversampling;
    
    WDL_TypedBuf<double> mResultBuf;
    WDL_TypedBuf<double> mOutEmptyBuf;
};

#endif /* defined(__Denoiser__Oversampler__) */
