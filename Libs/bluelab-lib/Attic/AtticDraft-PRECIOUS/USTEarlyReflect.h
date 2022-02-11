//
//  USTEarlyReflect.h
//  UST
//
//  Created by applematuer on 12/3/19.
//
//

#ifndef __UST__USTEarlyReflect__
#define __UST__USTEarlyReflect__

#include "IPlug_include_in_plug_hdr.h"

class revmodel;

class USTEarlyReflect
{
public:
    USTEarlyReflect(double sampleRate);
    
    virtual ~USTEarlyReflect();
    
    void Reset(double sampleRate);
    
    void Process(const WDL_TypedBuf<double> &input,
                 WDL_TypedBuf<double> *outputL,
                 WDL_TypedBuf<double> *outputR,
                 double revGain);
    
protected:
    void InitRevModel();
    
    revmodel *mRevModel;
};

#endif /* defined(__UST__USTEarlyReflect__) */
