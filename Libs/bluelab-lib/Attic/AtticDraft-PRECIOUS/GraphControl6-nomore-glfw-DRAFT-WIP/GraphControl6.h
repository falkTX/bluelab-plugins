//
//  Graph.h
//  Transient
//
//  Created by Apple m'a Tuer on 03/09/17.
//
//

#ifndef Transient_Graph6_h
#define Transient_Graph6_h

#include "../../WDL/IPlug/IControl.h"

#include <string>
#include <vector>
using namespace std;

#include <GraphCurve4.h>
#include <ParamSmoother.h>

#define PROFILE_GRAPH 0

#if PROFILE_GRAPH
#include <BlaTimer.h>
#include <Debug.h>
#endif

#define GRAPH_VALUE_UNDEFINED 1e16

#define GRAPHCONTROL_PIXEL_DENSITY 2.0

// Graph class (Use NanoVG internally).
// Bug correction: vertical axis and dB

// Direct OpenGL render, no more GLReadPixels (8 x faster !)
// Be sure to set OPENGL_YOULEAN_PATCH to 1, in Lice and IGraphicsMac.mm 

// Use with GraphCurve2
// ... then use with GraphCurve3

// From GraphControl5
// Precompute all possible before while adding the points
// (instead of computing at each draw)
// (useful for large number of points, e.g 500000 for Impulse with 15s)
struct NVGcontext;
struct NVGLUframebuffer;

class GLContext2;

// WARNING: Since GraphControl4 width must be a multiple of 4 !
class GraphControl6 : public IControl
{
public:
    GraphControl6(IPlugBase *pPlug, IRECT p, int paramIdx,
                 int numCurves, int numCurveValues, const char *fontPath);
    
    virtual ~GraphControl6();
    
    // Not tested
    void ResetNumCurveValues(int numCurveValues);
    
    int GetNumCurveValues();
    
    bool IsDirty();
    
    // Does nothing
    bool Draw(IGraphics* pGraphics) { return true; };
    
    const LICE_IBitmap *DrawGL();
    
    void AddHAxis(char *data[][2], int numData,
                  int axisColor[4], int axisLabelColor[4]);
    
    void ReplaceHAxis(char *data[][2], int numData);
    
    void AddVAxis(char *data[][2], int numData,
                  int axisColor[4], int axisLabelColor[4],
                  double offset = 0.0); // Tmp offset hack
    
    void SetXScale(bool dBFlag, double minX = 0.0, double maxX = 1.0);
    
    void SetAutoAdjust(bool flag, double smoothCoeff);
    
    void SetYScaleFactor(double factor);
    
    void SetClearColor(int r, int g, int b, int a);
    
    // Curves
    void ResetCurve(int curveNum, double resetVal);
    
    void SetCurveDescription(int curveNum, const char *description, int descrColor[4]);
    
    // Curves style
    void SetCurveYScale(int curveNum, bool flag,
                        double minY = -120.0, double maxY = 0.0);
    
    void SetCurveColor(int curveNum, int r, int g, int b);
    
    void SetCurveAlpha(int curveNum, double alpha);
    
    void SetCurveLineWidth(int curveNum, double lineWidth);
    
    void SetCurveSmooth(int curveNum, bool flag);
    
    void SetCurveFill(int curveNum, bool flag);
    
    void SetCurveFillAlpha(int curveNum, double alpha);
    
    // Curves data
    void CurveFillAllValues(int curveNum, double val);
    
    void SetCurveValues(int curveNum, const WDL_TypedBuf<double> *values);
    
    void SetCurveValues2(int curveNum, const WDL_TypedBuf<double> *values);
    
    // Optimized version
    // Remove points if there is more points density than graph pixel density
    void SetCurveValuesDecimate(int curveNum, const WDL_TypedBuf<double> *values,
                                bool isWaveSignal = false);
    
    // T is x normalized
    void SetCurveValue(int curveNum, double t, double val);
    
    void SetCurveSingleValueH(int curveNum, double val);
    
    void SetCurveSingleValueV(int curveNum, double val);
    
    void PushCurveValue(int curveNum, double val);
    
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
        double mT;
        string mText;
    } GraphAxisData;
    
    typedef struct
    {
        vector<GraphAxisData> mValues;
        
        int mColor[4];
        int mLabelColor[4];
        
        // Hack
        double mOffset;
    } GraphAxis;
    
    void DisplayCurveDescriptions();
    
    void AddAxis(GraphAxis *axis, char *data[][2], int numData,
                 int axisColor[4], int axisLabelColor[4],
                 double mindB, double maxdB);
    
    void DrawAxis(GraphAxis *axis, bool horizontal, bool lineLabelFlag);
    
    // If flag is true, we will draw only the lines
    // If flag is false, we will draw only the labels
    // Usefull because the curves must be drawn over the lines but under the labels
    void DrawAxis(bool lineLabelFlag);
    
    void DrawCurves();
    
    void DrawLineCurve(GraphCurve4 *curve);
    
    // Fill and stroke at the same time
    void DrawFillCurve(GraphCurve4 *curve);
    
    // Single value Horizontal
    void DrawLineCurveSVH(GraphCurve4 *curve);
    
    void DrawFillCurveSVH(GraphCurve4 *curve);
    
    // Single value Vertical
    void DrawLineCurveSVV(GraphCurve4 *curve);
    
    void DrawFillCurveSVV(GraphCurve4 *curve);
    
    void AutoAdjust();
    
    static double MillisToPoints(long long int elapsed, int sampleRate, int numSamplesPoint);
    
    // Text
    void InitFont(const char *fontPath);
    
    void DrawText(double x, double y, double fontSize, const char *text,
                  int color[4], int halign, int valign);
    
    // OpenGL
    
    // Separated function with all the OpenGL calls
    // May be useful if we want to render with a specific thread
    void DrawGraph();
    
    double ConvertY(GraphCurve4 *curve, double val, double height);
    
    void SetCurveDrawStyle(GraphCurve4 *curve);

    
    void InitNanoVg();
    void ExitNanoVg();
    
    int mNumCurveValues;
    int mNumCurves;
    
    vector<GraphCurve4 *> mCurves;
    
    // Auto adjust
    bool mAutoAdjustFlag;
    ParamSmoother mAutoAdjustParamSmoother;
    double mAutoAdjustFactor;
    
    double mYScaleFactor;
    
    CurveColor mClearColor;
    
    // NanoVG
    NVGcontext *mVg;
    
    bool mDirty;
    
    // X dB scale
    bool mXdBScale;
    double mMinX;
    double mMaxX;

    GraphAxis *mHAxis;
    GraphAxis *mVAxis;
    
    WDL_String mFontPath;
    
private:
    //bool mGLInitialized;
    
	bool mFontInitialized;
    
    LICE_IBitmap *mLiceFb;
    
#if PROFILE_GRAPH
    BlaTimer mDebugTimer;
    int mDebugCount;
#endif
};

#endif
