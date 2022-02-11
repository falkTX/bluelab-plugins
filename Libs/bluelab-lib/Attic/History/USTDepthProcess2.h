//
//  USTDepthProcess2.h
//  UST
//
//  Created by applematuer on 12/3/19.
//
//

#ifndef __UST__USTDepthProcess2__
#define __UST__USTDepthProcess2__

#include "IPlug_include_in_plug_hdr.h"

class BLReverb;
class DelayObj4;
class NRBJFilter;
class USTDepthProcess2
{
public:
    USTDepthProcess2(BL_FLOAT sampleRate);
    
    virtual ~USTDepthProcess2();
    
    void Reset(BL_FLOAT sampleRate);
    
    void Process(const WDL_TypedBuf<BL_FLOAT> &input,
                 WDL_TypedBuf<BL_FLOAT> *outputL,
                 WDL_TypedBuf<BL_FLOAT> *outputR
                 /*BL_FLOAT depth*/);
    
protected:
    BLReverb *mReverb;
    
    DelayObj4 *mPreDelay;
    
    NRBJFilter *mLowPassFilters[2];
};

#endif /* defined(__UST__USTDepthProcess2__) */
