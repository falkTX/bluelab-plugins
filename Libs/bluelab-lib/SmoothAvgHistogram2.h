//
//  AvgHistogram2.h
//  EQHack
//
//  Created by Apple m'a Tuer on 10/09/17.
//
//

#ifndef EQHack_SmoothAvgHistogram2_h
#define EQHack_SmoothAvgHistogram2_h

#include "IPlug_include_in_plug_hdr.h"

#include <BLTypes.h>

// SmoothAvgHistogram2: from SmoothAvgHistogram
// Take the smoothness param as time in millis, and adapts to sample rate

class SmoothAvgHistogram2
{
public:
    SmoothAvgHistogram2(BL_FLOAT sampleRate, int size,
                        BL_FLOAT smoothTimeMs, BL_FLOAT defaultValue);
    
    virtual ~SmoothAvgHistogram2();
    
    void AddValue(int index, BL_FLOAT val);
    
    void AddValues(const WDL_TypedBuf<BL_FLOAT> &values);
    
    void GetValues(WDL_TypedBuf<BL_FLOAT> *values);
    
    void Reset(BL_FLOAT sampleRate);
    
    void Reset(BL_FLOAT sampleRate, const WDL_TypedBuf<BL_FLOAT> &values);
    
    void Resize(int newSize);
    
protected:
    WDL_TypedBuf<BL_FLOAT> mData;

    // If smooth is 0, then we get instantaneous value.
    // If smooth is 1, then we get total avg
    BL_FLOAT mSmoothCoeff;

    BL_FLOAT mSampleRate;
    BL_FLOAT mSmoothTimeMs;
    
    BL_FLOAT mDefaultValue;
};

#endif
