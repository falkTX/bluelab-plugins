//
//  BLFireworks.h
//  UST
//
//  Created by applematuer on 8/21/19.
//
//

#ifndef __UST__BLFireworks__
#define __UST__BLFireworks__

#include "IPlug_include_in_plug_hdr.h"

// BLFireworks: from USTFireworks

class BLFireworks
{
public:
    BLFireworks(BL_FLOAT sampleRate);
    
    virtual ~BLFireworks();
    
    void Reset();
    
    void Reset(BL_FLOAT sampleRate);
    
    void ComputePoints(WDL_TypedBuf<BL_FLOAT> samplesIn[2],
                       WDL_TypedBuf<BL_FLOAT> points[2],
                       WDL_TypedBuf<BL_FLOAT> maxPoints[2]);
    
protected:
    BL_FLOAT mSampleRate;
    
    WDL_TypedBuf<BL_FLOAT> mPrevPolarLevelsMax;
};

#endif /* defined(__UST__BLFireworks__) */
