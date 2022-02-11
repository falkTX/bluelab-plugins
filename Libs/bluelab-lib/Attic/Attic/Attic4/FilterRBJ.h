//
//  FilterRBJ.h
//  UST
//
//  Created by applematuer on 8/30/19.
//
//

#ifndef UST_FilterRBJ_h
#define UST_FilterRBJ_h

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

class FilterRBJ
{
public:
  virtual ~FilterRBJ() {}
  
    virtual void SetCutoffFreq(BL_FLOAT freq) = 0;
    
    virtual void SetQFactor(BL_FLOAT q) = 0; // NEW
    
    virtual void SetSampleRate(BL_FLOAT sampleRate) = 0;
    
    virtual BL_FLOAT Process(BL_FLOAT sample) = 0;
    
    //virtual void Process(WDL_TypedBuf<BL_FLOAT> *result,
    //                     const WDL_TypedBuf<BL_FLOAT> &samples) = 0;
    virtual void Process(WDL_TypedBuf<BL_FLOAT> *ioSamples) = 0;
};

#endif
