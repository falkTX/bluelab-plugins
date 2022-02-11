//
//  AvgHistogram.h
//  EQHack
//
//  Created by Apple m'a Tuer on 10/09/17.
//
//

#ifndef EQHack_MinMaxAvgHistogramDB_h
#define EQHack_MinMaxAvgHistogramDB_h

#include "IControl.h"

class MinMaxAvgHistogramDB
{
public:
    MinMaxAvgHistogramDB(int size, BL_FLOAT smoothCoeff, BL_FLOAT defaultValue,
                         BL_FLOAT mindB, BL_FLOAT maxdB);
    
    virtual ~MinMaxAvgHistogramDB();
    
    void AddValue(int index, BL_FLOAT val);
    
    void AddValues(WDL_TypedBuf<BL_FLOAT> *values);
    
    void GetMinValues(WDL_TypedBuf<BL_FLOAT> *values);
    
    void GetMaxValues(WDL_TypedBuf<BL_FLOAT> *values);
    
    void GetAvgValues(WDL_TypedBuf<BL_FLOAT> *values);
    
    void Reset();
    
protected:
    WDL_TypedBuf<BL_FLOAT> mMinData;
    WDL_TypedBuf<BL_FLOAT> mMaxData;
    
    // If smooth is 0, then we get instantaneous value.
    // If smooth is 1, then we get total avg
    BL_FLOAT mSmoothCoeff;
    
    BL_FLOAT mDefaultValue;
    
    BL_FLOAT mMindB;
    BL_FLOAT mMaxdB;
};

#endif
