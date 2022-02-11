//
//  AvgHistogram.h
//  EQHack
//
//  Created by Apple m'a Tuer on 10/09/17.
//
//

#ifndef EQHack_SmoothAvgHistogramDB_h
#define EQHack_SmoothAvgHistogramDB_h

#include "IControl.h"


class SmoothAvgHistogramDB
{
public:
    SmoothAvgHistogramDB(int size, double smoothCoeff, double defaultValue, double mindB, double maxdB);
    
    virtual ~SmoothAvgHistogramDB();
    
    void AddValue(int index, double val);
    
    void AddValues(WDL_TypedBuf<double> *values);
    
    void GetValues(WDL_TypedBuf<double> *values);
    
    // Force the internal values to be the new values,
    // (withtout smoothing)
    void SetValues(const WDL_TypedBuf<double> *values);
    
    void Reset();
    
protected:
    WDL_TypedBuf<double> mData;

    // If smooth is 0, then we get instantaneous value.
    // If smooth is 1, then we get total avg
    double mSmoothCoeff;
    
    double mDefaultValue;
    
    double mMindB;
    double mMaxdB;
};

#endif
