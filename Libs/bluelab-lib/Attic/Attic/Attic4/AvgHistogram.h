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

#include <BLTypes.h>

class AvgHistogram
{
public:
    AvgHistogram(int size);
    
    virtual ~AvgHistogram();
    
    void AddValue(int index, BL_FLOAT val);
    
    void AddValues(WDL_TypedBuf<BL_FLOAT> *values);
    
    void GetValues(WDL_TypedBuf<BL_FLOAT> *values);
    
    void Reset();
    
protected:
    WDL_TypedBuf<BL_FLOAT> mData;
    WDL_TypedBuf<long int> mNumData;
};

#endif
