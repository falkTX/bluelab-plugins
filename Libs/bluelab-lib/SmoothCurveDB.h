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

#include <LockFreeObj.h>
#include <LockFreeQueue2.h>

#include "IPlug_include_in_plug_hdr.h"

class SmoothAvgHistogramDB;
class GraphCurve5;
class SmoothCurveDB : public LockFreeObj
{
public:
    SmoothCurveDB(GraphCurve5 *curve,
                  BL_FLOAT smoothFactor,
                  int size, BL_FLOAT defaultValue,
                  BL_FLOAT minDB, BL_FLOAT maxDB,
                  BL_FLOAT sampleRate);
    
    virtual ~SmoothCurveDB();
    
    void Reset(BL_FLOAT sampleRate, BL_FLOAT smoothFactor = -1.0);
    void ClearValues();
    void ClearValuesLF();
    
    void SetValues(const WDL_TypedBuf<BL_FLOAT> &values, bool reset = false);
    void SetValuesLF(const WDL_TypedBuf<BL_FLOAT> &values, bool reset = false);
    
    void GetHistogramValues(WDL_TypedBuf<BL_FLOAT> *values);
    void GetHistogramValuesDB(WDL_TypedBuf<BL_FLOAT> *values);

    // LockFreeObj
    void PushData() override;
    void PullData() override;
    void ApplyData() override;
    
protected:
    SmoothAvgHistogramDB *mHistogram;
    
    GraphCurve5 *mCurve;
    
    BL_FLOAT mMinDB;
    BL_FLOAT mMaxDB;

    BL_FLOAT mSampleRate;

    // Lock Free
    struct LockFreeCurve
    {
        enum Command
        {
            SET_VALUES = 0,
            CLEAR_VALUES
        };
        
        WDL_TypedBuf<BL_FLOAT> mValues;
        bool mReset;

        Command mCommand;
    };

    LockFreeQueue2<LockFreeCurve> mLockFreeQueues[LOCK_FREE_NUM_BUFFERS];

    bool ContainsClearValues(/*const*/ LockFreeQueue2<LockFreeCurve> &q);
    bool ContainsCurveReset(/*const*/ LockFreeQueue2<LockFreeCurve> &q);
        
private:
    // Tmp Buffers
    WDL_TypedBuf<BL_FLOAT> mTmpBuf0;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf1;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf2;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf3;
    LockFreeCurve mTmpBuf4;
    LockFreeCurve mTmpBuf5;
    LockFreeCurve mTmpBuf6;
    LockFreeCurve mTmpBuf7;
    LockFreeCurve mTmpBuf8;
};

#endif

#endif
