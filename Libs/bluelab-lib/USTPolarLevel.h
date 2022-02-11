//
//  USTPolarLevel.h
//  UST
//
//  Created by applematuer on 8/21/19.
//
//

#ifndef __UST__USTPolarLevel__
#define __UST__USTPolarLevel__

#include "IPlug_include_in_plug_hdr.h"

class USTPolarLevel
{
public:
    USTPolarLevel(BL_FLOAT sampleRate);
    
    virtual ~USTPolarLevel();
    
    void Reset();
    
    void Reset(BL_FLOAT sampleRate);
    
    void ComputePoints(WDL_TypedBuf<BL_FLOAT> samplesIn[2],
                       WDL_TypedBuf<BL_FLOAT> points[2],
                       WDL_TypedBuf<BL_FLOAT> maxPoints[2]);
    
protected:
    void ComputePolarLevelsSourcePos(const WDL_TypedBuf<BL_FLOAT> &rs,
                                     const WDL_TypedBuf<BL_FLOAT> &thetas,
                                     int numBins,
                                     WDL_TypedBuf<BL_FLOAT> *levels);

    
    void ProcessLevels(WDL_TypedBuf<BL_FLOAT> *ioLevels);
    
    // Smooth over time
    WDL_TypedBuf<BL_FLOAT> mPrevPolarsLevels;
    WDL_TypedBuf<BL_FLOAT> mPrevPolarLevelsMax;
    
    // Smooth over space
    WDL_TypedBuf<BL_FLOAT> mSmoothWin;
    
    BL_FLOAT mSampleRate;
};

#endif /* defined(__UST__USTPolarLevel__) */
