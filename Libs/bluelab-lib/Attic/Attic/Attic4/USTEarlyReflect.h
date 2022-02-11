//
//  USTEarlyReflect.h
//  UST
//
//  Created by applematuer on 12/3/19.
//
//

#ifndef __UST__USTEarlyReflect__
#define __UST__USTEarlyReflect__

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

class revmodel;

class USTEarlyReflect
{
public:
    USTEarlyReflect(BL_FLOAT sampleRate);
    
    virtual ~USTEarlyReflect();
    
    void Reset(BL_FLOAT sampleRate);
    
    void Process(const WDL_TypedBuf<BL_FLOAT> &input,
                 WDL_TypedBuf<BL_FLOAT> *outputL,
                 WDL_TypedBuf<BL_FLOAT> *outputR,
                 BL_FLOAT revGain);
    
protected:
    void InitRevModel();
    
    revmodel *mRevModel;
};

#endif /* defined(__UST__USTEarlyReflect__) */
