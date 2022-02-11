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

#include <string>
#include <vector>
using namespace std;

#include <GraphCurve.h>
#include <ParamSmoother.h>
#include <BlaTimer.h>

#define USE_GRAPH_ADJUSTED_SCROLLING 0

// Graph class (Use NanoVG internally).

struct NVGcontext;
struct NVGLUframebuffer;

class GLContext2;

class GraphControl2 : public IControl
{
public:
    GraphControl2(IPlugBase *pPlug, IRECT p,
                 int numCurves, int numCurveValues, const char *fontPath);
    
    virtual ~GraphControl2();
    
    bool IsDirty();
    
    bool Draw(IGraphics *pGraphics);
    
#if USE_GRAPH_ADJUSTED_SCROLLING
    // Scrolling.
    void SetScrollParams(int sampleRate, int numSamplesPoint);
    
    void UpdateScrolling();
    
    void LockScrolling();
    
    void UnlockScrolling();
#endif
    
    void AddHAxis(char *data[][2], int numData,
                  int axisColor[4], int axisLabelColor[4]);
    
    void AddVAxis(char *data[][2], int numData,
                  int axisColor[4], int axisLabelColor[4],
                  double offset = 0.0); // Tmp offset hack
    
    void SetXdBScale(bool flag, double minXdB = 0.0, double maxXdB = 1.0);
    
    void SetAutoAdjust(bool flag, double smoothCoeff);
    
    void SetYScaleFactor(double factor);
    
    void SetClearColor(int r, int g, int b, int a);
    
    void Clear();
    
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
                 int axisColor[4], int axisLabelColor[4]);
    
    void DrawAxis(GraphAxis *axis, bool horizontal, bool lineLabelFlag);
    
    // If flag is true, we will draw only the lines
    // If flag is false, we will draw only the labels
    // Usefull because the curves must be drawn over the lines but under the labels
    void DrawAxis(bool lineLabelFlag);
    
    void DrawCurves();
    
    void DrawLineCurve(const WDL_TypedBuf<double> *points, CurveColor color,
                       double alpha, double lineWidth, GraphCurve *curve);
    
    void DrawFillCurve(const WDL_TypedBuf<double> *points, CurveColor color, double alpha, GraphCurve *curve);
    
    // Single value
    void DrawLineCurveSV(double val, CurveColor color, double alpha, double lineWidth, GraphCurve *curve);
    
    void DrawFillCurveSV(double val, CurveColor color, double alpha, GraphCurve *curve);
    
    void AutoAdjust();
    
    static double MillisToPoints(long long int elapsed, int sampleRate, int numSamplesPoint);
    
    // Text
    void InitFont(const char *fontPath);
    
    void DrawText(double x, double y, double fontSize, const char *text,
                  int color[4], int halign);
    
    // Image
    static void SwapChannels(unsigned int *data, int numPixels);
    
#if USE_GRAPH_ADJUSTED_SCROLLING
    // Scrolling.
    void AdjustScrolling();
#endif
    
    // OpenGL
    void InitGL();
    
    long CreateContext();
    void DestroyContext();
    long SetContext();
    void RestoreContext();

    
    int mNumCurveValues;
    int mNumCurves;
    
    vector<GraphCurve *> mCurves;
    
    bool mAutoAdjustFlag;
    ParamSmoother mAutoAdjustParamSmoother;
    double mAutoAdjustFactor;
    
    double mYScaleFactor;
    
    CurveColor mClearColor;
    
    
    // NanoVG
    GLContext2 *mGLContext;
    NVGcontext *mVg;
    NVGLUframebuffer *mFbo;

    unsigned char *mPixels;
    
    bool mDirty;
    
    // X dB scale
    bool mXdBScale;
    double mMinXdB;
    double mMaxXdB;

    GraphAxis *mHAxis;
    GraphAxis *mVAxis;
    
    //
    // For smooth scrolling
    //
    
    int mSampleRate;
    
    // Num samples for one point
    int mNumSamplesPoint;
    
    BlaTimer mTimer;
    int mNumPointsPushed;
    BlaTimer mTimer2;
    
    double mScrollTransX;
    
    // Use a mutex to avoid curve values changed before we update the smooth scrolling
    WDL_Mutex mScrollMutex;
    
    // We must redraw faster than the points arrive, to have a smooth scroll
    int mSmoothStreamCoeff;
    
    int mPointsCounter;
    
    bool mFirstTime;

private:
	bool mFontInitialized;
};

#endif
