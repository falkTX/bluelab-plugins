//
//  Graph.h
//  Transient
//
//  Created by Apple m'a Tuer on 03/09/17.
//
//

#ifndef Transient_Graph_h
#define Transient_Graph_h

// #bl-iplug2
#include "IPlug_include_in_plug_hdr.h"
//#include "../../WDL/IPlug/IPlugStructs.h"
//#include "../../WDL/IPlug/IControl.h"
#include <lice.h>

#include <ParamSmoother.h>

#define MAX_NUM_CURVES 10


class GraphCurve
{
public:
    GraphCurve();
    
    virtual ~GraphCurve();
    
public:
    BL_FLOAT mPrevValue;
    
    LICE_pixel mColor;
    
    bool mDoSmooth;
    bool mPrevValueSet;
    bool mCurveFill;
    BL_FLOAT mFillAlpha;
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
    
    void SetCurveFillAlpha(int curveNum, BL_FLOAT alpha);
    
    void SetCurveSingleValue(int curveNum, bool flag);
    
    void SetCurveSqrtScale(int curveNum, bool flag);
    
    
    void Update();
    
    void Clear();
    
    void SaveBitmap();
    
    void RestoreBitmap();
    
    
    void AddPointValue(BL_FLOAT val, int r, int g, int b);
    
    void AddLineValue(int curveNum, BL_FLOAT val);
    
    void ScrollLeft();
    
protected:
    void BlitBitmap(LICE_MemBitmap *srcBitmap, LICE_MemBitmap *dstBitmap);
    
    void DrawCurve(BL_FLOAT val, BL_FLOAT prevValue, LICE_pixel color);
    
    void PutPixel(LICE_MemBitmap *bitmap, int x, int y, LICE_pixel color);
    
    void FillCurve(BL_FLOAT val, LICE_pixel color, BL_FLOAT alpha);
    
    void DrawCurveSV(BL_FLOAT val, BL_FLOAT prevValue, LICE_pixel color);
    
    void FillCurveSV(BL_FLOAT val, LICE_pixel color, BL_FLOAT alpha);
    
    void Debug();
    
    
    LICE_MemBitmap *mBitmap;
    
    // For BL_FLOAT buffering
    LICE_MemBitmap *mBackBitmap;
    
    IBitmapControl *mGraphControl;
    
    LICE_pixel mClearColor;
    
    GraphCurve mCurves[MAX_NUM_CURVES];
    
    LICE_MemBitmap *mSaveBitmap;
};

#endif
