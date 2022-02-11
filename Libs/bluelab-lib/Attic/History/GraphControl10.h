//
//  Graph.h
//  Transient
//
//  Created by Apple m'a Tuer on 03/09/17.
//
//

#ifndef Transient_Graph7_h
#define Transient_Graph7_h

// #bl-iplug2
//#include "../../WDL/IPlug/IControl.h"

#include <string>
#include <vector>
using namespace std;

#include "IPlug_include_in_plug_hdr.h"

#include <lice.h>
#include <IControl.h>

#include <BLTypes.h>

#include <GraphCurve4.h>
#include <ParamSmoother.h>

#define PROFILE_GRAPH 0

#if PROFILE_GRAPH
#include <BlaTimer.h>
#include <Debug.h>
#endif

#include <ImageDisplay.h>

#include "resource.h"


// MODIF FOR Ghost
//#ifdef WIN32
//#define SWAP_COLOR(__RGBA__)
//#endif

#if !GHOST_OPTIM_GL

#ifdef WIN32
// Convert from RGBA to ABGR
#define SWAP_COLOR(__RGBA__) \
{ int tmp[4] = { __RGBA__[0], __RGBA__[1], __RGBA__[2], __RGBA__[3] }; \
__RGBA__[0] = tmp[2]; \
__RGBA__[1] = tmp[1]; \
__RGBA__[2] = tmp[0]; \
__RGBA__[3] = tmp[3]; }
#endif

#else
	
#ifdef WIN32
// Convert from RGBA to ABGR
#define SWAP_COLOR(__RGBA__) \
{ int tmp[4] = { __RGBA__[0], __RGBA__[1], __RGBA__[2], __RGBA__[3] }; \
__RGBA__[0] = tmp[0]; \
__RGBA__[1] = tmp[1]; \
__RGBA__[2] = tmp[2]; \
__RGBA__[3] = tmp[3]; }
#endif

#endif

#if !GHOST_OPTIM_GL

#ifdef __APPLE__
// Convert from RGBA to ABGR
#define SWAP_COLOR(__RGBA__) \
{ int tmp[4] = { __RGBA__[0], __RGBA__[1], __RGBA__[2], __RGBA__[3] }; \
__RGBA__[0] = tmp[2]; \
__RGBA__[1] = tmp[1]; \
__RGBA__[2] = tmp[0]; \
__RGBA__[3] = tmp[3]; }
#endif

#else
	
#ifdef __APPLE__
// Convert from RGBA to ABGR
#define SWAP_COLOR(__RGBA__) \
{ int tmp[4] = { __RGBA__[0], __RGBA__[1], __RGBA__[2], __RGBA__[3] }; \
__RGBA__[0] = tmp[0]; \
__RGBA__[1] = tmp[1]; \
__RGBA__[2] = tmp[2]; \
__RGBA__[3] = tmp[3]; }
#endif

#endif

// Font size
#define FONT_SIZE 14.0

#define GRAPH_VALUE_UNDEFINED 1e16

#define GRAPHCONTROL_PIXEL_DENSITY 2.0

using namespace iplug;
using namespace iplug::igraphics;

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
// GraphControl8 : for BLSpectrogram3
//
// GraphControl9 : for BLSpectrogram3
//
// GraphControl10 : for SpectrogramDisplay
//
struct NVGcontext;
struct NVGLUframebuffer;

class GLContext3;

class BLSpectrogram3;
class SpectrogramDisplay;
class SpectrogramDisplayScroll;
class SpectrogramDisplayScroll2; // A bit drafty... (for InfrasonicViewer)

class IBitmapControl;

class ImageDisplay;

// Added this test to avoid redraw everything each time
// NOTE: added for StereoWidth
//
// With this flag, everything is redrawn only if the lements changes
// (not at each call to DrawGraph)
//
// => real optimization when nothing changes !
//
#define DIRTY_OPTIM 1

// Class to add special drawing, depending on the plugins
class GraphCustomDrawer
{
public:
    GraphCustomDrawer() {}
    
    virtual ~GraphCustomDrawer() {}
    
    // Manage its own mutex for audio / render thread
    virtual bool HasOwnMutex() { return false; }
    
    // Implement one of the two methods, depending on when
    // you whant to draw
    
    // Draw before everything
    virtual void PreDraw(NVGcontext *vg, int width, int height) {}
    
    // Draw after everything
    virtual void PostDraw(NVGcontext *vg, int width, int height) {}
};

class GraphCustomControl
{
public:
    GraphCustomControl() {}
    
    virtual ~GraphCustomControl() {}
    
    virtual void OnMouseDown(int x, int y, IMouseMod* pMod) {}
    virtual void OnMouseUp(int x, int y, IMouseMod* pMod) {}
    virtual void OnMouseDrag(int x, int y, int dX, int dY, IMouseMod* pMod) {}
    virtual bool OnMouseDblClick(int x, int y, IMouseMod* pMod) { return false; }
    virtual void OnMouseWheel(int x, int y, IMouseMod* pMod, BL_GUI_FLOAT d) {};
    virtual bool OnKeyDown(int x, int y, int key, IMouseMod* pMod) { return false; }
    
    virtual void OnMouseOver(int x, int y, IMouseMod* pMod) {}
    virtual void OnMouseOut() {}
    
    virtual void OnGUIIdle() {}
};

// WARNING: Since GraphControl4 width must be a multiple of 4 !
class GraphControl10 : public IControl
{
public:
    GraphControl10(IPluginBase *pPlug, IGraphics *pGraphics, IRECT p, int paramIdx,
                 int numCurves, int numCurveValues, const char *fontPath);
    
    virtual ~GraphControl10();
    
    void SetEnabled(bool flag);
    
    // NEW: when updating buffer size
    void SetNumCurveValues(int numCurveValues);
    
    void GetSize(int *width, int *height);
    void Resize(int width, int height);
    
    void SetBounds(BL_GUI_FLOAT x0, BL_GUI_FLOAT y0, BL_GUI_FLOAT x1, BL_GUI_FLOAT y1);
    
    // Not tested
    void Resize(int numCurveValues);
    
    int GetNumCurveValues();
    
#if GHOST_OPTIM_GL
    
    // Don't set dirty, otherwise IGraphics will Blit pixels
    // in the (big) region of the graph.
    // GraphControl will be drawn in overlay, everytime.
    bool IsDirty() { return false; };
#endif
    
#if 0 // Commented for StereoWidthProcess::DebugDrawer
    bool IsDirty();
  
    void SetDirty(bool flag);
#endif
    
//#if DIRTY_OPTIM
    // Used to force redraw when something changed that is not pcurve point change
    void SetMyDirty(bool flag);
//#endif
    
    // Does nothing
    bool Draw(IGraphics* pGraphics) { return true; };
    
    // #bl-iplug2
    //const LICE_IBitmap *DrawGL(bool update = false);
    void ResetGL();
    
	//void OnIdle();

    void OnGUIIdle();
    
    
    // Set a separator line at the bottom
    void SetSeparatorY0(BL_GUI_FLOAT lineWidth, int color[4]);
    
    void AddHAxis(char *data[][2], int numData, bool xDbScale,
                  int axisColor[4], int axisLabelColor[4],
                  BL_GUI_FLOAT offsetY = 0.0,
                  int axisOverlayColor[4] = NULL,
                  BL_GUI_FLOAT fontSizeCoeff = 1.0,
                  int axisLinesOverlayColor[4] = NULL);
    
    // HACK: for SpectralDiff
    void AddHAxis0(char *data[][2], int numData, bool xDbScale,
                   int axisColor[4], int axisLabelColor[4],
                   BL_GUI_FLOAT offsetY = 0.0,
                   int axisOverlayColor[4] = NULL,
                   BL_GUI_FLOAT fontSizeCoeff = 1.0,
                   int axisLinesOverlayColor[4] = NULL);
    
    void ReplaceHAxis(char *data[][2], int numData);
    
    void RemoveHAxis();
    
    void AddVAxis(char *data[][2], int numData,
                  int axisColor[4], int axisLabelColor[4],
                  BL_GUI_FLOAT offset = 0.0, BL_GUI_FLOAT offsetX = 0.0, // Tmp offset hack
                  int axisOverlayColor[4] = NULL,
                  BL_GUI_FLOAT fontSizeCoeff = 1.0,
                  bool alignTextRight = false,
                  int axisLinesOverlayColor[4] = NULL,
                  bool alignRight = true);
    
    void AddVAxis(char *data[][2], int numData,
                  int axisColor[4], int axisLabelColor[4],
                  bool dbFlag, BL_GUI_FLOAT minY, BL_GUI_FLOAT maxY,
                  BL_GUI_FLOAT offset = 0.0, // Tmp offset hack
                  int axisOverlayColor[4] = NULL,
                  BL_GUI_FLOAT fontSizeCoeff = 1.0,
                  bool alignTextRight = false,
                  int axisLinesOverlayColor[4] = NULL,
                  bool aligneRight = true);
    
    void AddVAxis(char *data[][2], int numData,
                  int axisColor[4], int axisLabelColor[4],
                  bool dbFlag, BL_GUI_FLOAT minY, BL_GUI_FLOAT maxY,
                  BL_GUI_FLOAT offset = 0.0, BL_GUI_FLOAT offsetX = 0.0,
                  int axisOverlayColor[4] = NULL,
                  BL_GUI_FLOAT fontSizeCoeff = 1.0,
                  bool alignTextRight = false,
                  int axisLinesOverlayColor[4] = NULL,
                  bool alignRight = true);

    void RemoveVAxis();
    
    void SetXScale(bool dBFlag, BL_GUI_FLOAT minX = 0.0, BL_GUI_FLOAT maxX = 1.0);
    
    void SetAutoAdjust(bool flag, BL_GUI_FLOAT smoothCoeff);
    
    void SetYScaleFactor(BL_GUI_FLOAT factor);
    
    void SetClearColor(int r, int g, int b, int a);
    
    // Curves
    void ResetCurve(int curveNum, BL_GUI_FLOAT resetVal);
    
    void SetCurveDescription(int curveNum, const char *description, int descrColor[4]);
    
    // Bounds
    void SetCurveLimitToBounds(int curveNum, bool flag);
    
    // Curves style
    void SetCurveYScale(int curveNum, bool flag,
                        BL_GUI_FLOAT minY = -120.0, BL_GUI_FLOAT maxY = 0.0);
    
    void SetCurveColor(int curveNum, int r, int g, int b);
    
    void SetCurveAlpha(int curveNum, BL_GUI_FLOAT alpha);
    
    void SetCurveLineWidth(int curveNum, BL_GUI_FLOAT lineWidth);
    
    // For UST
    void SetCurveBevel(int curveNum, bool bevelFlag);
    
    void SetCurveSmooth(int curveNum, bool flag);
    
    void SetCurveFill(int curveNum, bool flag, BL_GUI_FLOAT originY = 0.0);
    
    // NEW
    void SetCurveFillColor(int curveNum, int r, int g, int b);
    
    // Down
    // Fill under the curve
    void SetCurveFillAlpha(int curveNum, BL_GUI_FLOAT alpha);
    
    // Up
    // Fill over the curve
    void SetCurveFillAlphaUp(int curveNum, BL_GUI_FLOAT alpha);
    
    // Points
    void SetCurvePointSize(int curveNum, BL_GUI_FLOAT pointSize);
    
    // For UST
    void SetCurvePointOverlay(int curveNum, bool flag);

    void SetCurveWeightMultAlpha(int curveNum, bool flag);

    void SetCurveWeightTargetColor(int curveNum, int color[4]);
    
    void SetCurveXScale(int curveNum, bool dBFlag,
                        BL_GUI_FLOAT minX = 0.0, BL_GUI_FLOAT maxX = 1.0);

    void SetCurvePointStyle(int curveNum, bool flag,
                            bool pointsAsLinesPolar, bool pointsAsLines = false);
    
    void SetCurveValuesPoint(int curveNum,
                             const WDL_TypedBuf<BL_GUI_FLOAT> &xValues,
                             const WDL_TypedBuf<BL_GUI_FLOAT> &yValues);
    
    // For UST
    void SetCurveValuesPointEx(int curveNum,
                               const WDL_TypedBuf<BL_GUI_FLOAT> &xValues,
                               const WDL_TypedBuf<BL_GUI_FLOAT> &yValues,
                               bool singleScale = false, bool scaleX = true,
                               bool centerFlag = false);
    
    void SetCurveColorWeight(int curveNum, const WDL_TypedBuf<BL_GUI_FLOAT> &colorWeights);

    
    void SetCurveValuesPointWeight(int curveNum,
                                   const WDL_TypedBuf<BL_GUI_FLOAT> &xValues,
                                   const WDL_TypedBuf<BL_GUI_FLOAT> &yValues,
                                   const WDL_TypedBuf<BL_GUI_FLOAT> &weights);

    
    // Curves data
    void CurveFillAllValues(int curveNum, BL_GUI_FLOAT val);
    
    void SetCurveValues(int curveNum, const WDL_TypedBuf<BL_GUI_FLOAT> *values);
    
    void SetCurveValues2(int curveNum, const WDL_TypedBuf<BL_GUI_FLOAT> *values);
    
    void SetCurveValues3(int curveNum, const WDL_TypedBuf<BL_GUI_FLOAT> *values);
    
    // Use simple decimation
    void SetCurveValuesDecimateSimple(int curveNum, const WDL_TypedBuf<BL_GUI_FLOAT> *values);
    
    // Optimized version
    // Remove points if there is more points density than graph pixel density
    void SetCurveValuesDecimate(int curveNum, const WDL_TypedBuf<BL_GUI_FLOAT> *values,
                                bool isWaveSignal = false);
    
    // Fixed for positive and negative maxima for sample/wave type values
    void SetCurveValuesDecimate2(int curveNum, const WDL_TypedBuf<BL_GUI_FLOAT> *values,
                                 bool isWaveSignal = false);
    
    // Fixed for some flat sections at 0
    void SetCurveValuesDecimate3(int curveNum, const WDL_TypedBuf<BL_GUI_FLOAT> *values,
                                 bool isWaveSignal = false);
    
    // NEW: Apply db scale on x before decimation
    // (utility method)
    //
    // GOOD: very good accuracy on low frequencies !
    //
    // minValue is the min sample value, or the min db value
    void SetCurveValuesXDbDecimate(int curveNum, const WDL_TypedBuf<BL_GUI_FLOAT> *values,
                                   int bufferSize, BL_GUI_FLOAT sampleRate,
                                   BL_GUI_FLOAT decimFactor);
    
    // When the values are in dB
    void SetCurveValuesXDbDecimateDb(int curveNum, const WDL_TypedBuf<BL_GUI_FLOAT> *values,
                                     int bufferSize, BL_GUI_FLOAT sampleRate,
                                     BL_GUI_FLOAT decimFactor,
                                     BL_GUI_FLOAT minValueDb);
    
    // T is x normalized
    void SetCurveValue(int curveNum, BL_GUI_FLOAT t, BL_GUI_FLOAT val);
    
    void SetCurveSingleValueH(int curveNum, BL_GUI_FLOAT val);
    
    void SetCurveSingleValueV(int curveNum, BL_GUI_FLOAT val);
    
    void PushCurveValue(int curveNum, BL_GUI_FLOAT val);
    
    void SetCurveSingleValueH(int curveNum, bool flag);
    
    void SetCurveSingleValueV(int curveNum, bool flag);


#if 0
    void SetCurveValuePoint(int curveNum, BL_GUI_FLOAT x, BL_GUI_FLOAT y);
#endif
    
    void SetCurveOptimSameColor(int curveNum, bool flag);
    
    //
    static void DrawText(NVGcontext *vg, BL_GUI_FLOAT x, BL_GUI_FLOAT y,
                         BL_GUI_FLOAT fontSize, const char *text,
                         int color[4], int halign, int valign,
                         BL_GUI_FLOAT fontSizeCoeff = 1.0);
    
    // SpectrogramDisplay, for Ghost
    void SetSpectrogram(BLSpectrogram3 *spectro,
                        BL_GUI_FLOAT left, BL_GUI_FLOAT top,
                        BL_GUI_FLOAT right, BL_GUI_FLOAT bottom);
    SpectrogramDisplay *GetSpectrogramDisplay();
    
    // SpectrgramDisplayScroll, for Chroma and GhostViewer
    void SetSpectrogramScroll(BLSpectrogram3 *spectro,
                              BL_GUI_FLOAT left, BL_GUI_FLOAT top,
                              BL_GUI_FLOAT right, BL_GUI_FLOAT bottom);
    SpectrogramDisplayScroll *GetSpectrogramDisplayScroll();
    
    // For SpectrogramDisplayScroll2
    void SetSpectrogramScroll2(BLSpectrogram3 *spectro,
                               BL_GUI_FLOAT left, BL_GUI_FLOAT top,
                               BL_GUI_FLOAT right, BL_GUI_FLOAT bottom);
    SpectrogramDisplayScroll2 *GetSpectrogramDisplayScroll2();
    
    void UpdateSpectrogram(bool updateData, bool updateFullData);
    void UpdateSpectrogramColormap(bool updateData);
    
    
    // Custom drawers
    void AddCustomDrawer(GraphCustomDrawer *customDrawer);
    
// # bl-iplug2
    void CustomDrawersPreDraw(/*IPluginBase::IMutexLock *lock*/);
    
    void CustomDrawersPostDraw(/*IPluginBase::IMutexLock *lock*/);
    
    void DrawSeparatorY0();
    
    // Custom control
    void AddCustomControl(GraphCustomControl *customControl);
    
    void OnMouseDown(int x, int y, IMouseMod* pMod);
    void OnMouseUp(int x, int y, IMouseMod* pMod);
    void OnMouseDrag(int x, int y, int dX, int dY, IMouseMod* pMod);
    bool OnMouseDblClick(int x, int y, IMouseMod* pMod);
    void OnMouseWheel(int x, int y, IMouseMod* pMod, BL_GUI_FLOAT d);
    bool OnKeyDown(int x, int y, int key, IMouseMod* pMod);
    
    void OnMouseOver(int x, int y, IMouseMod* pMod);
    void OnMouseOut();
    
    // Must be called at the beginning of the plugin
    //void InitGL();
    
    // Must be called before destroying the plugin
    //void ExitGL();
    
    void DBG_PrintCoords(int x, int y);
    
    void SetBackgroundImage(LICE_IBitmap *bmp);
    
    void SetOverlayImage(LICE_IBitmap *bmp);

    // For UST
    void SetDisablePointOffsetHack(bool flag);
    
    // BUG: StereoWidth2, Reaper, Mac. Choose a mode using point rendering with quads (e.g POLAR_SAMPLE)
    // Save and quit. When we restart, this given mode doesn't display points (only the circle graph drawer)
    // This is because the mWhitePixImg image seems not correct
    // If we re-create it at the beginning of each loop, we don't have the problem.
    //
    // Set to true to recreate the mWhitePixImg image at the beginning of each loop
    void SetRecreateWhiteImageHack(bool flag);
    
    // Image display
    void CreateImageDisplay(BL_GUI_FLOAT left, BL_GUI_FLOAT top,
                            BL_GUI_FLOAT right, BL_GUI_FLOAT bottom,
                            ImageDisplay::Mode mode = ImageDisplay::MODE_LINEAR);
    ImageDisplay *GetImageDisplay();
    
protected:
    // Axis
    typedef struct
    {
        BL_GUI_FLOAT mT;
        string mText;
    } GraphAxisData;
    
    typedef struct
    {
        vector<GraphAxisData> mValues;
        
        int mColor[4];
        int mLabelColor[4];
        
        // Hack
        BL_GUI_FLOAT mOffset;
        
        // To be able to display the axis on the right
        BL_GUI_FLOAT mOffsetX;
        
        BL_GUI_FLOAT mOffsetY;
        
        // Overlay axis labels ?
        bool mOverlay;
        int mLabelOverlayColor[4];
        
        // Overlay axis lines ?
        bool mLinesOverlay;
        int mLinesOverlayColor[4];
        
        BL_GUI_FLOAT mFontSizeCoeff;
        
        // NEW
        bool mXdBScale;
        
        // NEW
        bool mAlignTextRight;
        
        // NEW
        bool mAlignRight;
        
    } GraphAxis;
    
    void DisplayCurveDescriptions();
    
    void AddAxis(GraphAxis *axis, char *data[][2], int numData,
                 int axisColor[4], int axisLabelColor[4],
                 BL_GUI_FLOAT mindB, BL_GUI_FLOAT maxdB,
                 int axisOverlayColor[4] = NULL,
                 int axisLinesOverlayColor[4] = NULL);
    
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
    
    // Draw points
    void DrawPointCurve(GraphCurve4 *curve);
    //void DrawPointCurveWeights(GraphCurve4 *curve);
    
    // Optimized version. Draw all points with the same color
    void DrawPointCurveOptimSameColor(GraphCurve4 *curve);
    
    // TEMPORARY
    // Draw lines, from a "middle" to each point
    void DrawPointCurveLinesPolar(GraphCurve4 *curve);
    
    void DrawPointCurveLinesPolarWeights(GraphCurve4 *curve);
    
    // For UST
    void DrawPointCurveLinesPolarFill(GraphCurve4 *curve);
    
    void DrawPointCurveLines(GraphCurve4 *curve);
    void DrawPointCurveLinesWeights(GraphCurve4 *curve);
    void DrawPointCurveLinesFill(GraphCurve4 *curve);
    
    void AutoAdjust();
    
    static BL_GUI_FLOAT MillisToPoints(long long int elapsed, int sampleRate, int numSamplesPoint);
    
    // Text
    void InitFont(const char *fontPath);
    
    void DrawText(BL_GUI_FLOAT x, BL_GUI_FLOAT y,
                  BL_GUI_FLOAT fontSize, const char *text,
                  int color[4], int halign, int valign,
                  BL_GUI_FLOAT fontSizeCoeff = 1.0);
    
    // OpenGL
    
    // Separated function with all the OpenGL calls
    // May be useful if we want to render with a specific thread
    void DrawGraph();
    
    BL_GUI_FLOAT ConvertX(GraphCurve4 *curve, BL_GUI_FLOAT val, BL_GUI_FLOAT width);
    
    BL_GUI_FLOAT ConvertY(GraphCurve4 *curve, BL_GUI_FLOAT val, BL_GUI_FLOAT height);
    
    void SetCurveDrawStyle(GraphCurve4 *curve);
    
    void SetCurveDrawStyleWeight(GraphCurve4 *curve, BL_GUI_FLOAT weight);
    
    // Bounds
    BL_GUI_FLOAT ConvertToBoundsX(BL_GUI_FLOAT t);
    BL_GUI_FLOAT ConvertToBoundsY(BL_GUI_FLOAT t);
    
    void UpdateBackgroundImage();
    void UpdateOverlayImage();
    
    void DrawBackgroundImage();
    void DrawOverlayImage();
    
    void InitNanoVg();
    void ExitNanoVg();
    
    // To check that we must draw the curve
    // minNumValues is 1 for points, and 2 for lines
    bool IsCurveUndefined(const WDL_TypedBuf<BL_GUI_FLOAT> &x,
                          const WDL_TypedBuf<BL_GUI_FLOAT> &y,
                          int minNumValues);

    
    //
    
    // For UST
    bool mIsEnabled;
    
    BL_GUI_FLOAT mBounds[4];
    
    int mNumCurveValues;
    int mNumCurves;
    
    vector<GraphCurve4 *> mCurves;
    
    // Auto adjust
    bool mAutoAdjustFlag;
    ParamSmoother mAutoAdjustParamSmoother;
    BL_GUI_FLOAT mAutoAdjustFactor;
    
    BL_GUI_FLOAT mYScaleFactor;
    
    CurveColor mClearColor;
    
    // NanoVG
    NVGcontext *mVg;
    
#if 0 // Commented for StereoWidthProcess::DebugDrawer
    // Was strange, overloaded the member variable of IControl
    bool mDirty;
#endif

#if DIRTY_OPTIM
    bool mMyDirty;
#endif
    
    bool mSeparatorY0;
    BL_GUI_FLOAT mSepY0LineWidth;
    int mSepY0Color[4];
    
    // X dB scale
    bool mXdBScale;
    BL_GUI_FLOAT mMinX;
    BL_GUI_FLOAT mMaxX;

    GraphAxis *mHAxis;
    GraphAxis *mVAxis;
    
    WDL_String mFontPath;
    
    // We will use either one or the other
    SpectrogramDisplay *mSpectrogramDisplay;
    SpectrogramDisplayScroll *mSpectrogramDisplayScroll;
    SpectrogramDisplayScroll2 *mSpectrogramDisplayScroll2;
    
    // Custom drawers
    vector<GraphCustomDrawer *> mCustomDrawers;
    
    // Custom control
    vector<GraphCustomControl *> mCustomControls;
    
    // Background image
    LICE_IBitmap *mBgImage;
    int mNvgBackgroundImage;
    
    // Background image
    LICE_IBitmap *mOverImage;
    int mNvgOverlayImage;
    
    // Dummy image of 1 white pixel
    // for quad rendering
    //
    // NOTE: rendering quads with or without texture: same perfs
    //
    int mWhitePixImg;
    
    // For gui resize
    bool mNeedResizeGraph;
    
    // For UST
    bool mDisablePointOffsetHack;
    
    //
    bool mRecreateWhiteImageHack;
    
    // ImageDisplay
    ImageDisplay *mImageDisplay;
    
private:    
	bool mFontInitialized;
    
    LICE_IBitmap *mLiceFb;
    
private:
    bool CreateCGLContext();
    void DestroyCGLContext();
    bool BindCGLContext();
    void UnBindCGLContext();

    
    GLContext3 *mGLContext;
    
    // For optimization
    //WDL_TypedBuf<BL_GUI_FLOAT> mTmpDrawPointsCenters;
    WDL_TypedBuf<float> mTmpDrawPointsCenters;
    
#if PROFILE_GRAPH
    BlaTimer mDebugTimer;
    int mDebugCount;
#endif
};

#endif
