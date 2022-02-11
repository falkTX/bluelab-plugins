//
//  GraphCurve.h
//  EQHack
//
//  Created by Apple m'a Tuer on 10/09/17.
//
//

#ifndef EQHack_GraphCurve2_h
#define EQHack_GraphCurve2_h

#include "../../WDL/IPlug/IControl.h"

#include <ParamSmoother.h>

#define FILL_CURVE_ALPHA 0.125

typedef float CurveColor [4];

class GraphCurve2
{
public:
    GraphCurve2(int numValues);
    
    virtual ~GraphCurve2();
    
    void SetYScale(bool dbFlag, BL_FLOAT minY, BL_FLOAT maxY);
    
    void FillAllValues(BL_FLOAT val);
    
    void SetValues(const WDL_TypedBuf<BL_FLOAT> *values);
    
    void SetValue(BL_FLOAT t, BL_FLOAT val);
    
    // The last value will be automatically poped
    void PushValue(BL_FLOAT val);
    
protected:
    friend class GraphControl5;
    
    // Scale
    bool mYdBScale;
    BL_FLOAT mMinY;
    BL_FLOAT mMaxY;
    
    CurveColor mColor;
    BL_FLOAT mAlpha;
    
    BL_FLOAT mLineWidth;
    
    // WON't BE USED HERE
    bool mDoSmooth;
    
    bool mCurveFill;
    BL_FLOAT mFillAlpha;
    
    bool mSingleValue;
    
    BL_FLOAT mPowerScaleX;
    
    int mNumValues;
    WDL_TypedBuf<BL_FLOAT> mValues;
    
    // Hack for scrolling
    int mPushCounter;
    BL_FLOAT mTmpPushValue;
    
protected:
    ParamSmoother mParamSmoother;
};

#endif
