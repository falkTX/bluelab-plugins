//
//  Graph.h
//  Transient
//
//  Created by Apple m'a Tuer on 03/09/17.
//
//

#ifndef Transient_Graph5_h
#define Transient_Graph5_h

#include "../../WDL/IPlug/IControl.h"

#include <string>
#include <vector>
using namespace std;

#include <GraphCurve3.h>
#include <ParamSmoother.h>

#define PROFILE_GRAPH 0

#if PROFILE_GRAPH
#include <BlaTimer.h>
#include <Debug.h>
#endif

#define GRAPH_VALUE_UNDEFINED 1e16

// Graph class (Use NanoVG internally).
// Bug correction: vertical axis and dB

// Direct OpenGL render, no more GLReadPixels (8 x faster !)
// Be sure to set OPENGL_YOULEAN_PATCH to 1, in Lice and IGraphicsMac.mm 

// Use with GraphCurve2
// ... then use with GraphCurve3

struct NVGcontext;
struct NVGLUframebuffer;

class GLContext2;

// WARNING: GraphControl4 width must be a multiple of 4 !
class GraphControl5 : public IControl
{
public:
    GraphControl5(IPlugBase *pPlug, IRECT p,
                 int numCurves, int numCurveValues, const char *fontPath);
    
    virtual ~GraphControl5();
    
    // Not tested
    void ResetNumCurveValues(int numCurveValues);
    
    int GetNumCurveValues();
    
    bool IsDirty();
    
    // Does nothing
    bool Draw(IGraphics* pGraphics) { return true; };
    
    const LICE_IBitmap *DrawGL();
    
    void AddHAxis(char *data[][2], int numData,
                  int axisColor[4], int axisLabelColor[4]);
    
    void AddVAxis(char *data[][2], int numData,
                  int axisColor[4], int axisLabelColor[4],
                  BL_FLOAT offset = 0.0); // Tmp offset hack
    
    void SetXScale(bool dBFlag, BL_FLOAT minXdB = 0.0, BL_FLOAT maxXdB = 1.0);
    
    void SetAutoAdjust(bool flag, BL_FLOAT smoothCoeff);
    
    void SetYScaleFactor(BL_FLOAT factor);
    
    void SetClearColor(int r, int g, int b, int a);
    
    // Curves
    
    void ResetCurve(int curveNum, BL_FLOAT resetVal);
    
    // Curves style
    void SetCurveYScale(int curveNum, bool flag,
                        BL_FLOAT minYdB = -120.0, BL_FLOAT maxYdB = 0.0);
    
    void SetCurveColor(int curveNum, int r, int g, int b);
    
    void SetCurveAlpha(int curveNum, BL_FLOAT alpha);
    
    void SetCurveLineWidth(int curveNum, BL_FLOAT lineWidth);
    
    void SetCurveSmooth(int curveNum, bool flag);
    
    void SetCurveFill(int curveNum, bool flag);
    
    void SetCurveFillAlpha(int curveNum, BL_FLOAT alpha);
    
    // Curves data
    void CurveFillAllValues(int curveNum, BL_FLOAT val);
    
    void SetCurveValues(int curveNum, const WDL_TypedBuf<BL_FLOAT> *values);
    
    void SetCurveValue(int curveNum, BL_FLOAT t, BL_FLOAT val);
    
    void SetCurveSingleValueH(int curveNum, BL_FLOAT val);
    
    void SetCurveSingleValueV(int curveNum, BL_FLOAT val);
    
    void PushCurveValue(int curveNum, BL_FLOAT val);
    
    void SetCurveSingleValueH(int curveNum, bool flag);
    
    void SetCurveSingleValueV(int curveNum, bool flag);
    
    // Must be called at the beginning of the plugin
    void InitGL();
    
    // Must be called before destroying the plugin
    void ExitGL();
    
protected:
    // Axis
    typedef struct
    {
        BL_FLOAT mT;
        string mText;
    } GraphAxisData;
    
    typedef struct
    {
        vector<GraphAxisData> mValues;
        
        int mColor[4];
        int mLabelColor[4];
        
        // Hack
        BL_FLOAT mOffset;
    } GraphAxis;
    
    void AddAxis(GraphAxis *axis, char *data[][2], int numData,
                 int axisColor[4], int axisLabelColor[4],
                 BL_FLOAT mindB, BL_FLOAT maxdB);
    
    void DrawAxis(GraphAxis *axis, bool horizontal, bool lineLabelFlag);
    
    // If flag is true, we will draw only the lines
    // If flag is false, we will draw only the labels
    // Usefull because the curves must be drawn over the lines but under the labels
    void DrawAxis(bool lineLabelFlag);
    
    void DrawCurves();
    
    void DrawLineCurve(const WDL_TypedBuf<BL_FLOAT> *points, CurveColor color,
                       BL_FLOAT alpha, BL_FLOAT lineWidth, GraphCurve3 *curve);
    
    // Fill and stroke at the same time
    void DrawFillCurve(const WDL_TypedBuf<BL_FLOAT> *points, CurveColor color, BL_FLOAT fillAlpha,
                       BL_FLOAT strokeAlphe, BL_FLOAT lineWidth, GraphCurve3 *curve);
    
    // Single value Horizontal
    void DrawLineCurveSVH(BL_FLOAT val, CurveColor color, BL_FLOAT alpha, BL_FLOAT lineWidth, GraphCurve3 *curve);
    
    void DrawFillCurveSVH(BL_FLOAT val, CurveColor color, BL_FLOAT alpha, GraphCurve3 *curve);
    
    // Single value Vertical
    void DrawLineCurveSVV(BL_FLOAT val, CurveColor color, BL_FLOAT alpha, BL_FLOAT lineWidth, GraphCurve3 *curve);
    
    void DrawFillCurveSVV(BL_FLOAT val, CurveColor color, BL_FLOAT alpha, GraphCurve3 *curve);
    
    void AutoAdjust();
    
    static BL_FLOAT MillisToPoints(long long int elapsed, int sampleRate, int numSamplesPoint);
    
    // Text
    void InitFont(const char *fontPath);
    
    void DrawText(BL_FLOAT x, BL_FLOAT y, BL_FLOAT fontSize, const char *text,
                  int color[4], int halign);
    
    // OpenGL
    
    // Separated function with all the OpenGL calls
    // May be useful if we want to render with a specific thread
    void DrawGraph();
    
    void InitNanoVg();
    void ExitNanoVg();
    
    int mNumCurveValues;
    int mNumCurves;
    
    vector<GraphCurve3 *> mCurves;
    
    // Auto adjust
    bool mAutoAdjustFlag;
    ParamSmoother mAutoAdjustParamSmoother;
    BL_FLOAT mAutoAdjustFactor;
    
    BL_FLOAT mYScaleFactor;
    
    CurveColor mClearColor;
    
    // NanoVG
    NVGcontext *mVg;
    
    bool mDirty;
    
    // X dB scale
    bool mXdBScale;
    BL_FLOAT mMinX;
    BL_FLOAT mMaxX;

    GraphAxis *mHAxis;
    GraphAxis *mVAxis;
    
    WDL_String mFontPath;
    
private:
    bool mGLInitialized;
    
	bool mFontInitialized;
    
    LICE_IBitmap *mLiceFb;
    
#if PROFILE_GRAPH
    BlaTimer mDebugTimer;
    int mDebugCount;
#endif
};

#endif
