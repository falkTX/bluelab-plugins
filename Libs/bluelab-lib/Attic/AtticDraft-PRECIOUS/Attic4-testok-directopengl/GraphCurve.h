//
//  GraphCurve.h
//  EQHack
//
//  Created by Apple m'a Tuer on 10/09/17.
//
//

#ifndef EQHack_GraphCurve_h
#define EQHack_GraphCurve_h

#include "../../WDL/IPlug/IControl.h"

#include <ParamSmoother.h>

#define FILL_CURVE_ALPHA 0.125

typedef float CurveColor [4];

class GraphCurve
{
public:
    GraphCurve(int numValues);
    
    virtual ~GraphCurve();
    
    void SetYdBScale(bool flag, double minYdB, double maxYdB);
    
    void FillAllValues(double val);
    
    void SetValues(const WDL_TypedBuf<double> *values);
    
    void SetValue(double t, double val);
    
    // The last value will be automatically poped
    void PushValue(double val);
    
protected:
    friend class GraphControl;
    friend class GraphControl2;
    friend class GraphControl3;
    friend class GraphControl4;
    
    // Scale
    bool mYdBScale;
    double mMinYdB;
    double mMaxYdB;
    
    CurveColor mColor;
    double mAlpha;
    
    double mLineWidth;
    
    // WON't BE USED HERE
    bool mDoSmooth;
    
    bool mCurveFill;
    double mFillAlpha;
    
    bool mSingleValue;
    
    double mPowerScaleX;
    
    int mNumValues;
    WDL_TypedBuf<double> mValues;
    
    // Hack for scrolling
    int mPushCounter;
    double mTmpPushValue;
    
protected:
    ParamSmoother mParamSmoother;
};

#endif
