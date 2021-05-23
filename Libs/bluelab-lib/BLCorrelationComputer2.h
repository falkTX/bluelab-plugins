//
//  BLCorrelationComputer2.h
//  UST
//
//  Created by applematuer on 1/2/20.
//
//

#ifndef __UST__BLCorrelationComputer2__
#define __UST__BLCorrelationComputer2__

#include <deque>
using namespace std;

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

// // See: https://dsp.stackexchange.com/questions/1671/calculate-phase-angle-between-two-signals-i-e-digital-phase-meter
//
// USTCorrelationComputer2: use atan2, to have correct instant correlation
//

// USTCorrelationComputer3: use real correlation computation
// See: http://www.rs-met.com/documents/tutorials/StereoProcessing.pdf

// USTCorrelationComputer4: same as USTCorrelationComputer3, but optimized
// => gives exactly the smae results as USTCorrelationComputer3 !

#define DEFAULT_SMOOTH_TIME_MS 100.0

class CParamSmooth;
class BLCorrelationComputer2
{
public:
    BLCorrelationComputer2(BL_FLOAT sampleRate,
                           BL_FLOAT smoothTimeMs = DEFAULT_SMOOTH_TIME_MS);
    
    virtual ~BLCorrelationComputer2();
    
    void Reset(BL_FLOAT sampleRate);
    
    void Reset();
    
    void Process(const WDL_TypedBuf<BL_FLOAT> samples[2]);
    
    void Process(BL_FLOAT l, BL_FLOAT r);
    
    BL_FLOAT GetCorrelation();
    
    BL_FLOAT GetSmoothWindowMs();
    
protected:
    BL_FLOAT mSampleRate;
    
    BL_FLOAT mCorrelation;
    
    BL_FLOAT mSmoothTimeMs;
    
    // Histories
    long mHistorySize;
    
    deque<BL_FLOAT> mXLXR;
    deque<BL_FLOAT> mXL2;
    deque<BL_FLOAT> mXR2;
    
    // Optimization
    BL_FLOAT mSumXLXR;
    BL_FLOAT mSumXL2;
    BL_FLOAT mSumXR2;
};

#endif /* defined(__UST__BLCorrelationComputer2__) */
