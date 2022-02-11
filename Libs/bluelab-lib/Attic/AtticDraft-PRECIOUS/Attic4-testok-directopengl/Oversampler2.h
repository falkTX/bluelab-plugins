//
//  Oversampler.h
//  Denoiser
//
//  Created by Apple m'a Tuer on 26/04/17.
//
//

#ifndef __Denoiser__Oversampler2__
#define __Denoiser__Oversampler2__

#include "../../WDL/IPlug/Containers.h"
#include "../../WDL/resample.h"

class Oversampler2
{
public:
    Oversampler2(int oversampling);
    virtual ~Oversampler2();
    
    const double *Upsample(double *inBuf, int size);
    const double *Downsample(double *inBuf, int size);
    
    double *GetOutEmptyBuffer();
    
    void Reset(double sampleRate);
    
protected:
    void Resample(const double *src, int srcSize, double srcRate,
                  double *dst, int dstSize, double dstRate);
    
    int mOversampling;
    
    WDL_TypedBuf<double> mResultBuf;
    WDL_TypedBuf<double> mOutEmptyBuf;
    
    double mSampleRate;
    WDL_Resampler mResampler;
};

#endif /* defined(__Denoiser__Oversampler2__) */
