//
//  USTFireworks.h
//  UST
//
//  Created by applematuer on 8/21/19.
//
//

#ifndef __UST__USTFireworks__
#define __UST__USTFireworks__

#include "IPlug_include_in_plug_hdr.h"

#include <BLTypes.h>

class USTFireworks
{
public:
    USTFireworks(BL_FLOAT sampleRate);
    
    virtual ~USTFireworks();
    
    void Reset();
    
    void Reset(BL_FLOAT sampleRate);
    
    void ComputePoints(WDL_TypedBuf<BL_GUI_FLOAT> samplesIn[2],
                       WDL_TypedBuf<BL_GUI_FLOAT> points[2],
                       WDL_TypedBuf<BL_GUI_FLOAT> maxPoints[2]);
    
protected:
    BL_FLOAT mSampleRate;
    
    WDL_TypedBuf<BL_GUI_FLOAT> mPrevPolarLevelsMax;
};

#endif /* defined(__UST__USTFireworks__) */
