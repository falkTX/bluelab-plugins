//
//  USTVumeterProcess.h
//  UST
//
//  Created by applematuer on 8/14/19.
//
//

#ifndef __UST__USTVumeterProcess__
#define __UST__USTVumeterProcess__

#include "IPlug_include_in_plug_hdr.h"

#include <BLTypes.h>

#define VUMETER_MIN_GAIN -90.0
#define VUMETER_MAX_GAIN 0.0

#define DEFAULT_DB_VALUE_STR "-90.0"

#define USE_SMOOTHER 0 // 1
class ParamSmoother;

class USTVumeterProcess
{
public:
    USTVumeterProcess(BL_FLOAT sampleRate);
    
    virtual ~USTVumeterProcess();
    
    void Reset(BL_FLOAT sampleRate);
    
    void AddSamples(const WDL_TypedBuf<BL_FLOAT> &samples);
    
    BL_FLOAT GetValue();
    BL_FLOAT GetPeakValue();
    
protected:
    void Update();
    
    void ComputeValue(const WDL_TypedBuf<BL_FLOAT> &samples, BL_FLOAT *ioResult);
    void ComputePeakValue(const WDL_TypedBuf<BL_FLOAT> &samples, BL_FLOAT *ioResult);
    
    long ComputeNumToConsume();
    
    
    BL_FLOAT mSampleRate;
    
    WDL_TypedBuf<BL_FLOAT> mSamples;
    
    BL_FLOAT mValue;
    BL_FLOAT mPeakValue;
    
#if USE_SMOOTHER
    ParamSmoother *mValueSmoother;
#endif
};

#endif /* defined(__UST__USTVumeterProcess__) */
