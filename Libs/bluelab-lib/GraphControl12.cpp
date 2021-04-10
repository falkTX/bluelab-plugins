//
//  Graph.cpp
//  Transient
//
//  Created by Apple m'a Tuer on 03/09/17.
//
//

#ifdef IGRAPHICS_NANOVG

#include <math.h>

#include <stdio.h>

#include <GraphSwapColor.h>

#include "SpectrogramDisplay.h"
#include "SpectrogramDisplayScroll.h"
#include "SpectrogramDisplayScroll2.h"
#include <BLUtils.h>

#include <ImageDisplay.h>
#include <UpTime.h>

#include <GraphAxis2.h>
#include <GraphTimeAxis6.h>

#include <BLTransport.h>

#include <BLDebug.h>

#include "GraphControl12.h"

//#define GRAPH_FONT "font"
//#define GRAPH_FONT "font-bold"

#define GRAPH_FONT "Roboto-Bold"

// Disabled for UST + IPlug2
// Because DrawFillCurve() took a lot of resources.
// And we don't have the curve fill bug anymore by default!
#define FILL_CURVE_HACK 0 //1

// To fix line holes between filled polygons
#define FILL_CURVE_HACK2 1

#define CURVE_DEBUG 0

// Good, but misses some maxima
#define CURVE_OPTIM      0
#define CURVE_OPTIM_THRS 2048

// -1 pixel
#define OVERLAY_OFFSET -1.0

// Fixed for UST
// FIX: when filling curves (not constant value),
// the last column of the graph was not filled
#define FIX_CURVE_FILL_LAST_COLUMN 1

// Hack for UST
// Dirty hack to finish avoiding black column at the end of the filled curves
#define FILL_CURVE_HACK3 1

// FIX for UST
// Fixes right column and down row that were not filled by bg image
#define FIX_BG_IMAGE 1

// For UST
#define CUSTOM_CONTROL_FIX 1

// FIX: sometimes all the plugin gui becomes lighter and lighter over the time
//
// Fixes for GHOST_OPTIM_GL + UST + disable background and foreground images
//
// BAD: (not working)
#define GRAPH_OPTIM_GL_FIX_OVER_BLEND 0 //1
// GOOD: hard way, but works !
#define GRAPH_OPTIM_GL_FIX_OVER_BLEND2 1

// Avoid redrawing all the graph curves if nothing has changed
// NOTE: does not really optimize for UST...
#define FIX_USELESS_GRAPH_REDRAW 1

// FIX(1/2): Sometimes with haxis, the vertical lines are displayed,
// and they shouldn't be
#define FIX_HAXIS_COLOR_SWAP 1

// FIX(2/2): initialize unititialized color values
#define FIX_INIT_COLORS 1

// Possibility to raw all the quads at the same time
// with the constraint that they have the same color
#define OPTIM_QUADS_SAME_COLOR 1

#define OPTIM_LINES_POLAR 1

// Avoid making nanovg calls if we don't draw any lines or stuff
// => would take more and more memory and wlow down the plugin
// if we don't check this
#define FIX_UNDEFINED_CURVES 1

#define OPTIM_CONVERT_XY 1

// Aligne axis lines to integer pixels
#define ALIGN_AXES_LINES_TO_PIX 1
// Must not aligne axes labels to pix
// e.g for scrolling time axis, this jitters more
#define ALIGN_AXES_LABELS_TO_PIX 0

// DEBUG
#define DBG_DISABLE_DRAW 0 //1

// Does not improve
// The audio thread lock time is similar, even sometimes
// bigger than with normal lock
// (maybe we bufferize too much with trylock, and it takes time to un-bufferize)
#define LOCK_FREE_USE_TRYLOCK 0 //1

GraphControl12::GraphControl12(Plugin *pPlug, IGraphics *graphics,
                               IRECT pR, int paramIdx,
                              const char *fontPath)
	: IControl(pR, paramIdx),
	mFontInitialized(false),
	mAutoAdjustParamSmoother(1.0, 0.9),
	mHAxis(NULL),
	mVAxis(NULL),
    mPlug(pPlug)
{
	mAutoAdjustFlag = false;
	mAutoAdjustFactor = 1.0;

	mYScaleFactor = 1.0;

    // Bottom separator
    mSeparatorY0 = false;
    mSepY0LineWidth = 1.0;
    
    mSepY0Color[0] = 0;
    mSepY0Color[1] = 0;
    mSepY0Color[2] = 0;
    mSepY0Color[3] = 0;
    
    // dB Scale
	//mXdBScale = false;
    //mMinX = 0.0;
	//mMaxX = 1.0;
    
    SetClearColor(0, 0, 0, 255);
        
    mFontPath.Set(fontPath);

    mVg = NULL;
    
    mBounds[0] = 0.0;
    mBounds[1] = 0.0;
    mBounds[2] = 1.0;
    mBounds[3] = 1.0;
    
    mWhitePixImg = -1;

    mNeedResizeGraph = false;
    
    mIsEnabled = true;
    
    mDisablePointOffsetHack = false;
    
    mRecreateWhiteImageHack = false;
    
#if USE_FBO
    mFBO = NULL;
#endif

    mDataChanged = true;
    
#if PROFILE_GRAPH
    mDebugCount = 0;
#endif
    
    mGraphTimeAxis = NULL;
    mTransport = NULL;
}

GraphControl12::~GraphControl12()
{
    // ??
    //WDL_MutexLock lock(&mMutex);
    
    for (int i = 0; i < mCurves.size(); i++)
    {
        GraphCurve5 *curve = mCurves[i];
        curve->SetGraph(NULL);
    }
    
    if (mHAxis != NULL)
    {
        mHAxis->SetGraph(NULL);
    }
    if (mHAxis != NULL)
    {
        mHAxis->SetGraph(NULL);
    }
    
    for (int i = 0; i < mCustomDrawers.size(); i++)
    {
        GraphCustomDrawer *drawer = mCustomDrawers[i];
        if (drawer->IsOwnedByGraph())
            delete drawer;
    }
    
#if USE_FBO && (defined IGRAPHICS_GL)
    if (mFBO != NULL)
    {
        IGraphicsNanoVG *graphics = (IGraphicsNanoVG *)mPlug->GetUI();
        graphics->DeleteFBO(mFBO);
    }
#endif
}

void
GraphControl12::SetEnabled(bool flag)
{
    mIsEnabled = flag;
}

void
GraphControl12::GetSize(int *width, int *height)
{
    *width = this->mRECT.W();
    *height = this->mRECT.H();
}

void
GraphControl12::Resize(int width, int height)
{    
    // BL-Waves debug: add a mutex here ?
    
    // We must have width and height multiple of 4
    width = (width/4)*4;
    height = (height/4)*4;
    
    //
    int dWidth = this->mRECT.W() - width;
    
    this->mRECT.R = this->mRECT.L + width;
    this->mRECT.B = this->mRECT.T + height;
    
    // For mouse
    this->mTargetRECT = this->mRECT;
    
    if (mVAxis != NULL)
    {
        if (mVAxis->mAlignRight) // Added a test to fix resize with SpectralDiff
            //mVAxis->mOffsetX -= dWidth;
            mVAxis->mOffsetX -= ((BL_FLOAT)dWidth)/width;
    }
    
    // FIXED: Resize the FBO instead of deleting and re-creating it
    //
    // Avoids loosing the display on a second similar plugin,
    // when resizing a given plugin (e.g Waves)
    // (because FBO numbers are the same in different instances
    // of plugin)
    // (Reaper, Mac)
    //
    // And use lazy evaluation to resize, because here we are in the event
    // thread, not in the graphic thread
    //
    mNeedResizeGraph = true;

    mDataChanged = true;
}

void
GraphControl12::SetBounds(BL_GUI_FLOAT x0, BL_GUI_FLOAT y0,
                          BL_GUI_FLOAT x1, BL_GUI_FLOAT y1)
{   
    mBounds[0] = x0;
    mBounds[1] = y0;
    mBounds[2] = x1;
    mBounds[3] = y1;
    
    mDataChanged = true;
}

// Added for StereoViz
void
GraphControl12::OnGUIIdle()
{        
    for (int i = 0; i < mCustomControls.size(); i++)
    {
        GraphCustomControl *control = mCustomControls[i];
        
        control->OnGUIIdle();
    }
}

void
GraphControl12::SetSeparatorY0(BL_GUI_FLOAT lineWidth, int color[4])
{    
    mSeparatorY0 = true;
    mSepY0LineWidth = lineWidth;
    
    for (int i = 0; i < 4; i++)
        mSepY0Color[i] = color[i];
    
    mDataChanged = true;
}

void
GraphControl12::AddCurve(GraphCurve5 *curve)
{
    curve->SetGraph(this);
    
    // NEW
    float width = this->mRECT.W();
    float height = this->mRECT.H();
    curve->SetViewSize(width, height);
    
    mCurves.push_back(curve);
}

void
GraphControl12::SetHAxis(GraphAxis2 *axis)
{
    mHAxis = axis;
    
    mHAxis->SetGraph(this);
}

void
GraphControl12::SetVAxis(GraphAxis2 *axis)
{
    mVAxis = axis;
    
    mVAxis->SetGraph(this);
}

/*void
GraphControl12::SetXScale(bool dbFlag, BL_GUI_FLOAT minX, BL_GUI_FLOAT maxX)
{
    mXdBScale = dbFlag;
    
    mMinX = minX;
    mMaxX = maxX;
    
    mDataChanged = true;
}
*/

void
GraphControl12::SetAutoAdjust(bool flag, BL_GUI_FLOAT smoothCoeff)
{
    mAutoAdjustFlag = flag;
    
    mAutoAdjustParamSmoother.SetSmoothCoeff(smoothCoeff);
    
    mDataChanged = true;
}

void
GraphControl12::SetYScaleFactor(BL_GUI_FLOAT factor)
{
    mYScaleFactor = factor;
    
    mDataChanged = true;
}

void
GraphControl12::SetClearColor(int r, int g, int b, int a)
{
    SET_COLOR_FROM_INT(mClearColor, r, g, b, a);
    
    mDataChanged = true;
}

void
GraphControl12::DrawText(NVGcontext *vg,
                         BL_GUI_FLOAT x, BL_GUI_FLOAT y,
                         BL_GUI_FLOAT graphWidth, BL_GUI_FLOAT graphHeight,
                         BL_GUI_FLOAT fontSize,
                         const char *text, int color[4],
                         int halign, int valign, BL_GUI_FLOAT fontSizeCoeff)
{
    // Optimization
    // (avoid displaying many empty texts)
    if (strlen(text) == 0)
        return;
    
    // Static method -> no mutex!
    
    nvgSave(vg);
        
    nvgFontSize(vg, fontSize*fontSizeCoeff);
	nvgFontFace(vg, GRAPH_FONT);
    nvgFontBlur(vg, 0);
	nvgTextAlign(vg, halign | valign);
    
    int sColor[4] = { color[0], color[1], color[2], color[3] };
    SWAP_COLOR(sColor);
    
    nvgFillColor(vg, nvgRGBA(sColor[0], sColor[1], sColor[2], sColor[3]));

    BL_GUI_FLOAT yf = y;
#if GRAPH_CONTROL_FLIP_Y
    yf = graphHeight - y;
#endif
    
	nvgText(vg, x, yf, text, NULL);
    
    nvgRestore(vg);
}

void
GraphControl12::AddCustomDrawer(GraphCustomDrawer *customDrawer)
{
    mCustomDrawers.push_back(customDrawer);
}

void
GraphControl12::RemoveCustomDrawer(GraphCustomDrawer *customDrawer)
{
    vector<GraphCustomDrawer *> newCustomDrawers;
    for (int i = 0; i < mCustomDrawers.size(); i++)
    {
        GraphCustomDrawer *d = mCustomDrawers[i];
        if (d != customDrawer)
            newCustomDrawers.push_back(d);
    }
    
    mCustomDrawers = newCustomDrawers;
    
    //
    mDataChanged = true;
}


void
GraphControl12::CustomDrawersPreDraw()
{
    int width = this->mRECT.W();
    int height = this->mRECT.H();
    
    for (int i = 0; i < mCustomDrawers.size(); i++)
    {
        GraphCustomDrawer *drawer = mCustomDrawers[i];
      
        // Draw
        drawer->PreDraw(mVg, width, height);
    }
}

void
GraphControl12::CustomDrawersPostDraw()
{
    int width = this->mRECT.W();
    int height = this->mRECT.H();
    
    for (int i = 0; i < mCustomDrawers.size(); i++)
    {
        GraphCustomDrawer *drawer = mCustomDrawers[i];
     
        // Draw
        drawer->PostDraw(mVg, width, height);
    }
}

void
GraphControl12::DrawSeparatorY0()
{
    if (!mSeparatorY0)
        return;
    
    nvgSave(mVg);
    nvgStrokeWidth(mVg, mSepY0LineWidth);
    
    int sepColor[4] = { mSepY0Color[0],  mSepY0Color[1],
                        mSepY0Color[2], mSepY0Color[3] };
    SWAP_COLOR(sepColor);
    
    nvgStrokeColor(mVg, nvgRGBA(sepColor[0], sepColor[1], sepColor[2], sepColor[3]));
    
    int width = this->mRECT.W();
    int height = this->mRECT.H();
    
    // Draw a vertical line ath the bottom
    nvgBeginPath(mVg);
    
    BL_GUI_FLOAT x0 = 0;
    BL_GUI_FLOAT x1 = width;
    
    BL_GUI_FLOAT y = mSepY0LineWidth/2.0;
    
    BL_GUI_FLOAT yf = y;
#if GRAPH_CONTROL_FLIP_Y
    yf = height - yf;
#endif
    
    nvgMoveTo(mVg, x0, yf);
    nvgLineTo(mVg, x1, yf);
                    
    nvgStroke(mVg);
    
    nvgRestore(mVg);
}

void
GraphControl12::AddCustomControl(GraphCustomControl *customControl)
{
    mCustomControls.push_back(customControl);
}

void
GraphControl12::OnMouseDown(float x, float y, const IMouseMod &mod)
{
    // ??
    //WDL_MutexLock lock(&mMutex);
    
    IControl::OnMouseDown(x, y, mod);
    
#if CUSTOM_CONTROL_FIX
    x -= mRECT.L;
    y -= mRECT.T;
#endif
    
    for (int i = 0; i < mCustomControls.size(); i++)
    {
        GraphCustomControl *control = mCustomControls[i];
        control->OnMouseDown(x, y, mod);
    }
}

void
GraphControl12::OnMouseUp(float x, float y, const IMouseMod &mod)
{   
    IControl::OnMouseUp(x, y, mod);
    
#if CUSTOM_CONTROL_FIX
    x -= mRECT.L;
    y -= mRECT.T;
#endif

    for (int i = 0; i < mCustomControls.size(); i++)
    {
        GraphCustomControl *control = mCustomControls[i];
        control->OnMouseUp(x, y, mod);
    }
}

void
GraphControl12::OnMouseDrag(float x, float y, float dX, float dY,
                            const IMouseMod &mod)
{   
    IControl::OnMouseDrag(x, y, dX, dY, mod);
    
#if CUSTOM_CONTROL_FIX
    x -= mRECT.L;
    y -= mRECT.T;
#endif

    for (int i = 0; i < mCustomControls.size(); i++)
    {
        GraphCustomControl *control = mCustomControls[i];
        control->OnMouseDrag(x, y, dX, dY, mod);
    }
}

void
GraphControl12::OnMouseDblClick(float x, float y, const IMouseMod &mod)
{    
    IControl::OnMouseDblClick(x, y, mod);
    
    // #bl-iplug2
    //if (!dblClickDone)
    //    return; // false;
    
#if CUSTOM_CONTROL_FIX
    x -= mRECT.L;
    y -= mRECT.T;
#endif

    for (int i = 0; i < mCustomControls.size(); i++)
    {
        GraphCustomControl *control = mCustomControls[i];
        control->OnMouseDblClick(x, y, mod);
    }
}

void
GraphControl12::OnMouseWheel(float x, float y, const IMouseMod &mod, float d)
{    
    IControl::OnMouseWheel(x, y, mod, d);
    
#if CUSTOM_CONTROL_FIX
    x -= mRECT.L;
    y -= mRECT.T;
#endif

    for (int i = 0; i < mCustomControls.size(); i++)
    {
        GraphCustomControl *control = mCustomControls[i];
        control->OnMouseWheel(x, y, mod, d);
    }
}

bool
GraphControl12::OnKeyDown(float x, float y, const IKeyPress& key)
{   
    // #bl-iplug2
    IControl::OnKeyDown(x, y, key);
    
#if CUSTOM_CONTROL_FIX
    x -= mRECT.L;
    y -= mRECT.T;
#endif

    bool res = false;
    for (int i = 0; i < mCustomControls.size(); i++)
    {
        GraphCustomControl *control = mCustomControls[i];
        res = control->OnKeyDown(x, y, key);
    }
    
    return res;
}

bool
GraphControl12::OnKeyUp(float x, float y, const IKeyPress& key)
{    
    // #bl-iplug2
    IControl::OnKeyUp(x, y, key);
    
#if CUSTOM_CONTROL_FIX
    x -= mRECT.L;
    y -= mRECT.T;
#endif
    
    bool res = false;
    for (int i = 0; i < mCustomControls.size(); i++)
    {
        GraphCustomControl *control = mCustomControls[i];
        res = control->OnKeyUp(x, y, key);
    }
    
    return res;
}

void
GraphControl12::OnMouseOver(float x, float y, const IMouseMod &mod)
{
    IControl::OnMouseOver(x, y, mod);
    
#if CUSTOM_CONTROL_FIX
    x -= mRECT.L;
    y -= mRECT.T;
#endif

    for (int i = 0; i < mCustomControls.size(); i++)
    {
        GraphCustomControl *control = mCustomControls[i];
        control->OnMouseOver(x, y, mod);
    }
}

void
GraphControl12::OnMouseOut()
{    
    IControl::OnMouseOut();
    
    for (int i = 0; i < mCustomControls.size(); i++)
    {
        GraphCustomControl *control = mCustomControls[i];
        control->OnMouseOut();
    }
}

void
GraphControl12::DBG_PrintCoords(int x, int y)
{
    int width = this->mRECT.W();
    int height = this->mRECT.H();
    
    BL_GUI_FLOAT tX = ((BL_GUI_FLOAT)x)/width;
    BL_GUI_FLOAT tY = ((BL_GUI_FLOAT)y)/height;
    
    tX = (tX - mBounds[0])/(mBounds[2] - mBounds[0]);
    tY = (tY - mBounds[1])/(mBounds[3] - mBounds[1]);
    
    fprintf(stderr, "(%g, %g)\n", tX, 1.0 - tY);
}

void
GraphControl12::SetBackgroundImage(IBitmap bmp)
{
    mBgImage = bmp;
    
    mDataChanged = true;
}

void
GraphControl12::SetOverlayImage(IBitmap bmp)
{
    mOverlayImage = bmp;
    
    mDataChanged = true;
}

void
GraphControl12::SetDisablePointOffsetHack(bool flag)
{
    mDisablePointOffsetHack = flag;
}

void
GraphControl12::SetRecreateWhiteImageHack(bool flag)
{
    mRecreateWhiteImageHack = flag;
}

void
GraphControl12::SetGraphTimeAxis(GraphTimeAxis6 *timeAxis)
{
    mGraphTimeAxis = timeAxis;
}

void
GraphControl12::SetTransport(BLTransport *transport)
{
    mTransport = transport;
}

void
GraphControl12::SetDataChanged()
{
    mDataChanged = true;
}

void
GraphControl12::SetDirty(bool triggerAction, int valIdx)
{
    // Set dirty only if the graph is enabled.
    // (otherwise we have the risk to try to draw a graph that is disabled).
    if (mIsEnabled)
    {
        IControl::SetDirty(triggerAction, valIdx);
    }
}

// Hack
bool
GraphControl12::IsDirty()
{
    // Always dirty => Force redraw!
    return true;
}

void
GraphControl12::SetValueToDefault(int valIdx)
{
    // Do nothing.

    // It is used to avoid "trying to reset" the graph parameter
    // when ctrl-click on the graph
    // If so, IControl would call SetDirty(true), then OnParamChange()
    // and finally make a pthread assertion
    // (certainly mutex_lock() called two times on the same mutex in the
    // same thread
}
    
void
GraphControl12::DisplayCurveDescriptions()
{
#define OFFSET_Y 4.0
    
#define DESCR_X 40.0
#define DESCR_Y0 10.0 + OFFSET_Y
    
#define DESCR_WIDTH 20
#define DESCR_Y_STEP 12
#define DESCR_SPACE 5
    
#define TEXT_Y_OFFSET 2
    
    int height = this->mRECT.H();
    
    int descrNum = 0;
    for (int i = 0; i < mCurves.size(); i++)
    {
        GraphCurve5 *curve = mCurves[i];
        char *descr = curve->mDescription;
        if (descr == NULL)
            continue;
        
        BL_GUI_FLOAT y = this->mRECT.H() - (DESCR_Y0 + descrNum*DESCR_Y_STEP);
        
        nvgSave(mVg);
        
        // Must force alpha to 1, because sometimes,
        // the plugins hide the curves, but we still
        // want to display the description
        BL_GUI_FLOAT prevAlpha = curve->mAlpha;
        curve->mAlpha = 1.0;
        
        SetCurveDrawStyle(curve);
        
        curve->mAlpha = prevAlpha;
        
        y += TEXT_Y_OFFSET;
        
        BL_GUI_FLOAT yf = y;
#if GRAPH_CONTROL_FLIP_Y
        yf = height - yf;
#endif
        
        nvgBeginPath(mVg);
        
        nvgMoveTo(mVg, DESCR_X, yf);
        nvgLineTo(mVg, DESCR_X + DESCR_WIDTH, yf);
        
        nvgStroke(mVg);
        
        DrawText(DESCR_X + DESCR_WIDTH + DESCR_SPACE,
                 y,
                 FONT_SIZE, descr,
                 curve->mDescrColor,
                 NVG_ALIGN_LEFT, NVG_ALIGN_MIDDLE);
        
        nvgRestore(mVg);
        
        descrNum++;
    }
}

void
GraphControl12::DrawAxis(bool lineLabelFlag)
{
    if (mHAxis != NULL)
        DrawAxis(mHAxis, true, lineLabelFlag);
    
    if (mVAxis != NULL)
        DrawAxis(mVAxis, false, lineLabelFlag);
}

void
GraphControl12::DrawAxis(GraphAxis2 *axis, bool horizontal, bool lineLabelFlag)
{
    nvgSave(mVg);
    
    nvgStrokeWidth(mVg, axis->mLineWidth);
    //nvgStrokeWidth(mVg, 1.0); // orig
    //nvgStrokeWidth(mVg, 1.5);
    
    int axisColor[4] = { axis->mColor[0], axis->mColor[1],
                         axis->mColor[2], axis->mColor[3] };
    SWAP_COLOR(axisColor);
    
    int axisLabelOverlayColor[4] = { axis->mLabelOverlayColor[0],
                                     axis->mLabelOverlayColor[1],
                                     axis->mLabelOverlayColor[2],
                                     axis->mLabelOverlayColor[3] };
    SWAP_COLOR(axisLabelOverlayColor);
    
    int axisLinesOverlayColor[4] = { axis->mLinesOverlayColor[0],
                                     axis->mLinesOverlayColor[1],
                                     axis->mLinesOverlayColor[2],
                                     axis->mLinesOverlayColor[3] };
    SWAP_COLOR(axisLinesOverlayColor);
    
    nvgStrokeColor(mVg, nvgRGBA(axisColor[0], axisColor[1],
                                axisColor[2], axisColor[3]));
    
    int width = this->mRECT.W();
    int height = this->mRECT.H();
    
    for (int i = 0; i < axis->mValues.size(); i++)
    {
        const GraphAxis2::GraphAxisData &data = axis->mValues[i];
        
        BL_GUI_FLOAT t = data.mT;
        const char *text = data.mText.c_str();
        
        if (horizontal)
        {
            BL_GUI_FLOAT textOffset = FONT_SIZE*0.2;
       
            // Do not scale to db or log here, this is done somewhere else!
            
            BL_GUI_FLOAT x = t*width;

            BL_GUI_FLOAT xLabel = x;
            xLabel += axis->mOffsetPixels;
            
            if (((i > 0) && (i < axis->mValues.size() - 1)) ||
                !axis->mAlignBorderLabels)
            {
                if (lineLabelFlag)
                {
                    BL_GUI_FLOAT y0 = 0.0;
                    BL_GUI_FLOAT y1 = height;
        
                    BL_GUI_FLOAT y0f = y0;
                    BL_GUI_FLOAT y1f = y1;
#if GRAPH_CONTROL_FLIP_Y
                    y0f = height - y0f;
                    y1f = height - y1f;
#endif

                    // Draw a vertical line
                    nvgBeginPath(mVg);

#if ALIGN_AXES_LINES_TO_PIX
                    if (axis->mAlignToScreenPixels)
                        x = (int)x;
#endif

#if ALIGN_AXES_LABELS_TO_PIX
                    if (axis->mAlignToScreenPixels)
                        xLabel = (int)xLabel;
#endif
                    
                    nvgMoveTo(mVg, x, y0f);
                    nvgLineTo(mVg, x, y1f);
    
                    nvgStroke(mVg);
                    
                    if (axis->mLinesOverlay)
                    {
                        nvgStrokeColor(mVg,
                                       nvgRGBA(axisLinesOverlayColor[0],
                                               axisLinesOverlayColor[1],
                                               axisLinesOverlayColor[2],
                                               axisLinesOverlayColor[3]));
                        
                        BL_GUI_FLOAT y0 = 0.0;
                        BL_GUI_FLOAT y1 = height;
                        
                        BL_GUI_FLOAT y0f = y0;
                        BL_GUI_FLOAT y1f = y1;
#if GRAPH_CONTROL_FLIP_Y
                        y0f = height - y0f;
                        y1f = height - y1f;
#endif
                        
                        // Draw a vertical line
                        nvgBeginPath(mVg);
                        
                        nvgMoveTo(mVg, x + OVERLAY_OFFSET, y0f);
                        nvgLineTo(mVg, x + OVERLAY_OFFSET, y1f);
                        nvgStroke(mVg);
                        
                        nvgStrokeColor(mVg, nvgRGBA(axisColor[0],
                                                    axisColor[1],
                                                    axisColor[2],
                                                    axisColor[3]));
                    }
                }
                else
                {
                    if (axis->mOverlay)
                    {
                        // Draw background text (for overlay)
                        DrawText(xLabel + OVERLAY_OFFSET,
                                 textOffset + axis->mOffsetY*height + OVERLAY_OFFSET,
                                 FONT_SIZE, text,
                                 axis->mLabelOverlayColor,
                                 NVG_ALIGN_CENTER, NVG_ALIGN_BOTTOM,
                                 axis->mFontSizeCoeff);
                    }
                    
                    DrawText(xLabel,
                             textOffset + axis->mOffsetY*height,
                             FONT_SIZE, text, axis->mLabelColor,
                             NVG_ALIGN_CENTER, NVG_ALIGN_BOTTOM,
                             axis->mFontSizeCoeff);
                }
            }

            if (!lineLabelFlag && axis->mAlignBorderLabels)
            {
                if (i == 0)
                {
                    if (axis->mOverlay)
                    {
                        // Draw background text (for overlay)
                        DrawText(xLabel + textOffset + OVERLAY_OFFSET,
                                 textOffset + axis->mOffsetY*height + OVERLAY_OFFSET,
                                 FONT_SIZE, text,
                                 axis->mLabelOverlayColor,
                                 NVG_ALIGN_LEFT, NVG_ALIGN_BOTTOM,
                                 axis->mFontSizeCoeff);
                    }

                    // First text: aligne left
                    DrawText(xLabel + textOffset,
                             textOffset + axis->mOffsetY*height, FONT_SIZE,
                             text, axis->mLabelColor,
                             NVG_ALIGN_LEFT, NVG_ALIGN_BOTTOM,
                             axis->mFontSizeCoeff);
                }
        
                if (i == axis->mValues.size() - 1)
                {
                    if (axis->mOverlay)
                    {
                        // Draw background text (for overlay)
                        DrawText(xLabel - textOffset + OVERLAY_OFFSET,
                                 textOffset + axis->mOffsetY*height + OVERLAY_OFFSET,
                                 FONT_SIZE, text,
                                 axis->mLabelOverlayColor,
                                 NVG_ALIGN_RIGHT, NVG_ALIGN_BOTTOM,
                                 axis->mFontSizeCoeff);
                    }
                    
                    // Last text: align right
                    DrawText(xLabel - textOffset,
                             textOffset + axis->mOffsetY*height,
                             FONT_SIZE, text, axis->mLabelColor,
                             NVG_ALIGN_RIGHT, NVG_ALIGN_BOTTOM,
                             axis->mFontSizeCoeff);
                }
            }
        }
        else
            // Vertical
        {
            BL_GUI_FLOAT textOffset = FONT_SIZE*0.2;

            t = ConvertToBoundsY(t);

            t = ConvertToAxisBounds(axis, t);
            
            BL_GUI_FLOAT y = t*height;
            
            // Hack
            // See Transient, with it 2 vertical axis
            //y += axis->mOffset;
            
            //y += axis->mOffsetY; // For Ghost
            y += axis->mOffsetY*height; // For Ghost

            BL_GUI_FLOAT yLabel = y;
            yLabel += axis->mOffsetPixels;
            
            if (((i > 0) && (i < axis->mValues.size() - 1)) ||
                !axis->mAlignBorderLabels)
                // First and last: don't draw axis line
            {
                if (lineLabelFlag)
                {
                    BL_GUI_FLOAT x0 = 0.0;
                    BL_GUI_FLOAT x1 = width;
                    
                    // If overlay, put the two lines at y-0.5 and y+0.5
                    // (so this looks more accurate in Chroma for example)
                    if (axis->mLinesOverlay)
                        y -= OVERLAY_OFFSET*0.5;

#if ALIGN_AXES_LINES_TO_PIX
                    if (axis->mAlignToScreenPixels)
                        y = (int)y;
#endif

#if ALIGN_AXES_LABELS_TO_PIX
                    if (axis->mAlignToScreenPixels)
                        yLabel = (int)yLabel;
#endif
                    
                    BL_GUI_FLOAT yf = y;
                    // NOTE: set from 0 to GRAPH_CONTROL_FLIP_Y for SpectralDiff
                    // (when using [-119dB-10dB), so the lines are well aligned with the labels)
#if GRAPH_CONTROL_FLIP_Y // 0 ??
                    yf = height - yf;
#endif
                    
                    // Draw a horizontal line
                    nvgBeginPath(mVg);
                    
                    nvgMoveTo(mVg, x0, yf);
                    nvgLineTo(mVg, x1, yf);
                    
                    nvgStroke(mVg);
                    
                    if (axis->mLinesOverlay)
                    {
                        nvgStrokeColor(mVg,
                                       nvgRGBA(axisLinesOverlayColor[0],
                                               axisLinesOverlayColor[1],
                                               axisLinesOverlayColor[2],
                                               axisLinesOverlayColor[3]));
                        
                        BL_GUI_FLOAT x0 = 0.0;
                        BL_GUI_FLOAT x1 = width;
                        
                        BL_GUI_FLOAT yf = y + OVERLAY_OFFSET;
#if GRAPH_CONTROL_FLIP_Y
                        yf = height - yf;
#endif
                        
                        // Draw a vertical line
                        nvgBeginPath(mVg);
                        
                        nvgMoveTo(mVg, x0, yf);
                        nvgLineTo(mVg, x1, yf);
                        nvgStroke(mVg);
                        
                        nvgStrokeColor(mVg, nvgRGBA(axisColor[0], axisColor[1],
                                                    axisColor[2], axisColor[3]));
                    }
                }
                else
                {
                    int align = NVG_ALIGN_LEFT;
                    if (axis->mAlignTextRight)
                        align = NVG_ALIGN_RIGHT;
                    
                    if (axis->mOverlay)
                    {
                        // Draw background text (for overlay)
                        //DrawText(textOffset + axis->mOffsetX + OVERLAY_OFFSET,
                        DrawText(textOffset + axis->mOffsetX*width + OVERLAY_OFFSET,
                                 yLabel + OVERLAY_OFFSET,
                                 FONT_SIZE, text, axis->mLabelOverlayColor,
                                 align | NVG_ALIGN_MIDDLE, NVG_ALIGN_BOTTOM,
                                 axis->mFontSizeCoeff);
                    }
                    
                    //DrawText(textOffset + axis->mOffsetX,
                    DrawText(textOffset + axis->mOffsetX*width,
                             yLabel, FONT_SIZE, text,
                             axis->mLabelColor,
                             align | NVG_ALIGN_MIDDLE,
                             NVG_ALIGN_BOTTOM,
                             axis->mFontSizeCoeff);
                }
            }
            
            if (!lineLabelFlag && axis->mAlignBorderLabels)
            {
                if (i == 0)
                    // First text: align "top"
                {
                    if (axis->mOverlay)
                    {
                        // Draw background text (for overlay)
                        //DrawText(textOffset + axis->mOffsetX + OVERLAY_OFFSET,
                        DrawText(textOffset + axis->mOffsetX*width + OVERLAY_OFFSET,
                                 yLabel + FONT_SIZE*0.75 + OVERLAY_OFFSET,
                                 FONT_SIZE, text, axis->mLabelOverlayColor,
                                 NVG_ALIGN_LEFT, NVG_ALIGN_BOTTOM,
                                 axis->mFontSizeCoeff);
                    }

                    DrawText(textOffset + axis->mOffsetX*width,
                             yLabel + FONT_SIZE*0.75, FONT_SIZE, text,
                             axis->mLabelColor, NVG_ALIGN_LEFT, NVG_ALIGN_BOTTOM,
                             axis->mFontSizeCoeff);
                }
                
                if (i == axis->mValues.size() - 1)
                    // Last text: align "bottom"
                {
                    if (axis->mOverlay)
                    {
                        // Draw background text (for overlay)
                        DrawText(textOffset + axis->mOffsetX*width + OVERLAY_OFFSET,
                                 yLabel - FONT_SIZE*1.5 + OVERLAY_OFFSET,
                                 FONT_SIZE, text, axis->mLabelOverlayColor,
                                 NVG_ALIGN_LEFT, NVG_ALIGN_BOTTOM,
                                 axis->mFontSizeCoeff);
                    }
                    
                    DrawText(textOffset + axis->mOffsetX*width,
                             yLabel - FONT_SIZE*1.5, FONT_SIZE, text,
                             axis->mLabelColor, NVG_ALIGN_LEFT, NVG_ALIGN_BOTTOM,
                             axis->mFontSizeCoeff);
                }
            }
        }
    }
    
    nvgRestore(mVg);
}

void
GraphControl12::DrawCurves()
{
    for (int i = 0; i < mCurves.size(); i++)
    {
        if (!mCurves[i]->mSingleValueH && !mCurves[i]->mSingleValueV)
        {
            if (mCurves[i]->mPointStyle)
            {
                if (!mCurves[i]->mPointsAsLinesPolar && !mCurves[i]->mPointsAsLines)
                {
#if !OPTIM_QUADS_SAME_COLOR
                    DrawPointCurve(mCurves[i]);
#else
                    if (!mCurves[i]->mOptimSameColor)
                    {
                        DrawPointCurve(mCurves[i]);
                    }
                    else
                        DrawPointCurveOptimSameColor(mCurves[i]);
#endif
                }
                else
                {
                    if (mCurves[i]->mPointsAsLinesPolar)
                    {
                        if (!mCurves[i]->mCurveFill)
                        {
                            if (mCurves[i]->mWeights.GetSize() == 0)
                                DrawPointCurveLinesPolar(mCurves[i]);
                            else
                                DrawPointCurveLinesPolarWeights(mCurves[i]);
                        }
                        else
                            DrawPointCurveLinesPolarFill(mCurves[i]);
                    }
                    else
                    {
                        if (mCurves[i]->mPointsAsLines)
                        {
                            if (!mCurves[i]->mCurveFill)
                            {
                                if (mCurves[i]->mWeights.GetSize() == 0)
                                    DrawPointCurveLines(mCurves[i]);
                                else
                                    DrawPointCurveLinesWeights(mCurves[i]);
                            }
                            else
                                DrawPointCurveLinesFill(mCurves[i]);
                        }
                    }
                }
            }
            else if (mCurves[i]->mCurveFill)
            {
                DrawFillCurve(mCurves[i]);
                
#if FILL_CURVE_HACK
                DrawLineCurve(mCurves[i]);
#endif
            }
            else
            {
                DrawLineCurve(mCurves[i]);
            }
        }
        else
        {
            if (mCurves[i]->mSingleValueH)
            {
                if (mCurves[i]->mYValues.GetSize() != 0)
                {
                    if (mCurves[i]->mCurveFill)
                    {
                        DrawFillCurveSVH(mCurves[i]);
                    
#if FILL_CURVE_HACK
                        DrawLineCurveSVH(mCurves[i]);
#endif
                    }
            
                    DrawLineCurveSVH(mCurves[i]);
                }
            }
            else
                if (mCurves[i]->mSingleValueV)
                {
                    if (mCurves[i]->mXValues.GetSize() != 0)
                    {
                        if (mCurves[i]->mCurveFill)
                        {
                            DrawFillCurveSVV(mCurves[i]);
                            
#if FILL_CURVE_HACK
                            DrawLineCurveSVV(mCurves[i]);
#endif
                        }
                        
                        DrawLineCurveSVV(mCurves[i]);
                    }
                }
        }
    }
}

void
GraphControl12::DrawLineCurve(GraphCurve5 *curve)
{
#if CURVE_DEBUG
    int numPointsDrawn = 0;
#endif
    
    // For UST
    if (curve->mLineWidth < 0.0)
        return;
    
#if FIX_UNDEFINED_CURVES
    bool curveUndefined = IsCurveUndefined(curve->mXValues, curve->mYValues, 2);
    if (curveUndefined)
        return;
#endif
    
    int height = this->mRECT.H();
    
    nvgSave(mVg);
    
    SetCurveDrawStyle(curve);
    
    nvgBeginPath(mVg);
    
#if CURVE_OPTIM
    BL_GUI_FLOAT prevX = -1.0;
#endif
    
    bool firstPoint = true;
    for (int i = 0; i < curve->mXValues.GetSize(); i ++)
    {
        BL_GUI_FLOAT x = curve->mXValues.Get()[i];
        
        if (x >= CURVE_VALUE_UNDEFINED)
            continue;
        
#if CURVE_OPTIM
        if (mNumCurveValues >= CURVE_OPTIM_THRS)
        {
            if (x - prevX < 1.0)
                // Less than 1 pixel. Do not display.
                continue;
        }
        
        prevX = x;
#endif
        
#if CURVE_DEBUG
        numPointsDrawn++;
#endif
        
        BL_GUI_FLOAT y = curve->mYValues.Get()[i];
        if (y >= CURVE_VALUE_UNDEFINED)
            continue;
        
        BL_GUI_FLOAT yf = y;
#if GRAPH_CONTROL_FLIP_Y
        yf = height - yf;
#endif
        
        if (firstPoint)
        {
            nvgMoveTo(mVg, x, yf);
            
            firstPoint = false;
        }
        
        nvgLineTo(mVg, x, yf);
    }
    
    nvgStroke(mVg);
    nvgRestore(mVg);
    
#if CURVE_DEBUG
    fprintf(stderr, "GraphControl12::DrawLineCurve - num points: %d\n", numPointsDrawn);
#endif
}

#if !FILL_CURVE_HACK
// Bug with direct rendering
// It seems we have no stencil, and we can only render convex polygons
void
GraphControl12::DrawFillCurve(GraphCurve5 *curve)
{
#if FIX_UNDEFINED_CURVES
    bool curveUndefined = IsCurveUndefined(curve->mXValues, curve->mYValues, 2);
    if (curveUndefined)
        return;
#endif
    
    // Offset used to draw the closing of the curve outside the viewport
    // Because we draw both stroke and fill at the same time
    float offset = curve->mLineWidth;
    
    int height = this->mRECT.H();
    
    nvgSave(mVg);
    
    CurveColor fillColor;
    for (int i = 0; i < 4; i++)
    {
        fillColor[i] = curve->mColor[i];
        
        if (curve->mFillColorSet)
            fillColor[i] = curve->mFillColor[i];
    }
    
    int sFillColor[4] = { (int)(fillColor[0]*255), (int)(fillColor[1]*255),
                          (int)(fillColor[2]*255), (int)(curve->mFillAlpha*255) };
    SWAP_COLOR(sFillColor);
    
    int sStrokeColor[4] = { (int)(curve->mColor[0]*255), (int)(curve->mColor[1]*255),
                            (int)(curve->mColor[2]*255), (int)(curve->mAlpha*255) };
    SWAP_COLOR(sStrokeColor);
    
    nvgBeginPath(mVg);
    
    BL_GUI_FLOAT x0 = 0.0;
    for (int i = 0; i < curve->mXValues.GetSize(); i ++)
    {        
        BL_GUI_FLOAT x = curve->mXValues.Get()[i];
        BL_GUI_FLOAT y = curve->mYValues.Get()[i];
        
        if (x >= CURVE_VALUE_UNDEFINED)
            continue;
        
        if (y >= CURVE_VALUE_UNDEFINED)
            continue;
        
#if CURVE_DEBUG
        numPointsDrawn++;
#endif
        
        BL_GUI_FLOAT yf = y;
        BL_GUI_FLOAT y1f = curve->mCurveFillOriginY*height - offset;
#if GRAPH_CONTROL_FLIP_Y
        yf = height - yf;
        y1f = height - y1f;
#endif
        
        if (i == 0)
        {
            x0 = x;
            
            nvgMoveTo(mVg, x0 - offset, y1f);
            nvgLineTo(mVg, x - offset, yf);
        }
        
        nvgLineTo(mVg, x, yf);
        
        if (i >= curve->mXValues.GetSize() - 1)
            // Close
        {
            nvgLineTo(mVg, x + offset, yf);
            nvgLineTo(mVg, x + offset, y1f);
            
            nvgClosePath(mVg);
        }
    }
    
    nvgFillColor(mVg, nvgRGBA(sFillColor[0], sFillColor[1], sFillColor[2], sFillColor[3]));
	nvgFill(mVg);
    
    nvgStrokeColor(mVg, nvgRGBA(sStrokeColor[0], sStrokeColor[1],
                                sStrokeColor[2], sStrokeColor[3]));
    
    nvgStrokeWidth(mVg, curve->mLineWidth);
    nvgStroke(mVg);

    nvgRestore(mVg);
}
#endif

#if FILL_CURVE_HACK
// Due to a bug, we render only convex polygons when filling curves
// So we separate in rectangles
void
GraphControl12::DrawFillCurve(GraphCurve5 *curve)
{
#if FIX_UNDEFINED_CURVES
    bool curveUndefined = IsCurveUndefined(curve->mXValues, curve->mYValues, 2);
    if (curveUndefined)
        return;
#endif

#if CURVE_DEBUG
    int numPointsDrawn = 0;
#endif
    
    int height = this->mRECT.H();
    BL_GUI_FLOAT originY = curve->mCurveFillOriginY*height;
    
    // Offset used to draw the closing of the curve outside the viewport
    // Because we draw both stroke and fill at the same time
#define OFFSET lineWidth
    
    nvgSave(mVg);
    
    // Take the curve color, or the fill color if set appart (certainly different)
    CurveColor color;
    for (int i = 0; i < 4; i++)
    {
        color[i] = curve->mColor[i];
        if (curve->mFillColorSet)
            color[i] = curve->mFillColor[i];
    }
    
    int sFillColor[4] = { (int)(color[0]*255), (int)(color[1]*255),
        (int)(color[2]*255), (int)(curve->mFillAlpha*255) };
    SWAP_COLOR(sFillColor);
    
    BL_GUI_FLOAT prevX = -1.0;
    BL_GUI_FLOAT prevY = originY;
    
#if !FIX_CURVE_FILL_LAST_COLUMN
    for (int i = 0; i < curve->mXValues.GetSize() - 1; i ++)
#else
    for (int i = 0; i < curve->mXValues.GetSize(); i++)
#endif
    {
        BL_GUI_FLOAT x = curve->mXValues.Get()[i];
        if (x >= CURVE_VALUE_UNDEFINED)
            continue;
        
        BL_GUI_FLOAT y = curve->mYValues.Get()[i];
        if (y >= CURVE_VALUE_UNDEFINED)
            continue;
        
        if (prevX < 0.0)
            // Init
        {
            prevX = x;
            prevY = y;
            
            continue;
        }
        
#if !FIX_CURVE_FILL_LAST_COLUMN
        // Avoid any overlap
        // The problem can bee seen using alpha
        if (x - prevX < 2.0) // More than 1.0
            continue;
#else
        // For last point, do not discard
        if (i < curve->mXValues.GetSize() - 1)
        {
            if (x - prevX < 2.0) // More than 1.0
                continue;
        }
#endif
        
#if FILL_CURVE_HACK2
        // Go back from 1 pixel
        // (otherwise there will be line holes between filles polygons...)
        prevX -= 1.0;
#endif
        
        // Offset x by 1, for drawing
#if FILL_CURVE_HACK3
        x += 1;
        prevX += 1;
#endif
        
        
        BL_GUI_FLOAT yf = y;
        BL_GUI_FLOAT prevYf = prevY;
        BL_GUI_FLOAT originYf = originY;
#if GRAPH_CONTROL_FLIP_Y
        yf = height - yf;
        prevYf = height - prevYf;
        originYf = height - originYf;
#endif
        
        nvgBeginPath(mVg);
        
        nvgMoveTo(mVg, prevX, prevYf);
        nvgLineTo(mVg, x, yf);
        
        nvgLineTo(mVg, x, originYf);
        nvgLineTo(mVg, prevX, originYf);
        nvgLineTo(mVg, prevX, prevYf);
        
        nvgClosePath(mVg);
        
        nvgFillColor(mVg, nvgRGBA(sFillColor[0], sFillColor[1], sFillColor[2], sFillColor[3]));
        nvgFill(mVg);
        
        // Revert offset
#if FILL_CURVE_HACK3
        x -= 1;
        prevX -= 1;
#endif
        
        prevX = x;
        prevY = y;
    }
    
    // Fill upside if necessary
    if (curve->mFillAlphaUpFlag)
        // Fill the upper area with specific alpha
    {
        int sFillColorUp[4] = { (int)(color[0]*255), (int)(color[1]*255),
                                (int)(color[2]*255), (int)(curve->mFillAlphaUp*255) };
        
        SWAP_COLOR(sFillColorUp);
        
        int height = this->mRECT.H();
        
        BL_GUI_FLOAT prevX = -1.0;
        BL_GUI_FLOAT prevY = originY;
#if !FIX_CURVE_FILL_LAST_COLUMN
        for (int i = 0; i < curve->mXValues.GetSize() - 1; i ++)
#else
        for (int i = 0; i < curve->mXValues.GetSize(); i ++)
#endif
        {
            BL_GUI_FLOAT x = curve->mXValues.Get()[i];
            if (x >= CURVE_VALUE_UNDEFINED)
                continue;
            
            BL_GUI_FLOAT y = curve->mYValues.Get()[i];
            if (y >= CURVE_VALUE_UNDEFINED)
                continue;
            
            if (prevX < 0.0)
                // Init
            {
                prevX = x;
                prevY = y;
                
                continue;
            }
            
#if !FIX_CURVE_FILL_LAST_COLUMN
            // Avoid any overlap
            // The problem can bee seen using alpha
            if (x - prevX < 2.0) // More than 1.0
                continue;
#else
            // For last point, do not discard
            if (i < curve->mXValues.GetSize() - 1)
            {
                if (x - prevX < 2.0) // More than 1.0
                    continue;
            }
#endif
            
            BL_GUI_FLOAT prevYf = prevY;
            BL_GUI_FLOAT yf = y;
            BL_GUI_FLOAT heightf = height;
#if GRAPH_CONTROL_FLIP_Y
            prevYf = height - prevYf;
            yf = height - yf;
            heightf = 0;
#endif
            
            nvgBeginPath(mVg);
            
            nvgMoveTo(mVg, prevX, prevYf);
            nvgLineTo(mVg, x, yf);
            
            nvgLineTo(mVg, x, heightf);
            nvgLineTo(mVg, prevX, heightf);
            nvgLineTo(mVg, prevX, prevYf);
            
            nvgClosePath(mVg);
            
            nvgFillColor(mVg, nvgRGBA(sFillColorUp[0], sFillColorUp[1], sFillColorUp[2], sFillColorUp[3]));
            nvgFill(mVg);
            
            prevX = x;
            prevY = y;
        }
    }
    
    nvgRestore(mVg);
    
#if CURVE_DEBUG
    fprintf(stderr, "GraphControl12::DrawFillCurve - num points: %d\n", numPointsDrawn);
#endif
}
#endif

void
GraphControl12::DrawLineCurveSVH(GraphCurve5 *curve)
{
    if (curve->mYValues.GetSize() == 0)
        return;
    
    BL_GUI_FLOAT val = curve->mYValues.Get()[0];
    if (val >= CURVE_VALUE_UNDEFINED)
        return;
    
    int width = this->mRECT.W();
    int height = this->mRECT.H();
    
    nvgSave(mVg);
    nvgStrokeWidth(mVg, curve->mLineWidth);
    
    int sColor[4] = { (int)(curve->mColor[0]*255), (int)(curve->mColor[1]*255),
                      (int)(curve->mColor[2]*255), (int)(curve->mAlpha*255) };
    SWAP_COLOR(sColor);
    
    nvgStrokeColor(mVg, nvgRGBA(sColor[0], sColor[1], sColor[2], sColor[3]));
    
    BL_GUI_FLOAT valf = val;
#if GRAPH_CONTROL_FLIP_Y
    valf = height - valf;
#endif
    
    nvgBeginPath(mVg);
    
    BL_GUI_FLOAT x0 = 0;
    BL_GUI_FLOAT x1 = width;
    
    nvgMoveTo(mVg, x0, valf);
    nvgLineTo(mVg, x1, valf);
    
    nvgStroke(mVg);
    nvgRestore(mVg);
}

void
GraphControl12::DrawFillCurveSVH(GraphCurve5 *curve)
{
    if (curve->mYValues.GetSize() == 0)
        return;
    
    BL_GUI_FLOAT val = curve->mYValues.Get()[0];
    if (val >= CURVE_VALUE_UNDEFINED)
        return;
    
    int width = this->mRECT.W();
    int height = this->mRECT.H();
    
    nvgSave(mVg);
    
    int sColor[4] = { (int)(curve->mColor[0]*255), (int)(curve->mColor[1]*255),
                      (int)(curve->mColor[2]*255), (int)(curve->mFillAlpha*255) };
    SWAP_COLOR(sColor);
    
    nvgStrokeColor(mVg, nvgRGBA(sColor[0], sColor[1], sColor[2], sColor[3]));
    
    nvgBeginPath(mVg);
        
    BL_GUI_FLOAT x0 = 0;
    BL_GUI_FLOAT x1 = width;
    BL_GUI_FLOAT y0 = 0;
    BL_GUI_FLOAT y1 = val;
    
    BL_GUI_FLOAT y0f = y0;
    BL_GUI_FLOAT y1f = y1;
#if GRAPH_CONTROL_FLIP_Y
    y0f = height - y0f;
    y1f = height - y1f;
#endif
    
    nvgMoveTo(mVg, x0, y0f);
    nvgLineTo(mVg, x0, y1f);
    nvgLineTo(mVg, x1, y1f);
    nvgLineTo(mVg, x1, y0f);
    
    nvgClosePath(mVg);
    
    nvgFillColor(mVg, nvgRGBA(sColor[0], sColor[1], sColor[2], sColor[3]));
	nvgFill(mVg);
    
    nvgStroke(mVg);
    
    // Fill upside if necessary
    if (curve->mFillAlphaUpFlag)
    // Fill the upper area with specific alpha
    {
        int height = this->mRECT.H();
        
        int sFillColorUp[4] = { (int)(curve->mColor[0]*255), (int)(curve->mColor[1]*255),
                                (int)(curve->mColor[2]*255), (int)(curve->mFillAlphaUp*255) };
        SWAP_COLOR(sFillColorUp);
        
        nvgStrokeColor(mVg, nvgRGBA(sColor[0], sColor[1], sColor[2], sColor[3]));
        
        nvgBeginPath(mVg);
        
        BL_GUI_FLOAT x0 = 0;
        BL_GUI_FLOAT x1 = width;
        BL_GUI_FLOAT y0 = height;
        BL_GUI_FLOAT y1 = val;
        
        BL_GUI_FLOAT y0f = y0;
        BL_GUI_FLOAT y1f = y1;
#if GRAPH_CONTROL_FLIP_Y
        y0f = height - y0f;
        y1f = height - y1f;
#endif
        
        nvgMoveTo(mVg, x0, y0f);
        nvgLineTo(mVg, x0, y1f);
        nvgLineTo(mVg, x1, y1f);
        nvgLineTo(mVg, x1, y0f);
        
        nvgClosePath(mVg);
        
        nvgFillColor(mVg, nvgRGBA(sFillColorUp[0], sFillColorUp[1],
                                  sFillColorUp[2], sFillColorUp[3]));
        nvgFill(mVg);
    }
    
    
    nvgRestore(mVg);
}

void
GraphControl12::DrawLineCurveSVV(GraphCurve5 *curve)
{
    // Finally, take the Y value
    // We will have to care about the curve Y scale !
    if (curve->mYValues.GetSize() == 0)
        return;
    
    int width = this->mRECT.W();
    int height = this->mRECT.H();
    
    BL_GUI_FLOAT val = curve->mYValues.Get()[0];
    if (val >= CURVE_VALUE_UNDEFINED)
        return;
    
    // Hack...
    val /= height;
    val *= width;
    
    nvgSave(mVg);
    nvgStrokeWidth(mVg, curve->mLineWidth);
    
    int sColor[4] = { (int)(curve->mColor[0]*255), (int)(curve->mColor[1]*255),
        (int)(curve->mColor[2]*255), (int)(curve->mAlpha*255) };
    SWAP_COLOR(sColor);
    
    nvgStrokeColor(mVg, nvgRGBA(sColor[0], sColor[1], sColor[2], sColor[3]));
    
    nvgBeginPath(mVg);
    
    BL_GUI_FLOAT y0 = 0;
    BL_GUI_FLOAT y1 = height;
    
    BL_GUI_FLOAT x = val;
    
    BL_GUI_FLOAT y0f = y0;
    BL_GUI_FLOAT y1f = y1;
#if GRAPH_CONTROL_FLIP_Y
    y0f = height - y0f;
    y1f = height - y1f;
#endif
    
    nvgMoveTo(mVg, x, y0f);
    nvgLineTo(mVg, x, y1f);
    
    nvgStroke(mVg);
    nvgRestore(mVg);
}

// Fill right (only, for the moment)
void
GraphControl12::DrawFillCurveSVV(GraphCurve5 *curve)
{
    // Finally, take the Y value
    // We will have to care about the curve Y scale !
    if (curve->mYValues.GetSize() == 0)
        return;
    
    int width = this->mRECT.W();
    int height = this->mRECT.H();
    
    BL_GUI_FLOAT val = curve->mYValues.Get()[0];
    if (val >= CURVE_VALUE_UNDEFINED)
        return;
    
    // Hack...
    val /= height;
    val *= width;
    
    nvgSave(mVg);
    
    int sColor[4] = { (int)(curve->mColor[0]*255), (int)(curve->mColor[1]*255),
        (int)(curve->mColor[2]*255), (int)(curve->mFillAlpha*255) };
    SWAP_COLOR(sColor);
    
    nvgStrokeColor(mVg, nvgRGBA(sColor[0], sColor[1], sColor[2], sColor[3]));
    
    nvgBeginPath(mVg);
    
    BL_GUI_FLOAT y0 = 0;
    BL_GUI_FLOAT y1 = height;
    BL_GUI_FLOAT x0 = val;
    BL_GUI_FLOAT x1 = width;
    
    BL_GUI_FLOAT y0f = y0;
    BL_GUI_FLOAT y1f = y1;
#if GRAPH_CONTROL_FLIP_Y
    y0f = height - y0f;
    y1f = height - y1f;
#endif
    
    nvgMoveTo(mVg, x0, y0f);
    nvgLineTo(mVg, x0, y1f);
    nvgLineTo(mVg, x1, y1f);
    nvgLineTo(mVg, x1, y0f);
    
    nvgClosePath(mVg);
    
    nvgFillColor(mVg, nvgRGBA(sColor[0], sColor[1], sColor[2], sColor[3]));
	nvgFill(mVg);
    
    nvgStroke(mVg);
    nvgRestore(mVg);
}

// Optimized !
// Optimization2: nvgQuad
void
GraphControl12::DrawPointCurve(GraphCurve5 *curve)
{
#if FIX_UNDEFINED_CURVES
    bool curveUndefined = IsCurveUndefined(curve->mXValues, curve->mYValues, 1);
    if (curveUndefined)
        return;
#endif

    int height = this->mRECT.H();
    
    nvgSave(mVg);
    
    SetCurveDrawStyle(curve);
    
    if (curve->mPointOverlay)
        nvgGlobalCompositeOperation(mVg, NVG_LIGHTER);
    
    BL_GUI_FLOAT pointSize = curve->mPointSize;
    
    if (mRecreateWhiteImageHack)
    {
        unsigned char white[4] = { 255, 255, 255, 255 };
        if (mWhitePixImg >= 0)
            nvgDeleteImage(mVg, mWhitePixImg);
        mWhitePixImg = nvgCreateImageRGBA(mVg,
                                          1, 1,
                                          NVG_IMAGE_NEAREST,
                                          white);
    }
    
    for (int i = 0; i < curve->mXValues.GetSize(); i ++)
    {
        BL_GUI_FLOAT x = curve->mXValues.Get()[i];
        if (x >= CURVE_VALUE_UNDEFINED)
            continue;
        
        BL_GUI_FLOAT y = curve->mYValues.Get()[i];
        if (y >= CURVE_VALUE_UNDEFINED)
            continue;
        
        if (curve->mWeights.GetSize() == curve->mXValues.GetSize())
        {
            BL_GUI_FLOAT weight = curve->mWeights.Get()[i];
            
            SetCurveDrawStyleWeight(curve, weight);
        }
        
        if (!mDisablePointOffsetHack)
        {
            // FIX: when points are very big, they are not centered
            x -= pointSize/2.0;
            y -= pointSize/2.0;
        }
        
        BL_GUI_FLOAT yf = y;
#if GRAPH_CONTROL_FLIP_Y
        yf = height - yf;
#endif
        
        BL_GUI_FLOAT size = pointSize;
        float corners[4][2] = { { (float)(x - size/2.0), (float)(yf - size/2.0) },
                                { (float)(x + size/2.0), (float)(yf - size/2.0) },
                                { (float)(x + size/2.0), (float)(yf + size/2.0) },
                                { (float)(x - size/2.0), (float)(yf + size/2.0) } };
        
        if (mWhitePixImg < 0)
        {
            unsigned char white[4] = { 255, 255, 255, 255 };
            mWhitePixImg = nvgCreateImageRGBA(mVg,
                                              1, 1,
                                              NVG_IMAGE_NEAREST,
                                              white);
        }
        
        nvgQuad(mVg, corners, mWhitePixImg);
    }
    
#if GRAPH_OPTIM_GL_FIX_OVER_BLEND
    if (curve->mPointOverlay)
        nvgGlobalCompositeOperation(mVg, NVG_SOURCE_OVER);
#endif
    
    nvgRestore(mVg);
}

void
GraphControl12::DrawPointCurveOptimSameColor(GraphCurve5 *curve)
{
#if FIX_UNDEFINED_CURVES
    bool curveUndefined = IsCurveUndefined(curve->mXValues, curve->mYValues, 1);
    if (curveUndefined)
        return;
#endif

    int height = this->mRECT.H();
    
    nvgSave(mVg);
    
    SetCurveDrawStyle(curve);
    
    if (curve->mPointOverlay)
        nvgGlobalCompositeOperation(mVg, NVG_LIGHTER);
    
    if (mRecreateWhiteImageHack)
    {
        unsigned char white[4] = { 255, 255, 255, 255 };
        if (mWhitePixImg >= 0)
            nvgDeleteImage(mVg, mWhitePixImg);
        mWhitePixImg = nvgCreateImageRGBA(mVg,
                                          1, 1,
                                          NVG_IMAGE_NEAREST,
                                          white);
    }
    
    if (mWhitePixImg < 0)
    {
        unsigned char white[4] = { 255, 255, 255, 255 };
        mWhitePixImg = nvgCreateImageRGBA(mVg,
                                          1, 1,
                                          NVG_IMAGE_NEAREST,
                                          white);
    }
    
    int numCenters = 0;
    for (int i = 0; i < curve->mXValues.GetSize(); i++)
    {
        BL_GUI_FLOAT x = curve->mXValues.Get()[i];
        if (x >= CURVE_VALUE_UNDEFINED)
            break;;
        
        BL_GUI_FLOAT y = curve->mYValues.Get()[i];
        if (y >= CURVE_VALUE_UNDEFINED)
            break;
        
        numCenters++;
    }
    
    if (mTmpDrawPointsCenters.GetSize() != numCenters*2)
        mTmpDrawPointsCenters.Resize(numCenters*2);
    float *centersBuf = mTmpDrawPointsCenters.Get();
    
    //
    BL_GUI_FLOAT pointSize = curve->mPointSize;
    
    for (int i = 0; i < numCenters; i++)
    {
        BL_GUI_FLOAT x = curve->mXValues.Get()[i];
        BL_GUI_FLOAT y = curve->mYValues.Get()[i];
        
        if (!mDisablePointOffsetHack)
        {
            // FIX: when points are very big, they are not centered
            x -= pointSize/2.0;
            y -= pointSize/2.0;
        }
        
        BL_GUI_FLOAT yf = y;
#if GRAPH_CONTROL_FLIP_Y
        yf = height - yf;
#endif
        
        centersBuf[i*2] = x;
        centersBuf[i*2 + 1] = yf;
    }
    
#if GRAPH_OPTIM_GL_FIX_OVER_BLEND
    if (curve->mPointOverlay)
        nvgGlobalCompositeOperation(mVg, NVG_SOURCE_OVER);
#endif
    
    nvgQuads(mVg, centersBuf, numCenters, pointSize, mWhitePixImg);
    
    nvgRestore(mVg);
}

// TEST (to display lines instead of points)
void
GraphControl12::DrawPointCurveLinesPolar(GraphCurve5 *curve)
{
#if FIX_UNDEFINED_CURVES
    bool curveUndefined = IsCurveUndefined(curve->mXValues, curve->mYValues, 2);
    if (curveUndefined)
        return;
#endif

    int height = this->mRECT.H();
    
    nvgSave(mVg);
    
    SetCurveDrawStyle(curve);
    
    BL_GUI_FLOAT pointSize = curve->mPointSize;
    nvgStrokeWidth(mVg, pointSize);
    
    int width = this->mRECT.W();

    nvgBeginPath(mVg);
    
    for (int i = 0; i < curve->mXValues.GetSize(); i ++)
    {
        BL_GUI_FLOAT x = curve->mXValues.Get()[i];
        if (x >= CURVE_VALUE_UNDEFINED)
            continue;
        
        BL_GUI_FLOAT y = curve->mYValues.Get()[i];
        if (y >= CURVE_VALUE_UNDEFINED)
            continue;
        
        BL_GUI_FLOAT yf = y;
        BL_GUI_FLOAT y0f = 0.0;
#if GRAPH_CONTROL_FLIP_Y
        yf = height - yf;
        y0f = height - y0f;
#endif

#if !OPTIM_LINES_POLAR
        nvgBeginPath(mVg);
        nvgMoveTo(mVg, width/2.0, y0f);
        nvgLineTo(mVg, x, yf);
        nvgStroke(mVg);
#else
        nvgMoveTo(mVg, width/2.0, y0f);
        nvgLineTo(mVg, x, yf);
#endif
    }
    
#if OPTIM_LINES_POLAR
    nvgStroke(mVg);
#endif
    
    nvgRestore(mVg);
}

void
GraphControl12::DrawPointCurveLinesPolarWeights(GraphCurve5 *curve)
{
#if FIX_UNDEFINED_CURVES
    bool curveUndefined = IsCurveUndefined(curve->mXValues, curve->mYValues, 2);
    if (curveUndefined)
        return;
#endif

    int height = this->mRECT.H();

    nvgSave(mVg);
    
    SetCurveDrawStyle(curve);
    
    BL_GUI_FLOAT pointSize = curve->mPointSize;
    nvgStrokeWidth(mVg, pointSize);
    
    int width = this->mRECT.W();
    
    for (int i = 0; i < curve->mXValues.GetSize(); i ++)
    {        
        BL_GUI_FLOAT x = curve->mXValues.Get()[i];
        if (x >= CURVE_VALUE_UNDEFINED)
            continue;
        
        BL_GUI_FLOAT y = curve->mYValues.Get()[i];
        if (y >= CURVE_VALUE_UNDEFINED)
            continue;
        
        if (curve->mWeights.GetSize() == curve->mXValues.GetSize())
        {
            BL_GUI_FLOAT weight = curve->mWeights.Get()[i];
            
            SetCurveDrawStyleWeight(curve, weight);
        }
        
        BL_GUI_FLOAT yf = y;
        BL_GUI_FLOAT zeroYf = 0.0;
#if GRAPH_CONTROL_FLIP_Y
        yf = height - yf;
        zeroYf = height - zeroYf;
#endif
        
        // NEW
        nvgBeginPath(mVg);
        
#if !OPTIM_LINES_POLAR
        nvgBeginPath(mVg);
        nvgMoveTo(mVg, width/2.0, zeroYf);
        nvgLineTo(mVg, x, yf);
        nvgStroke(mVg);
#else
        nvgMoveTo(mVg, width/2.0, zeroYf);
        nvgLineTo(mVg, x, yf);
#endif
        
#if OPTIM_LINES_POLAR
        nvgStroke(mVg);
#endif
    }
    
    nvgRestore(mVg);
}

void
GraphControl12::DrawPointCurveLinesPolarFill(GraphCurve5 *curve)
{
#if FIX_UNDEFINED_CURVES
    bool curveUndefined = IsCurveUndefined(curve->mXValues, curve->mYValues, 2);
    if (curveUndefined)
        return;
#endif

    int height = this->mRECT.H();
    
    nvgSave(mVg);
    
    SetCurveDrawStyle(curve);
    
    BL_GUI_FLOAT pointSize = curve->mPointSize;
    nvgStrokeWidth(mVg, pointSize);
    
    int width = this->mRECT.W();
        
    int lastValidIdx = -1;
    for (int i = 0; i < curve->mXValues.GetSize(); i++)
    {
        BL_GUI_FLOAT x = curve->mXValues.Get()[i];
        if (x >= CURVE_VALUE_UNDEFINED)
            continue;
        
        BL_GUI_FLOAT y = curve->mYValues.Get()[i];
        if (y >= CURVE_VALUE_UNDEFINED)
            continue;
        
        if (lastValidIdx == -1)
        {
            lastValidIdx = i;
            
            continue;
        }
        
        BL_GUI_FLOAT lastX = curve->mXValues.Get()[lastValidIdx];
        BL_GUI_FLOAT lastY = curve->mYValues.Get()[lastValidIdx];
        
        BL_GUI_FLOAT lastYf = lastY;
        BL_GUI_FLOAT zeroYf = 0.0;
        BL_GUI_FLOAT yf = y;
#if GRAPH_CONTROL_FLIP_Y
        lastYf = height - lastYf;
        zeroYf = height - zeroYf;
        yf = height - y;
#endif
        
        // If not 0, performance drown
#define EPS 0.0 //2.0
        
        nvgBeginPath(mVg);
        nvgMoveTo(mVg, width/2.0, zeroYf);
        nvgLineTo(mVg, lastX + EPS, lastYf);
        nvgLineTo(mVg, x, yf);
        nvgLineTo(mVg, width/2.0, zeroYf);
        nvgClosePath(mVg);
        
        nvgFill(mVg);
        
        lastValidIdx = i;
    }
    
    nvgRestore(mVg);
}

void
GraphControl12::DrawPointCurveLines(GraphCurve5 *curve)
{
#if FIX_UNDEFINED_CURVES
    bool curveUndefined = IsCurveUndefined(curve->mXValues, curve->mYValues, 2);
    if (curveUndefined)
        return;
#endif

   int height = this->mRECT.H();
    
    nvgSave(mVg);
    
    SetCurveDrawStyle(curve);
    
    BL_GUI_FLOAT pointSize = curve->mPointSize;
    nvgStrokeWidth(mVg, pointSize);
  
    if (curve->mXValues.GetSize() == 0)
        return;
    
    BL_GUI_FLOAT y0f = curve->mYValues.Get()[0];
#if GRAPH_CONTROL_FLIP_Y
    y0f = height - y0f;
#endif
    
    nvgBeginPath(mVg);
    
    nvgMoveTo(mVg, curve->mXValues.Get()[0], y0f);
    
    for (int i = 0; i < curve->mXValues.GetSize(); i ++)
    {
        BL_GUI_FLOAT x = curve->mXValues.Get()[i];
        if (x >= CURVE_VALUE_UNDEFINED)
            continue;
        
        BL_GUI_FLOAT y = curve->mYValues.Get()[i];
        if (y >= CURVE_VALUE_UNDEFINED)
            continue;
        
        BL_GUI_FLOAT yf = y;
#if GRAPH_CONTROL_FLIP_Y
        yf = height - yf;
#endif
        
        nvgLineTo(mVg, x, yf);
    }
    
    nvgStroke(mVg);
    
    nvgRestore(mVg);
}

void
GraphControl12::DrawPointCurveLinesWeights(GraphCurve5 *curve)
{
#if FIX_UNDEFINED_CURVES
    bool curveUndefined = IsCurveUndefined(curve->mXValues, curve->mYValues, 2);
    if (curveUndefined)
        return;
#endif

    int height = this->mRECT.H();
    
    nvgSave(mVg);
    
    if (curve->mPointOverlay)
        nvgGlobalCompositeOperation(mVg, NVG_LIGHTER);
    
    SetCurveDrawStyle(curve);
    
    BL_GUI_FLOAT pointSize = curve->mPointSize;
    nvgStrokeWidth(mVg, pointSize);
    
    if (curve->mXValues.GetSize() == 0)
        return;
    
    BL_FLOAT prevX = curve->mXValues.Get()[0];
    BL_FLOAT prevY = curve->mYValues.Get()[0];
    
    for (int i = 0; i < curve->mXValues.GetSize(); i ++)
    {
        BL_GUI_FLOAT x = curve->mXValues.Get()[i];
        if (x >= CURVE_VALUE_UNDEFINED)
            continue;
        
        BL_GUI_FLOAT y = curve->mYValues.Get()[i];
        if (y >= CURVE_VALUE_UNDEFINED)
            continue;
    
        if (curve->mWeights.GetSize() == curve->mXValues.GetSize())
        {
            BL_GUI_FLOAT weight = curve->mWeights.Get()[i];
            
            SetCurveDrawStyleWeight(curve, weight);
        }
    
        BL_GUI_FLOAT prevYf = prevY;
        BL_GUI_FLOAT yf = y;
#if GRAPH_CONTROL_FLIP_Y
        prevYf = height - prevYf;
        yf = height - yf;
#endif
        
        nvgBeginPath(mVg);
        
        nvgMoveTo(mVg, prevX, prevYf);
        nvgLineTo(mVg, x, yf);
        
        nvgStroke(mVg);
        
        prevX = x;
        prevY = y;
    }
    
    nvgRestore(mVg);
}

// Doesn't work well (for non convex polygons)
void
GraphControl12::DrawPointCurveLinesFill(GraphCurve5 *curve)
{
#if FIX_UNDEFINED_CURVES
    bool curveUndefined = IsCurveUndefined(curve->mXValues, curve->mYValues, 2);
    if (curveUndefined)
        return;
#endif

    int height = this->mRECT.H();
    
    nvgSave(mVg);
    
    SetCurveDrawStyle(curve);
    
    BL_GUI_FLOAT pointSize = curve->mPointSize;
    nvgStrokeWidth(mVg, pointSize);
    
    if (curve->mXValues.GetSize() == 0)
        return;
    
    BL_GUI_FLOAT y0f = curve->mYValues.Get()[0];
#if GRAPH_CONTROL_FLIP_Y
    y0f = height - y0f;
#endif
    
    nvgBeginPath(mVg);
    nvgMoveTo(mVg, curve->mXValues.Get()[0], y0f);
    
    for (int i = 0; i < curve->mXValues.GetSize(); i++)
    {
        BL_GUI_FLOAT x = curve->mXValues.Get()[i];
        if (x >= CURVE_VALUE_UNDEFINED)
            continue;
        
        BL_GUI_FLOAT y = curve->mYValues.Get()[i];
        if (y >= CURVE_VALUE_UNDEFINED)
            continue;
        
        BL_GUI_FLOAT yf = y;
#if GRAPH_CONTROL_FLIP_Y
        yf = height - yf;
#endif
        
        nvgLineTo(mVg, x, y);
    }
    
    nvgClosePath(mVg);
    nvgFill(mVg);
    nvgRestore(mVg);
}

void
GraphControl12::AutoAdjust()
{
    // First, compute the maximum value of all the curves
    BL_GUI_FLOAT max = -1e16;
    for (int i = 0; i < mCurves.size(); i++)
    {
        GraphCurve5 *curve = mCurves[i];
        
        for (int j = curve->mYValues.GetSize(); j >= 0; j--)
        {
            BL_GUI_FLOAT val = curve->mYValues.Get()[j];
            
            if (val > max)
                max = val;
        }
    }
    
    // Compute the scale factor
    
    // keep a margin, do not scale to the maximum
    BL_GUI_FLOAT margin = 0.25;
    BL_GUI_FLOAT factor = 1.0;
    
    if (max > 0.0)
         factor = (1.0 - margin)/max;
    
    // Then smooth the factor
    mAutoAdjustParamSmoother.SetNewValue(factor);
    mAutoAdjustParamSmoother.Update();
    factor = mAutoAdjustParamSmoother.GetCurrentValue();
    
    mAutoAdjustFactor = factor;
    
    mDataChanged = true;
}

BL_GUI_FLOAT
GraphControl12::MillisToPoints(long long int elapsed,
                               int sampleRate, int numSamplesPoint)
{
    BL_GUI_FLOAT numSamples = (((BL_GUI_FLOAT)elapsed)/1000.0)*sampleRate;
    
    BL_GUI_FLOAT numPoints = numSamples/numSamplesPoint;
    
    return numPoints;
}

void
GraphControl12::DrawText(BL_GUI_FLOAT x, BL_GUI_FLOAT y, BL_GUI_FLOAT fontSize,
                        const char *text, int color[4],
                        int halign, int valign, BL_GUI_FLOAT fontSizeCoeff)
{
    int width = this->mRECT.W();
    int height = this->mRECT.H();
    
    DrawText(mVg, x, y, width, height,
             fontSize, text, color,
             halign, valign, fontSizeCoeff);
}

#if !USE_FBO || (!defined IGRAPHICS_GL)
void
GraphControl12::Draw(IGraphics &graphics)
{
    PullAllData();
    
    mVg = (NVGcontext *)graphics.GetDrawContext();
    
    DoDraw(graphics);
    
    mVg = NULL;
}
#endif

// From IGraphics/Tests/TestGLControl.h
#if USE_FBO && (defined IGRAPHICS_GL)
void
GraphControl12::Draw(IGraphics &graphics)
{
    PullAllData();
    
#if DBG_DISABLE_DRAW
    return;
#endif
    
    // Checked: if we fall here, the graph is sure to have mIsEnabled = true!
    if (!mIsEnabled)
        return;
    
    mVg = (NVGcontext *)graphics.GetDrawContext();
    
    CheckCustomDrawersRedraw();
    
    int w = mRECT.W();
    int h = mRECT.H();
    
    // Regenerate the fbo only if we need to draw new data
    if (mDataChanged || (mFBO == NULL))
    {
        if(mFBO == NULL)
        {
            // TODO: delete correctly this FBO
            mFBO = nvgCreateFramebuffer(mVg, w, h, 0);
        }
    
        nvgEndFrame(mVg);
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &mInitialFBO);
    
        nvgBindFramebuffer(mFBO);
        nvgBeginFrame(mVg, w, h, 1.0f);
        GLint vp[4];
        glGetIntegerv(GL_VIEWPORT, vp);
    
        glViewport(0, 0, w, h);
    
        glScissor(0, 0, w, h);
        
        //glClearColor(0.f, 0.f, 0.f, 0.f);
        glClearColor(mClearColor[0], mClearColor[1], mClearColor[2], mClearColor[3]);
        
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        
        DoDraw(graphics);
    
        nvgEndFrame(mVg);
    
        // BUG fixed here!
        glViewport(vp[0], vp[1], vp[2], vp[3]);
               
        glBindFramebuffer(GL_FRAMEBUFFER, mInitialFBO);
    
        float graphicsWidth = graphics.WindowWidth();
        float graphicsHeight = graphics.WindowHeight();
        nvgBeginFrame(mVg, graphicsWidth, graphicsHeight, 1.0f);
        
        // Really avoid drawing when nothing changed
        // (tested with Wav3s)
        mDataChanged = false;
    }
    
    APIBitmap apibmp {mFBO->image, w, h, 1, 1.};
    IBitmap bmp {&apibmp, 1, false};
    
    graphics.DrawBitmap(bmp, mRECT);
    
    mVg = NULL;
}
#endif

void
GraphControl12::PushAllData()
{
#if !LOCK_FREE_USE_TRYLOCK
    mMutex.Enter();
#else
    bool entered = mMutex.TryEnter();
    if (!entered)
        return;
#endif
    
    // Copy from buffer 0 to buffer 1
    for (int i = 0; i < mCustomDrawers.size(); i++)
    {
        GraphCustomDrawer *drawer = mCustomDrawers[i];
        drawer->PushData();
    }

    if (mTransport != NULL)
        mTransport->PushData();
    
    mMutex.Leave();
}

void
GraphControl12::PullAllData()
{
#if !LOCK_FREE_USE_TRYLOCK
    mMutex.Enter();
#else
    bool entered = mMutex.TryEnter();
    if (!entered)
        return;
#endif

    // Copy from buffer 1 to buffer 2
    for (int i = 0; i < mCustomDrawers.size(); i++)
    {
        GraphCustomDrawer *drawer = mCustomDrawers[i];
        drawer->PullData();
    }

    if (mTransport != NULL)
    {
        mTransport->PullData();
    }
    
    // Leave mutex here, we have finished with critical section
    mMutex.Leave();

    // Apply data without locking (this can be a bit long
    for (int i = 0; i < mCustomDrawers.size(); i++)
    {
        GraphCustomDrawer *drawer = mCustomDrawers[i];
        drawer->ApplyData();
    }

    if (mTransport != NULL)
    {
        mTransport->ApplyData();
    }
}

void
GraphControl12::DoDraw(IGraphics &graphics)
{
#if DBG_DISABLE_DRAW
    return;
#endif
    
    if (mTransport != NULL)
        mTransport->Update();
    
    mVg = (NVGcontext *)graphics.GetDrawContext();
    
    // Checked: if we fall here, the graph is sur to have mIsEnabled = true!
    if (!mIsEnabled)
    {
        return;
    }
    
    nvgSave(mVg);
    
    nvgReset(mVg);
    
#if !USE_FBO || (!defined IGRAPHICS_GL)
    // #bl-iplug2
    // Be sure to draw only in the graph.
    // So when using IPlug2, we won't draw on the other GUI componenets.
    // NOTE: This is required since we commented glClear() in IPlug2.
    nvgScissor(mVg, this->mRECT.L, this->mRECT.T,
               this->mRECT.W(), this->mRECT.H());
    
    nvgTranslate(mVg, this->mRECT.L, this->mRECT.T);
#endif
    
    DrawBackgroundImage(graphics);
    
    if (mAutoAdjustFlag)
    {
        AutoAdjust();
    }
    
    CustomDrawersPreDraw();
    
    // Update the time axis, so we are very accurate at each Draw() call
    if (mGraphTimeAxis != NULL)
        mGraphTimeAxis->UpdateFromDraw();
    
    DrawAxis(true);
    
    nvgSave(mVg);
    
    DrawCurves();
    
    nvgRestore(mVg);
    
    // DO NOT DELETE THIS BEFORE INTRGRATING GHOST-X
#if 0
    if (mSpectrogramDisplay != NULL)
    {
        // Display MiniView after curves
        // So we have the mini waveform, then the selection over
        mSpectrogramDisplay->DrawMiniView(width, height);
    }
#endif
    
    DrawAxis(false);
    
    DisplayCurveDescriptions();

    CustomDrawersPostDraw();
    
    DrawSeparatorY0();
   
    DrawOverlayImage(graphics);
    
    nvgRestore(mVg);
}

void
GraphControl12::SetCurveDrawStyle(GraphCurve5 *curve)
{
    nvgStrokeWidth(mVg, curve->mLineWidth);
    
    if (curve->mBevelFlag)
    {
        nvgLineJoin(mVg, NVG_BEVEL);
        //nvgLineJoin(mVg, NVG_ROUND);
        //nvgMiterLimit(mVg, 10.0);
    }
    
    int sColor[4] = { (int)(curve->mColor[0]*255), (int)(curve->mColor[1]*255),
        (int)(curve->mColor[2]*255), (int)(curve->mAlpha*255) };
    SWAP_COLOR(sColor);

    nvgStrokeColor(mVg, nvgRGBA(sColor[0], sColor[1], sColor[2], sColor[3]));
    
    int sFillColor[4] = { (int)(curve->mColor[0]*255), (int)(curve->mColor[1]*255),
        (int)(curve->mColor[2]*255), (int)(curve->mFillAlpha*255) };
    SWAP_COLOR(sFillColor);
    
    nvgFillColor(mVg, nvgRGBA(sFillColor[0], sFillColor[1],
                              sFillColor[2], sFillColor[3]));
}

void
GraphControl12::SetCurveDrawStyleWeight(GraphCurve5 *curve, BL_GUI_FLOAT weight)
{
    int sColor[4];
    if (!curve->mUseWeightTargetColor)
    {
        sColor[0] = (int)(curve->mColor[0]*255*weight);
        sColor[1] = (int)(curve->mColor[1]*255*weight);
        sColor[2] = (int)(curve->mColor[2]*255*weight);
        sColor[3] = (int)(curve->mAlpha*255);
        
        if (curve->mWeightMultAlpha)
            sColor[3] *= weight;
    }
    else
    {
        sColor[0] = (int)(curve->mColor[0]*(1.0 - weight)*255 + curve->mWeightTargetColor[0]*weight*255);
        sColor[1] = (int)(curve->mColor[1]*(1.0 - weight)*255 + curve->mWeightTargetColor[1]*weight*255);
        sColor[2] = (int)(curve->mColor[2]*(1.0 - weight)*255 + curve->mWeightTargetColor[2]*weight*255);
        sColor[3] = (int)(curve->mAlpha*(1.0 - weight)*255    + curve->mWeightTargetColor[3]*weight*255);
    }
    
    for (int i = 0; i < 4; i++)
    {
        if (sColor[i] < 0)
            sColor[i] = 0;
        
        if (sColor[i] > 255)
            sColor[i] = 255;
    }
    SWAP_COLOR(sColor);
    
    nvgStrokeColor(mVg, nvgRGBA(sColor[0], sColor[1], sColor[2], sColor[3]));
    
    int sFillColor[4];
    
    if (!curve->mUseWeightTargetColor)
    {
        sFillColor[0] = (int)(curve->mColor[0]*255*weight);
        sFillColor[1] = (int)(curve->mColor[1]*255*weight);
        sFillColor[2] = (int)(curve->mColor[2]*255*weight);
        sFillColor[3] = (int)(curve->mFillAlpha*255*weight);
        
        if (curve->mWeightMultAlpha)
            sColor[3] *= weight;
    }
    else
    {
        sFillColor[0] = (int)(curve->mColor[0]*(1.0 - weight)*255 + curve->mWeightTargetColor[0]*weight*255);
        sFillColor[1] = (int)(curve->mColor[1]*(1.0 - weight)*255 + curve->mWeightTargetColor[1]*weight*255);
        sFillColor[2] = (int)(curve->mColor[2]*(1.0 - weight)*255 + curve->mWeightTargetColor[2]*weight*255);
        sFillColor[3] = (int)(curve->mFillAlpha*(1.0 - weight)*255    + curve->mWeightTargetColor[3]*weight*255);
    }
    
    for (int i = 0; i < 4; i++)
    {
        if (sFillColor[i] < 0)
            sFillColor[i] = 0;
        
        if (sFillColor[i] > 255)
            sFillColor[i] = 255;
    }
    SWAP_COLOR(sFillColor);
    
    nvgFillColor(mVg, nvgRGBA(sFillColor[0], sFillColor[1],
                              sFillColor[2], sFillColor[3]));
}

BL_GUI_FLOAT
GraphControl12::ConvertToBoundsX(BL_GUI_FLOAT t)
{
    // Rescale
    t *= (mBounds[2] - mBounds[0]);
    t += mBounds[0];
    
    // Clip
    if (t < 0.0)
        t = 0.0;
    if (t > 1.0)
        t = 1.0;
    
    return t;
}

BL_GUI_FLOAT
GraphControl12::ConvertToBoundsY(BL_GUI_FLOAT t)
{
    // Rescale
    t *= (mBounds[3] - mBounds[1]);
    t += (1.0 - mBounds[3]);
    
    // Clip
    if (t < 0.0)
        t = 0.0;
    if (t > 1.0)
        t = 1.0;
    
    return t;
}

BL_GUI_FLOAT
GraphControl12::ConvertToAxisBounds(GraphAxis2 *axis, BL_GUI_FLOAT t)
{
    // Rescale
    t *= (axis->mBounds[1] - axis->mBounds[0]);
    //t += (1.0 - axis->mBounds[1]);
    t += axis->mBounds[0];
    
    // Clip
    if (t < 0.0)
        t = 0.0;
    if (t > 1.0)
        t = 1.0;
    
    return t;
}

void
GraphControl12::DrawBackgroundImage(IGraphics &graphics)
{
    if (mBgImage.GetAPIBitmap() == NULL)
        return;
    
    nvgSave(mVg);
    nvgTranslate(mVg, -this->mRECT.L, -this->mRECT.T);
    
    IRECT bounds = GetRECT();
    graphics.DrawBitmap(mBgImage, bounds, 0, 0);
    
    nvgRestore(mVg);
}

void
GraphControl12::DrawOverlayImage(IGraphics &graphics)
{
    if (mOverlayImage.GetAPIBitmap() == NULL)
        return;
    
    nvgSave(mVg);
    nvgTranslate(mVg, -this->mRECT.L, -this->mRECT.T);
    
    IRECT bounds = GetRECT();
    graphics.DrawBitmap(mOverlayImage, bounds, 0, 0);
    
    nvgRestore(mVg);
}

bool
GraphControl12::IsCurveUndefined(const WDL_TypedBuf<BL_GUI_FLOAT> &x,
                                 const WDL_TypedBuf<BL_GUI_FLOAT> &y,
                                 int minNumValues)
{
    if (x.GetSize() != y.GetSize())
        return true;
    
    int numDefinedValues = 0;
    for (int i = 0; i < x.GetSize(); i++)
    {
        BL_GUI_FLOAT x0 = x.Get()[i];
        if (x0 >= CURVE_VALUE_UNDEFINED)
            continue;
        
        BL_GUI_FLOAT y0 = y.Get()[i];
        if (y0 >= CURVE_VALUE_UNDEFINED)
            continue;
        
        numDefinedValues++;
        
        if (numDefinedValues >= minNumValues)
            // The curve is defined
            return false;
    }
    
    return true;
}

void
GraphControl12::CheckCustomDrawersRedraw()
{
    for (int i = 0; i < mCustomDrawers.size(); i++)
    {
        GraphCustomDrawer *drawer = mCustomDrawers[i];
        if (drawer->NeedRedraw())
        {
            // Need to redraw anyway
            mDataChanged = true;
            
            return;
        }
    }
}

#endif // IGRAPHICS_NANOVG
