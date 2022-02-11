//
//  SourceComputer.h
//  UST
//
//  Created by applematuer on 8/21/19.
//
//

#ifndef __UST__SourceComputer__
#define __UST__SourceComputer__

#include "IPlug_include_in_plug_hdr.h"

// BLFireworks: from USTFireworks
// SourceComputer: from BLFireworks

class FftProcessObj16;
class StereoWidthProcessDisp;

class SourceComputer
{
public:
    SourceComputer(BL_FLOAT sampleRate);
    
    virtual ~SourceComputer();
    
    void Reset();
    
    void Reset(BL_FLOAT sampleRate);
    
    void ComputePoints(WDL_TypedBuf<BL_FLOAT> samplesIn[2],
                       WDL_TypedBuf<BL_FLOAT> points[2]);
    
protected:
    void Init(int oversampling, int freqRes, BL_FLOAT sampleRate);
    
    //
    FftProcessObj16 *mFftObj;
    StereoWidthProcessDisp *mStereoWidthProcessDisp;
};

#endif /* defined(__UST__SourceComputer__) */
