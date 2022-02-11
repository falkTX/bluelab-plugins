//
//  FifoDecimator.h
//  BL-TransientShaper
//
//  Created by Pan on 11/04/18.
//
//

#ifndef __BL_TransientShaper__FifoDecimator__
#define __BL_TransientShaper__FifoDecimator__

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

class FifoDecimator
{
public:
    // If we want to process samples (waveform, set isSamples to true)
    FifoDecimator(long maxSize, BL_FLOAT decimFactor, bool isSamples);
    
    FifoDecimator(bool isSamples);
    
    virtual ~FifoDecimator();
    
    void Reset();
    
    void SetParams(long maxSize, BL_FLOAT decimFactor);
    
    void SetParams(long maxSize, BL_FLOAT decimFactor, bool isSamples);
    
    void AddValues(const WDL_TypedBuf<BL_FLOAT> &samples);
    
    void GetValues(WDL_TypedBuf<BL_FLOAT> *values);
    
protected:
    long mMaxSize;
    BL_FLOAT mDecimFactor;
    bool mIsSamples;
    
    WDL_TypedBuf<BL_FLOAT> mValues;
};

#endif /* defined(__BL_TransientShaper__FifoDecimator__) */
