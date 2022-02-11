//
//  Oversampler4.h
//  Denoiser
//
//  Created by Apple m'a Tuer on 26/04/17.
//
//

#ifndef __Denoiser__Oversampler4__
#define __Denoiser__Oversampler4__

#include "IPlug_include_in_plug_hdr.h"
//#include "../../WDL/IPlug/Containers.h"
#include "../../WDL/resample.h"

#include <BLTypes.h>

// For FIX_CONTINUITY_LINERP
#define CONTINUITY_LERP_HISTO_SIZE 4

// See IPlugResampler
// Continuous resampler
// Does not need OLA
//
// Oversampler4: from Oversampler3
// - fixed click in the first buffer (detected in UST clipper)
// NOTE: good continuity, but filters a bit high frequencies when upsampling
class Oversampler4
{
public:
    // Up or downsample
    Oversampler4(BL_FLOAT oversampling, bool upSample);
    
    virtual ~Oversampler4();
    
    void Reset(BL_FLOAT sampleRate, int blockSize);
    
    int GetLatency();
    
    void Resample(WDL_TypedBuf<BL_FLOAT> *ioBuf);
    
    BL_FLOAT GetOversampling();
    
    BL_FLOAT *GetOutEmptyBuffer();
    
protected:
    BL_FLOAT mOversampling;
    
    WDL_TypedBuf<BL_FLOAT> mResultBuf;
    WDL_TypedBuf<BL_FLOAT> mOutEmptyBuf;
    
    BL_FLOAT mSampleRate;
    WDL_Resampler mResampler;
    
    // Tell if we upsample or downsample
    bool mUpSample;
    
    BL_FLOAT mRemainingSamples;
    
    // For FIX_CONTINUITY_LINERP
    //
    
    // For good interpolation continuity between buffers.
    BL_FLOAT mPrevSampValues[CONTINUITY_LERP_HISTO_SIZE];
    WDL_TypedBuf<BL_FLOAT> mTmpContinuityBuf;
};

#endif /* defined(__Denoiser__Oversampler4__) */
