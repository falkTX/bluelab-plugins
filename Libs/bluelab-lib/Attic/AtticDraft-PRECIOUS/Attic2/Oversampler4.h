//
//  Oversampler4.h
//  Denoiser
//
//  Created by Apple m'a Tuer on 26/04/17.
//
//

#ifndef __Denoiser__Oversampler4__
#define __Denoiser__Oversampler4__

#include "../../WDL/IPlug/Containers.h"
#include "../../WDL/resample.h"

// For FIX_CONTINUITY_LINERP
#define CONTINUITY_LERP_HISTO_SIZE 4 //8 //4 //2

// See IPlugResampler
// Continuous resampler
// Does not need OLA
//
// Oversampler4: from Oversampler3
// - fixed click in the first buffer (detected in UST clipper)
class Oversampler4
{
public:
    // Up or downsample
    Oversampler4(int oversampling, bool upSample);
    
    virtual ~Oversampler4();
    
    void Resample(WDL_TypedBuf<double> *ioBuf);
    
    int GetOversampling();
    
    double *GetOutEmptyBuffer();
    
    void Reset(double sampleRate);
    
protected:
    int mOversampling;
    
    WDL_TypedBuf<double> mResultBuf;
    WDL_TypedBuf<double> mOutEmptyBuf;
    
    double mSampleRate;
    WDL_Resampler mResampler;
    
    // Tell if we upsample or downsample
    bool mUpSample;
    
    double mRemainingSamples;
    
    // For good interpolation continuity between buffers.
    double mPrevSampValues[CONTINUITY_LERP_HISTO_SIZE];
};

#endif /* defined(__Denoiser__Oversampler4__) */
