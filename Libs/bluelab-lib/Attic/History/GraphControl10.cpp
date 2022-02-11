//
//  Graph.cpp
//  Transient
//
//  Created by Apple m'a Tuer on 03/09/17.
//
//

#include <math.h>

#include <stdio.h>

#include <GL/glew.h>

#ifdef __APPLE__
#include <OpenGL/OpenGL.h>
#include <OpenGL/glu.h>
#endif

#include "nanovg.h"

// Warning: Niko hack in NanoVg to support FBO even on GL2
#define NANOVG_GL2_IMPLEMENTATION

#include "nanovg_gl.h"
#include "nanovg_gl_utils.h"

#include "../../WDL/lice/lice_glbitmap.h"

#include "../../WDL/IPlug/IPlugBase.h"

// Plugin resource file
#include "resource.h"

#if !GHOST_OPTIM_GL
#include "GLContext3.h"
#endif

#include "SpectrogramDisplay.h"
#include "SpectrogramDisplayScroll.h"
#include "SpectrogramDisplayScroll2.h"
#include <BLUtils.h>
#include "GraphControl10.h"

#include <ImageDisplay.h>

#include <UpTime.h> // TEST

#define FILL_CURVE_HACK 1

// To fix line holes between filled polygons
#define FILL_CURVE_HACK2 1

#define CURVE_DEBUG 0

// Good, but misses some maxima
#define CURVE_OPTIM      0
#define CURVE_OPTIM_THRS 2048

// -1 pixel
#define OVERLAY_OFFSET -1.0


// Optimizations for rendering and clicks
//

// Optimize a little, by rendering pure opengl
// without locking the audio thread
#define END_FRAME_NO_MUTEX 0 //1

// FIX: Remove audio clicks on Waves for example
// Let the custom drawer manage their own mutexes
// So possibly avoid locking the audio thread for a too long time
#define DRAWER_OWN_MUTEX_OPTIM 0 //1

// GOOD: it seems good
// Works well on Waves to avoid clicks,
// by avoiding locking two times, and then
// we couldn't unlock correctly before
// when displaying LinesRender
#define FIX_WEIRD_LOCK 0 //1

// BAD: makes crashes (due to concurrentcy on member variables)
// (only for testing)
#define DEBUG_REMOVE_DRAW_MUTEX 0

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

#if GHOST_OPTIM_GL
// Avoid redrawing all the graph curves if nothing has changed
// NOTE: does not really optimize for UST...
#define FIX_USELESS_GRAPH_REDRAW 1
#endif

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

GraphControl10::GraphControl10(IPlugBase *pPlug, IGraphics *pGraphics,
                               IRECT pR, int paramIdx,
                              int numCurves, int numCurveValues,
                              const char *fontPath)
	: IControl(pPlug, pR, paramIdx),
	mFontInitialized(false),
	mAutoAdjustParamSmoother(1.0, 0.9),
	mNumCurves(numCurves),
	mNumCurveValues(numCurveValues),
	mHAxis(NULL),
	mVAxis(NULL)
{
	for (int i = 0; i < mNumCurves; i++)
	{
		GraphCurve4 *curve = new GraphCurve4(numCurveValues);

		mCurves.push_back(curve);
	}

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
	mXdBScale = false;
	//mMinX = 0.1;
    mMinX = 0.0;
	mMaxX = 1.0;
    
    SetClearColor(0, 0, 0, 255);
    
    mDirty = true;
    
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
        
    mFontPath.Set(fontPath);

	mVg = NULL;

    // Init GL
    mGLContext = NULL;
    
#if !GHOST_OPTIM_GL
    mGLContext = new GLContext3(BUNDLE_NAME, pGraphics);
    
//#if 0 // TEST: Ableton 10 Win 10: do not make call to glewInit() here
    //because we are not sure it it the correct thread
    // InitGL() will be done later, in OnIdle()
    
    // Seems like GL must be initialized in this thread, otherwise
    // it will make Ableton 10 on windows to consume all the resources
    //   InitGL();
    
    InitNanoVg();
//#endif
#endif
    
    mLiceFb = NULL;
    
    mSpectrogramDisplay = NULL;
    mSpectrogramDisplayScroll = NULL;
    mSpectrogramDisplayScroll2 = NULL;
    
    mImageDisplay = NULL;
    
    mBounds[0] = 0.0;
    mBounds[1] = 0.0;
    mBounds[2] = 1.0;
    mBounds[3] = 1.0;
    
    mBgImage = NULL;
    mNvgBackgroundImage = 0;
    
    mOverImage = NULL;
    mNvgOverlayImage = 0;
    
    mWhitePixImg = -1;

    mNeedResizeGraph = false;
    
    mIsEnabled = true;
    
    mOverImage = NULL;
    
    mDisablePointOffsetHack = false;
    
    mRecreateWhiteImageHack = false;
 
    // Override from IControl
    mIsGLControl = true;
    
#if PROFILE_GRAPH
    mDebugCount = 0;
#endif
}

GraphControl10::~GraphControl10()
{
    for (int i = 0; i < mNumCurves; i++)
        delete mCurves[i];
    
    if (mHAxis != NULL)
        delete mHAxis;
    
    if (mVAxis != NULL)
        delete mVAxis;

#if GHOST_OPTIM_GL
    bool result = true;
#else
    bool result = mGLContext->Enter();
#endif
    
    // FIX: delete the spectrogram display IN the context !
    // (as it should be)
    // FIXES: in Reaper, insert two Ghost-X in two tracks
    // - play the first track to display the spectrogram
    // - delete the second Ghost-X
    // => The first spectrogram is not displayed anymore (black)
    // (but the waveform is still displayed)
    //
    if (mSpectrogramDisplay != NULL)
        delete mSpectrogramDisplay;
    
    if (mSpectrogramDisplayScroll != NULL)
        delete mSpectrogramDisplayScroll;
   
    if (mSpectrogramDisplayScroll2 != NULL)
        delete mSpectrogramDisplayScroll2;
    
    //
    if (mImageDisplay != NULL)
        delete mImageDisplay;
    
    // Will be delete in the plugin !
    //if (mBgImage != NULL)
    //    delete mBgImage;
    
	//bool result = mGLContext->Enter();

    if (mNvgBackgroundImage != 0)
	{
		// FIX: Ableton 10 win 10
		// mVg could be null if GL has not succeed to initialize before
		if (result && (mVg != NULL))
			nvgDeleteImage(mVg, mNvgBackgroundImage);
	}

    if (mNvgOverlayImage != 0)
	{
		// FIX: Ableton 10 win 10
		// mVg could be null if GL has not succeed to initialize before
		if (result && (mVg != NULL))
			nvgDeleteImage(mVg, mNvgOverlayImage);
	}
    
#if 0 // OLD
	// FIX: Ableton 10, windows 10
	// fix crash when scanning plugin (real crash)
	if (mGLInitialized)
	{
		// FIX Mixcraft crash: VST3, Windows: load AutoGain project, play, inser Denoiser, play, remove Denoiser, play !> this crashed
		// This fix works.
#if 1
		// Mixcraft FIX: do not unbind, bind instead
		// Otherwise ExitNanoVg() crases with Mixcraft if no context is bound
		GLContext3 *context = GLContext3::Get();
		if (context != NULL)
			context->Bind();
#endif

		// QUESTION: is this fix for Windows or Mac ?
#if 1 // AbletonLive need these two lines
		// Otherwise when destroying the plugin in AbletonLive,
		// there are many graphical bugs (huge white rectangles)
		// and the software becomes unusable
		//GLContext *context = GLContext::Get();
		//if (context != NULL)
		//context->Unbind();
#endif
		ExitNanoVg();
	}
#endif
    
	if (result)
		ExitNanoVg();
    
    if (mLiceFb != NULL)
        delete mLiceFb; 
    mLiceFb = NULL;
    
#if 0
	// FIX: Ableton 10, windows 10
	// fix crash when scanning plugin (real crash)
	if (mGLInitialized)
	{
	   // No need anymore since this is static
	 ExitGL();
	}
#endif
    
#if !GHOST_OPTIM_GL
    delete mGLContext;
#endif
}

void
GraphControl10::SetEnabled(bool flag)
{
    mIsEnabled = flag;
}

// Same as Resize()
void
GraphControl10::SetNumCurveValues(int numCurveValues)
{
    mNumCurveValues = numCurveValues;
    
    for (int i = 0; i < mNumCurves; i++)
	{
		GraphCurve4 *curve = mCurves[i];
        
		curve->ResetNumValues(mNumCurveValues);
	}
    
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

void
GraphControl10::GetSize(int *width, int *height)
{
    *width = this->mRECT.W();
    *height = this->mRECT.H();
}

void
GraphControl10::Resize(int width, int height)
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
            mVAxis->mOffsetX -= dWidth;
    }
    
#if 0
    // BUG: instantiate 2 Wave plugins, change the gui size of one
    // => somethimes we loose the display on the other (Reaper, Mac)
    
    // Plan to re-create the fbo, to have a good definition when zooming
    if (mLiceFb != NULL)
        delete mLiceFb;
    mLiceFb = NULL;
#endif
    
#if 1 // FIXED: Resize the FBO instead of deleting and re-creating it
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
#endif
    
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

void
GraphControl10::SetBounds(BL_GUI_FLOAT x0, BL_GUI_FLOAT y0, BL_GUI_FLOAT x1, BL_GUI_FLOAT y1)
{
    mBounds[0] = x0;
    mBounds[1] = y0;
    mBounds[2] = x1;
    mBounds[3] = y1;
    
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

void
GraphControl10::Resize(int numCurveValues)
{
    mNumCurveValues = numCurveValues;
    
    for (int i = 0; i < mNumCurves; i++)
	{
		GraphCurve4 *curve = mCurves[i];
        
		curve->ResetNumValues(mNumCurveValues);
	}
    
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

int
GraphControl10::GetNumCurveValues()
{
    return mNumCurveValues;
}

#if 0 // Commented for StereoWidthProcess::DebugDrawer
bool
GraphControl10::IsDirty()
{
    return mDirty;
}

void
GraphControl10::SetDirty(bool flag)
{
    mDirty = flag;
}
#endif

void
GraphControl10::SetMyDirty(bool flag)
{
#if DIRTY_OPTIM
    mMyDirty = flag;
#endif
}


const LICE_IBitmap *
GraphControl10::DrawGL(bool update)
{
#if FIX_USELESS_GRAPH_REDRAW
    if (update && !mDirty && !mMyDirty && (mLiceFb != NULL))
        return mLiceFb;
#endif

    if (!update)
    {
        return mLiceFb;
    }
    
#if PROFILE_GRAPH
    mDebugTimer.Start();
#endif
    
#if !FIX_WEIRD_LOCK

#if !DEBUG_REMOVE_DRAW_MUTEX
    IPlugBase::IMutexLock lock(mPlug);
#endif
    
    if (!mIsEnabled)
        return NULL;
    
#endif

#if GHOST_OPTIM_GL
    InitNanoVg();
    
    if (mSpectrogramDisplay != NULL)
        mSpectrogramDisplay->SetNvgContext(mVg);
    
    if (mSpectrogramDisplayScroll != NULL)
        mSpectrogramDisplayScroll->SetNvgContext(mVg);
    
    if (mSpectrogramDisplayScroll2 != NULL)
        mSpectrogramDisplayScroll2->SetNvgContext(mVg);
    
    //
    if (mImageDisplay != NULL)
        mImageDisplay->SetNvgContext(mVg);
#endif
    
#if 0
	// FIX: Ableton 10 win 10: if gl is not yet initialized, try to initialize it here
	// (may solve, since we are here in the draw thread, whereas the InitGL()
	// in the constructor is certainly in the other thread.
	if (!mGLInitialized)
	{
		// Commented because if we try to initialize GL in this thread,
		// Ableton 10 on windows will consume all the resources

		//InitGL();

		//InitNanoVg();

		//if (!mGLInitialized)
			// Still not initialized
			// Do not try to draw otherwise it will crash
		//return NULL;
		return NULL;
	}
#endif
    
#if !GHOST_OPTIM_GL
    if (!mGLContext->IsInitialized())
        return NULL;
    
    bool success = mGLContext->Enter();
    if (!success)
        return NULL;
#endif
    
    // Added this test to avoid redraw everything each time
    // NOTE: added for StereoWidth
#if DIRTY_OPTIM
    if (mMyDirty)
#endif
        DrawGraph();
    
    // Must be commented, otherwise it make a refresh bug on Protools
    // FIX: on Protools, after zooming, the data is not refreshed until
    // we make a tiny zoom or we modify slightly a parameter.
    //mDirty = false;
    
#if DIRTY_OPTIM
    mMyDirty = false;
#endif
        
#if PROFILE_GRAPH
    mDebugTimer.Stop();
    
    if (mDebugCount++ % 100 == 0)
    {
        long t = mDebugTimer.Get();
        mDebugTimer.Reset();
        
        fprintf(stderr, "GraphControl10 - profile: %ld\n", t);
        
        char debugMessage[512];
        sprintf(debugMessage, "GraphControl10 - profile: %ld", t);
        BLDebug::AppendMessage("graph-profile.txt", debugMessage);
    }
#endif

#if !GHOST_OPTIM_GL
    mGLContext->Leave();
#endif
    
#if GRAPH_OPTIM_GL_FIX_OVER_BLEND2
    glDisable(GL_BLEND);
#endif
    
    return mLiceFb;
}

void
GraphControl10::ResetGL()
{
#if GHOST_OPTIM_GL
    if (mSpectrogramDisplay != NULL)
        mSpectrogramDisplay->ResetGL();

    if (mSpectrogramDisplayScroll != NULL)
        mSpectrogramDisplayScroll->ResetGL();
    
    if (mSpectrogramDisplayScroll2 != NULL)
        mSpectrogramDisplayScroll2->ResetGL();
    
    //
    if (mImageDisplay != NULL)
        mImageDisplay->ResetGL();
    
    if (mNvgBackgroundImage != 0)
	{
		if (mVg != NULL)
			nvgDeleteImage(mVg, mNvgBackgroundImage);
        
        mNvgBackgroundImage = 0;
	}
    
    if (mNvgOverlayImage != 0)
	{
		if (mVg != NULL)
			nvgDeleteImage(mVg, mNvgOverlayImage);
        
        mNvgOverlayImage = 0;
	}
    
    ExitNanoVg();
    
    if (mLiceFb != NULL)
        delete mLiceFb;
    mLiceFb = NULL;
    
    // FIX(1/2): fixes Logic, close plug window, re-open => the graph control view was blank
#if 1
    mDirty = true;
    
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
#endif
    
#endif
}

//void
//GraphControl10::OnIdle()
//{
	// FIX: Albeton 10 windows 10
	//
	// Try to initialize OpenGL if not already
	// Called from the audio processing thread as it should be

//	if (!mGLInitialized)
//	{
//		InitGL();

//		InitNanoVg();
//	}
//}

// Added for StereoViz
void
GraphControl10::OnGUIIdle()
{
    for (int i = 0; i < mCustomControls.size(); i++)
    {
        GraphCustomControl *control = mCustomControls[i];
        
        control->OnGUIIdle();
    }
}

void
GraphControl10::SetSeparatorY0(BL_GUI_FLOAT lineWidth, int color[4])
{
    mSeparatorY0 = true;
    mSepY0LineWidth = lineWidth;
    
    for (int i = 0; i < 4; i++)
        mSepY0Color[i] = color[i];
    
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

void
GraphControl10::AddHAxis(char *data[][2], int numData, bool xDbScale,
                         int axisColor[4], int axisLabelColor[4],
                         BL_GUI_FLOAT offsetY,
                         int axisOverlayColor[4],
                         BL_GUI_FLOAT fontSizeCoeff,
                         int axisLinesOverlayColor[4])
{
    mHAxis = new GraphAxis();
    mHAxis->mOffset = 0.0;
    
    // Warning, offset Y is normalized value
    mHAxis->mOffsetY = offsetY;
    mHAxis->mOverlay = false;
    mHAxis->mLinesOverlay = false;
    
    mHAxis->mFontSizeCoeff = fontSizeCoeff;
    
    mHAxis->mXdBScale = xDbScale;
    
    mHAxis->mAlignTextRight = false;
    mHAxis->mAlignRight = true;//
    
    AddAxis(mHAxis, data, numData, axisColor, axisLabelColor, mMinX, mMaxX,
            axisOverlayColor, axisLinesOverlayColor);
    
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

void
GraphControl10::AddHAxis0(char *data[][2], int numData, bool xDbScale,
                          int axisColor[4], int axisLabelColor[4],
                          BL_GUI_FLOAT offsetY,
                          int axisOverlayColor[4],
                          BL_GUI_FLOAT fontSizeCoeff,
                          int axisLinesOverlayColor[4])
{
    mHAxis = new GraphAxis();
    mHAxis->mOffset = 0.0;
    
    // Warning, offset Y is normalized value
    mHAxis->mOffsetY = offsetY;
    mHAxis->mOverlay = false;
    mHAxis->mLinesOverlay = false;
    
    mHAxis->mFontSizeCoeff = fontSizeCoeff;
    
    mHAxis->mXdBScale = xDbScale;
    
    mHAxis->mAlignTextRight = false;
    mHAxis->mAlignTextRight = true;
    
    AddAxis(mHAxis, data, numData, axisColor, axisLabelColor, 0.0, mMaxX,
            axisOverlayColor, axisLinesOverlayColor);
    
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

void
GraphControl10::ReplaceHAxis(char *data[][2], int numData)
{
#if !FIX_HAXIS_COLOR_SWAP
    int axisColor[4];
    int axisLabelColor[4];
    BL_GUI_FLOAT offsetY = 0.0;
    
    int labelOverlayColor[4];
    int linesOverlayColor[4];
    
    bool xDbScale;
#else
    // Initialize the colors !
    int axisColor[4] = { 0, 0, 0, 0 };
    int axisLabelColor[4] = { 0, 0, 0, 0 };

    BL_GUI_FLOAT offset = 0.0; // NEW
    BL_GUI_FLOAT offsetX = 0.0; // NEW
    
    BL_GUI_FLOAT offsetY = 0.0;

    bool overlay = false; // NEW
    bool linesOverlay = false; // NEW
    
    int labelOverlayColor[4] = { 0, 0, 0, 0 };
    int linesOverlayColor[4] = { 0, 0, 0, 0 };
    
    bool xDbScale = false;

    bool alignTextRight; // NEW
    bool alignRight; // NEW
#endif
    
    BL_GUI_FLOAT fontSizeCoeff = mHAxis->mFontSizeCoeff;
    
    if (mHAxis != NULL)
    {
        for (int i = 0; i < 4; i++)
            axisColor[i] = mHAxis->mColor[i];
        
        for (int i = 0; i < 4; i++)
            axisLabelColor[i] = mHAxis->mLabelColor[i];

	offset = mHAxis->mOffset; // NEW
	offsetX = mHAxis->mOffsetX; // NEW
	
        offsetY = mHAxis->mOffsetY;

	overlay = mHAxis->mOverlay; // NEW
	linesOverlay = mHAxis->mLinesOverlay; // NEW
	
        for (int i = 0; i < 4; i++)
            labelOverlayColor[i] = mHAxis->mLabelOverlayColor[i];
        
        for (int i = 0; i < 4; i++)
            linesOverlayColor[i] = mHAxis->mLinesOverlayColor[i];
        
        xDbScale = mHAxis->mXdBScale;

	alignTextRight = mHAxis->mAlignTextRight; // NEW
	alignRight = mHAxis->mAlignRight; // NEW
	
        delete mHAxis;
    }
    
    AddHAxis(data, numData, xDbScale,
             axisColor, axisLabelColor, offsetY, labelOverlayColor,
             fontSizeCoeff, linesOverlayColor);
    
#if FIX_HAXIS_COLOR_SWAP
    // Set correclty the saved colors
    // (Because AddHAxis() swaps the colors, and set them to the members.
    // So with the previous code, the colors are swapped twice).
    for (int i = 0; i < 4; i++)
        mHAxis->mColor[i] = axisColor[i];
    
    for (int i = 0; i < 4; i++)
        mHAxis->mLabelColor[i] = axisLabelColor[i];
    
    for (int i = 0; i < 4; i++)
        mHAxis->mLabelOverlayColor[i] = labelOverlayColor[i];
    
    for (int i = 0; i < 4; i++)
        mHAxis->mLinesOverlayColor[i] = linesOverlayColor[i];

    // NEW
    //
    mHAxis->mFontSizeCoeff = fontSizeCoeff;
 
    mHAxis->mOffset = offset;
    mHAxis->mOffsetX = offsetX;
	
    mHAxis->mOffsetY = offsetY;

    mHAxis->mOverlay = overlay;
    mHAxis->mLinesOverlay = linesOverlay;

    mHAxis->mXdBScale = xDbScale;

    mHAxis->mAlignTextRight = alignTextRight;
    mHAxis->mAlignRight = alignRight;
#endif
    
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

void
GraphControl10::RemoveHAxis()
{
    if (mHAxis != NULL)
        delete mHAxis;
    
    mHAxis = NULL;
}

void
GraphControl10::AddVAxis(char *data[][2], int numData, int axisColor[4],
                         int axisLabelColor[4], BL_GUI_FLOAT offset, BL_GUI_FLOAT offsetX,
                         int axisLabelOverlayColor[4], BL_GUI_FLOAT fontSizeCoeff,
                         bool alignTextRight, int axisLinesOverlayColor[4],
                         bool alignRight)
{
    mVAxis = new GraphAxis();
    mVAxis->mOverlay = false;
    mVAxis->mLinesOverlay = false;
    mVAxis->mOffset = offset;
    mVAxis->mOffsetX = offsetX;
    mVAxis->mOffsetY = 0.0;
    
    mVAxis->mFontSizeCoeff = fontSizeCoeff;
    
    mVAxis->mXdBScale = false;
    
    //mVAxis->mAlignRight = false;
    mVAxis->mAlignTextRight = alignTextRight;
    mVAxis->mAlignRight = alignRight;
    
    // Retreive the Y db scale
    BL_GUI_FLOAT minY = -40.0;
    BL_GUI_FLOAT maxY = 40.0;
    
    if (!mCurves.empty())
        // Get from the first curve
    {
        minY = mCurves[0]->mMinY;
        maxY = mCurves[0]->mMaxY;
    }
    
    AddAxis(mVAxis, data, numData, axisColor, axisLabelColor, minY, maxY,
            axisLabelOverlayColor, axisLinesOverlayColor);
    
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

void
GraphControl10::RemoveVAxis()
{
    if (mVAxis != NULL)
        delete mVAxis;
    
    mVAxis = NULL;
}

void
GraphControl10::AddVAxis(char *data[][2], int numData, int axisColor[4], int axisLabelColor[4],
                        bool dbFlag, BL_GUI_FLOAT minY, BL_GUI_FLOAT maxY,
                        BL_GUI_FLOAT offset, int axisOverlayColor[4],
                        BL_GUI_FLOAT fontSizeCoeff, bool alignTextRight,
                        int axisLinesOverlayColor[4],
                         bool alignRight)
{
    mVAxis = new GraphAxis();
    mVAxis->mOverlay = false;
    mVAxis->mLinesOverlay = false;
    mVAxis->mOffset = offset;
    mVAxis->mOffsetY = 0.0;
    
    mVAxis->mFontSizeCoeff = fontSizeCoeff;
    
    mVAxis->mXdBScale = false;
    
    //mVAxis->mAlignRight = false;
    mVAxis->mAlignTextRight = alignTextRight;
    mVAxis->mAlignRight = alignRight;
    
    AddAxis(mVAxis, data, numData, axisColor, axisLabelColor, minY, maxY,
            axisOverlayColor, axisLinesOverlayColor);
    
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

void
GraphControl10::AddVAxis(char *data[][2], int numData, int axisColor[4], int axisLabelColor[4],
                         bool dbFlag, BL_GUI_FLOAT minY, BL_GUI_FLOAT maxY,
                         BL_GUI_FLOAT offset, BL_GUI_FLOAT offsetX, int axisOverlayColor[4],
                         BL_GUI_FLOAT fontSizeCoeff, bool alignTextRight,
                         int axisLinesOverlayColor[4],
                         bool alignRight)
{
    mVAxis = new GraphAxis();
    mVAxis->mOverlay = false;
    mVAxis->mLinesOverlay = false;
    mVAxis->mOffset = offset;
    mVAxis->mOffsetX = offsetX;
    mVAxis->mOffsetY = 0.0;
    
    mVAxis->mFontSizeCoeff = fontSizeCoeff;
    
    mVAxis->mXdBScale = false;
    
    //mVAxis->mAlignRight = false;
    mVAxis->mAlignTextRight = alignTextRight;
    mVAxis->mAlignRight = alignRight;
    
    AddAxis(mVAxis, data, numData, axisColor, axisLabelColor, minY, maxY,
            axisOverlayColor, axisLinesOverlayColor);
    
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

void
GraphControl10::SetXScale(bool dbFlag, BL_GUI_FLOAT minX, BL_GUI_FLOAT maxX)
{
    mXdBScale = dbFlag;
    
    mMinX = minX;
    mMaxX = maxX;
    
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

void
GraphControl10::SetAutoAdjust(bool flag, BL_GUI_FLOAT smoothCoeff)
{
    mAutoAdjustFlag = flag;
    
    mAutoAdjustParamSmoother.SetSmoothCoeff(smoothCoeff);
    
    mDirty = true;
    
    // Not sur we must still use mDirty
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

void
GraphControl10::SetYScaleFactor(BL_GUI_FLOAT factor)
{
    mYScaleFactor = factor;
    
    mDirty = true;
    
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

void
GraphControl10::SetClearColor(int r, int g, int b, int a)
{
    SET_COLOR_FROM_INT(mClearColor, r, g, b, a);
    
    mDirty = true;
    
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

void
GraphControl10::SetCurveColor(int curveNum, int r, int g, int b)
{
    if (curveNum >= mNumCurves)
        return;
    
    SET_COLOR_FROM_INT(mCurves[curveNum]->mColor, r, g, b, 255);
    
    mDirty = true;
    
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

void
GraphControl10::SetCurveAlpha(int curveNum, BL_GUI_FLOAT alpha)
{
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->mAlpha = alpha;
    
    mDirty = true;
    
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

void
GraphControl10::SetCurveLineWidth(int curveNum, BL_GUI_FLOAT lineWidth)
{
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->mLineWidth = lineWidth;
    
    mDirty = true;
    
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

void
GraphControl10::SetCurveBevel(int curveNum, bool bevelFlag)
{
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->mBevelFlag = bevelFlag;
    
    mDirty = true;
    
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

void
GraphControl10::SetCurveSmooth(int curveNum, bool flag)
{
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->mDoSmooth = flag;
    
    mDirty = true;
    
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

void
GraphControl10::SetCurveFill(int curveNum, bool flag, BL_GUI_FLOAT originY)
{
    if (curveNum >= mNumCurves)
        return;

    mCurves[curveNum]->mCurveFill = flag;
    mCurves[curveNum]->mCurveFillOriginY = originY;
    
    mDirty = true;
    
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

void
GraphControl10::SetCurveFillColor(int curveNum, int r, int g, int b)
{
    if (curveNum >= mNumCurves)
        return;
    
    SET_COLOR_FROM_INT(mCurves[curveNum]->mFillColor, r, g, b, 255);
    mCurves[curveNum]->mFillColorSet = true;
    
    mDirty = true;
    
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

void
GraphControl10::SetCurveFillAlpha(int curveNum, BL_GUI_FLOAT alpha)
{
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->mFillAlpha = alpha;
    
    mDirty = true;
    
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

void
GraphControl10::SetCurveFillAlphaUp(int curveNum, BL_GUI_FLOAT alpha)
{
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->mFillAlphaUpFlag = true; //
    mCurves[curveNum]->mFillAlphaUp = alpha;
    
    mDirty = true;
    
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}


void
GraphControl10::SetCurvePointSize(int curveNum, BL_GUI_FLOAT pointSize)
{
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->mPointSize = pointSize;
    
    mDirty = true;
    
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

void
GraphControl10::SetCurvePointOverlay(int curveNum, bool flag)
{
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->mPointOverlay = flag;
    
    mDirty = true;
    
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

void
GraphControl10::SetCurveWeightMultAlpha(int curveNum, bool flag)
{
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->mWeightMultAlpha = flag;
    
    mDirty = true;
    
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

void
GraphControl10::SetCurveWeightTargetColor(int curveNum, int color[4])
{
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->SetWeightTargetColor(color);
    
    mDirty = true;
    
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

void
GraphControl10::SetCurveXScale(int curveNum, bool dbFlag, BL_GUI_FLOAT minX, BL_GUI_FLOAT maxX)
{
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->SetXScale(dbFlag, minX, maxX);
    
    mDirty = true;
    
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

void
GraphControl10::SetCurvePointStyle(int curveNum, bool pointFlag,
                                   bool pointsAsLinesPolar,
                                   bool pointsAsLines)
{
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->mPointStyle = pointFlag;
    mCurves[curveNum]->mPointsAsLinesPolar = pointsAsLinesPolar;
    mCurves[curveNum]->mPointsAsLines = pointsAsLines;
    
    mDirty = true;
    
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

void
GraphControl10::SetCurveValuesPoint(int curveNum,
                                   const WDL_TypedBuf<BL_GUI_FLOAT> &xValues,
                                   const WDL_TypedBuf<BL_GUI_FLOAT> &yValues)
{
    // Must lock otherwise with DATA_STEP, curve will jerk
    IPlugBase::IMutexLock lock(mPlug); // ??
    
    if (curveNum >= mNumCurves)
        return;
    
    GraphCurve4 *curve = mCurves[curveNum];
    curve->mPointStyle = true;
    
    curve->ClearValues();
    
    //mCurves[curveNum]->SetPointValues(xValues, yValues);
    
    if (curveNum >= mNumCurves)
        return;
    
#if 0 // For points, we don't care about num values
    if (xValues.GetSize() < mNumCurveValues)
        // Something went wrong
        return;
#endif
    
    int width = this->mRECT.W();
    int height = this->mRECT.H();
    
#if 0
    for (int i = 0; i < mNumCurveValues; i++)
#endif
    for (int i = 0; i < xValues.GetSize(); i++)
    {
        BL_GUI_FLOAT x = xValues.Get()[i];
        
        if (i >= yValues.GetSize())
            // Avoids a crash
            continue;
        
        BL_GUI_FLOAT y = yValues.Get()[i];
        
        x = ConvertX(curve, x, width);
        y = ConvertY(curve, y, height);
        
        curve->mXValues.Get()[i] = x;
        curve->mYValues.Get()[i] = y;
        //SetCurveValuePoint(curveNum, x, y);
    }
    
    mDirty = true;
    
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

void
GraphControl10::SetCurveValuesPointEx(int curveNum,
                                      const WDL_TypedBuf<BL_GUI_FLOAT> &xValues,
                                      const WDL_TypedBuf<BL_GUI_FLOAT> &yValues,
                                      bool singleScale, bool scaleX, bool centerFlag)
{
    // Must lock otherwise with DATA_STEP, curve will jerk
    IPlugBase::IMutexLock lock(mPlug); // ??
    
    if (curveNum >= mNumCurves)
        return;
    
    GraphCurve4 *curve = mCurves[curveNum];
    curve->mPointStyle = true;
    
    curve->ClearValues();
    
    //mCurves[curveNum]->SetPointValues(xValues, yValues);
    
    if (curveNum >= mNumCurves)
        return;
    
#if 0 // For points, we don't care about num values
    if (xValues.GetSize() < mNumCurveValues)
        // Something went wrong
        return;
#endif
    
    int width = this->mRECT.W();
    int height = this->mRECT.H();
    
#if 0
    for (int i = 0; i < mNumCurveValues; i++)
#endif
        for (int i = 0; i < xValues.GetSize(); i++)
        {
            BL_GUI_FLOAT x = xValues.Get()[i];
            
            if (i >= yValues.GetSize())
                // Avoids a crash
                continue;
            
            BL_GUI_FLOAT y = yValues.Get()[i];
            
            if (!singleScale)
            {
                x = ConvertX(curve, x, width);
                y = ConvertY(curve, y, height);
            }
            else
            {
                if (scaleX)
                {
                    x = ConvertX(curve, x, width);
                    y = ConvertY(curve, y, width);
                    
                    // TODO: test this
                    if (centerFlag)
                        x -= (width - height)*0.5;
                }
                else
                {
                    x = ConvertX(curve, x, height);
                    y = ConvertY(curve, y, height);
                    
                    if (centerFlag)
                        x += (width - height)*0.5;
                }
            }
            
            curve->mXValues.Get()[i] = x;
            curve->mYValues.Get()[i] = y;
            //SetCurveValuePoint(curveNum, x, y);
        }
    
    mDirty = true;
    
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

void
GraphControl10::SetCurveValuesPointWeight(int curveNum,
                                         const WDL_TypedBuf<BL_GUI_FLOAT> &xValues,
                                         const WDL_TypedBuf<BL_GUI_FLOAT> &yValues,
                                         const WDL_TypedBuf<BL_GUI_FLOAT> &weights)
{
    // Must lock otherwise with DATA_STEP, curve will jerk
    IPlugBase::IMutexLock lock(mPlug); // ??
    
    if (curveNum >= mNumCurves)
        return;
    
    GraphCurve4 *curve = mCurves[curveNum];
    curve->mPointStyle = true;
    
    curve->ClearValues();
    
    //mCurves[curveNum]->SetPointValues(xValues, yValues);
    
    if (curveNum >= mNumCurves)
        return;
    
#if 0 // For points, we don't care about num values
    if (xValues.GetSize() < mNumCurveValues)
        // Something went wrong
        return;
#endif
    
    int width = this->mRECT.W();
    int height = this->mRECT.H();
    
#if 0
    for (int i = 0; i < mNumCurveValues; i++)
#endif
        for (int i = 0; i < xValues.GetSize(); i++)
        {
            BL_GUI_FLOAT x = xValues.Get()[i];
            BL_GUI_FLOAT y = yValues.Get()[i];
            
            x = ConvertX(curve, x, width);
            y = ConvertY(curve, y, height);
            
            curve->mXValues.Get()[i] = x;
            curve->mYValues.Get()[i] = y;
            //SetCurveValuePoint(curveNum, x, y);
        }
    
    curve->mWeights = weights;
    
    mDirty = true;
    
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

void
GraphControl10::SetCurveColorWeight(int curveNum,
                                   const WDL_TypedBuf<BL_GUI_FLOAT> &colorWeights)
{
    // Must lock otherwise with DATA_STEP, curve will jerk
    IPlugBase::IMutexLock lock(mPlug); // ??
    
    if (curveNum >= mNumCurves)
        return;
    
    GraphCurve4 *curve = mCurves[curveNum];
    
    curve->mWeights = colorWeights;
    
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

void
GraphControl10::SetCurveSingleValueH(int curveNum, bool flag)
{
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->mSingleValueH = flag;
    
    mDirty = true;
    
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

void
GraphControl10::SetCurveSingleValueV(int curveNum, bool flag)
{
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->mSingleValueV = flag;
    
    mDirty = true;
    
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

void
GraphControl10::SetCurveOptimSameColor(int curveNum, bool flag)
{
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->mOptimSameColor = flag;
    
    mDirty = true;
    
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

void
GraphControl10::DrawText(NVGcontext *vg, BL_GUI_FLOAT x, BL_GUI_FLOAT y, BL_GUI_FLOAT fontSize,
                        const char *text, int color[4],
                        int halign, int valign, BL_GUI_FLOAT fontSizeCoeff)
{
    nvgSave(vg);
    
    nvgFontSize(vg, fontSize*fontSizeCoeff);
	nvgFontFace(vg, "font");
    nvgFontBlur(vg, 0);
	nvgTextAlign(vg, halign | valign);
    
    int sColor[4] = { color[0], color[1], color[2], color[3] };
    SWAP_COLOR(sColor);
    
    nvgFillColor(vg, nvgRGBA(sColor[0], sColor[1], sColor[2], sColor[3]));
    
	nvgText(vg, x, y, text, NULL);
    
    nvgRestore(vg);
}

void
GraphControl10::SetSpectrogram(BLSpectrogram3 *spectro,
                               BL_GUI_FLOAT left, BL_GUI_FLOAT top, BL_GUI_FLOAT right, BL_GUI_FLOAT bottom)
{
    if (mSpectrogramDisplay != NULL)
        delete mSpectrogramDisplay;
    
    mSpectrogramDisplay = new SpectrogramDisplay(mVg);
    mSpectrogramDisplay->SetSpectrogram(spectro,
                                        left, top, right, bottom);
}

SpectrogramDisplay *
GraphControl10::GetSpectrogramDisplay()
{
    return mSpectrogramDisplay;
}

void
GraphControl10::SetSpectrogramScroll(BLSpectrogram3 *spectro,
                                     BL_GUI_FLOAT left, BL_GUI_FLOAT top, BL_GUI_FLOAT right, BL_GUI_FLOAT bottom)
{
    if (mSpectrogramDisplayScroll != NULL)
        delete mSpectrogramDisplayScroll;
    
    mSpectrogramDisplayScroll = new SpectrogramDisplayScroll(mPlug, mVg);
    mSpectrogramDisplayScroll->SetSpectrogram(spectro,
                                              left, top, right, bottom);
}

SpectrogramDisplayScroll *
GraphControl10::GetSpectrogramDisplayScroll()
{
    return mSpectrogramDisplayScroll;
}

// 2
void
GraphControl10::SetSpectrogramScroll2(BLSpectrogram3 *spectro,
                                     BL_GUI_FLOAT left, BL_GUI_FLOAT top, BL_GUI_FLOAT right, BL_GUI_FLOAT bottom)
{
    if (mSpectrogramDisplayScroll2 != NULL)
        delete mSpectrogramDisplayScroll2;
    
    mSpectrogramDisplayScroll2 = new SpectrogramDisplayScroll2(mPlug, mVg);
    mSpectrogramDisplayScroll2->SetSpectrogram(spectro,
                                              left, top, right, bottom);
}

SpectrogramDisplayScroll2 *
GraphControl10::GetSpectrogramDisplayScroll2()
{
    return mSpectrogramDisplayScroll2;
}

void
GraphControl10::UpdateSpectrogram(bool updateData, bool updateFullData)
{
    if (mSpectrogramDisplay != NULL)
        mSpectrogramDisplay->UpdateSpectrogram(updateData, updateFullData);
    
    if (mSpectrogramDisplayScroll != NULL)
        mSpectrogramDisplayScroll->UpdateSpectrogram(updateData);
    
    if (mSpectrogramDisplayScroll2 != NULL)
        mSpectrogramDisplayScroll2->UpdateSpectrogram(updateData);
    
    mDirty = true;

#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

void
GraphControl10::UpdateSpectrogramColormap(bool updateData)
{
    if (mSpectrogramDisplayScroll != NULL)
        mSpectrogramDisplayScroll->UpdateColormap(updateData);
    
    if (mSpectrogramDisplayScroll2 != NULL)
        mSpectrogramDisplayScroll2->UpdateColormap(updateData);
    
    mDirty = true;
    
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

void
GraphControl10::AddCustomDrawer(GraphCustomDrawer *customDrawer)
{
    mCustomDrawers.push_back(customDrawer);
    
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

void
GraphControl10::CustomDrawersPreDraw(IPlugBase::IMutexLock *lock)
{
    int width = this->mRECT.W();
    int height = this->mRECT.H();
    
    for (int i = 0; i < mCustomDrawers.size(); i++)
    {
        GraphCustomDrawer *drawer = mCustomDrawers[i];
        
#if DRAWER_OWN_MUTEX_OPTIM
        // Unlock
        
        // Leave the mutex if necessary
        // Useful if PreDraw() consumes a lot, to let the drawer
        // manage its own mutex and then possibly unlock
        // the audio thread
        bool hasOwnMutex = drawer->HasOwnMutex();
        if (hasOwnMutex)
        {
            if (lock != NULL)
                lock->mpMutex->Leave();
        }
#endif
        // Draw
        drawer->PreDraw(mVg, width, height);
        
#if DRAWER_OWN_MUTEX_OPTIM
        // Re-lock
        if (hasOwnMutex)
        {
            if (lock != NULL)
                lock->mpMutex->Enter();
        }
#endif
    }
}

void
GraphControl10::CustomDrawersPostDraw(IPlugBase::IMutexLock *lock)
{
    int width = this->mRECT.W();
    int height = this->mRECT.H();
    
    for (int i = 0; i < mCustomDrawers.size(); i++)
    {
        GraphCustomDrawer *drawer = mCustomDrawers[i];
        
#if DRAWER_OWN_MUTEX_OPTIM
        // Unlock
        
        // Leave the mutex if necessary
        // Useful if PreDraw() consumes a lot, to let the drawer
        // manage its own mutex and then possibly unlock
        // the audio thread
        bool hasOwnMutex = drawer->HasOwnMutex();
        if (hasOwnMutex)
        {
            if (lock != NULL)
                lock->mpMutex->Leave();
        }
#endif
        
        // Draw
        drawer->PostDraw(mVg, width, height);
        
#if DRAWER_OWN_MUTEX_OPTIM
        // Re-lock
        if (hasOwnMutex)
        {
            if (lock != NULL)
                lock->mpMutex->Enter();
        }
#endif
    }
}

void
GraphControl10::DrawSeparatorY0()
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
   
    // Draw a vertical line ath the bottom
    nvgBeginPath(mVg);
    
    BL_GUI_FLOAT x0 = 0;
    BL_GUI_FLOAT x1 = width;
    
    BL_GUI_FLOAT y = mSepY0LineWidth/2.0;
                    
    nvgMoveTo(mVg, x0, y);
    nvgLineTo(mVg, x1, y);
                    
    nvgStroke(mVg);
    
    nvgRestore(mVg);
}

void
GraphControl10::AddCustomControl(GraphCustomControl *customControl)
{
    mCustomControls.push_back(customControl);
}

void
GraphControl10::OnMouseDown(int x, int y, IMouseMod* pMod)
{
    IControl::OnMouseDown(x, y, pMod);
    
#if CUSTOM_CONTROL_FIX
    x -= mRECT.L;
    y -= mRECT.T;
#endif
    
    for (int i = 0; i < mCustomControls.size(); i++)
    {
        GraphCustomControl *control = mCustomControls[i];
        control->OnMouseDown(x, y, pMod);
    }
}

void
GraphControl10::OnMouseUp(int x, int y, IMouseMod* pMod)
{
    IControl::OnMouseUp(x, y, pMod);
    
#if CUSTOM_CONTROL_FIX
    x -= mRECT.L;
    y -= mRECT.T;
#endif

    for (int i = 0; i < mCustomControls.size(); i++)
    {
        GraphCustomControl *control = mCustomControls[i];
        control->OnMouseUp(x, y, pMod);
    }
}

void
GraphControl10::OnMouseDrag(int x, int y, int dX, int dY, IMouseMod* pMod)
{
    IControl::OnMouseDrag(x, y, dX, dY, pMod);
    
#if CUSTOM_CONTROL_FIX
    x -= mRECT.L;
    y -= mRECT.T;
#endif

    for (int i = 0; i < mCustomControls.size(); i++)
    {
        GraphCustomControl *control = mCustomControls[i];
        control->OnMouseDrag(x, y, dX, dY, pMod);
    }
}

bool
GraphControl10::OnMouseDblClick(int x, int y, IMouseMod* pMod)
{
    bool dblClickDone = IControl::OnMouseDblClick(x, y, pMod);
    if (!dblClickDone)
        return false;
    
#if CUSTOM_CONTROL_FIX
    x -= mRECT.L;
    y -= mRECT.T;
#endif

    for (int i = 0; i < mCustomControls.size(); i++)
    {
        GraphCustomControl *control = mCustomControls[i];
        control->OnMouseDblClick(x, y, pMod);
    }
    
    return true;
}

void
GraphControl10::OnMouseWheel(int x, int y, IMouseMod* pMod, BL_GUI_FLOAT d)
{
    IControl::OnMouseWheel(x, y, pMod, d);
    
#if CUSTOM_CONTROL_FIX
    x -= mRECT.L;
    y -= mRECT.T;
#endif

    for (int i = 0; i < mCustomControls.size(); i++)
    {
        GraphCustomControl *control = mCustomControls[i];
        control->OnMouseWheel(x, y, pMod, d);
    }
}

bool
GraphControl10::OnKeyDown(int x, int y, int key, IMouseMod* pMod)
{
    IControl::OnKeyDown(x, y, key, pMod);
    
#if CUSTOM_CONTROL_FIX
    x -= mRECT.L;
    y -= mRECT.T;
#endif

    bool res = false;
    for (int i = 0; i < mCustomControls.size(); i++)
    {
        GraphCustomControl *control = mCustomControls[i];
        res = control->OnKeyDown(x, y, key, pMod);
    }
    
    return res;
}

void
GraphControl10::OnMouseOver(int x, int y, IMouseMod* pMod)
{
    IControl::OnMouseOver(x, y, pMod);
    
#if CUSTOM_CONTROL_FIX
    x -= mRECT.L;
    y -= mRECT.T;
#endif

    for (int i = 0; i < mCustomControls.size(); i++)
    {
        GraphCustomControl *control = mCustomControls[i];
        control->OnMouseOver(x, y, pMod);
    }
}

void
GraphControl10::OnMouseOut()
{
    IControl::OnMouseOut();
    
    for (int i = 0; i < mCustomControls.size(); i++)
    {
        GraphCustomControl *control = mCustomControls[i];
        control->OnMouseOut();
    }
}

#if 0
void
GraphControl10::InitGL()
{
    if (mGLInitialized)
        return;
	
    GLContext3::Ref();
    GLContext3 *context = GLContext3::Get();

	// FIX: Ableton 10 on Windows 10
	// The plugin didn't load because it crashed when scanned in the host
	//
	if (context == NULL)
		return;

    context->Bind();

    mGLInitialized = true;
}

void
GraphControl10::ExitGL()
{
    GLContext3 *context = GLContext3::Get();
    
	// FIX: crash Ableton 10 Windows 10 (plugins not recognized) 
	if (context != NULL)
		context->Unbind();

    GLContext3::Unref();

    mGLInitialized = false;
}
#endif

void
GraphControl10::DBG_PrintCoords(int x, int y)
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
GraphControl10::SetBackgroundImage(LICE_IBitmap *bmp)
{
    mBgImage = bmp;
    
#if 0 // CRASHES on Protools
    if (mNvgBackgroundImage != 0)
        nvgDeleteImage(mVg, mNvgBackgroundImage);
#endif
    
    mNvgBackgroundImage = 0;
    
    mDirty = true;
    
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

void
GraphControl10::SetOverlayImage(LICE_IBitmap *bmp)
{
    mOverImage = bmp;
    
    mNvgOverlayImage = 0;
    
    mDirty = true;
    
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

void
GraphControl10::SetDisablePointOffsetHack(bool flag)
{
    mDisablePointOffsetHack = flag;
}

void
GraphControl10::SetRecreateWhiteImageHack(bool flag)
{
    mRecreateWhiteImageHack = flag;
}

void
GraphControl10::CreateImageDisplay(BL_GUI_FLOAT left, BL_GUI_FLOAT top, BL_GUI_FLOAT right, BL_GUI_FLOAT bottom,
                                   ImageDisplay::Mode mode)
{
    if (mImageDisplay != NULL)
        delete mImageDisplay;
    
    mImageDisplay = new ImageDisplay(mVg, mode);
    mImageDisplay->SetBounds(left, top, right, bottom);
}

ImageDisplay *
GraphControl10::GetImageDisplay()
{
    return mImageDisplay;
}

void
GraphControl10::DisplayCurveDescriptions()
{
    // NEW
#define OFFSET_Y 4.0
    
#define DESCR_X 40.0
#define DESCR_Y0 10.0 + OFFSET_Y
    
#define DESCR_WIDTH 20
#define DESCR_Y_STEP 12
#define DESCR_SPACE 5
    
#define TEXT_Y_OFFSET 2
    
    int descrNum = 0;
    for (int i = 0; i < mCurves.size(); i++)
    {
        GraphCurve4 *curve = mCurves[i];
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
        
        nvgBeginPath(mVg);
        
        nvgMoveTo(mVg, DESCR_X, y);
        nvgLineTo(mVg, DESCR_X + DESCR_WIDTH, y);
        
        nvgStroke(mVg);
        
        DrawText(DESCR_X + DESCR_WIDTH + DESCR_SPACE, y + TEXT_Y_OFFSET,
                 FONT_SIZE, descr,
                 curve->mDescrColor,
                 NVG_ALIGN_LEFT, NVG_ALIGN_MIDDLE);
        
        nvgRestore(mVg);
        
        descrNum++;
    }
}

void
GraphControl10::AddAxis(GraphAxis *axis, char *data[][2], int numData, int axisColor[4],
                       int axisLabelColor[4],
                       BL_GUI_FLOAT minVal, BL_GUI_FLOAT maxVal, int axisLabelOverlayColor[4],
                       int axisLinesOverlayColor[4])
{
    // Copy color
    for (int i = 0; i < 4; i++)
    {
        int sAxisColor[4] = { axisColor[0], axisColor[1], axisColor[2], axisColor[3] };
        SWAP_COLOR(sAxisColor);
        
        axis->mColor[i] = sAxisColor[i];
        
        int sLabelColor[4] = { axisLabelColor[0], axisLabelColor[1],
                               axisLabelColor[2], axisLabelColor[3] };
        SWAP_COLOR(sLabelColor);
        
        axis->mLabelColor[i] = sLabelColor[i];

#if FIX_INIT_COLORS
	axis->mLabelOverlayColor[i] = 0;
#endif
	
        if (axisLabelOverlayColor != NULL)
        {
            axis->mOverlay = true;
            
            int sOverColor[4] = { axisLabelOverlayColor[0], axisLabelOverlayColor[1],
                axisLabelOverlayColor[2], axisLabelOverlayColor[3] };
            SWAP_COLOR(sOverColor);
            
            axis->mLabelOverlayColor[i] = sOverColor[i];
        }
      
#if FIX_INIT_COLORS
	axis->mLinesOverlayColor[i] = 0;
#endif
  
        if (axisLinesOverlayColor != NULL)
        {
            axis->mLinesOverlay = true;
            
            int sOverColor[4] = { axisLinesOverlayColor[0], axisLinesOverlayColor[1],
                                  axisLinesOverlayColor[2], axisLinesOverlayColor[3] };
            SWAP_COLOR(sOverColor);
            
            axis->mLinesOverlayColor[i] = sOverColor[i];
        }
    }
    
    // Copy data
    for (int i = 0; i < numData; i++)
    {
        char *cData[2] = { data[i][0], data[i][1] };
        
        BL_GUI_FLOAT t = atof(cData[0]);
        
        string text(cData[1]);
        
        // Error here, if we add an Y axis, we must not use mMinXdB
        GraphAxisData aData;
        aData.mT = (t - minVal)/(maxVal - minVal);
        aData.mText = text;
        
        axis->mValues.push_back(aData);
    }
    
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

void
GraphControl10::SetCurveDescription(int curveNum, const char *description, int descrColor[4])
{
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->SetDescription(description, descrColor);
    
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

void
GraphControl10::SetCurveLimitToBounds(int curveNum, bool flag)
{
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->SetLimitToBounds(flag);
    
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

void
GraphControl10::ResetCurve(int curveNum, BL_GUI_FLOAT val)
{
    IPlugBase::IMutexLock lock(mPlug);
    
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->FillAllXValues(mMinX, mMaxX);
    mCurves[curveNum]->FillAllYValues(val);
    
    mDirty = true;
    
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

void
GraphControl10::SetCurveYScale(int curveNum, bool dbFlag, BL_GUI_FLOAT minY, BL_GUI_FLOAT maxY)
{
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->SetYScale(dbFlag, minY, maxY);
    
    mDirty = true;
    
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

void
GraphControl10::CurveFillAllValues(int curveNum, BL_GUI_FLOAT val)
{
    // Must lock otherwise with DATA_STEP, curve will jerk
    IPlugBase::IMutexLock lock(mPlug);
    
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->FillAllXValues(mMinX, mMaxX);
    mCurves[curveNum]->FillAllYValues(val);
    
    mDirty = true;
    
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

void
GraphControl10::SetCurveValues(int curveNum, const WDL_TypedBuf<BL_GUI_FLOAT> *values)
{
    // Must lock otherwise with DATA_STEP, curve will jerk
    IPlugBase::IMutexLock lock(mPlug);
    
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->SetYValues(values, mMinX, mMaxX);
    
    mDirty = true;
    
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

void
GraphControl10::SetCurveValues2(int curveNum, const WDL_TypedBuf<BL_GUI_FLOAT> *values)
{
    // Must lock otherwise with DATA_STEP, curve will jerk
    IPlugBase::IMutexLock lock(mPlug);
    
    if (curveNum >= mNumCurves)
        return;
    
    if (values->GetSize() < mNumCurveValues)
        // Something went wrong
        return;
    
    for (int i = 0; i < mNumCurveValues; i++)
    {
        BL_GUI_FLOAT t = ((BL_GUI_FLOAT)i)/(values->GetSize() - 1);
        BL_GUI_FLOAT y = values->Get()[i];
        
        SetCurveValue(curveNum, t, y);
    }
    
    mDirty = true;
    
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

void
GraphControl10::SetCurveValues3(int curveNum, const WDL_TypedBuf<BL_GUI_FLOAT> *values)
{
    // Must lock otherwise with DATA_STEP, curve will jerk
    IPlugBase::IMutexLock lock(mPlug);
    
    if (curveNum >= mNumCurves)
        return;
    
    GraphCurve4 *curve = mCurves[curveNum];
    curve->ClearValues();
    
    if (values->GetSize() == 0)
        return;
    
    for (int i = 0; i < mNumCurveValues; i++)
    {
        BL_GUI_FLOAT t = ((BL_GUI_FLOAT)i)/(values->GetSize() - 1);
        BL_GUI_FLOAT y = values->Get()[i];
        
        SetCurveValue(curveNum, t, y);
    }
    
    mDirty = true;
    
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

void
GraphControl10::SetCurveValuesDecimateSimple(int curveNum,
                                             const WDL_TypedBuf<BL_GUI_FLOAT> *values)
{
    // Must lock otherwise with DATA_STEP, curve will jerk
    IPlugBase::IMutexLock lock(mPlug);
    
    if (curveNum >= mNumCurves)
        return;
    
    int width = this->mRECT.W();
    
    BL_GUI_FLOAT prevX = -1.0;
    
    for (int i = 0; i < values->GetSize(); i++)
    {
        BL_GUI_FLOAT t = ((BL_GUI_FLOAT)i)/(values->GetSize() - 1);
        
        BL_GUI_FLOAT x = t*width;
        BL_GUI_FLOAT y = values->Get()[i];
        
        if (x - prevX < 1.0)
            continue;
        
        prevX = x;
        
        SetCurveValue(curveNum, t, y);
    }
    
    // Avoid last value at 0 !
    // (would make a traversing line in the display)
    BL_GUI_FLOAT lastValue = values->Get()[values->GetSize() - 1];
    SetCurveValue(curveNum, 1.0, lastValue);
    
    mDirty = true;
    
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}


void
GraphControl10::SetCurveValuesDecimate(int curveNum,
                                      const WDL_TypedBuf<BL_GUI_FLOAT> *values,
                                      bool isWaveSignal)
{    
    // Must lock otherwise with DATA_STEP, curve will jerk
    IPlugBase::IMutexLock lock(mPlug);
    
    if (curveNum >= mNumCurves)
        return;
    
    GraphCurve4 *curve = mCurves[curveNum];
    
    int width = this->mRECT.W();
    
    BL_GUI_FLOAT prevX = -1.0;
    BL_GUI_FLOAT maxY = -1.0;
    
    if (isWaveSignal)
        maxY = (curve->mMinY + curve->mMaxY)/2.0;
        
    BL_GUI_FLOAT thrs = 1.0/GRAPHCONTROL_PIXEL_DENSITY;
    for (int i = 0; i < values->GetSize(); i++)
    {
        BL_GUI_FLOAT t = ((BL_GUI_FLOAT)i)/(values->GetSize() - 1);
        
        BL_GUI_FLOAT x = t*width;
        BL_GUI_FLOAT y = values->Get()[i];
        
        // Keep the maximum
        // (we prefer keeping the maxima, and not discard them)
        if (!isWaveSignal)
        {
	  if (std::fabs(y) > maxY)
                maxY = y;
        }
        else
        {
	  if (std::fabs(y) > std::fabs(maxY))
                maxY = y;
        }
        
        if (x - prevX < thrs)
            continue;
        
        prevX = x;
        
        SetCurveValue(curveNum, t, maxY);
        
        maxY = -1.0;
        
        if (isWaveSignal)
            maxY = (curve->mMinY + curve->mMaxY)/2.0;
    }
    
    mDirty = true;
    
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

void
GraphControl10::SetCurveValuesDecimate2(int curveNum,
                                        const WDL_TypedBuf<BL_GUI_FLOAT> *values,
                                        bool isWaveSignal)
{
    // Must lock otherwise with DATA_STEP, curve will jerk
    IPlugBase::IMutexLock lock(mPlug);
    
    if (curveNum >= mNumCurves)
        return;
    
    GraphCurve4 *curve = mCurves[curveNum];
    curve->ClearValues();
    
    if (values->GetSize() == 0)
        return;
    
    // Decimate
    
    //GRAPHCONTROL_PIXEL_DENSITY ?
    BL_GUI_FLOAT decFactor = ((BL_GUI_FLOAT)mNumCurveValues)/values->GetSize();
    
    WDL_TypedBuf<BL_GUI_FLOAT> decimValues;
    if (isWaveSignal)
        BLUtils::DecimateSamples(&decimValues, *values, decFactor);
    else
        BLUtils::DecimateValues(&decimValues, *values, decFactor);
    
    for (int i = 0; i < decimValues.GetSize(); i++)
    {
        BL_GUI_FLOAT t = ((BL_GUI_FLOAT)i)/(decimValues.GetSize() - 1);
        
        BL_GUI_FLOAT y = decimValues.Get()[i];
        
        SetCurveValue(curveNum, t, y);
    }
    
    mDirty = true;
    
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

void
GraphControl10::SetCurveValuesDecimate3(int curveNum,
                                        const WDL_TypedBuf<BL_GUI_FLOAT> *values,
                                        bool isWaveSignal)
{
    // Must lock otherwise with DATA_STEP, curve will jerk
    IPlugBase::IMutexLock lock(mPlug);
    
    if (curveNum >= mNumCurves)
        return;
    
    GraphCurve4 *curve = mCurves[curveNum];
    curve->ClearValues();
    
    if (values->GetSize() == 0)
        return;
    
    // Decimate
    
    //GRAPHCONTROL_PIXEL_DENSITY ?
    BL_GUI_FLOAT decFactor = ((BL_GUI_FLOAT)mNumCurveValues)/values->GetSize();
    
    WDL_TypedBuf<BL_GUI_FLOAT> decimValues;
    if (isWaveSignal)
        BLUtils::DecimateSamples2(&decimValues, *values, decFactor);
    else
        BLUtils::DecimateValues(&decimValues, *values, decFactor);
    
    for (int i = 0; i < decimValues.GetSize(); i++)
    {
        BL_GUI_FLOAT t = ((BL_GUI_FLOAT)i)/(decimValues.GetSize() - 1);
        
        BL_GUI_FLOAT y = decimValues.Get()[i];
        
        SetCurveValue(curveNum, t, y);
    }
    
    mDirty = true;
    
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

void
GraphControl10::SetCurveValuesXDbDecimate(int curveNum, const WDL_TypedBuf<BL_GUI_FLOAT> *values,
                                          int nativeBufferSize, BL_GUI_FLOAT sampleRate,
                                          BL_GUI_FLOAT decimFactor)
{
    int bufferSize = BLUtils::PlugComputeBufferSize(nativeBufferSize, sampleRate);
    BL_GUI_FLOAT hzPerBin = sampleRate/bufferSize;
    
    BL_GUI_FLOAT minHzValue;
    BL_GUI_FLOAT maxHzValue;
    BLUtils::GetMinMaxFreqAxisValues(&minHzValue, &maxHzValue,
                                   bufferSize, sampleRate);
    
    WDL_TypedBuf<BL_GUI_FLOAT> logSignal;
    BLUtils::FreqsToDbNorm(&logSignal, *values, hzPerBin,
                         minHzValue, maxHzValue);
    
    WDL_TypedBuf<BL_GUI_FLOAT> decimSignal;
    BLUtils::DecimateValues(&decimSignal, logSignal, decimFactor);
    
    //BLUtils::DecimateSamples(&decimSignal, logSignal, GRAPH_DEC_FACTOR/bufferSizeCoeff);
    
    for (int i = 0; i < decimSignal.GetSize(); i++)
    {
        BL_GUI_FLOAT t = ((BL_GUI_FLOAT)i) / (decimSignal.GetSize() - 1);
        
        SetCurveValue(curveNum, t, decimSignal.Get()[i]);
    }
}

void
GraphControl10::SetCurveValuesXDbDecimateDb(int curveNum, const WDL_TypedBuf<BL_GUI_FLOAT> *values,
                                           int nativeBufferSize, BL_GUI_FLOAT sampleRate,
                                           BL_GUI_FLOAT decimFactor, BL_GUI_FLOAT minValueDb)
{
    int bufferSize = BLUtils::PlugComputeBufferSize(nativeBufferSize, sampleRate);
    BL_GUI_FLOAT hzPerBin = sampleRate/bufferSize;
    
    BL_GUI_FLOAT minHzValue;
    BL_GUI_FLOAT maxHzValue;
    BLUtils::GetMinMaxFreqAxisValues(&minHzValue, &maxHzValue,
                                   bufferSize, sampleRate);
    
    WDL_TypedBuf<BL_GUI_FLOAT> logSignal;
    BLUtils::FreqsToDbNorm(&logSignal, *values, hzPerBin,
                         minHzValue, maxHzValue);
    
    WDL_TypedBuf<BL_GUI_FLOAT> decimSignal;
    BLUtils::DecimateValuesDb(&decimSignal, logSignal, decimFactor, minValueDb);
    
    //BLUtils::DecimateValues(&decimSignal, logSignal, decimFactor, minValue);
    //BLUtils::DecimateSamples(&decimSignal, logSignal, GRAPH_DEC_FACTOR/bufferSizeCoeff);
    
    for (int i = 0; i < decimSignal.GetSize(); i++)
    {
        BL_GUI_FLOAT t = ((BL_GUI_FLOAT)i) / (decimSignal.GetSize() - 1);
        
        SetCurveValue(curveNum, t, decimSignal.Get()[i]);
    }
}

void
GraphControl10::SetCurveValue(int curveNum, BL_GUI_FLOAT t, BL_GUI_FLOAT val)
{
    // Must lock otherwise we may have curve will jerk (??)
    IPlugBase::IMutexLock lock(mPlug);
    
    if (curveNum >= mNumCurves)
        return;
    
    GraphCurve4 *curve = mCurves[curveNum];
    
    // Normalize, then adapt to the graph
    int width = this->mRECT.W();
    int height = this->mRECT.H();
    
    BL_GUI_FLOAT x = t;
    
    //if (x != GRAPH_VALUE_UNDEFINED) // for double
    if (x < GRAPH_VALUE_UNDEFINED) // for float
    {
        if (mXdBScale)
            x = BLUtils::NormalizedXTodB(x, mMinX, mMaxX);
        
        // X should be already normalize in input
        //else
        //    x = (x - mMinX)/(mMaxX - mMinX);
    
        // Scale for the interface
        x = x * width;
    }
    
    BL_GUI_FLOAT y = ConvertY(curve, val, height);
    
    // For Ghost and mini view
    if (curve->mLimitToBounds)
    {
        if (y < (1.0 - mBounds[3])*height)
            y = (1.0 - mBounds[3])*height;
        
        if (y > (1.0 - mBounds[1])*height)
            y = (1.0 - mBounds[1])*height;
    }
    
    curve->SetValue(t, x, y);
    
    mDirty = true;
    
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

#if 0
void
GraphControl10::SetCurveValuePoint(int curveNum, BL_GUI_FLOAT x, BL_GUI_FLOAT y)
{
    if (curveNum >= mNumCurves)
        return;
    
    if (curveNum >= mNumCurves)
        return;
    
    GraphCurve4 *curve = mCurves[curveNum];
    
    BL_GUI_FLOAT t = (x - curve->mMinX)/(curve->mMaxX - curve->mMinX);
    
    SetCurveValue(curveNum, t, y);
}
#endif

void
GraphControl10::SetCurveSingleValueH(int curveNum, BL_GUI_FLOAT val)
{
    // Must lock otherwise with DATA_STEP, curve will jerk
    IPlugBase::IMutexLock lock(mPlug);
    
    if (curveNum >= mNumCurves)
        return;
    
    //mCurves[curveNum]->SetValue(0.0, 0.0, val);
    SetCurveValue(curveNum, 0.0, val);
    
    mDirty = true;
    
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

void
GraphControl10::SetCurveSingleValueV(int curveNum, BL_GUI_FLOAT val)
{
    // Must lock otherwise with DATA_STEP, curve will jerk
    IPlugBase::IMutexLock lock(mPlug);
    
    if (curveNum >= mNumCurves)
        return;
    
    //mCurves[curveNum]->SetValue(0.0, 0.0, val);
    SetCurveValue(curveNum, 0.0, val);
    
    mDirty = true;
    
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

void
GraphControl10::PushCurveValue(int curveNum, BL_GUI_FLOAT val)
{
    IPlugBase::IMutexLock lock(mPlug);
    
    if (curveNum >= mNumCurves)
        return;
    
    int height = this->mRECT.H();
    
    GraphCurve4 *curve = mCurves[curveNum];
    val = ConvertY(curve, val, height);
    
    BL_GUI_FLOAT dummyX = 1.0;
    curve->PushValue(dummyX, val);
    
    BL_GUI_FLOAT maxXValue = this->mRECT.W();
    curve->NormalizeXValues(maxXValue);
    
    mDirty = true;
    
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

void
GraphControl10::DrawAxis(bool lineLabelFlag)
{
    if (mHAxis != NULL)
        DrawAxis(mHAxis, true, lineLabelFlag);
    
    if (mVAxis != NULL)
        DrawAxis(mVAxis, false, lineLabelFlag);
}

void
GraphControl10::DrawAxis(GraphAxis *axis, bool horizontal, bool lineLabelFlag)
{
    nvgSave(mVg);
    nvgStrokeWidth(mVg, 1.0);
    
    int axisColor[4] = { axis->mColor[0], axis->mColor[1], axis->mColor[2], axis->mColor[3] };
    SWAP_COLOR(axisColor);
    
    int axisLabelOverlayColor[4] = { axis->mLabelOverlayColor[0], axis->mLabelOverlayColor[1],
                                     axis->mLabelOverlayColor[2], axis->mLabelOverlayColor[3] };
    SWAP_COLOR(axisLabelOverlayColor);
    
    int axisLinesOverlayColor[4] = { axis->mLinesOverlayColor[0], axis->mLinesOverlayColor[1],
                                     axis->mLinesOverlayColor[2], axis->mLinesOverlayColor[3] };
    SWAP_COLOR(axisLinesOverlayColor);
    
    nvgStrokeColor(mVg, nvgRGBA(axisColor[0], axisColor[1], axisColor[2], axisColor[3]));
    
    int width = this->mRECT.W();
    int height = this->mRECT.H();
    
    for (int i = 0; i < axis->mValues.size(); i++)
    {
        const GraphAxisData &data = axis->mValues[i];
        
        BL_GUI_FLOAT t = data.mT;
        const char *text = data.mText.c_str();

        if (horizontal)
        {
            BL_GUI_FLOAT textOffset = FONT_SIZE*0.2;
            
            //if (mXdBScale)
            if (axis->mXdBScale)
                t = BLUtils::NormalizedXTodB(t, mMinX, mMaxX);
            //else
            //    t = (t - mMinX)/(mMaxX - mMinX);
            
            BL_GUI_FLOAT x = t*width;
        
            // For Impulse
            ///bool validateFirst = true;
            
            //bool validateFirst = lineLabelFlag ? (t > 0.0) : (i > 0);
            if ((i > 0) && (i < axis->mValues.size() - 1))
            //if (validateFirst && (i < axis->mValues.size() - 1))
                // First and last: don't draw axis line
            {
                if (lineLabelFlag)
                {
                    // Draw a vertical line
                    nvgBeginPath(mVg);

                    BL_GUI_FLOAT y0 = 0.0;
                    BL_GUI_FLOAT y1 = height;
        
                    nvgMoveTo(mVg, x, y0);
                    nvgLineTo(mVg, x, y1);
    
                    nvgStroke(mVg);
                    
                    if (axis->mLinesOverlay)
                    {
                        nvgStrokeColor(mVg, nvgRGBA(axisLinesOverlayColor[0], axisLinesOverlayColor[1],
                                                    axisLinesOverlayColor[2], axisLinesOverlayColor[3]));
                        
                        // Draw a vertical line
                        nvgBeginPath(mVg);
                        BL_GUI_FLOAT y0 = 0.0;
                        BL_GUI_FLOAT y1 = height;
                        
                        nvgMoveTo(mVg, x + OVERLAY_OFFSET, y0);
                        nvgLineTo(mVg, x + OVERLAY_OFFSET, y1);
                        nvgStroke(mVg);
                        
                        nvgStrokeColor(mVg, nvgRGBA(axisColor[0], axisColor[1], axisColor[2], axisColor[3]));
                    }
                }
                else
                {
                    if (axis->mOverlay)
                    {
                        // Draw background text (for overlay)
                        DrawText(x + OVERLAY_OFFSET,
                                 textOffset + axis->mOffsetY*height + OVERLAY_OFFSET,
                                 FONT_SIZE, text,
                                 axis->mLabelOverlayColor,
                                 NVG_ALIGN_CENTER, NVG_ALIGN_BOTTOM,
                                 axis->mFontSizeCoeff);
                    }
                    
                    DrawText(x, textOffset + axis->mOffsetY*height,
                             FONT_SIZE, text, axis->mLabelColor,
                             NVG_ALIGN_CENTER, NVG_ALIGN_BOTTOM,
                             axis->mFontSizeCoeff);
                }
            }
     
            if (!lineLabelFlag)
            {
                if (i == 0)
                {
                    if (axis->mOverlay)
                    {
                        // Draw background text (for overlay)
                        DrawText(x + textOffset + OVERLAY_OFFSET,
                                 textOffset + axis->mOffsetY*height + OVERLAY_OFFSET,
                                 FONT_SIZE, text,
                                 axis->mLabelOverlayColor, NVG_ALIGN_LEFT, NVG_ALIGN_BOTTOM,
                                 axis->mFontSizeCoeff);
                    }
                    
                    // First text: aligne left
                    DrawText(x + textOffset, textOffset + axis->mOffsetY*height, FONT_SIZE,
                             text, axis->mLabelColor, NVG_ALIGN_LEFT, NVG_ALIGN_BOTTOM,
                             axis->mFontSizeCoeff);
                }
        
                if (i == axis->mValues.size() - 1)
                {
                    if (axis->mOverlay)
                    {
                        // Draw background text (for overlay)
                        DrawText(x - textOffset + OVERLAY_OFFSET,
                                 textOffset + axis->mOffsetY*height + OVERLAY_OFFSET,
                                 FONT_SIZE, text,
                                 axis->mLabelOverlayColor, NVG_ALIGN_RIGHT, NVG_ALIGN_BOTTOM,
                                 axis->mFontSizeCoeff);
                    }
                    
                    // Last text: aligne right
                    DrawText(x - textOffset, textOffset + axis->mOffsetY*height,
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
            
            // Re-added dB normalization for Ghost
            if (!mCurves.empty())
            {
                if (mCurves[0]->mYdBScale)
                    t = BLUtils::NormalizedXTodB(t, mCurves[0]->mMinY, mCurves[0]->mMaxY);
            }
            
            t = ConvertToBoundsY(t);
            
            // Do not need to nomalize on the Y axis, because we want to display
            // the dB linearly (this is the curve values that are displayed exponentially).
            //t = BLUtils::NormalizedYTodB3(t, mMinXdB, mMaxXdB);
            
            // For Impulse
            //t = (t - mCurves[0]->mMinY)/(mCurves[0]->mMaxY - mCurves[0]->mMinY);
            
            BL_GUI_FLOAT y = t*height;
            
            // Hack
            // See Transient, with it 2 vertical axis
            y += axis->mOffset;
            
            if ((i > 0) && (i < axis->mValues.size() - 1))
                // First and last: don't draw axis line
            {
                if (lineLabelFlag)
                {
                    // Draw a horizontal line
                    nvgBeginPath(mVg);
                
                    BL_GUI_FLOAT x0 = 0.0;
                    BL_GUI_FLOAT x1 = width;
                
                    nvgMoveTo(mVg, x0, y);
                    nvgLineTo(mVg, x1, y);
                
                    nvgStroke(mVg);
                    
                    if (axis->mLinesOverlay)
                    {
                        nvgStrokeColor(mVg, nvgRGBA(axisLinesOverlayColor[0], axisLinesOverlayColor[1],
                                                    axisLinesOverlayColor[2], axisLinesOverlayColor[3]));
                        
                        // Draw a vertical line
                        nvgBeginPath(mVg);
                        BL_GUI_FLOAT x0 = 0.0;
                        BL_GUI_FLOAT x1 = width;
                        
                        nvgMoveTo(mVg, x0, y + OVERLAY_OFFSET);
                        nvgLineTo(mVg, x1, y + OVERLAY_OFFSET);
                        nvgStroke(mVg);
                        
                        nvgStrokeColor(mVg, nvgRGBA(axisColor[0], axisColor[1], axisColor[2], axisColor[3]));
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
                        DrawText(textOffset + axis->mOffsetX + OVERLAY_OFFSET,
                                 y + OVERLAY_OFFSET,
                                 FONT_SIZE, text, axis->mLabelOverlayColor,
                                 align/*NVG_ALIGN_LEFT*/ | NVG_ALIGN_MIDDLE, NVG_ALIGN_BOTTOM,
                                 axis->mFontSizeCoeff);
                    }
                    
                    DrawText(textOffset + axis->mOffsetX, y, FONT_SIZE, text,
                             axis->mLabelColor,
                             align/*NVG_ALIGN_LEFT*/ | NVG_ALIGN_MIDDLE,
                             NVG_ALIGN_BOTTOM,
                             axis->mFontSizeCoeff);
                }
            }
            
            if (!lineLabelFlag)
            {
                if (i == 0)
                    // First text: align "top"
                {
                    if (axis->mOverlay)
                    {
                        // Draw background text (for overlay)
                        DrawText(textOffset + axis->mOffsetX + OVERLAY_OFFSET,
                                 y + FONT_SIZE*0.75 + OVERLAY_OFFSET,
                                 FONT_SIZE, text, axis->mLabelOverlayColor,
                                 NVG_ALIGN_LEFT, NVG_ALIGN_BOTTOM,
                                 axis->mFontSizeCoeff);
                    }
                    
                    DrawText(textOffset + axis->mOffsetX, y + FONT_SIZE*0.75, FONT_SIZE, text,
                             axis->mLabelColor, NVG_ALIGN_LEFT, NVG_ALIGN_BOTTOM,
                             axis->mFontSizeCoeff);
                }
                
                if (i == axis->mValues.size() - 1)
                    // Last text: align "bottom"
                {
                    if (axis->mOverlay)
                    {
                        // Draw background text (for overlay)
                        DrawText(textOffset + axis->mOffsetX + OVERLAY_OFFSET,
                                 y - FONT_SIZE*1.5 + OVERLAY_OFFSET,
                                 FONT_SIZE, text, axis->mLabelOverlayColor,
                                 NVG_ALIGN_LEFT, NVG_ALIGN_BOTTOM,
                                 axis->mFontSizeCoeff);
                    }
                    
                    DrawText(textOffset + axis->mOffsetX, y - FONT_SIZE*1.5, FONT_SIZE, text,
                             axis->mLabelColor, NVG_ALIGN_LEFT, NVG_ALIGN_BOTTOM,
                             axis->mFontSizeCoeff);
                }
            }
        }
    }
    
    nvgRestore(mVg);
}

void
GraphControl10::DrawCurves()
{
    for (int i = 0; i < mNumCurves; i++)
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
                        //if (mCurves[i]->mWeights.GetSize() == 0)
                            DrawPointCurve(mCurves[i]);
                        //else
                        //    DrawPointCurveWeights(mCurves[i]);
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
                if (!mCurves[i]->mYValues.GetSize() == 0)
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
                    if (!mCurves[i]->mXValues.GetSize() == 0)
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
GraphControl10::DrawLineCurve(GraphCurve4 *curve)
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
        
        //if (x == GRAPH_VALUE_UNDEFINED) // for double
        if (x >= GRAPH_VALUE_UNDEFINED) // for float
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
        //if (y == GRAPH_VALUE_UNDEFINED) // for double
        if (y >= GRAPH_VALUE_UNDEFINED) // for float
            continue;
        
        if (firstPoint)
        {
            nvgMoveTo(mVg, x, y);
            
            firstPoint = false;
        }
        
        nvgLineTo(mVg, x, y);
    }
    
    nvgStroke(mVg);
    nvgRestore(mVg);
    
    mDirty = true;
    
    // ??
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
    
#if CURVE_DEBUG
    fprintf(stderr, "GraphControl10::DrawLineCurve - num points: %d\n", numPointsDrawn);
#endif
}

#if !FILL_CURVE_HACK
        // Bug with direct rendering
        // It seems we have no stencil, and we can only render convex polygons
void
GraphControl10::DrawFillCurve(GraphCurve4 *curve)
{
    // TEST for compilation
    BL_GUI_FLOAT lineWidth = 1.0;
    
    // Offset used to draw the closing of the curve outside the viewport
    // Because we draw both stroke and fill at the same time
#define OFFSET lineWidth
    
    nvgSave(mVg);
    
    int sFillColor[4] = { (int)(curve->mColor[0]*255), (int)(curve->mColor[1]*255),
                      (int)(curve->mColor[2]*255), (int)(curve->mFillAlpha*255) };
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
        
        //if (x == GRAPH_VALUE_UNDEFINED) // for double
        if (x >= GRAPH_VALUE_UNDEFINED) // for float
            continue;
        
        //if (y == GRAPH_VALUE_UNDEFINED) // for double
        if (y >= GRAPH_VALUE_UNDEFINED) // for float
            continue;
        
#if CURVE_DEBUG
        numPointsDrawn++;
#endif
        
        if (i == 0)
        {
            x0 = x;
            
            nvgMoveTo(mVg, x0 - OFFSET, 0 - OFFSET);
            nvgLineTo(mVg, x - OFFSET, y);
        }
        
        nvgLineTo(mVg, x, y);
        
        if (i >= curve->mXValues.GetSize() - 1)
            // Close
        {
            nvgLineTo(mVg, x + OFFSET, y);
            nvgLineTo(mVg, x + OFFSET, 0 - OFFSET);
            
            nvgClosePath(mVg);
        }
    }
    
    nvgFillColor(mVg, nvgRGBA(sFillColor[0], sFillColor[1], sFillColor[2], sFillColor[3]));
	nvgFill(mVg);
    
    nvgStrokeColor(mVg, nvgRGBA(sStrokeColor[0], sStrokeColor[1], sStrokeColor[2], sStrokeColor[3]));
    nvgStrokeWidth(mVg, lineWidth);
    nvgStroke(mVg);

    nvgRestore(mVg);
    
    mDirty = true;

    // ??
#if DIRTY_OPTIM
    mMyDirty = true;
#endif

}
#endif

#if FILL_CURVE_HACK
// Due to a bug, we render only convex polygons when filling curves
// So we separate in rectangles
void
GraphControl10::DrawFillCurve(GraphCurve4 *curve)
{
#if FIX_UNDEFINED_CURVES
    bool curveUndefined = IsCurveUndefined(curve->mXValues, curve->mYValues, 2);
    if (curveUndefined)
        return;
#endif

#if CURVE_DEBUG
    int numPointsDrawn = 0;
#endif
    
    IRECT *rect = GetRECT();
    int height = rect->H();
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
    
    //int sStrokeColor[4] = { (int)(curve->mColor[0]*255), (int)(curve->mColor[1]*255),
    //    (int)(curve->mColor[2]*255), (int)(curve->mAlpha*255) };
    //SWAP_COLOR(sStrokeColor);
    
    BL_GUI_FLOAT prevX = -1.0;
    BL_GUI_FLOAT prevY = originY; //0.0;
    
#if !FIX_CURVE_FILL_LAST_COLUMN
    for (int i = 0; i < curve->mXValues.GetSize() - 1; i ++)
#else
    for (int i = 0; i < curve->mXValues.GetSize(); i++)
#endif
    {
        BL_GUI_FLOAT x = curve->mXValues.Get()[i];
        
        //if (x == GRAPH_VALUE_UNDEFINED) // for double
        if (x >= GRAPH_VALUE_UNDEFINED) // for float
            continue;
        
        BL_GUI_FLOAT y = curve->mYValues.Get()[i];
        
        //if (y == GRAPH_VALUE_UNDEFINED) // for double
        if (y >= GRAPH_VALUE_UNDEFINED) // for float
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
        
        nvgBeginPath(mVg);
        
        nvgMoveTo(mVg, prevX, prevY);
        nvgLineTo(mVg, x, y);
        
        nvgLineTo(mVg, x, originY/*0*/);
        nvgLineTo(mVg, prevX, originY/*0*/);
        nvgLineTo(mVg, prevX, prevY);
        
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
        //int sFillColorUp[4] = { (int)(curve->mColor[0]*255), (int)(curve->mColor[1]*255),
        //    (int)(curve->mColor[2]*255), (int)(curve->mFillAlphaUp*255) };
        
        int sFillColorUp[4] = { (int)(color[0]*255), (int)(color[1]*255),
                                (int)(color[2]*255), (int)(curve->mFillAlphaUp*255) };
        
        SWAP_COLOR(sFillColorUp);
        
        int height = this->mRECT.H();
        
        BL_GUI_FLOAT prevX = -1.0;
        BL_GUI_FLOAT prevY = originY; //0.0;
#if !FIX_CURVE_FILL_LAST_COLUMN
        for (int i = 0; i < curve->mXValues.GetSize() - 1; i ++)
#else
        for (int i = 0; i < curve->mXValues.GetSize(); i ++)
#endif
        {
            BL_GUI_FLOAT x = curve->mXValues.Get()[i];
            
            //if (x == GRAPH_VALUE_UNDEFINED) // for double
            if (x >= GRAPH_VALUE_UNDEFINED) // for float
                continue;
            
            BL_GUI_FLOAT y = curve->mYValues.Get()[i];
            
            //if (y == GRAPH_VALUE_UNDEFINED) // for double
            if (y >= GRAPH_VALUE_UNDEFINED) // for float
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
            
            nvgBeginPath(mVg);
            
            nvgMoveTo(mVg, prevX, prevY);
            nvgLineTo(mVg, x, y);
            
            nvgLineTo(mVg, x, height);
            nvgLineTo(mVg, prevX, height);
            nvgLineTo(mVg, prevX, prevY);
            
            nvgClosePath(mVg);
            
            nvgFillColor(mVg, nvgRGBA(sFillColorUp[0], sFillColorUp[1], sFillColorUp[2], sFillColorUp[3]));
            nvgFill(mVg);
            
            prevX = x;
            prevY = y;
        }
    }
    
    nvgRestore(mVg);
    
    mDirty = true;
    
    // ??
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
    
#if CURVE_DEBUG
    fprintf(stderr, "GraphControl10::DrawFillCurve - num points: %d\n", numPointsDrawn);
#endif
}
#endif

void
GraphControl10::DrawLineCurveSVH(GraphCurve4 *curve)
{
    if (curve->mYValues.GetSize() == 0)
        return;
    
    BL_GUI_FLOAT val = curve->mYValues.Get()[0];
    
    //if (val == GRAPH_VALUE_UNDEFINED) // for double
    if (val >= GRAPH_VALUE_UNDEFINED) // for float
        return;
    
    int width = this->mRECT.W();
    
    nvgSave(mVg);
    nvgStrokeWidth(mVg, curve->mLineWidth);
    
    int sColor[4] = { (int)(curve->mColor[0]*255), (int)(curve->mColor[1]*255),
                      (int)(curve->mColor[2]*255), (int)(curve->mAlpha*255) };
    SWAP_COLOR(sColor);
    
    nvgStrokeColor(mVg, nvgRGBA(sColor[0], sColor[1], sColor[2], sColor[3]));
    
    nvgBeginPath(mVg);
    
    BL_GUI_FLOAT x0 = 0;
    BL_GUI_FLOAT x1 = width;
    
    nvgMoveTo(mVg, x0, val);
    nvgLineTo(mVg, x1, val);
    
    nvgStroke(mVg);
    nvgRestore(mVg);
    
    mDirty = true;
    
    // ??
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

void
GraphControl10::DrawFillCurveSVH(GraphCurve4 *curve)
{
    if (curve->mYValues.GetSize() == 0)
        return;
    
    BL_GUI_FLOAT val = curve->mYValues.Get()[0];
    
    //if (val == GRAPH_VALUE_UNDEFINED) // for double
    if (val >= GRAPH_VALUE_UNDEFINED) // float float
        return;
    
    int width = this->mRECT.W();
    
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
    
    nvgMoveTo(mVg, x0, y0);
    nvgLineTo(mVg, x0, y1);
    nvgLineTo(mVg, x1, y1);
    nvgLineTo(mVg, x1, y0);
    
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
        
        nvgMoveTo(mVg, x0, y0);
        nvgLineTo(mVg, x0, y1);
        nvgLineTo(mVg, x1, y1);
        nvgLineTo(mVg, x1, y0);
        
        nvgClosePath(mVg);
        
        nvgFillColor(mVg, nvgRGBA(sFillColorUp[0], sFillColorUp[1],
                                  sFillColorUp[2], sFillColorUp[3]));
        nvgFill(mVg);
    }
    
    
    nvgRestore(mVg);
    
    mDirty = true;
    
    // ??
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

void
GraphControl10::DrawLineCurveSVV(GraphCurve4 *curve)
{
    // Finally, take the Y value
    // We will have to care about the curve Y scale !
    if (curve->mYValues.GetSize() == 0)
        return;
    
    int width = this->mRECT.W();
    int height = this->mRECT.H();
    
    BL_GUI_FLOAT val = curve->mYValues.Get()[0];
    
    //if (val == GRAPH_VALUE_UNDEFINED) // for double
    if (val >= GRAPH_VALUE_UNDEFINED) // for float
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
    
    nvgMoveTo(mVg, x, y0);
    nvgLineTo(mVg, x, y1);
    
    nvgStroke(mVg);
    nvgRestore(mVg);
    
    mDirty = true;
    
    // ??
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

// Fill right
// (only, for the moment)
void
GraphControl10::DrawFillCurveSVV(GraphCurve4 *curve)
{
    // Finally, take the Y value
    // We will have to care about the curve Y scale !
    if (curve->mYValues.GetSize() == 0)
        return;
    
    int width = this->mRECT.W();
    int height = this->mRECT.H();
    
    BL_GUI_FLOAT val = curve->mYValues.Get()[0];
    
    //if (val == GRAPH_VALUE_UNDEFINED) // for double
    if (val >= GRAPH_VALUE_UNDEFINED) // for float
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
    
    nvgMoveTo(mVg, x0, y0);
    nvgLineTo(mVg, x0, y1);
    nvgLineTo(mVg, x1, y1);
    nvgLineTo(mVg, x1, y0);
    
    nvgClosePath(mVg);
    
    nvgFillColor(mVg, nvgRGBA(sColor[0], sColor[1], sColor[2], sColor[3]));
	nvgFill(mVg);
    
    nvgStroke(mVg);
    nvgRestore(mVg);
    
    mDirty = true;
    
    // ??
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

#if 0 // OLD ? => TODO: check and remove
// TEST: draw lines instead of points
void
GraphControl10::DrawPointCurveLines(GraphCurve4 *curve)
{
#if FIX_UNDEFINED_CURVES
    bool curveUndefined = IsCurveUndefined(curve->mXValues, curve->mYValues, 2);
    if (curveUndefined)
        return;
#endif

    nvgSave(mVg);
    
    SetCurveDrawStyle(curve);
    
    int width = this->mRECT.W();
    BL_GUI_FLOAT pointSize = curve->mPointSize; ///width;
    
    nvgBeginPath(mVg);
    
    bool firstPoint = true;
    for (int i = 0; i < curve->mXValues.GetSize(); i ++)
    {
        BL_GUI_FLOAT x = curve->mXValues.Get()[i];
        
        //if (x == GRAPH_VALUE_UNDEFINED) // for double
        if (x >= GRAPH_VALUE_UNDEFINED) // for float
            continue;
        
        BL_GUI_FLOAT y = curve->mYValues.Get()[i];
        //if (y == GRAPH_VALUE_UNDEFINED) // for double
        if (y >= GRAPH_VALUE_UNDEFINED) // for float
            continue;
        
        if (firstPoint)
        {
            nvgMoveTo(mVg, x, y);
        
            firstPoint = false;
        }
        
        nvgLineTo(mVg, x, y);
    }
    
    nvgStroke(mVg);
    
    nvgRestore(mVg);
    
    mDirty = true;
    
    // ??
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}
#endif

#if 0 // Good but not optimal: use strokes
// Optimized !
void
GraphControl10::DrawPointCurve(GraphCurve4 *curve)
{
#if FIX_UNDEFINED_CURVES
    bool curveUndefined = IsCurveUndefined(curve->mXValues, curve->mYValues, 1);
    if (curveUndefined)
        return;
#endif

#define FILL_RECTS 1
    
    nvgSave(mVg);
    
    SetCurveDrawStyle(curve);
    
    BL_GUI_FLOAT pointSize = curve->mPointSize;
    
#if !FILL_RECTS
    nvgBeginPath(mVg);
#endif
    
    for (int i = 0; i < curve->mXValues.GetSize(); i ++)
    {
        BL_GUI_FLOAT x = curve->mXValues.Get()[i];
        
        //if (x == GRAPH_VALUE_UNDEFINED) // for double
        if (x >= GRAPH_VALUE_UNDEFINED) // for float
            continue;
        
        BL_GUI_FLOAT y = curve->mYValues.Get()[i];
        //if (y == GRAPH_VALUE_UNDEFINED) // for double
        if (y >= GRAPH_VALUE_UNDEFINED) // floar float
            continue;
    
        if (curve->mWeights.GetSize() == curve->mXValues.GetSize())
        {
            BL_GUI_FLOAT weight = curve->mWeights.Get()[i];
            SetCurveDrawStyleWeight(curve, weight);
        }
      
        // FIX: when points are very big, they are not centered
        x -= pointSize/2.0;
        y -= pointSize/2.0;
        
#if FILL_RECTS
        nvgBeginPath(mVg);
#endif
        nvgRect(mVg, x, y, pointSize, pointSize);
        
#if FILL_RECTS
        nvgFill(mVg);
#endif
    }
    
#if !FILL_RECTS
    nvgStroke(mVg);
#endif
    
    nvgRestore(mVg);
    
    mDirty = true;
    
    // ??
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}
#endif

// Optimized !
// Optimization2: nvgQuad
void
GraphControl10::DrawPointCurve(GraphCurve4 *curve)
{
#if FIX_UNDEFINED_CURVES
    bool curveUndefined = IsCurveUndefined(curve->mXValues, curve->mYValues, 1);
    if (curveUndefined)
        return;
#endif

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
        
        //if (x == GRAPH_VALUE_UNDEFINED) // for double
        if (x >= GRAPH_VALUE_UNDEFINED) // for float
            continue;
        
        BL_GUI_FLOAT y = curve->mYValues.Get()[i];
        //if (y == GRAPH_VALUE_UNDEFINED) // for double
        if (y >= GRAPH_VALUE_UNDEFINED) // for float
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
        
        BL_GUI_FLOAT size = pointSize;
        BL_GUI_FLOAT corners[4][2] = { { x - size/2.0, y - size/2.0 },
                                 { x + size/2.0, y - size/2.0 },
                                 { x + size/2.0, y + size/2.0 },
                                 { x - size/2.0, y + size/2.0 } };
        
        if (mWhitePixImg < 0)
        {
            unsigned char white[4] = { 255, 255, 255, 255 };
            mWhitePixImg = nvgCreateImageRGBA(mVg,
                                              1, 1,
                                              NVG_IMAGE_NEAREST,
                                              white);
        }
        
#if BL_GUI_TYPE_FLOAT
        nvgQuad(mVg, corners, mWhitePixImg);
#else
        float cornersf[4][2];
        for (int j = 0; j < 4; j++)
        {
            for (int k = 0; k < 2; k++)
            {
                cornersf[j][k] = corners[j][k];
            }
        }
        
        nvgQuad(mVg, cornersf, mWhitePixImg);
#endif
    }
    
#if GRAPH_OPTIM_GL_FIX_OVER_BLEND
    if (curve->mPointOverlay)
        nvgGlobalCompositeOperation(mVg, NVG_SOURCE_OVER);
#endif
    
    nvgRestore(mVg);
    
    mDirty = true;
    
    // ??
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

#if 0
void
GraphControl10::DrawPointCurveWeights(GraphCurve4 *curve)
{
#if FIX_UNDEFINED_CURVES
    bool curveUndefined = IsCurveUndefined(curve->mXValues, curve->mYValues, 1);
    if (curveUndefined)
        return;
#endif

    nvgSave(mVg);
    
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
        
        //if (x == GRAPH_VALUE_UNDEFINED) // for double
        if (x >= GRAPH_VALUE_UNDEFINED) // for float
            continue;
        
        BL_GUI_FLOAT y = curve->mYValues.Get()[i];
        //if (y == GRAPH_VALUE_UNDEFINED) // for double
        if (y >= GRAPH_VALUE_UNDEFINED) // for float
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
        
        BL_GUI_FLOAT size = pointSize;
        BL_GUI_FLOAT corners[4][2] = { { x - size/2.0, y - size/2.0 },
            { x + size/2.0, y - size/2.0 },
            { x + size/2.0, y + size/2.0 },
            { x - size/2.0, y + size/2.0 } };
        
        if (mWhitePixImg < 0)
        {
            unsigned char white[4] = { 255, 255, 255, 255 };
            mWhitePixImg = nvgCreateImageRGBA(mVg,
                                              1, 1,
                                              NVG_IMAGE_NEAREST,
                                              white);
        }
        
        //SetCurveDrawStyle(curve);
        
#if BL_GUI_TYPE_FLOAT
        nvgQuad(mVg, corners, mWhitePixImg);
#else
        float cornersf[4][2];
        for (int j = 0; j < 4; j++)
        {
            for (int k = 0; k < 2; k++)
            {
                cornersf[j][k] = corners[j][k];
            }
        }
        
        nvgQuad(mVg, cornersf, mWhitePixImg);
#endif
    }
    
#if GRAPH_OPTIM_GL_FIX_OVER_BLEND
    if (curve->mPointOverlay)
        nvgGlobalCompositeOperation(mVg, NVG_SOURCE_OVER);
#endif
    
    nvgRestore(mVg);
    
    mDirty = true;
    
    // ??
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}
#endif

void
GraphControl10::DrawPointCurveOptimSameColor(GraphCurve4 *curve)
{
#if FIX_UNDEFINED_CURVES
    bool curveUndefined = IsCurveUndefined(curve->mXValues, curve->mYValues, 1);
    if (curveUndefined)
        return;
#endif

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
        if (x >= GRAPH_VALUE_UNDEFINED)
            break;;
        
        BL_GUI_FLOAT y = curve->mYValues.Get()[i];
        if (y >= GRAPH_VALUE_UNDEFINED)
            break;
        
        numCenters++;
    }
    
    if (mTmpDrawPointsCenters.GetSize() != numCenters*2)
        mTmpDrawPointsCenters.Resize(numCenters*2);
    /*BL_GUI_FLOAT*/ float *centersBuf = mTmpDrawPointsCenters.Get();
    
    //
    BL_GUI_FLOAT pointSize = curve->mPointSize;
    
    for (int i = 0; i < numCenters; i++)
    {
        BL_GUI_FLOAT x = curve->mXValues.Get()[i];
        BL_GUI_FLOAT y = curve->mYValues.Get()[i];
        
        /*if (curve->mWeights.GetSize() == curve->mXValues.GetSize())
        {
            BL_GUI_FLOAT weight = curve->mWeights.Get()[i];
            
            SetCurveDrawStyleWeight(curve, weight);
        }*/
        
        if (!mDisablePointOffsetHack)
        {
            // FIX: when points are very big, they are not centered
            x -= pointSize/2.0;
            y -= pointSize/2.0;
        }
        
        centersBuf[i*2] = x;
        centersBuf[i*2 + 1] = y;
    }
    
#if GRAPH_OPTIM_GL_FIX_OVER_BLEND
    if (curve->mPointOverlay)
        nvgGlobalCompositeOperation(mVg, NVG_SOURCE_OVER);
#endif
    
    nvgQuads(mVg, centersBuf, numCenters, pointSize, mWhitePixImg);
    
    nvgRestore(mVg);
    
    mDirty = true;
    
    // ??
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

// TEST (to display lines instead of points)
void
GraphControl10::DrawPointCurveLinesPolar(GraphCurve4 *curve)
{
#if FIX_UNDEFINED_CURVES
    bool curveUndefined = IsCurveUndefined(curve->mXValues, curve->mYValues, 2);
    if (curveUndefined)
        return;
#endif

    nvgSave(mVg);
    
    SetCurveDrawStyle(curve);
    
    BL_GUI_FLOAT pointSize = curve->mPointSize;
    nvgStrokeWidth(mVg, pointSize);
    
    int width = this->mRECT.W();
    
#if !FILL_RECTS
    nvgBeginPath(mVg);
#endif
    
#if OPTIM_LINES_POLAR
    nvgBeginPath(mVg);
#endif
    
    for (int i = 0; i < curve->mXValues.GetSize(); i ++)
    {
        BL_GUI_FLOAT x = curve->mXValues.Get()[i];
        
        //if (x == GRAPH_VALUE_UNDEFINED) // for double
        if (x >= GRAPH_VALUE_UNDEFINED) // for float
            continue;
        
        BL_GUI_FLOAT y = curve->mYValues.Get()[i];
        //if (y == GRAPH_VALUE_UNDEFINED) // for double
        if (y >= GRAPH_VALUE_UNDEFINED) // for float
            continue;
        
#if !OPTIM_LINES_POLAR
        nvgBeginPath(mVg);
        nvgMoveTo(mVg, width/2.0, 0.0);
        nvgLineTo(mVg, x, y);
        nvgStroke(mVg);
#else
        nvgMoveTo(mVg, width/2.0, 0.0);
        nvgLineTo(mVg, x, y);
#endif
    }
    
#if OPTIM_LINES_POLAR
    nvgStroke(mVg);
#endif
    
    nvgRestore(mVg);
    
    mDirty = true;
    
    // ??
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

void
GraphControl10::DrawPointCurveLinesPolarWeights(GraphCurve4 *curve)
{
#if FIX_UNDEFINED_CURVES
    bool curveUndefined = IsCurveUndefined(curve->mXValues, curve->mYValues, 2);
    if (curveUndefined)
        return;
#endif

    nvgSave(mVg);
    
    SetCurveDrawStyle(curve);
    
    BL_GUI_FLOAT pointSize = curve->mPointSize;
    nvgStrokeWidth(mVg, pointSize);
    
    int width = this->mRECT.W();
    
    for (int i = 0; i < curve->mXValues.GetSize(); i ++)
    {        
        BL_GUI_FLOAT x = curve->mXValues.Get()[i];
        
        //if (x == GRAPH_VALUE_UNDEFINED) // for double
        if (x >= GRAPH_VALUE_UNDEFINED) // for float
            continue;
        
        BL_GUI_FLOAT y = curve->mYValues.Get()[i];
        //if (y == GRAPH_VALUE_UNDEFINED) // for double
        if (y >= GRAPH_VALUE_UNDEFINED) // for float
            continue;
        
        if (curve->mWeights.GetSize() == curve->mXValues.GetSize())
        {
            BL_GUI_FLOAT weight = curve->mWeights.Get()[i];
            
            SetCurveDrawStyleWeight(curve, weight);
        }
        
/*#if !FILL_RECTS
        nvgBeginPath(mVg);
#endif
        
#if OPTIM_LINES_POLAR
        nvgBeginPath(mVg);
#endif*/
        
        // NEW
        nvgBeginPath(mVg);
        
#if !OPTIM_LINES_POLAR
        nvgBeginPath(mVg);
        nvgMoveTo(mVg, width/2.0, 0.0);
        nvgLineTo(mVg, x, y);
        nvgStroke(mVg);
#else
        nvgMoveTo(mVg, width/2.0, 0.0);
        nvgLineTo(mVg, x, y);
#endif
        
#if OPTIM_LINES_POLAR
        nvgStroke(mVg);
#endif
    }
    
    nvgRestore(mVg);
    
    mDirty = true;
    
    // ??
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

void
GraphControl10::DrawPointCurveLinesPolarFill(GraphCurve4 *curve)
{
#if FIX_UNDEFINED_CURVES
    bool curveUndefined = IsCurveUndefined(curve->mXValues, curve->mYValues, 2);
    if (curveUndefined)
        return;
#endif

    nvgSave(mVg);
    
    SetCurveDrawStyle(curve);
    
    BL_GUI_FLOAT pointSize = curve->mPointSize;
    nvgStrokeWidth(mVg, pointSize);
    
    int width = this->mRECT.W();
        
    int lastValidIdx = -1;
    for (int i = 0; i < curve->mXValues.GetSize(); i++)
    {
        BL_GUI_FLOAT x = curve->mXValues.Get()[i];
        
        //if (x == GRAPH_VALUE_UNDEFINED) // for double
        if (x >= GRAPH_VALUE_UNDEFINED) // for float
            continue;
        
        BL_GUI_FLOAT y = curve->mYValues.Get()[i];
        //if (y == GRAPH_VALUE_UNDEFINED) // for double
        if (y >= GRAPH_VALUE_UNDEFINED) // for float
            continue;
        
        if (lastValidIdx == -1)
        {
            lastValidIdx = i;
            
            continue;
        }
        
        BL_GUI_FLOAT lastX = curve->mXValues.Get()[lastValidIdx];
        BL_GUI_FLOAT lastY = curve->mYValues.Get()[lastValidIdx];
        
        // If not 0, performance drown
#define EPS 0.0 //2.0
        
        nvgBeginPath(mVg);
        nvgMoveTo(mVg, width/2.0, 0.0);
        nvgLineTo(mVg, lastX + EPS, lastY);
        nvgLineTo(mVg, x, y);
        nvgLineTo(mVg, width/2.0, 0.0); //
        nvgClosePath(mVg);
        
        nvgFill(mVg);
        
        //nvgStroke(mVg);
        
        lastValidIdx = i;
    }
    
    nvgRestore(mVg);
    
    mDirty = true;
    
    // ??
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

void
GraphControl10::DrawPointCurveLines(GraphCurve4 *curve)
{
#if FIX_UNDEFINED_CURVES
    bool curveUndefined = IsCurveUndefined(curve->mXValues, curve->mYValues, 2);
    if (curveUndefined)
        return;
#endif

    nvgSave(mVg);
    
    SetCurveDrawStyle(curve);
    
    BL_GUI_FLOAT pointSize = curve->mPointSize;
    nvgStrokeWidth(mVg, pointSize);
    
    //int width = this->mRECT.W();
    
//#if !FILL_RECTS
//    nvgBeginPath(mVg);
//#endif
  
    if (curve->mXValues.GetSize() == 0)
        return;
    
    nvgBeginPath(mVg);
    nvgMoveTo(mVg, curve->mXValues.Get()[0], curve->mYValues.Get()[0]);
    
    for (int i = 0; i < curve->mXValues.GetSize(); i ++)
    {
        BL_GUI_FLOAT x = curve->mXValues.Get()[i];
        
        //if (x == GRAPH_VALUE_UNDEFINED) // for double
        if (x >= GRAPH_VALUE_UNDEFINED) // for float
            continue;
        
        BL_GUI_FLOAT y = curve->mYValues.Get()[i];
        //if (y == GRAPH_VALUE_UNDEFINED) // for double
        if (y >= GRAPH_VALUE_UNDEFINED) // for float
            continue;
        
        nvgLineTo(mVg, x, y);
    }
    
    nvgStroke(mVg);
    
    nvgRestore(mVg);
    
    mDirty = true;
    
    // ??
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

void
GraphControl10::DrawPointCurveLinesWeights(GraphCurve4 *curve)
{
#if FIX_UNDEFINED_CURVES
    bool curveUndefined = IsCurveUndefined(curve->mXValues, curve->mYValues, 2);
    if (curveUndefined)
        return;
#endif

    nvgSave(mVg);
    
    if (curve->mPointOverlay)
        nvgGlobalCompositeOperation(mVg, NVG_LIGHTER);
    
    SetCurveDrawStyle(curve);
    
    BL_GUI_FLOAT pointSize = curve->mPointSize;
    nvgStrokeWidth(mVg, pointSize);
    
    //int width = this->mRECT.W();
    
    //#if !FILL_RECTS
    //    nvgBeginPath(mVg);
    //#endif
    
    if (curve->mXValues.GetSize() == 0)
        return;
    
    BL_FLOAT prevX = curve->mXValues.Get()[0];
    BL_FLOAT prevY = curve->mYValues.Get()[0];
    
    for (int i = 0; i < curve->mXValues.GetSize(); i ++)
    {
        BL_GUI_FLOAT x = curve->mXValues.Get()[i];
        
        //if (x == GRAPH_VALUE_UNDEFINED) // for double
        if (x >= GRAPH_VALUE_UNDEFINED) // for float
            continue;
        
        BL_GUI_FLOAT y = curve->mYValues.Get()[i];
        //if (y == GRAPH_VALUE_UNDEFINED) // for double
        if (y >= GRAPH_VALUE_UNDEFINED) // for float
            continue;
    
        if (curve->mWeights.GetSize() == curve->mXValues.GetSize())
        {
            BL_GUI_FLOAT weight = curve->mWeights.Get()[i];
            
            SetCurveDrawStyleWeight(curve, weight);
        }
    
        nvgBeginPath(mVg);
        
        nvgMoveTo(mVg, prevX, prevY);
        nvgLineTo(mVg, x, y);
        
        nvgStroke(mVg);
        
        prevX = x;
        prevY = y;
    }
    
    nvgRestore(mVg);
    
    mDirty = true;
    
    // ??
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

// doesn't work well (for non convex polygons)
void
GraphControl10::DrawPointCurveLinesFill(GraphCurve4 *curve)
{
#if FIX_UNDEFINED_CURVES
    bool curveUndefined = IsCurveUndefined(curve->mXValues, curve->mYValues, 2);
    if (curveUndefined)
        return;
#endif

    nvgSave(mVg);
    
    SetCurveDrawStyle(curve);
    
    BL_GUI_FLOAT pointSize = curve->mPointSize;
    nvgStrokeWidth(mVg, pointSize);
    
    if (curve->mXValues.GetSize() == 0)
        return;
    
    nvgBeginPath(mVg);
    nvgMoveTo(mVg, curve->mXValues.Get()[0], curve->mYValues.Get()[0]);
    
    for (int i = 0; i < curve->mXValues.GetSize(); i++)
    {
        BL_GUI_FLOAT x = curve->mXValues.Get()[i];
        
        //if (x == GRAPH_VALUE_UNDEFINED) // for double
        if (x >= GRAPH_VALUE_UNDEFINED) // for float
            continue;
        
        BL_GUI_FLOAT y = curve->mYValues.Get()[i];
        //if (y == GRAPH_VALUE_UNDEFINED) // for double
        if (y >= GRAPH_VALUE_UNDEFINED) // for float
            continue;
        
        nvgLineTo(mVg, x, y);
    }
    
    nvgClosePath(mVg);
    nvgFill(mVg);
    nvgRestore(mVg);
    
    mDirty = true;
    
    // ??
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

void
GraphControl10::AutoAdjust()
{
    // First, compute the maximum value of all the curves
    BL_GUI_FLOAT max = -1e16;
    for (int i = 0; i < mNumCurves; i++)
    {
        GraphCurve4 *curve = mCurves[i];
        
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
    
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

BL_GUI_FLOAT
GraphControl10::MillisToPoints(long long int elapsed, int sampleRate, int numSamplesPoint)
{
    BL_GUI_FLOAT numSamples = (((BL_GUI_FLOAT)elapsed)/1000.0)*sampleRate;
    
    BL_GUI_FLOAT numPoints = numSamples/numSamplesPoint;
    
    return numPoints;
}

void
GraphControl10::InitFont(const char *fontPath)
{
#ifndef WIN32
    nvgCreateFont(mVg, "font", fontPath);

	mFontInitialized = true;
#else //  On windows, resources are not external files 
	
	// Load the resource in memory, then create the nvg font

	IGraphicsWin *graphics = (IGraphicsWin *)GetGUI();
	if (graphics == NULL)
		return;

	HINSTANCE instance = graphics->GetHInstance();
	if (instance == NULL)
		return;

	HRSRC fontResource = ::FindResource(instance, MAKEINTRESOURCE(FONT_ID), RT_RCDATA);

	HMODULE module = instance;

	unsigned int fontResourceSize = ::SizeofResource(module, fontResource);
	HGLOBAL fontResourceData = ::LoadResource(module, fontResource);
	void* pBinaryData = ::LockResource(fontResourceData);

	if (pBinaryData == NULL)
		return;

	unsigned char* data = (unsigned char*)malloc(fontResourceSize);
	int ndata = fontResourceSize;
	memcpy(data, fontResourceData, ndata);

	nvgCreateFontMem(mVg, "font", data, ndata, 1);

	mFontInitialized = true;
#endif
}

void
GraphControl10::DrawText(BL_GUI_FLOAT x, BL_GUI_FLOAT y, BL_GUI_FLOAT fontSize,
                        const char *text, int color[4],
                        int halign, int valign, BL_GUI_FLOAT fontSizeCoeff)
{
    DrawText(mVg, x, y, fontSize, text, color, halign, valign, fontSizeCoeff);
}

void
GraphControl10::DrawGraph()
{
#if !DEBUG_REMOVE_DRAW_MUTEX
    // Already locked ?
    IPlugBase::IMutexLock lock(mPlug);
#endif
    
    // Update first, before displaying
    if (mSpectrogramDisplay != NULL)
    {
        bool updated = mSpectrogramDisplay->DoUpdateSpectrogram();
        
        if (updated)
        {
            mDirty = true;
            
#if DIRTY_OPTIM
            mMyDirty = true;
#endif
        }
    }
   
    //
    if (mSpectrogramDisplayScroll != NULL)
    {
        bool updated = mSpectrogramDisplayScroll->DoUpdateSpectrogram();
        if (updated)
        {
            mDirty = true;
            
#if DIRTY_OPTIM
            mMyDirty = true;
#endif
        }
    }
    
    // 2
    if (mSpectrogramDisplayScroll2 != NULL)
    {
        bool updated = mSpectrogramDisplayScroll2->DoUpdateSpectrogram();
        if (updated)
        {
            mDirty = true;
            
#if DIRTY_OPTIM
            mMyDirty = true;
#endif
        }
    }
    
    //
    if (mImageDisplay != NULL)
    {
        bool updated = mImageDisplay->DoUpdateImage();
        
        if (updated)
        {
            mDirty = true;
            
#if DIRTY_OPTIM
            mMyDirty = true;
#endif
        }
    }
    
    if ((mBgImage != NULL) && (mNvgBackgroundImage == 0))
        UpdateBackgroundImage();
    
    if ((mOverImage != NULL) && (mNvgOverlayImage == 0))
        UpdateOverlayImage();
    
	// On Windows, we need lazy evaluation, because we need HInstance and IGraphics
	if (!mFontInitialized)
		InitFont(NULL);
    
    if (mLiceFb == NULL)
    {
        // Take care of the following macro in Lice ! : DISABLE_LICE_EXTENSIONS
        
        mLiceFb = new LICE_GL_SysBitmap(0, 0);
        
        IRECT *r = GetRECT();
        
        int w = r->W();
        int h = r->H();
        mLiceFb->resize(w, h);
    }
    
    // Resize with lazy evaluation
    // (avoids managing FBO in the event thread)
    if (mNeedResizeGraph)
    {
        if (mLiceFb != NULL)
        {
            IRECT *r = GetRECT();
            
            int w = r->W();
            int h = r->H();
            mLiceFb->resize(w, h);
        }
        
        mNeedResizeGraph = false;
    }
    
    int width = this->mRECT.W();
    int height = this->mRECT.H();
    
    // Clear with Lice, because BitBlt needs it.
    unsigned char clearColor[4] = { (unsigned char)(mClearColor[0]*255), (unsigned char)(mClearColor[1]*255),
                                    (unsigned char)(mClearColor[2]*255), (unsigned char)(mClearColor[3]*255) };

    //unsigned char clearColor[4] = { 0, 255, 0, 0 };
    
    // Valgrind: error here ?
    
    // Clear the GL Bitmap and bind the FBO at the same time !
    LICE_Clear(mLiceFb, *((LICE_pixel *)&clearColor));
    
    // Set pixel ratio to 1.0
    // Otherwise, fonts will be blurry,
    // and lines too I guess...
    nvgBeginFrame(mVg, width, height, 1.0);
    
    DrawBackgroundImage();
    
    if (mAutoAdjustFlag)
    {
        AutoAdjust();
    }
    
#if !DEBUG_REMOVE_DRAW_MUTEX
    CustomDrawersPreDraw(&lock);
#else
    CustomDrawersPreDraw(NULL);
#endif
    
    // Draw spectrogram first
    //nvgSave(mVg);
    
    // New: set colormap only in the spectrogram state
//#if GLSL_COLORMAP
//    nvgSetColormap(mVg, mNvgColormapImage);
//#endif
    
    if (mSpectrogramDisplay != NULL)
        mSpectrogramDisplay->DrawSpectrogram(width, height);
    
    if (mSpectrogramDisplayScroll != NULL)
        mSpectrogramDisplayScroll->DrawSpectrogram(width, height);
    
    if (mSpectrogramDisplayScroll2 != NULL)
        mSpectrogramDisplayScroll2->DrawSpectrogram(width, height);

    //nvgRestore(mVg);
    
    DrawAxis(true);
    
    nvgSave(mVg);
    
    DrawCurves();
    
    nvgRestore(mVg);
    
    if (mSpectrogramDisplay != NULL)
    {
        // Display MiniView after curves
        // So we have the mini waveform, then the selection over
        mSpectrogramDisplay->DrawMiniView(width, height);
    }
    
    //
    if (mImageDisplay != NULL)
        mImageDisplay->DrawImage(width, height);
    
    DrawAxis(false);
    
    DisplayCurveDescriptions();
    
    //DrawSpectrogram();

#if !DEBUG_REMOVE_DRAW_MUTEX
    CustomDrawersPostDraw(&lock);
#else
    CustomDrawersPostDraw(NULL);
#endif
    
    DrawSeparatorY0();
   
    // FIX: Waves: audio clicks sometimes (END_FRAME_NO_MUTEX)
    //
    // NOTE: for Waves for example, just the call to nvgEndFrame()
    // takes about 70% of the CPU of DrawGraph()
    //
    // Release the mutex just before so:
    // - we will still protect accessors and member variables of DrawGraph()
    // - we will avoid locking the audio thread when calling long nvgEndFrame()
    
    DrawOverlayImage();
    
#if END_FRAME_NO_MUTEX
    
#if !DEBUG_REMOVE_DRAW_MUTEX
    lock.Destroy();
#endif
    
#endif
    
    nvgEndFrame(mVg);
}

BL_GUI_FLOAT
GraphControl10::ConvertX(GraphCurve4 *curve, BL_GUI_FLOAT val, BL_GUI_FLOAT width)
{
    BL_GUI_FLOAT x = val;
    //if (x != GRAPH_VALUE_UNDEFINED) // for double
    if (x < GRAPH_VALUE_UNDEFINED) // for float
    {
        if (curve->mXdBScale)
        {
            if (val > 0.0)
                // Avoid -INF values
                x = BLUtils::NormalizedYTodB(x, curve->mMinX, curve->mMaxX);
        }
        else
            x = (x - curve->mMinX)/(curve->mMaxX - curve->mMinX);
        
        //x = x * mAutoAdjustFactor * mXScaleFactor * width;
        x = x * width;
    }
    
    return x;
}

BL_GUI_FLOAT
GraphControl10::ConvertY(GraphCurve4 *curve, BL_GUI_FLOAT val, BL_GUI_FLOAT height)
{
    BL_GUI_FLOAT y = val;
    //if (y != GRAPH_VALUE_UNDEFINED) // for double
    if (y < GRAPH_VALUE_UNDEFINED) // for float
    {
        if (curve->mYdBScale)
        {
            if (val > 0.0)
            // Avoid -INF values
                y = BLUtils::NormalizedYTodB(y, curve->mMinY, curve->mMaxY);
        }
        else
            y = (y - curve->mMinY)/(curve->mMaxY - curve->mMinY);
        
        y = y * mAutoAdjustFactor * mYScaleFactor * height;
    }
    
    return y;
}

void
GraphControl10::SetCurveDrawStyle(GraphCurve4 *curve)
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
    
    nvgFillColor(mVg, nvgRGBA(sFillColor[0], sFillColor[1], sFillColor[2], sFillColor[3]));
    
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

void
GraphControl10::SetCurveDrawStyleWeight(GraphCurve4 *curve, BL_GUI_FLOAT weight)
{
    //nvgStrokeWidth(mVg, curve->mLineWidth);
    
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
    
    nvgFillColor(mVg, nvgRGBA(sFillColor[0], sFillColor[1], sFillColor[2], sFillColor[3]));
    
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

BL_GUI_FLOAT
GraphControl10::ConvertToBoundsX(BL_GUI_FLOAT t)
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
GraphControl10::ConvertToBoundsY(BL_GUI_FLOAT t)
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

void
GraphControl10::UpdateBackgroundImage()
{
    if (mBgImage == NULL)
        return;
    
    int w = mBgImage->getWidth();
    int h = mBgImage->getHeight();
    
    LICE_pixel *bits = mBgImage->getBits();
    
    mNvgBackgroundImage = nvgCreateImageRGBA(mVg,
                                             w, h,
                                             NVG_IMAGE_NEAREST, // 0
                                             (unsigned char *)bits);
    
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

void
GraphControl10::UpdateOverlayImage()
{
    if (mOverImage == NULL)
        return;
    
    int w = mOverImage->getWidth();
    int h = mOverImage->getHeight();
    
    LICE_pixel *bits = mOverImage->getBits();
    
    mNvgOverlayImage = nvgCreateImageRGBA(mVg,
                                          w, h,
                                          NVG_IMAGE_NEAREST, // 0
                                          (unsigned char *)bits);
    
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

void
GraphControl10::DrawBackgroundImage()
{
    if (mNvgBackgroundImage == 0)
        return;
    
    int width = this->mRECT.W();
    int height = this->mRECT.H();
    
    nvgSave(mVg);
                 
    // Flip upside down !
    BL_GUI_FLOAT bounds[4] = { 0.0, 1.0, 1.0, 0.0 };
    
    BL_GUI_FLOAT alpha = 1.0;
    NVGpaint fullImgPaint = nvgImagePattern(mVg,
                                            bounds[0]*width, bounds[1]*height,
                                            (bounds[2] - bounds[0])*width,
                                            (bounds[3] - bounds[1])*height,
                                            0.0, mNvgBackgroundImage,
                                            alpha);
    nvgBeginPath(mVg);
    
    // Corner (x, y) => bottom-left
    
#if !FIX_BG_IMAGE
    nvgRect(mVg,
            bounds[0]*width, bounds[1]*height,
            (bounds[2] - bounds[0])*width,
            (bounds[3] - bounds[1])*height);
#else
    nvgRect(mVg,
            bounds[0]*width, bounds[1]*height,
            (bounds[2] - bounds[0])*(width + 1),
            (bounds[3] - bounds[1])*(height + 1));
#endif
    
    nvgFillPaint(mVg, fullImgPaint);
    
    nvgFill(mVg);
    
    nvgRestore(mVg);
    
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

void
GraphControl10::DrawOverlayImage()
{
    if (mNvgOverlayImage == 0)
        return;
    
    int width = this->mRECT.W();
    int height = this->mRECT.H();
    
    nvgSave(mVg);
    
    // Flip upside down !
    BL_GUI_FLOAT bounds[4] = { 0.0, 1.0, 1.0, 0.0 };
    
    BL_GUI_FLOAT alpha = 1.0;
    NVGpaint fullImgPaint = nvgImagePattern(mVg,
                                            bounds[0]*width, bounds[1]*height,
                                            (bounds[2] - bounds[0])*width,
                                            (bounds[3] - bounds[1])*height,
                                            0.0, mNvgOverlayImage,
                                            alpha);
    nvgBeginPath(mVg);
    
    // Corner (x, y) => bottom-left
    
#if !FIX_BG_IMAGE
    nvgRect(mVg,
            bounds[0]*width, bounds[1]*height,
            (bounds[2] - bounds[0])*width,
            (bounds[3] - bounds[1])*height);
#else
    nvgRect(mVg,
            bounds[0]*width, bounds[1]*height,
            (bounds[2] - bounds[0])*(width + 1),
            (bounds[3] - bounds[1])*(height + 1));
#endif
    
    nvgFillPaint(mVg, fullImgPaint);
    nvgFill(mVg);
    
    nvgRestore(mVg);
    
#if DIRTY_OPTIM
    mMyDirty = true;
#endif
}

void
GraphControl10::InitNanoVg()
{
#if GHOST_OPTIM_GL
    if (mVg != NULL)
        return;
    
    if(glewInit() != GLEW_OK)
        return;
    
#endif
	
#if !GHOST_OPTIM_GL
    if (!mGLContext->IsInitialized())
        return;
    mGLContext->Enter();
#endif
    
    // NVG_NIKO_ANTIALIAS_SKIP_FRINGES :
    // For Spectrogram, colormap, and display one image over the other
    // => The borders of the image were black
    mVg = nvgCreateGL2(NVG_ANTIALIAS
                       | NVG_NIKO_ANTIALIAS_SKIP_FRINGES
                       | NVG_STENCIL_STROKES
                       //| NVG_DEBUG
                       );
	if (mVg == NULL)
    {
#if !GHOST_OPTIM_GL
        mGLContext->Leave();
#endif
        
		return;
    }
    
    InitFont(mFontPath.Get());

#if !GHOST_OPTIM_GL
    mGLContext->Leave();
#endif
}

void
GraphControl10::ExitNanoVg()
{
	if (mVg != NULL)
		nvgDeleteGL2(mVg);

	// Just in case
	// NOTE: added for Mixcraft crash fix
	mVg = NULL;
}

bool
GraphControl10::IsCurveUndefined(const WDL_TypedBuf<BL_GUI_FLOAT> &x,
                                 const WDL_TypedBuf<BL_GUI_FLOAT> &y,
                                 int minNumValues)
{
    if (x.GetSize() != y.GetSize())
        return true;
    
    int numDefinedValues = 0;
    for (int i = 0; i < x.GetSize(); i++)
    {
        BL_GUI_FLOAT x0 = x.Get()[i];
        if (x0 >= GRAPH_VALUE_UNDEFINED)
            continue;
        
        BL_GUI_FLOAT y0 = y.Get()[i];
        
        if (y0 >= GRAPH_VALUE_UNDEFINED)
            continue;
        
        numDefinedValues++;
        
        if (numDefinedValues >= minNumValues)
            // The curve is defined
            return false;
    }
    
    return true;
}
