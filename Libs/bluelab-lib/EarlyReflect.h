//
//  EarlyReflect.h
//  UST
//
//  Created by applematuer on 1/16/20.
//
//

#ifndef UST_EarlyReflect_h
#define UST_EarlyReflect_h

#include "IPlug_include_in_plug_hdr.h"

extern "C" {
#include <reverb.h>
}

class EarlyReflect
{
public:
    EarlyReflect(BL_FLOAT sampleRate, BL_FLOAT factor = 1.0, BL_FLOAT width = 0.0);
    
    virtual ~EarlyReflect();
    
    void Reset(BL_FLOAT sampleRate);
    
    void Process(const WDL_TypedBuf<BL_FLOAT> &input,
                 WDL_TypedBuf<BL_FLOAT> *outputL,
                 WDL_TypedBuf<BL_FLOAT> *outputR);
    
protected:
    sf_rv_earlyref_st mRev;
    
    BL_FLOAT mSampleRate;
    BL_FLOAT mFactor;
    BL_FLOAT mWidth;
};

#endif
