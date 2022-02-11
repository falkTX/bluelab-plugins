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
    MinMaxAvgHistogramDB(int size, double smoothCoeff, double defaultValue,
                         double mindB, double maxdB);
    
    virtual ~MinMaxAvgHistogramDB();
    
    void AddValue(int index, double val);
    
    void AddValues(WDL_TypedBuf<double> *values);
    
    void GetMinValues(WDL_TypedBuf<double> *values);
    
    void GetMaxValues(WDL_TypedBuf<double> *values);
    
    void GetAvgValues(WDL_TypedBuf<double> *values);
    
    void Reset();
    
protected:
    WDL_TypedBuf<double> mMinData;
    WDL_TypedBuf<double> mMaxData;
    
    // If smooth is 0, then we get instantaneous value.
    // If smooth is 1, then we get total avg
    double mSmoothCoeff;
    
    double mDefaultValue;
    
    double mMindB;
    double mMaxdB;
};

#endif
