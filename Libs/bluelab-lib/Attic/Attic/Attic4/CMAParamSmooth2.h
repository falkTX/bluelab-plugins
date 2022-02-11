//
//  CMAParamSmooth2.h
//  UST
//
//  Created by applematuer on 2/29/20.
//
//

#ifndef __UST__CMAParamSmooth2__
#define __UST__CMAParamSmooth2__

#include <deque>
using namespace std;

#include <CMAParamSmooth.h>

class CMAParamSmooth2
{
public:
    CMAParamSmooth2(BL_FLOAT smoothingTimeMs, BL_FLOAT samplingRate);
    
    virtual ~CMAParamSmooth2();
    
    void Reset(BL_FLOAT samplingRate);
    void Reset(BL_FLOAT samplingRate, BL_FLOAT val);
    
    void SetSmoothTimeMs(BL_FLOAT smoothingTimeMs);
    
    BL_FLOAT Process(BL_FLOAT inVal);
    
private:
    CMAParamSmooth *mSmoothers[2];
};

#endif /* defined(__UST__CMAParamSmooth2__) */
