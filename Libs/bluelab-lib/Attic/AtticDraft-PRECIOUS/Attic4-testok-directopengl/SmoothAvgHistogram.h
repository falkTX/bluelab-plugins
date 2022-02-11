//
//  AvgHistogram.h
//  EQHack
//
//  Created by Apple m'a Tuer on 10/09/17.
//
//

#ifndef EQHack_SmoothAvgHistogram_h
#define EQHack_SmoothAvgHistogram_h

#include "IControl.h"

class SmoothAvgHistogram
{
public:
    SmoothAvgHistogram(int size, double smoothCoeff, double defaultValue);
    
    virtual ~SmoothAvgHistogram();
    
    void AddValue(int index, double val);
    
    void AddValues(WDL_TypedBuf<double> *values);
    
    void GetValues(WDL_TypedBuf<double> *values);
    
    void Reset();
    
protected:
    WDL_TypedBuf<double> mData;

    // If smooth is 0, then we get instantaneous value.
    // If smooth is 1, then we get total avg
    double mSmoothCoeff;
    
    double mDefaultValue;
};

#endif
