//
//  AvgHistogram.h
//  EQHack
//
//  Created by Apple m'a Tuer on 10/09/17.
//
//

#ifndef EQHack_SmoothAvgHistogramDB_h
#define EQHack_SmoothAvgHistogramDB_h

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"
#include "IControl.h"
//#include "../../WDL/IPlug/Containers.h"

// Normalize Y to dB internally
class SmoothAvgHistogramDB
{
public:
    SmoothAvgHistogramDB(int size, BL_FLOAT smoothCoeff,
                         BL_FLOAT defaultValue, BL_FLOAT mindB, BL_FLOAT maxdB);
    
    virtual ~SmoothAvgHistogramDB();
    
    void AddValue(int index, BL_FLOAT val);
    
    void AddValues(const WDL_TypedBuf<BL_FLOAT> &values);
    
    int GetNumValues();

    // Get values scaled back to amp
    void GetValues(WDL_TypedBuf<BL_FLOAT> *values);

    // OPTIM: Get internal values, which are in DB
    void GetValuesDB(WDL_TypedBuf<BL_FLOAT> *values);
    
    // Force the internal values to be the new values,
    // (withtout smoothing)
    void SetValues(const WDL_TypedBuf<BL_FLOAT> *values,
                   bool convertToDB = false);
    
    void Reset();
    
protected:
    WDL_TypedBuf<BL_FLOAT> mData;

    // If smooth is 0, then we get instantaneous value.
    // If smooth is 1, then we get total avg
    BL_FLOAT mSmoothCoeff;
    
    BL_FLOAT mDefaultValue;
    
    BL_FLOAT mMindB;
    BL_FLOAT mMaxdB;

private:
    // Tmp buffers
    WDL_TypedBuf<BL_FLOAT> mTmpBuf0;
};

#endif
