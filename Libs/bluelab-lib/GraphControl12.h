//
//  Graph.h
//  Transient
//
//  Created by Apple m'a Tuer on 03/09/17.
//
//

#ifndef GraphControl12_h
#define GraphControl12_h

#ifdef IGRAPHICS_NANOVG

#include <string>
#include <vector>
using namespace std;

#include "IPlug_include_in_plug_hdr.h"

//#include <lice.h>
#include <IControl.h>

#include <BLTypes.h>

#include <GraphCurve5.h>
#include <ParamSmoother.h>

#include <LockFreeObj.h>

#define PROFILE_GRAPH 0

#if PROFILE_GRAPH
#include <BlaTimer.h>
#include <Debug.h>
#endif


// FBO rendering
// Avoids blinking
// NOTE: disable on linux? Looked useless since fixes in mDataChanged
// NOTE: when disabled, bugs in Ghost app when zooming
// (zoomed graph goes over the knobs)
#define USE_FBO 1 //0

// Font size
#define FONT_SIZE 14.0

// #bl-iplug2
// Every element had y reversed, except the bitmaps
#define GRAPH_CONTROL_FLIP_Y 1

#if USE_FBO
typedef struct NVGLUframebuffer NVGLUframebuffer;
#endif

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
// GraphControl12: for IPlug2

struct NVGcontext;
struct NVGLUframebuffer;

class GraphAxis2;
class GraphTimeAxis6;

// Add special drawing, depending on the plugins
class GraphCustomDrawer : public LockFreeObj
{
public:
    GraphCustomDrawer() {}
    
    virtual ~GraphCustomDrawer() {}
    
    // Implement one of the two methods, depending on when
    // you whant to draw
    
    // Draw before everything
    virtual void PreDraw(NVGcontext *vg, int width, int height) {}
    
    // Draw after everything
    virtual void PostDraw(NVGcontext *vg, int width, int height) {}
    
    // If it is ownaed by graph, it will be delete each time
    // the graph is deleted.
    virtual bool IsOwnedByGraph() { return false; }
    
    //virtual bool NeedRedraw() { return true; }
    virtual bool NeedRedraw() { return false; }

    virtual void OnUIClose() {}
};

class GraphCustomControl
{
public:
    GraphCustomControl() {}
    
    virtual ~GraphCustomControl() {}

    virtual void OnMouseDown(float x, float y, const IMouseMod &mod) {}
    virtual void OnMouseUp(float x, float y, const IMouseMod &mod) {}
    virtual void OnMouseDrag(float x, float y, float dX, float dY,
                             const IMouseMod &mod) {}
    virtual void OnMouseDblClick(float x, float y,
                                 const IMouseMod &mod) {};
    virtual void OnMouseWheel(float x, float y,
                              const IMouseMod &mod, float d) {};
    virtual bool OnKeyDown(float x, float y, const IKeyPress& key) { return false; }
    virtual bool OnKeyUp(float x, float y, const IKeyPress& key) { return false; }
    
    virtual void OnMouseOver(float x, float y, const IMouseMod &mod) {}
    virtual void OnMouseOut() {}
    
    virtual void OnGUIIdle() {}
};

class BLTransport;
// WARNING: Since GraphControl4 width must be a multiple of 4 !
class GraphControl12 : public IControl
{
public:
    GraphControl12(Plugin *pPlug, IGraphics *pGraphics,
                   IRECT p, int paramIdx, const char *fontPath);
    
    virtual ~GraphControl12();

    // Hack to avoid a crash
    void ClearLinkedObjects();

    void SetEnabled(bool flag);
    
    void GetSize(int *width, int *height);
    void Resize(int width, int height);
    
    void SetBounds(BL_GUI_FLOAT x0, BL_GUI_FLOAT y0,
                   BL_GUI_FLOAT x1, BL_GUI_FLOAT y1);
    
    void Draw(IGraphics &graphics) override;
    
    void OnGUIIdle() override;
    
    // Set a separator line at the bottom
    void SetSeparatorY0(BL_GUI_FLOAT lineWidth, int color[4]);
    void SetSeparatorX1(BL_GUI_FLOAT lineWidth, int color[4]);
    
    void AddCurve(GraphCurve5 *curve);
    
    void SetHAxis(GraphAxis2 *axis);
    void SetVAxis(GraphAxis2 *axis);
    
    void SetAutoAdjust(bool flag, BL_GUI_FLOAT smoothCoeff);
    
    void SetYScaleFactor(BL_GUI_FLOAT factor);
    
    void SetClearColor(int r, int g, int b, int a);
    
    //
    static void DrawText(NVGcontext *vg,
                         BL_GUI_FLOAT x, BL_GUI_FLOAT y,
                         BL_GUI_FLOAT graphWidth, BL_GUI_FLOAT graphHeight,
                         BL_GUI_FLOAT fontSize, const char *text,
                         int color[4], int halign, int valign,
                         BL_GUI_FLOAT fontSizeCoeff = 1.0);
    
    // Custom drawers
    void AddCustomDrawer(GraphCustomDrawer *customDrawer);
    void RemoveCustomDrawer(GraphCustomDrawer *customDrawer);
    
    void CustomDrawersPreDraw();
    void CustomDrawersPostDraw();
    
    void DrawSeparatorY0();
    void DrawSeparatorX1();
    
    // Custom control
    void AddCustomControl(GraphCustomControl *customControl);

    void OnMouseDown(float x, float y, const IMouseMod &mod) override;
    void OnMouseUp(float x, float y, const IMouseMod &mod) override;
    void OnMouseDrag(float x, float y, float dX, float dY,
                     const IMouseMod &mod) override;
    void OnMouseDblClick(float x, float y, const IMouseMod &mod) override;
    void OnMouseWheel(float x, float y, const IMouseMod &mod, float d) override;
    bool OnKeyDown(float x, float y, const IKeyPress& key) override;
    // NEW iPlug2
    bool OnKeyUp(float x, float y, const IKeyPress& key) override;
    
    void OnMouseOver(float x, float y, const IMouseMod &mod) override;
    void OnMouseOut() override;
    
    void DBG_PrintCoords(int x, int y);

    void SetBackgroundImage(IGraphics *graphics, IBitmap bmp);
    void SetOverlayImage(IGraphics *graphics, IBitmap bmp);
    
    // For UST
    void SetDisablePointOffsetHack(bool flag);
    
    // BUG: StereoWidth2, Reaper, Mac. Choose a mode using point rendering
    // with quads (e.g POLAR_SAMPLE)
    // Save and quit. When we restart, this given mode doesn't display points
    // (only the circle graph drawer)
    // This is because the mWhitePixImg image seems not correct
    // If we re-create it at the beginning of each loop, we don't have the problem.
    //
    // Set to true to recreate the mWhitePixImg image at the beginning of each loop
    void SetRecreateWhiteImageHack(bool flag);
    
    void SetGraphTimeAxis(GraphTimeAxis6 *timeAxis);
    void SetTransport(BLTransport *transport);
    
    void SetDataChanged();
    
    //
    void SetDirty(bool triggerAction = true, int valIdx = kNoValIdx) override;
    
    // Hacked, always return true
    bool IsDirty() override;

    // Hack
    void SetValueToDefault(int valIdx) override;

    // Legacy mechanism
    // NOTE: still used sometimes e.g in Ghost
    void SetUseLegacyLock(bool flag);
    void Lock();
    void Unlock();
    
    // Lock free
    void PushAllData();
    void PullAllData();

    // Add controls over the graph, so they won't blink when the graph is redisplayed
    void AddControlOverGraph(IControl *control);
    void ClearControlsOverGraph();
    
protected:
    void DoDraw(IGraphics &graphics);
    
    void DisplayCurveDescriptions();
    
    void DrawAxis(GraphAxis2 *axis, bool horizontal, bool lineLabelFlag);
    
    // If flag is true, we will draw only the lines
    // If flag is false, we will draw only the labels
    // Usefull because the curves must be drawn over the lines but under the labels
    void DrawAxis(bool lineLabelFlag);
    
    void DrawCurves();
    
    void DrawLineCurve(GraphCurve5 *curve);
    
    // Fill and stroke at the same time
    void DrawFillCurve(GraphCurve5 *curve);
    
    // Single value Horizontal
    void DrawLineCurveSVH(GraphCurve5 *curve);
    
    void DrawFillCurveSVH(GraphCurve5 *curve);
    
    // Single value Vertical
    void DrawLineCurveSVV(GraphCurve5 *curve);
    
    void DrawFillCurveSVV(GraphCurve5 *curve);
    
    // Draw points
    void DrawPointCurve(GraphCurve5 *curve);
    
    // Optimized version. Draw all points with the same color
    void DrawPointCurveOptimSameColor(GraphCurve5 *curve);
    
    // Draw lines, from a "middle" to each point
    void DrawPointCurveLinesPolar(GraphCurve5 *curve);
    
    void DrawPointCurveLinesPolarWeights(GraphCurve5 *curve);
    
    // For UST
    void DrawPointCurveLinesPolarFill(GraphCurve5 *curve);
    
    void DrawPointCurveLines(GraphCurve5 *curve);
    void DrawPointCurveLinesWeights(GraphCurve5 *curve);
    void DrawPointCurveLinesFill(GraphCurve5 *curve);
    
    void AutoAdjust();
    
    static BL_GUI_FLOAT MillisToPoints(long long int elapsed,
                                       int sampleRate, int numSamplesPoint);

    void DrawText(BL_GUI_FLOAT x, BL_GUI_FLOAT y,
                  BL_GUI_FLOAT fontSize, const char *text,
                  int color[4], int halign, int valign,
                  BL_GUI_FLOAT fontSizeCoeff = 1.0);
    
    void SetCurveDrawStyle(GraphCurve5 *curve);
    void SetCurveDrawStyleWeight(GraphCurve5 *curve, BL_GUI_FLOAT weight);
    
    // Graph bounds
    BL_GUI_FLOAT ConvertToBoundsX(BL_GUI_FLOAT t);
    BL_GUI_FLOAT ConvertToBoundsY(BL_GUI_FLOAT t);
    
    // Axis bounds in the graph
    BL_GUI_FLOAT ConvertToAxisBounds(GraphAxis2 *axis, BL_GUI_FLOAT t);

    
    void DrawBackgroundImage(IGraphics &graphics);
    void DrawOverlayImage(IGraphics &graphics);
    
    // To check that we must draw the curve
    // minNumValues is 1 for points, and 2 for lines
    bool IsCurveUndefined(const WDL_TypedBuf<BL_GUI_FLOAT> &x,
                          const WDL_TypedBuf<BL_GUI_FLOAT> &y,
                          int minNumValues);

    void CheckCustomDrawersRedraw();
    
    void CheckCurvesRedraw();

    // For Panogram, when rotating the view by M_PI/2
    void ApplyViewOrientation(int width, int height,
                              const GraphAxis2 &axis,
                              BL_FLOAT *x, BL_FLOAT *y,
                              int *labelHAlign = NULL);

    void DrawControlsOverGraph(IGraphics &graphics);
    
    //
    
    // For UST
    bool mIsEnabled;
    
    BL_GUI_FLOAT mBounds[4];
    
    vector<GraphCurve5 *> mCurves;
    
    // Auto adjust
    bool mAutoAdjustFlag;
    ParamSmoother mAutoAdjustParamSmoother;
    BL_GUI_FLOAT mAutoAdjustFactor;
    
    BL_GUI_FLOAT mYScaleFactor;
    
    CurveColor mClearColor;
    
    // NanoVG
    NVGcontext *mVg;

    // Horizontal, bottom
    bool mSeparatorY0;
    BL_GUI_FLOAT mSepY0LineWidth;
    int mSepY0Color[4];

    // Vertical, right
    bool mSeparatorX1;
    BL_GUI_FLOAT mSepX1LineWidth;
    int mSepX1Color[4];
    
    //
    GraphAxis2 *mHAxis;
    GraphAxis2 *mVAxis;
    
    WDL_String mFontPath;
    
    // Custom drawers
    vector<GraphCustomDrawer *> mCustomDrawers;
    
    // Custom control
    vector<GraphCustomControl *> mCustomControls;
    
    // Background image
    IBitmap mBgImage;
    
    // Background image
    IBitmap mOverlayImage;
    
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
    
    bool mRecreateWhiteImageHack;
    
    GraphTimeAxis6 *mGraphTimeAxis;

    BLTransport *mTransport;
    
private:    
	bool mFontInitialized;
    
    WDL_TypedBuf<float> mTmpDrawPointsCenters;
    
    Plugin *mPlug;
    
#if USE_FBO
    NVGframebuffer *mFBO;
    int mInitialFBO;
#endif
    
    // If anything has changed and the graph needs to be redrawn
    bool mDataChanged;
    
#if PROFILE_GRAPH
    BlaTimer mDebugTimer;
    int mDebugCount;
#endif

    bool mUseLegacyLock;
    WDL_Mutex mMutex;

    // IControls over graph
    vector<IControl *> mControlsOverGraph;
};

#endif // IGRAPHICS_NANOVG

#endif
