//
//  OversamplingObj.h
//  Saturate
//
//  Created by Apple m'a Tuer on 12/11/17.
//
//

#ifndef __Saturate__OversampProcessObj__
#define __Saturate__OversampProcessObj__

//#include "../../WDL/IPlug/Containers.h"
#include "IPlug_include_in_plug_hdr.h"

class Oversampler3;

class FilterRBJNX;

class OversampProcessObj
{
public:
    OversampProcessObj(int oversampling, BL_FLOAT sampleRate,
                       bool filterNyquist = false);
    
    virtual ~OversampProcessObj();
    
    // Must be called at least once
    void Reset(BL_FLOAT sampleRate);
    
    void Process(BL_FLOAT *input, BL_FLOAT *output, int nFrames);
    
protected:
    virtual void ProcessSamplesBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer) = 0;
    
    Oversampler3 *mUpOversampler;
    Oversampler3 *mDownOversampler;
    
    WDL_TypedBuf<BL_FLOAT> mTmpBuf;
    
    int mOversampling;
    
    FilterRBJNX *mFilter;
};

#endif /* defined(__Saturate__OversampProcessObj__) */
