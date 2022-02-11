//
//  Graph.h
//  Transient
//
//  Created by Apple m'a Tuer on 03/09/17.
//
//

#ifndef Transient_Graph4_h
#define Transient_Graph4_h

#include "../../WDL/IPlug/IControl.h"

#include <string>
#include <vector>
using namespace std;

#include <GraphCurve.h>
#include <ParamSmoother.h>

#define PROFILE_GRAPH 1

#if PROFILE_GRAPH
#include <BlaTimer.h>
#include <Debug.h>
#endif

// Graph class (Use NanoVG internally).
// Bug correction: vertical axis and dB

// Direct OpenGL render, no more GLReadPixels (8 x faster !)
// Be sure to set OPENGL_YOULEAN_PATCH to 1, in Lice and IGraphicsMac.mm 

struct NVGcontext;
struct NVGLUframebuffer;

class GLContext2;

// WARNING: GraphControl4 width must be a multiple of 4 !
class GraphControl4 : public IControl
{
public:
    GraphControl4(IPlugBase *pPlug, IRECT p,
                 int numCurves, int numCurveValues, const char *fontPath);
    
    virtual ~GraphControl4();
    
    bool IsDirty();
    
    bool Draw(IGraphics *pGraphics);
    
    void AddHAxis(char *data[][2], int numData,
                  int axisColor[4], int axisLabelColor[4]);
    
    void AddVAxis(char *data[][2], int numData,
                  int axisColor[4], int axisLabelColor[4],
                  double offset = 0.0); // Tmp offset hack
    
    void SetXdBScale(bool flag, double minXdB = 0.0, double maxXdB = 1.0);
    
    void SetAutoAdjust(bool flag, double smoothCoeff);
    
    void SetYScaleFactor(double factor);
    
    void SetClearColor(int r, int g, int b, int a);
    
    // Curves
    
    void ResetCurve(int curveNum, double resetVal);
    
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
    
    void SetCurveSingleValue(int curveNum, double val);
    
    void PushCurveValue(int curveNum, double val);
    
    void SetCurveSingleValue(int curveNum, bool flag);
    
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
    
    void AddAxis(GraphAxis *axis, char *data[][2], int numData,
                 int axisColor[4], int axisLabelColor[4],
                 double mindB, double maxdB);
    
    void DrawAxis(GraphAxis *axis, bool horizontal, bool lineLabelFlag);
    
    // If flag is true, we will draw only the lines
    // If flag is false, we will draw only the labels
    // Usefull because the curves must be drawn over the lines but under the labels
    void DrawAxis(bool lineLabelFlag);
    
    void DrawCurves();
    
    void DrawLineCurve(const WDL_TypedBuf<double> *points, CurveColor color,
                       double alpha, double lineWidth, GraphCurve *curve);
    
    // Fill and stroke at the same time
    void DrawFillCurve(const WDL_TypedBuf<double> *points, CurveColor color, double fillAlpha,
                       double strokeAlphe, double lineWidth, GraphCurve *curve);
    
    // Single value
    void DrawLineCurveSV(double val, CurveColor color, double alpha, double lineWidth, GraphCurve *curve);
    
    void DrawFillCurveSV(double val, CurveColor color, double alpha, GraphCurve *curve);
    
    void AutoAdjust();
    
    static double MillisToPoints(long long int elapsed, int sampleRate, int numSamplesPoint);
    
    // Text
    void InitFont(const char *fontPath);
    
    void DrawText(double x, double y, double fontSize, const char *text,
                  int color[4], int halign);
    
    // OpenGL
    
    // Separated function with all the OpenGL calls
    // May be useful if we want to render with a specific thread
    void DrawGL();
    
    void InitGL();
    void ExitGL();
    
    void InitNanoVg();
    void ExitNanoVg();
    
    int mNumCurveValues;
    int mNumCurves;
    
    vector<GraphCurve *> mCurves;
    
    // Auto adjust
    bool mAutoAdjustFlag;
    ParamSmoother mAutoAdjustParamSmoother;
    double mAutoAdjustFactor;
    
    double mYScaleFactor;
    
    CurveColor mClearColor;
    
    // OpenGL
    bool mGLInitialized;
    
    // NanoVG
    NVGcontext *mVg;
    
    bool mDirty;
    
    // X dB scale
    bool mXdBScale;
    double mMinXdB;
    double mMaxXdB;

    GraphAxis *mHAxis;
    GraphAxis *mVAxis;
    
    WDL_String mFontPath;
    
private:
	bool mFontInitialized;
    
    LICE_IBitmap *mLiceFb;
    
#if PROFILE_GRAPH
    BlaTimer mDebugTimer;
    int mDebugCount;
#endif
};

#endif
