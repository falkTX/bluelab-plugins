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

#if PROFILE_GRAPH
#include <BlaTimer.h>
#include <Debug.h>
#endif

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
    
    void AddHAxis(char *data[][2], int numData,
                  int axisColor[4], int axisLabelColor[4]);
    
    void AddVAxis(char *data[][2], int numData,
                  int axisColor[4], int axisLabelColor[4],
                  BL_FLOAT offset = 0.0); // Tmp offset hack
    
    void SetXdBScale(bool flag, BL_FLOAT minXdB = 0.0, BL_FLOAT maxXdB = 1.0);
    
    void SetAutoAdjust(bool flag, BL_FLOAT smoothCoeff);
    
    void SetYScaleFactor(BL_FLOAT factor);
    
    void SetClearColor(int r, int g, int b, int a);
    
    // Curves
    
    void ResetCurve(int curveNum, BL_FLOAT resetVal);
    
    // Curves style
    void SetCurveYdBScale(int curveNum, bool flag,
                          BL_FLOAT minYdB = -120.0, BL_FLOAT maxYdB = 0.0);
    
    void SetCurveColor(int curveNum, int r, int g, int b);
    
    void SetCurveAlpha(int curveNum, BL_FLOAT alpha);
    
    void SetCurveLineWidth(int curveNum, BL_FLOAT lineWidth);
    
    void SetCurveSmooth(int curveNum, bool flag);
    
    void SetCurveFill(int curveNum, bool flag);
    
    void SetCurveFillAlpha(int curveNum, BL_FLOAT alpha);
    
    // Curves data
    void SetCurveValues(int curveNum, const WDL_TypedBuf<BL_FLOAT> *values);
    
    void SetCurveValue(int curveNum, BL_FLOAT t, BL_FLOAT val);
    
    void SetCurveSingleValue(int curveNum, BL_FLOAT val);
    
    void PushCurveValue(int curveNum, BL_FLOAT val);
    
    void SetCurveSingleValue(int curveNum, bool flag);
    
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
                 int axisColor[4], int axisLabelColor[4]);
    
    void DrawAxis(GraphAxis *axis, bool horizontal, bool lineLabelFlag);
    
    // If flag is true, we will draw only the lines
    // If flag is false, we will draw only the labels
    // Usefull because the curves must be drawn over the lines but under the labels
    void DrawAxis(bool lineLabelFlag);
    
    void DrawCurves();
    
    void DrawLineCurve(const WDL_TypedBuf<BL_FLOAT> *points, CurveColor color,
                       BL_FLOAT alpha, BL_FLOAT lineWidth, GraphCurve *curve);
    
    // Fill and stroke at the same time
    void DrawFillCurve(const WDL_TypedBuf<BL_FLOAT> *points, CurveColor color, BL_FLOAT fillAlpha,
                       BL_FLOAT strokeAlphe, BL_FLOAT lineWidth, GraphCurve *curve);
    
    // Single value
    void DrawLineCurveSV(BL_FLOAT val, CurveColor color, BL_FLOAT alpha, BL_FLOAT lineWidth, GraphCurve *curve);
    
    void DrawFillCurveSV(BL_FLOAT val, CurveColor color, BL_FLOAT alpha, GraphCurve *curve);
    
    void AutoAdjust();
    
    static BL_FLOAT MillisToPoints(long long int elapsed, int sampleRate, int numSamplesPoint);
    
    // Text
    void InitFont(const char *fontPath);
    
    void DrawText(BL_FLOAT x, BL_FLOAT y, BL_FLOAT fontSize, const char *text,
                  int color[4], int halign);
    
    // Image
    static void SwapChannels(unsigned int *data, int numPixels);
    
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
    BL_FLOAT mAutoAdjustFactor;
    
    BL_FLOAT mYScaleFactor;
    
    CurveColor mClearColor;
    
    // OpenGL
    bool mGLInitialized;
    
    // NanoVG
    NVGcontext *mVg;
    NVGLUframebuffer *mFbo;
    
    unsigned char *mPixels;
    
    bool mDirty;
    
    // X dB scale
    bool mXdBScale;
    BL_FLOAT mMinXdB;
    BL_FLOAT mMaxXdB;

    GraphAxis *mHAxis;
    GraphAxis *mVAxis;
    
    WDL_String mFontPath;
    
private:
	bool mFontInitialized;
    
#if PROFILE_GRAPH
    BlaTimer mDebugTimer;
    int mDebugCount;
#endif
};

#endif
