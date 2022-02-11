//
//  AvgHistogram.h
//  EQHack
//
//  Created by Apple m'a Tuer on 10/09/17.
//
//

#ifndef EQHack_SmoothAvgHistogram_h
#define EQHack_SmoothAvgHistogram_h

#include "IPlug_include_in_plug_hdr.h"

#include <BLTypes.h>

class SmoothAvgHistogram
{
public:
    SmoothAvgHistogram(int size, BL_FLOAT smoothCoeff, BL_FLOAT defaultValue);
    
    virtual ~SmoothAvgHistogram();
    
    void AddValue(int index, BL_FLOAT val);
    
    void AddValues(const WDL_TypedBuf<BL_FLOAT> &values);
    
    void GetValues(WDL_TypedBuf<BL_FLOAT> *values);
    
    void Reset();
    
    void Reset(const WDL_TypedBuf<BL_FLOAT> &values);
    
    void Resize(int newSize);
    
protected:
    WDL_TypedBuf<BL_FLOAT> mData;

    // If smooth is 0, then we get instantaneous value.
    // If smooth is 1, then we get total avg
    BL_FLOAT mSmoothCoeff;
    
    BL_FLOAT mDefaultValue;
};

#endif
