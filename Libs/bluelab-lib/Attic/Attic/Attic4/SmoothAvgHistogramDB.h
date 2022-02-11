//
//  AvgHistogram.h
//  EQHack
//
//  Created by Apple m'a Tuer on 10/09/17.
//
//

#ifndef EQHack_SmoothAvgHistogramDB_h
#define EQHack_SmoothAvgHistogramDB_h

#include "IPlug_include_in_plug_hdr.h"
#include "IControl.h"
//#include "../../WDL/IPlug/Containers.h"

class SmoothAvgHistogramDB
{
public:
    SmoothAvgHistogramDB(int size, BL_FLOAT smoothCoeff, BL_FLOAT defaultValue, BL_FLOAT mindB, BL_FLOAT maxdB);
    
    virtual ~SmoothAvgHistogramDB();
    
    void AddValue(int index, BL_FLOAT val);
    
    void AddValues(const WDL_TypedBuf<BL_FLOAT> &values);
    
    void GetValues(WDL_TypedBuf<BL_FLOAT> *values);
    
    // Force the internal values to be the new values,
    // (withtout smoothing)
    void SetValues(const WDL_TypedBuf<BL_FLOAT> *values);
    
    void Reset();
    
protected:
    WDL_TypedBuf<BL_FLOAT> mData;

    // If smooth is 0, then we get instantaneous value.
    // If smooth is 1, then we get total avg
    BL_FLOAT mSmoothCoeff;
    
    BL_FLOAT mDefaultValue;
    
    BL_FLOAT mMindB;
    BL_FLOAT mMaxdB;
};

#endif
