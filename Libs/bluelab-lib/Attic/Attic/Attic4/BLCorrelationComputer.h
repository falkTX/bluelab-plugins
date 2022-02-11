//
//  BLCorrelationComputer.h
//  UST
//
//  Created by applematuer on 1/2/20.
//
//

#ifndef __UST__BLCorrelationComputer__
#define __UST__BLCorrelationComputer__

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

// // See: https://dsp.stackexchange.com/questions/1671/calculate-phase-angle-between-two-signals-i-e-digital-phase-meter
//
// USTCorrelationComputer2: use atan2, to have correct instant correlation
// BLCorrelationComputer: from USTCorrelationComputer2

#define DEFAULT_SMOOTH_TIME_MS 100.0 

class CParamSmooth;

class BLCorrelationComputer
{
public:
    enum Mode
    {
      STANDARD,
        
      // Keep only negative correlation
      // Set positive ones to 0.0
      KEEP_ONLY_NEGATIVE
    };

    //
    BLCorrelationComputer(BL_FLOAT sampleRate,
                            BL_FLOAT smoothTimeMs = DEFAULT_SMOOTH_TIME_MS,
                            Mode mode = STANDARD);
    
    virtual ~BLCorrelationComputer();
    
    void Reset(BL_FLOAT sampleRate);
    
    void Reset();
    
    void Process(const WDL_TypedBuf<BL_FLOAT> samples[2],
                 bool ponderateDist = false);
    
    void Process(BL_FLOAT l, BL_FLOAT r);
    
    BL_FLOAT GetCorrelation();
    
    // Not smoothed
    BL_FLOAT GetInstantCorrelation();
    
    BL_FLOAT GetSmoothWindowMs();
    
protected:
    //
    Mode mMode;
    
    BL_FLOAT mSampleRate;
    
    CParamSmooth *mParamSmooth;
    
    BL_FLOAT mCorrelation;
    BL_FLOAT mInstantCorrelation;
    
    bool mWasJustReset;
    
    BL_FLOAT mSmoothTimeMs;
};

#endif /* defined(__UST__BLCorrelationComputer__) */
