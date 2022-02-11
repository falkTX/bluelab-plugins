//
//  USTDepthProcess.h
//  UST
//
//  Created by applematuer on 12/3/19.
//
//

#ifndef __UST__USTDepthProcess__
#define __UST__USTDepthProcess__

#include "IPlug_include_in_plug_hdr.h"

class EarlyReflect;
class USTDepthProcess
{
public:
    USTDepthProcess(BL_FLOAT sampleRate);
    
    virtual ~USTDepthProcess();
    
    void Reset(BL_FLOAT sampleRate);
    
    void Process(const WDL_TypedBuf<BL_FLOAT> &input,
                 WDL_TypedBuf<BL_FLOAT> *outputL,
                 WDL_TypedBuf<BL_FLOAT> *outputR,
                 BL_FLOAT depth);
    
protected:
    EarlyReflect *mRev;
};

#endif /* defined(__UST__USTDepthProcess__) */
