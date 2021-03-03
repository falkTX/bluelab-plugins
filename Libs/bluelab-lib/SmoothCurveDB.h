//
//  SmoothCurveDB.h
//
//  Created by Apple m'a Tuer on 10/09/17.
//
//

#ifndef SmoothCurveDB_h
#define SmoothCurveDB_h

#ifdef IGRAPHICS_NANOVG

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

class SmoothAvgHistogramDB;
class GraphCurve5;
class SmoothCurveDB
{
public:
    SmoothCurveDB(GraphCurve5 *curve,
                  BL_FLOAT smoothFactor,
                  int size, BL_FLOAT defaultValue,
                  BL_FLOAT minDB, BL_FLOAT maxDB,
                  BL_FLOAT sampleRate);
    
    virtual ~SmoothCurveDB();
    
    void Reset(BL_FLOAT sampleRate);
    void ClearValues();
    
    void SetValues(const WDL_TypedBuf<BL_FLOAT> &values, bool reset = false);
    
    void GetHistogramValues(WDL_TypedBuf<BL_FLOAT> *values);
    void GetHistogramValuesDB(WDL_TypedBuf<BL_FLOAT> *values);
        
protected:
    SmoothAvgHistogramDB *mHistogram;
    
    GraphCurve5 *mCurve;
    
    BL_FLOAT mMinDB;
    BL_FLOAT mMaxDB;

    BL_FLOAT mSampleRate;
    
private:
    // Tmp Buffers
    WDL_TypedBuf<BL_FLOAT> mTmpBuf0;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf1;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf2;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf3;
};

#endif

#endif
