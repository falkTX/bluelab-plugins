//
//  Graph.h
//  Transient
//
//  Created by Apple m'a Tuer on 03/09/17.
//
//

#ifndef Transient_Graph_h
#define Transient_Graph_h

#include "../../WDL/IPlug/IPlugStructs.h"
#include "../../WDL/IPlug/IControl.h"

#include <ParamSmoother.h>

#define MAX_NUM_CURVES 10


class GraphCurve
{
public:
    GraphCurve();
    
    virtual ~GraphCurve();
    
public:
    double mPrevValue;
    
    LICE_pixel mColor;
    
    bool mDoSmooth;
    bool mPrevValueSet;
    bool mCurveFill;
    double mFillAlpha;
    bool mSingleValue;
    bool mSqrtScale;
    
    ParamSmoother mParamSmoother;
};


class Graph
{
public:
    Graph(IBitmapControl *bitmapControl, IBitmap *bitmap);
    
    virtual ~Graph();
    
    void SetClearColor(int r, int g, int b, int a);
    
    
    void SetCurveColor(int curveNum, int r, int g, int b, int a);
    
    void SetCurveSmooth(int curveNum, bool flag);
    
    void SetCurveFill(int curveNum, bool flag);
    
    void SetCurveFillAlpha(int curveNum, double alpha);
    
    void SetCurveSingleValue(int curveNum, bool flag);
    
    void SetCurveSqrtScale(int curveNum, bool flag);
    
    
    void Update();
    
    void Clear();
    
    void SaveBitmap();
    
    void RestoreBitmap();
    
    
    void AddPointValue(double val, int r, int g, int b);
    
    void AddLineValue(int curveNum, double val);
    
    void ScrollLeft();
    
protected:
    void BlitBitmap(LICE_MemBitmap *srcBitmap, LICE_MemBitmap *dstBitmap);
    
    void DrawCurve(double val, double prevValue, LICE_pixel color);
    
    void PutPixel(LICE_MemBitmap *bitmap, int x, int y, LICE_pixel color);
    
    void FillCurve(double val, LICE_pixel color, double alpha);
    
    void DrawCurveSV(double val, double prevValue, LICE_pixel color);
    
    void FillCurveSV(double val, LICE_pixel color, double alpha);
    
    void Debug();
    
    
    LICE_MemBitmap *mBitmap;
    
    // For double buffering
    LICE_MemBitmap *mBackBitmap;
    
    IBitmapControl *mGraphControl;
    
    LICE_pixel mClearColor;
    
    GraphCurve mCurves[MAX_NUM_CURVES];
    
    LICE_MemBitmap *mSaveBitmap;
};

#endif
