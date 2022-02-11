//
//  CParamSmooth.h
//  UST
//
//  Created by applematuer on 1/4/20.
//
//

#ifndef UST_CParamSmooth_h
#define UST_CParamSmooth_h

#include <math.h>

// See: http://www.musicdsp.org/en/latest/Filters/257-1-pole-lpf-for-smooth-parameter-changes.html
//
// Advanced version of simple smooth usually used by me
// - the result won't vary if we change the sample rate
// - we specify the smooth with smooth time (ms)
//
class CParamSmooth
{
public:
    CParamSmooth(BL_FLOAT smoothingTimeMs, BL_FLOAT samplingRate)
    {
        mSmoothingTimeMs = smoothingTimeMs;
        mSampleRate = samplingRate;
        
        Reset(samplingRate);
    }
    
    virtual ~CParamSmooth() {}
    
    void Reset(BL_FLOAT samplingRate)
    {
        Reset(samplingRate, 0.0);
    }
    
    void Reset(BL_FLOAT samplingRate, BL_FLOAT val)
    {
        mSampleRate = samplingRate;
        
        const BL_FLOAT c_twoPi = 6.283185307179586476925286766559;
        
        mA = exp(-c_twoPi / (mSmoothingTimeMs * 0.001 * samplingRate));
        mB = 1.0 - mA;
        //mZ = 0.0;
        mZ = val;
    }
    
    void SetSmoothTimeMs(BL_FLOAT smoothingTimeMs)
    {
        mSmoothingTimeMs = smoothingTimeMs;
        
        Reset(mSampleRate);
    }
    
    inline BL_FLOAT Process(BL_FLOAT inVal)
    {
        mZ = (inVal * mB) + (mZ * mA);
        
        return mZ;
    }
    
private:
    BL_FLOAT mSmoothingTimeMs;
    BL_FLOAT mSampleRate;
    
    BL_FLOAT mA;
    BL_FLOAT mB;
    BL_FLOAT mZ;
};

#endif
