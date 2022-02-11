//
//  FilterIIRLow12dB.h
//  BL-Infra
//
//  Created by applematuer on 7/9/19.
//
//

#ifndef __BL_Infra__FilterIIRLow12dB__
#define __BL_Infra__FilterIIRLow12dB__

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

// Parameter that can be changed
#define AMP 1.0

class FilterIIRLow12dB
{
public:
    FilterIIRLow12dB();

    virtual ~FilterIIRLow12dB();

    void Init(BL_FLOAT resoFreq, BL_FLOAT sampleRate);

    BL_FLOAT Process(BL_FLOAT sample);

    void Process(WDL_TypedBuf<BL_FLOAT> *result,
                 const WDL_TypedBuf<BL_FLOAT> &samples);
    
protected:
    BL_FLOAT mR;
    BL_FLOAT mC;
    BL_FLOAT mVibraPos;
    BL_FLOAT mVibraSpeed;
};

#endif /* defined(__BL_Infra__FilterIIRLow12dB__) */
