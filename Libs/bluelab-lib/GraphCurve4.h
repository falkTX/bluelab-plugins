//
//  GraphCurve.h
//  EQHack
//
//  Created by Apple m'a Tuer on 10/09/17.
//
//

#ifndef EQHack_GraphCurve4_h
#define EQHack_GraphCurve4_h

#ifdef IGRAPHICS_NANOVG

#include <BLTypes.h>
#include <ParamSmoother.h>

#include "IPlug_include_in_plug_hdr.h"

#define FILL_CURVE_ALPHA 0.125

typedef float CurveColor [4];

// Single value Horizontal and Vertical

// GraphCurve4: for GraphControl6
// added X values
class GraphCurve4
{
public:
    GraphCurve4(int numValues);
    
    virtual ~GraphCurve4();
    
    void SetDescription(const char *description, int descrColor[4]);
    
    void ResetNumValues(int numValues);
    
    void ClearValues();
    
    void SetYScale(bool dbFlag, BL_GUI_FLOAT minY, BL_GUI_FLOAT maxY);
    
    // Fill linearly
    void FillAllXValues(BL_GUI_FLOAT minX, BL_GUI_FLOAT maxX);
    
    void FillAllYValues(BL_GUI_FLOAT val);
    
    void SetYValues(const WDL_TypedBuf<BL_GUI_FLOAT> *yValues,
                    BL_GUI_FLOAT minX, BL_GUI_FLOAT maxX);
    
    void SetValue(BL_GUI_FLOAT t, BL_GUI_FLOAT x, BL_GUI_FLOAT y);
    
    // The last value will be automatically poped
    void PushValue(BL_GUI_FLOAT x, BL_GUI_FLOAT y);
    
    void NormalizeXValues(BL_GUI_FLOAT maxXValue);
    
    // Points
    void SetXScale(bool dbFlag, BL_GUI_FLOAT minX, BL_GUI_FLOAT maxX);
    
    void SetPointValues(const WDL_TypedBuf<BL_GUI_FLOAT> &xValues,
                        const WDL_TypedBuf<BL_GUI_FLOAT> &yValues);
    
    // Bounds
    void SetLimitToBounds(bool flag);
    
    void SetWeightTargetColor(int color[4]);
    
protected:
    friend class GraphControl11;
    
    // Description
    char *mDescription;
    int mDescrColor[4];
    
    // Scale
    bool mYdBScale;
    BL_GUI_FLOAT mMinY;
    BL_GUI_FLOAT mMaxY;
    
    // For Points
    
    // If true, display points instead of curves
    bool mPointStyle;
    bool mPointsAsLinesPolar;
    bool mPointsAsLines;
    
    bool mXdBScale;
    BL_GUI_FLOAT mMinX;
    BL_GUI_FLOAT mMaxX;
    
    CurveColor mColor;
    BL_GUI_FLOAT mAlpha;
    
    BL_GUI_FLOAT mLineWidth;
    BL_GUI_FLOAT mPointSize;
    
    // For UST
    //
    
    // Avoid peaks in lines that have bit stroke width
    
    bool mBevelFlag;
    
    bool mPointOverlay;
    
    
    // WON't BE USED HERE
    bool mDoSmooth;
    
    bool mCurveFill;
    BL_GUI_FLOAT mCurveFillOriginY;
    
    BL_GUI_FLOAT mFillAlpha;
    // To fill over the curve
    BL_GUI_FLOAT mFillAlphaUp;
    //BL_GUI_FLOAT mFillAlphaUpFlag; // NEW, UST
    bool mFillAlphaUpFlag; // NEW, UST
    
    CurveColor mFillColor;
    bool mFillColorSet;
    
    bool mSingleValueH;
    bool mSingleValueV;
    
    BL_GUI_FLOAT mPowerScaleX;
    
    int mNumValues;
    WDL_TypedBuf<BL_GUI_FLOAT> mXValues;
    WDL_TypedBuf<BL_GUI_FLOAT> mYValues;
    
    WDL_TypedBuf<BL_GUI_FLOAT> mWeights;
    bool mWeightMultAlpha;
    CurveColor mWeightTargetColor;
    bool mUseWeightTargetColor;
    
    // Hack for scrolling
    int mPushCounter;
    BL_GUI_FLOAT mTmpPushValue;
    
    // Bounds
    bool mLimitToBounds;
    
    // Optimization, when all the points or lines have the same color
    bool mOptimSameColor;
    
protected:
    ParamSmoother mParamSmoother;
};

#endif // IGRAPHICS_NANOVG

#endif
