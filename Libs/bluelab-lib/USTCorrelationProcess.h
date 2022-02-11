//
//  USTCorrelationProcess.h
//  UST
//
//  Created by applematuer on 10/18/19.
//
//

#ifndef __UST__USTCorrelationProcess__
#define __UST__USTCorrelationProcess__

#include "IPlug_include_in_plug_hdr.h"

// Bufferized
class USTCorrelationProcess
{
public:
    USTCorrelationProcess(BL_FLOAT sampleRate);
    
    virtual ~USTCorrelationProcess();
    
    void Reset(BL_FLOAT sampleRate);
    
    void AddSamples(const WDL_TypedBuf<BL_FLOAT> samples[2]);

    bool WasUpdated();
    
    BL_FLOAT GetCorrelation();

protected:
    BL_FLOAT mSampleRate;
    
    WDL_TypedBuf<BL_FLOAT> mSamples[2];
    
    BL_FLOAT mCorrelation;
    
    bool mWasUpdated;
};

#endif /* defined(__UST__USTCorrelationProcess__) */
