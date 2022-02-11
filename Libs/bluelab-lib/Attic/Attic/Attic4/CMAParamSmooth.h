//
//  CMAParamSmooth.h
//  UST
//
//  Created by applematuer on 2/29/20.
//
//

#ifndef __UST__CMAParamSmooth__
#define __UST__CMAParamSmooth__

#include <deque>
using namespace std;

#include <BLTypes.h>

class CMAParamSmooth
{
public:
    CMAParamSmooth(BL_FLOAT smoothingTimeMs, BL_FLOAT samplingRate);
    
    virtual ~CMAParamSmooth();
    
    void Reset(BL_FLOAT samplingRate);
    void Reset(BL_FLOAT samplingRate, BL_FLOAT val);
    
    void SetSmoothTimeMs(BL_FLOAT smoothingTimeMs);
    
    BL_FLOAT Process(BL_FLOAT inVal);
    
private:
    BL_FLOAT mSmoothingTimeMs;
    BL_FLOAT mSampleRate;
    
    int mWindowSize;
    
    BL_FLOAT mCurrentAvg;
    
    deque<BL_FLOAT> mPrevValues;
};

#endif /* defined(__UST__CMAParamSmooth__) */
