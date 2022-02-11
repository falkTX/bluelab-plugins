//
//  Graph.h
//  Transient
//
//  Created by Apple m'a Tuer on 03/09/17.
//
//

#ifndef Transient_Graph2_h
#define Transient_Graph2_h

#include "../../WDL/IPlug/IControl.h"

#include <vector>
using namespace std;

#include <cairo.h>

#include <GraphCurve.h>
#include <ParamSmoother.h>

// Graph class (Use Cairo internally).

class GraphControl : public IControl
{
public:
    GraphControl(IPlugBase *pPlug, IRECT p,
                 int numCurves, int numCurveValues);
    
    virtual ~GraphControl();
    
    bool IsDirty();
    
    bool Draw(IGraphics *pGraphics);
    
    void SetXdBScale(bool flag, double minXdB = 0.0, double maxXdB = 1.0);
    
    void SetAutoAdjust(bool flag, double smoothCoeff);
    
    void SetYScaleFactor(double factor);
    
    void SetClearColor(int r, int g, int b, int a);
    
    void Clear();
    
    // Curves
    
    // Curves style
    void SetCurveYdBScale(int curveNum, bool flag,
                          double minYdB = -120.0, double maxYdB = 0.0);
    
    void SetCurveColor(int curveNum, int r, int g, int b);
    
    void SetCurveAlpha(int curveNum, double alpha);
    
    void SetCurveLineWidth(int curveNum, double lineWidth);
    
    void SetCurveSmooth(int curveNum, bool flag);
    
    void SetCurveFill(int curveNum, bool flag);
    
    void SetCurveFillAlpha(int curveNum, double alpha);
    
    // Curves data
    void SetCurveValues(int curveNum, const WDL_TypedBuf<double> *values);
    
    void SetCurveValue(int curveNum, double t, double val);
    
    void PushCurveValue(int curveNum, double val);
    
    void SetCurveSingleValue(int curveNum, bool flag);
    
protected:
    void DrawCurves();
    
    void DrawLineCurve(const WDL_TypedBuf<double> *points, CurveColor color,
                       double alpha, double lineWidth, GraphCurve *curve);
    
    void DrawFillCurve(const WDL_TypedBuf<double> *points, CurveColor color, double alpha, GraphCurve *curve);
    
    // Single value
    void DrawLineCurveSV(double val, CurveColor color, double alpha, double lineWidth, GraphCurve *curve);
    
    void DrawFillCurveSV(double val, CurveColor color, double alpha, GraphCurve *curve);
    
    void AutoAdjust();
    
    // Image
    static void SwapChannels(unsigned int *data, int numPixels);
    
    
    int mNumCurves;
    
    vector<GraphCurve *> mCurves;
    
    bool mAutoAdjustFlag;
    ParamSmoother mAutoAdjustParamSmoother;
    double mAutoAdjustFactor;
    
    double mYScaleFactor;
    
    CurveColor mClearColor;
    
    // Cairo back buffer
    cairo_surface_t *mSurface;
    cairo_t *mCr;
    
    bool mDirty;
    
    // X dB scale
    bool mXdBScale;
    double mMinXdB;
    double mMaxXdB;
};

#endif
