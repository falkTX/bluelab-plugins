//
//  ParamSmoother2.h
//  UST
//
//  Created by applematuer on 1/4/20.
//
//

#ifndef UST_ParamSmoother2_h
#define UST_ParamSmoother2_h

#include <cmath>

#include <BLTypes.h>
#include <BLUtilsMath.h>

// 140ms => coeff 0.999 at 44100Hz
#define DEFAULT_SMOOTHING_TIME_MS 140.0

// See: http://www.musicdsp.org/en/latest/Filters/257-1-pole-lpf-for-smooth-parameter-changes.html
//
// Advanced version of simple smooth usually used by me
// - the result won't vary if we change the sample rate
// - we specify the smooth with smooth time (ms)
//

// ParamSmoother2: from ParamSmoother2
class ParamSmoother2
{
public:
    ParamSmoother2(BL_FLOAT sampleRate,
                   BL_FLOAT value,
                   BL_FLOAT smoothingTimeMs = DEFAULT_SMOOTHING_TIME_MS)
    {
        mSmoothingTimeMs = smoothingTimeMs;
        mSampleRate = sampleRate;

        mZ = value;
        mTargetValue = value;
        
        Reset(sampleRate);
    }
    
    virtual ~ParamSmoother2() {}
    
    inline void Reset(BL_FLOAT sampleRate, BL_FLOAT smoothingTimeMs = -1.0)
    {
        mSampleRate = sampleRate;

        if (smoothingTimeMs >= 0.0)
            mSmoothingTimeMs = smoothingTimeMs;
        
        mA = std::exp(-(BL_FLOAT)TWO_PI/
                      (mSmoothingTimeMs * (BL_FLOAT)0.001 * sampleRate));
        mB = 1.0 - mA;
        //mZ = 0.0;
        mZ = mTargetValue;
    }

    // Set smooth time, but without resetting
    inline void SetSmoothTimeMs(BL_FLOAT smoothingTimeMs)
    {
        mSmoothingTimeMs = smoothingTimeMs;
        
        mA = std::exp(-(BL_FLOAT)TWO_PI/
                      (mSmoothingTimeMs * (BL_FLOAT)0.001 * mSampleRate));
        mB = 1.0 - mA;
    }
    
    inline void SetTargetValue(BL_FLOAT val)
    {
        mTargetValue = val;
    }

    inline void ResetToTargetValue(BL_FLOAT val)
    {
        mTargetValue = val;
        mZ = mTargetValue;
    }

    inline BL_FLOAT Process()
    {
        mZ = (mTargetValue * mB) + (mZ * mA);

        FIX_FLT_DENORMAL(mZ);
        
        return mZ;
    }

    // Get current value without processing
    inline BL_FLOAT PickCurrentValue()
    {
        return mZ;
    }
    
    inline bool IsStable()
    {
        return (std::fabs(mZ - mTargetValue) < BL_EPS10);
    }

    // Convert ms delay to smooth factor
    //
    // NOTE: the inverse computation is:
    // ms = -2Pi/(ln(factor)*0.001*sampleRate)
    // WARNING: uses ln(), not log()!
    static inline BL_FLOAT
    ComputeSmoothFactor(BL_FLOAT smoothingTimeMs, BL_FLOAT sampleRate)
    {
        BL_FLOAT factor = std::exp(-(BL_FLOAT)TWO_PI/
                                   (smoothingTimeMs * (BL_FLOAT)0.001 * sampleRate));
        return factor;
    }
    
protected:
    BL_FLOAT mSmoothingTimeMs;
    BL_FLOAT mSampleRate;
    
    BL_FLOAT mA;
    BL_FLOAT mB;
    BL_FLOAT mZ;

    BL_FLOAT mTargetValue;
};

#endif
