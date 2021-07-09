#ifndef SATURATE_OVER_OBJ_H
#define SATURATE_OVER_OBJ_H

#include <BLTypes.h>

#include <OversampProcessObj.h>

#include "IPlug_include_in_plug_hdr.h"

// For InfraSynth
// NOT USED ?
class SaturateOverObj : public OversampProcessObj
{
public:
    SaturateOverObj(int overlaping, BL_FLOAT sampleRate);
    
    virtual ~SaturateOverObj();
    
    void ProcessSamplesBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer) override;
    
    void SetRatio(BL_FLOAT ratio);
    
protected:
    void ComputeSaturation(BL_FLOAT inSample, BL_FLOAT *outSample, BL_FLOAT ratio);
    
    BL_FLOAT mRatio;
    
    WDL_TypedBuf<BL_FLOAT> mCopyBuffer;
};

#endif
