//
//  Graph.h
//  Transient
//
//  Created by Apple m'a Tuer on 03/09/17.
//
//

#ifndef Transient_Graph7_h
#define Transient_Graph7_h

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
//
// GraphControl7 : added Spectrograms
//

struct NVGcontext;
struct NVGLUframebuffer;

class GLContext2;

class BLSpectrogram;

// WARNING: Since GraphControl4 width must be a multiple of 4 !
class GraphControl7 : public IControl
{
public:
    GraphControl7(IPlugBase *pPlug, IRECT p, int paramIdx,
                 int numCurves, int numCurveValues, const char *fontPath);
    
    virtual ~GraphControl7();
    
    // Not tested
    void Resize(int numCurveValues);
    
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
                  BL_FLOAT offset = 0.0); // Tmp offset hack
    
    void AddVAxis(char *data[][2], int numData,
                  int axisColor[4], int axisLabelColor[4],
                  bool dbFlag, BL_FLOAT minY, BL_FLOAT maxY,
                  BL_FLOAT offset = 0.0); // Tmp offset hack
    
    void SetXScale(bool dBFlag, BL_FLOAT minX = 0.0, BL_FLOAT maxX = 1.0);
    
    void SetAutoAdjust(bool flag, BL_FLOAT smoothCoeff);
    
    void SetYScaleFactor(BL_FLOAT factor);
    
    void SetClearColor(int r, int g, int b, int a);
    
    // Curves
    void ResetCurve(int curveNum, BL_FLOAT resetVal);
    
    void SetCurveDescription(int curveNum, const char *description, int descrColor[4]);
    
    // Curves style
    void SetCurveYScale(int curveNum, bool flag,
                        BL_FLOAT minY = -120.0, BL_FLOAT maxY = 0.0);
    
    void SetCurveColor(int curveNum, int r, int g, int b);
    
    void SetCurveAlpha(int curveNum, BL_FLOAT alpha);
    
    void SetCurveLineWidth(int curveNum, BL_FLOAT lineWidth);
    
    void SetCurveSmooth(int curveNum, bool flag);
    
    void SetCurveFill(int curveNum, bool flag);
    
    void SetCurveFillAlpha(int curveNum, BL_FLOAT alpha);
    
    // Curves data
    void CurveFillAllValues(int curveNum, BL_FLOAT val);
    
    void SetCurveValues(int curveNum, const WDL_TypedBuf<BL_FLOAT> *values);
    
    void SetCurveValues2(int curveNum, const WDL_TypedBuf<BL_FLOAT> *values);
    
    // Optimized version
    // Remove points if there is more points density than graph pixel density
    void SetCurveValuesDecimate(int curveNum, const WDL_TypedBuf<BL_FLOAT> *values,
                                bool isWaveSignal = false);
    
    // T is x normalized
    void SetCurveValue(int curveNum, BL_FLOAT t, BL_FLOAT val);
    
    void SetCurveSingleValueH(int curveNum, BL_FLOAT val);
    
    void SetCurveSingleValueV(int curveNum, BL_FLOAT val);
    
    void PushCurveValue(int curveNum, BL_FLOAT val);
    
    void SetCurveSingleValueH(int curveNum, bool flag);
    
    void SetCurveSingleValueV(int curveNum, bool flag);
    
    // Spectrogram
    void SetSpectrogram(BLSpectrogram *spectro,
                        BL_FLOAT left, BL_FLOAT top, BL_FLOAT right, BL_FLOAT bottom);
    
    void ShowSpectrogram(bool flag);
    
    void UpdateSpectrogram(bool updateData = true);
    
    void SetSpectrogramZoomX(BL_FLOAT zoomX);
    void SetSpectrogramZoomY(BL_FLOAT zoomY);
    
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
    
    void DisplayCurveDescriptions();
    
    void AddAxis(GraphAxis *axis, char *data[][2], int numData,
                 int axisColor[4], int axisLabelColor[4],
                 BL_FLOAT mindB, BL_FLOAT maxdB);
    
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
    
    static BL_FLOAT MillisToPoints(long long int elapsed, int sampleRate, int numSamplesPoint);
    
    // Text
    void InitFont(const char *fontPath);
    
    void DrawText(BL_FLOAT x, BL_FLOAT y, BL_FLOAT fontSize, const char *text,
                  int color[4], int halign, int valign);
    
    // OpenGL
    
    // Separated function with all the OpenGL calls
    // May be useful if we want to render with a specific thread
    void DrawGraph();
    
    BL_FLOAT ConvertY(GraphCurve4 *curve, BL_FLOAT val, BL_FLOAT height);
    
    void SetCurveDrawStyle(GraphCurve4 *curve);
    
    void DoUpdateSpectrogram();
    
    void DrawSpectrogram();
    
    void InitNanoVg();
    void ExitNanoVg();
    
    int mNumCurveValues;
    int mNumCurves;
    
    vector<GraphCurve4 *> mCurves;
    
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
    
    // Spectrogram
    BLSpectrogram *mSpectrogram;
    BL_FLOAT mSpectrogramBounds[4];
    int mNvgSpectroImage;
    WDL_TypedBuf<unsigned char> mSpectroImageData;
    bool mMustUpdateSpectrogram;
    bool mMustUpdateSpectrogramData;
    
    bool mShowSpectrogram;
    
    BL_FLOAT mSpectrogramZoomX;
    BL_FLOAT mSpectrogramZoomY;
    
    BL_FLOAT mSpectrogramGain;
    
private:
    bool mGLInitialized;
    
	bool mFontInitialized;
    
    LICE_IBitmap *mLiceFb;
    
private:
    bool CreateCGLContext();
    void DestroyCGLContext();
    bool BindCGLContext();
    void UnBindCGLContext();

    
    void *mGLContext;
    
#if PROFILE_GRAPH
    BlaTimer mDebugTimer;
    int mDebugCount;
#endif
};

#endif
