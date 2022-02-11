//
//  AvgHistogram.h
//  EQHack
//
//  Created by Apple m'a Tuer on 10/09/17.
//
//

#ifndef EQHack_AvgHistogram_h
#define EQHack_AvgHistogram_h

#include "IControl.h"

class AvgHistogram
{
public:
    AvgHistogram(int size);
    
    virtual ~AvgHistogram();
    
    void AddValue(int index, double val);
    
    void AddValues(WDL_TypedBuf<double> *values);
    
    void GetValues(WDL_TypedBuf<double> *values);
    
    void Reset();
    
protected:
    WDL_TypedBuf<double> mData;
    WDL_TypedBuf<long int> mNumData;
};

#endif