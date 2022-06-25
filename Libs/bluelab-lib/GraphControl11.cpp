/* Copyright (C) 2022 Nicolas Dittlo <deadlab.plugins@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this software; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
 
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

// Plugin resource file
#include "resource.h"

#include "SpectrogramDisplay.h"
#include "SpectrogramDisplayScroll.h"
#include "SpectrogramDisplayScroll2.h"

#include <BLUtils.h>
#include <BLUtilsDecim.h>
#include <BLUtilsPlug.h>

#include <ImageDisplay.h>
#include <UpTime.h>

#include <GraphTimeAxis4.h>

#include <GraphSwapColor.h>

#include "GraphControl11.h"

//#define GRAPH_FONT "font"
#define GRAPH_FONT "font-bold"

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

GraphControl11::GraphControl11(Plugin *pPlug, IGraphics *graphics,
                               IRECT pR, int paramIdx,
                              int numCurves, int numCurveValues,
                              const char *fontPath)
	: IControl(pR, paramIdx),
	mFontInitialized(false),
	mAutoAdjustParamSmoother(1.0, 0.9),
	mNumCurves(numCurves),
	mNumCurveValues(numCurveValues),
	mHAxis(NULL),
	mVAxis(NULL),
    mPlug(pPlug)
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
    mMinX = 0.0;
	mMaxX = 1.0;
    
    SetClearColor(0, 0, 0, 255);
        
    mFontPath.Set(fontPath);

    mVg = NULL;
    
    mSpectrogramDisplay = NULL;
    mSpectrogramDisplayScroll = NULL;
    mSpectrogramDisplayScroll2 = NULL;
    
    mImageDisplay = NULL;
    
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
    mGraphics = NULL;
#endif

    mDataChanged = true;
    
#if PROFILE_GRAPH
    mDebugCount = 0;
#endif
    
    mGraphTimeAxis = NULL;
}

GraphControl11::~GraphControl11()
{
    WDL_MutexLock lock(&mMutex);
    
    for (int i = 0; i < mNumCurves; i++)
        delete mCurves[i];
    
    if (mHAxis != NULL)
        delete mHAxis;
    
    if (mVAxis != NULL)
        delete mVAxis;
    
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
    
#if USE_FBO && (defined IGRAPHICS_GL)
    if (mFBO != NULL)
    {
        //nvgDeleteFramebuffer(mFBO);
        
        ((IGraphicsNanoVG *)mGraphics)->DeleteFBO(mFBO);
    }
#endif
}

void
GraphControl11::SetEnabled(bool flag)
{
    WDL_MutexLock lock(&mMutex);
    
    mIsEnabled = flag;
}

// Same as Resize()
void
GraphControl11::SetNumCurveValues(int numCurveValues)
{
    WDL_MutexLock lock(&mMutex);
    
    mNumCurveValues = numCurveValues;
    
    for (int i = 0; i < mNumCurves; i++)
	{
		GraphCurve4 *curve = mCurves[i];
        
		curve->ResetNumValues(mNumCurveValues);
	}
 
    mDirty = true;
    mDataChanged = true;
}

void
GraphControl11::GetSize(int *width, int *height)
{
    *width = this->mRECT.W();
    *height = this->mRECT.H();
}

void
GraphControl11::Resize(int width, int height)
{
    WDL_MutexLock lock(&mMutex);
    
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
    
    ResetGfx();
    
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
    
    mDirty = true;
    mDataChanged = true;
}

void
GraphControl11::ResetGfx()
{
     WDL_MutexLock lock(&mMutex);
    
#if USE_FBO && (defined IGRAPHICS_GL)
    if (mFBO != NULL)
    {
        //nvgDeleteFramebuffer(mFBO);
        
        ((IGraphicsNanoVG *)mGraphics)->DeleteFBO(mFBO);
        
        mFBO = NULL;
    }
#endif
    
    if (mWhitePixImg >= 0)
    {
        nvgDeleteImage(mVg, mWhitePixImg);
    
        mWhitePixImg = 0;
    }
    
    if (mSpectrogramDisplay != NULL)
        mSpectrogramDisplay->ResetGfx();
    
    if (mSpectrogramDisplayScroll != NULL)
        mSpectrogramDisplayScroll->ResetGfx();
    
    if (mSpectrogramDisplayScroll2 != NULL)
        mSpectrogramDisplayScroll2->ResetGfx();
    
    mVg = NULL;
}

void
GraphControl11::RefreshGfx()
{
    WDL_MutexLock lock(&mMutex);
    
    if (mSpectrogramDisplay != NULL)
        mSpectrogramDisplay->RefreshGfx();
    
    if (mSpectrogramDisplayScroll != NULL)
        mSpectrogramDisplayScroll->RefreshGfx();
    
    if (mSpectrogramDisplayScroll2 != NULL)
        mSpectrogramDisplayScroll2->RefreshGfx();
}

void
GraphControl11::SetBounds(BL_GUI_FLOAT x0, BL_GUI_FLOAT y0,
                          BL_GUI_FLOAT x1, BL_GUI_FLOAT y1)
{
    WDL_MutexLock lock(&mMutex);
    
    mBounds[0] = x0;
    mBounds[1] = y0;
    mBounds[2] = x1;
    mBounds[3] = y1;
    
    mDirty = true;
    mDataChanged = true;
}

void
GraphControl11::Resize(int numCurveValues)
{
    WDL_MutexLock lock(&mMutex);
    
    mNumCurveValues = numCurveValues;
    
    for (int i = 0; i < mNumCurves; i++)
	{
		GraphCurve4 *curve = mCurves[i];
        
		curve->ResetNumValues(mNumCurveValues);
	}
    
    mDirty = true;
    mDataChanged = true;
}

int
GraphControl11::GetNumCurveValues()
{
    return mNumCurveValues;
}

// Added for StereoViz
void
GraphControl11::OnGUIIdle()
{
    WDL_MutexLock lock(&mMutex);
    
    for (int i = 0; i < mCustomControls.size(); i++)
    {
        GraphCustomControl *control = mCustomControls[i];
        
        control->OnGUIIdle();
    }
}

void
GraphControl11::SetSeparatorY0(BL_GUI_FLOAT lineWidth, int color[4])
{
    WDL_MutexLock lock(&mMutex);
    
    mSeparatorY0 = true;
    mSepY0LineWidth = lineWidth;
    
    for (int i = 0; i < 4; i++)
        mSepY0Color[i] = color[i];
    
    mDirty = true;
    mDataChanged = true;
}

void
GraphControl11::AddHAxis(char *data[][2], int numData, bool xDbScale,
                         int axisColor[4], int axisLabelColor[4],
                         BL_GUI_FLOAT offsetY,
                         int axisOverlayColor[4],
                         BL_GUI_FLOAT fontSizeCoeff,
                         int axisLinesOverlayColor[4])
{
    WDL_MutexLock lock(&mMutex);
    
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
    
    mDirty = true;
    mDataChanged = true;
}

void
GraphControl11::AddHAxis0(char *data[][2], int numData, bool xDbScale,
                          int axisColor[4], int axisLabelColor[4],
                          BL_GUI_FLOAT offsetY,
                          int axisOverlayColor[4],
                          BL_GUI_FLOAT fontSizeCoeff,
                          int axisLinesOverlayColor[4])
{
    WDL_MutexLock lock(&mMutex);
    
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
    
    mDirty = true;
    mDataChanged = true;
}

void
GraphControl11::ReplaceHAxis(char *data[][2], int numData)
{
    WDL_MutexLock lock(&mMutex);
    
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

    BL_GUI_FLOAT offset = 0.0;
    BL_GUI_FLOAT offsetX = 0.0;
    
    BL_GUI_FLOAT offsetY = 0.0;

    bool overlay = false;
    bool linesOverlay = false;
    
    int labelOverlayColor[4] = { 0, 0, 0, 0 };
    int linesOverlayColor[4] = { 0, 0, 0, 0 };
    
    bool xDbScale = false;

    bool alignTextRight;
    bool alignRight;
#endif
    
    BL_GUI_FLOAT fontSizeCoeff = mHAxis->mFontSizeCoeff;
    
    if (mHAxis != NULL)
    {
        for (int i = 0; i < 4; i++)
            axisColor[i] = mHAxis->mColor[i];
        
        for (int i = 0; i < 4; i++)
            axisLabelColor[i] = mHAxis->mLabelColor[i];

        offset = mHAxis->mOffset;
        offsetX = mHAxis->mOffsetX;
	
        offsetY = mHAxis->mOffsetY;

        overlay = mHAxis->mOverlay;
        linesOverlay = mHAxis->mLinesOverlay;
	
        for (int i = 0; i < 4; i++)
            labelOverlayColor[i] = mHAxis->mLabelOverlayColor[i];
        
        for (int i = 0; i < 4; i++)
            linesOverlayColor[i] = mHAxis->mLinesOverlayColor[i];
        
        xDbScale = mHAxis->mXdBScale;

        alignTextRight = mHAxis->mAlignTextRight;
        alignRight = mHAxis->mAlignRight;
	
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
#endif
    
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

    //
    mDirty = true;
    mDataChanged = true;
}

void
GraphControl11::RemoveHAxis()
{
    WDL_MutexLock lock(&mMutex);
    
    if (mHAxis != NULL)
        delete mHAxis;
    
    mHAxis = NULL;
}

void
GraphControl11::AddVAxis(char *data[][2], int numData, int axisColor[4],
                         int axisLabelColor[4], BL_GUI_FLOAT offset, BL_GUI_FLOAT offsetX,
                         int axisLabelOverlayColor[4], BL_GUI_FLOAT fontSizeCoeff,
                         bool alignTextRight, int axisLinesOverlayColor[4],
                         bool alignRight)
{
    WDL_MutexLock lock(&mMutex);
    
    mVAxis = new GraphAxis();
    mVAxis->mOverlay = false;
    mVAxis->mLinesOverlay = false;
    mVAxis->mOffset = offset;
    mVAxis->mOffsetX = offsetX;
    mVAxis->mOffsetY = 0.0;
    
    mVAxis->mFontSizeCoeff = fontSizeCoeff;
    
    mVAxis->mXdBScale = false;
    
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
    
    mDirty = true;
    mDataChanged = true;
}

void
GraphControl11::RemoveVAxis()
{
    WDL_MutexLock lock(&mMutex);
    
    if (mVAxis != NULL)
        delete mVAxis;
    
    mVAxis = NULL;
}

void
GraphControl11::ReplaceVAxis(char *data[][2], int numData)
{
    WDL_MutexLock lock(&mMutex);
    
    // Initialize the colors !
    int axisColor[4] = { 0, 0, 0, 0 };
    int axisLabelColor[4] = { 0, 0, 0, 0 };
    
    BL_GUI_FLOAT offset = 0.0;
    BL_GUI_FLOAT offsetX = 0.0;
    
    BL_GUI_FLOAT offsetY = 0.0;
    
    bool overlay = false;
    bool linesOverlay = false;
    
    int labelOverlayColor[4] = { 0, 0, 0, 0 };
    int linesOverlayColor[4] = { 0, 0, 0, 0 };
    
    bool xDbScale = false;
    
    bool alignTextRight;
    bool alignRight;

    //
    BL_GUI_FLOAT fontSizeCoeff = mVAxis->mFontSizeCoeff;
    
    if (mVAxis != NULL)
    {
        for (int i = 0; i < 4; i++)
            axisColor[i] = mVAxis->mColor[i];
        
        for (int i = 0; i < 4; i++)
            axisLabelColor[i] = mVAxis->mLabelColor[i];
        
        offset = mVAxis->mOffset;
        offsetX = mVAxis->mOffsetX;
        
        offsetY = mVAxis->mOffsetY;
        
        overlay = mVAxis->mOverlay;
        linesOverlay = mVAxis->mLinesOverlay;
        
        for (int i = 0; i < 4; i++)
            labelOverlayColor[i] = mVAxis->mLabelOverlayColor[i];
        
        for (int i = 0; i < 4; i++)
            linesOverlayColor[i] = mVAxis->mLinesOverlayColor[i];
        
        xDbScale = mVAxis->mXdBScale;
        
        alignTextRight = mVAxis->mAlignTextRight;
        alignRight = mVAxis->mAlignRight;
        
        delete mVAxis;
    }
    
    AddVAxis(data, numData, axisColor, axisLabelColor,
             offset, offsetX, labelOverlayColor);
    
    // Set correclty the saved colors
    // (Because AddHAxis() swaps the colors, and set them to the members.
    // So with the previous code, the colors are swapped twice).
    for (int i = 0; i < 4; i++)
        mVAxis->mColor[i] = axisColor[i];
    
    for (int i = 0; i < 4; i++)
        mVAxis->mLabelColor[i] = axisLabelColor[i];
    
    for (int i = 0; i < 4; i++)
        mVAxis->mLabelOverlayColor[i] = labelOverlayColor[i];
    
    for (int i = 0; i < 4; i++)
        mVAxis->mLinesOverlayColor[i] = linesOverlayColor[i];
    
    //
    mVAxis->mFontSizeCoeff = fontSizeCoeff;
    
    mVAxis->mOffset = offset;
    mVAxis->mOffsetX = offsetX;
    
    mVAxis->mOffsetY = offsetY;
    
    mVAxis->mOverlay = overlay;
    mVAxis->mLinesOverlay = linesOverlay;
    
    mVAxis->mXdBScale = xDbScale;
    
    mVAxis->mAlignTextRight = alignTextRight;
    mVAxis->mAlignRight = alignRight;
    
    //
    mDirty = true;
    mDataChanged = true;
}

void
GraphControl11::AddVAxis(char *data[][2], int numData,
                         int axisColor[4], int axisLabelColor[4],
                        bool dbFlag, BL_GUI_FLOAT minY, BL_GUI_FLOAT maxY,
                        BL_GUI_FLOAT offset, int axisOverlayColor[4],
                        BL_GUI_FLOAT fontSizeCoeff, bool alignTextRight,
                        int axisLinesOverlayColor[4],
                        bool alignRight)
{
    WDL_MutexLock lock(&mMutex);
    
    mVAxis = new GraphAxis();
    mVAxis->mOverlay = false;
    mVAxis->mLinesOverlay = false;
    mVAxis->mOffset = offset;
    mVAxis->mOffsetY = 0.0;
    
    mVAxis->mFontSizeCoeff = fontSizeCoeff;
    
    mVAxis->mXdBScale = false;
    
    mVAxis->mAlignTextRight = alignTextRight;
    mVAxis->mAlignRight = alignRight;
    
    AddAxis(mVAxis, data, numData, axisColor, axisLabelColor, minY, maxY,
            axisOverlayColor, axisLinesOverlayColor);
    
    mDirty = true;
    mDataChanged = true;
}

void
GraphControl11::AddVAxis(char *data[][2], int numData,
                         int axisColor[4], int axisLabelColor[4],
                         bool dbFlag, BL_GUI_FLOAT minY, BL_GUI_FLOAT maxY,
                         BL_GUI_FLOAT offset, BL_GUI_FLOAT offsetX,
                         int axisOverlayColor[4],
                         BL_GUI_FLOAT fontSizeCoeff, bool alignTextRight,
                         int axisLinesOverlayColor[4],
                         bool alignRight)
{
    WDL_MutexLock lock(&mMutex);
    
    mVAxis = new GraphAxis();
    mVAxis->mOverlay = false;
    mVAxis->mLinesOverlay = false;
    mVAxis->mOffset = offset;
    mVAxis->mOffsetX = offsetX;
    mVAxis->mOffsetY = 0.0;
    
    mVAxis->mFontSizeCoeff = fontSizeCoeff;
    
    mVAxis->mXdBScale = false;
    
    mVAxis->mAlignTextRight = alignTextRight;
    mVAxis->mAlignRight = alignRight;
    
    AddAxis(mVAxis, data, numData, axisColor, axisLabelColor, minY, maxY,
            axisOverlayColor, axisLinesOverlayColor);
    
    mDirty = true;
    mDataChanged = true;
}

void
GraphControl11::SetXScale(bool dbFlag, BL_GUI_FLOAT minX, BL_GUI_FLOAT maxX)
{
    WDL_MutexLock lock(&mMutex);
    
    mXdBScale = dbFlag;
    
    mMinX = minX;
    mMaxX = maxX;
    
    mDirty = true;
    mDataChanged = true;
}

void
GraphControl11::SetAutoAdjust(bool flag, BL_GUI_FLOAT smoothCoeff)
{
    WDL_MutexLock lock(&mMutex);
    
    mAutoAdjustFlag = flag;
    
    mAutoAdjustParamSmoother.SetSmoothCoeff(smoothCoeff);
    
    mDirty = true;
    mDataChanged = true;
}

void
GraphControl11::SetYScaleFactor(BL_GUI_FLOAT factor)
{
    WDL_MutexLock lock(&mMutex);
    
    mYScaleFactor = factor;
    
    mDirty = true;
    mDataChanged = true;
}

void
GraphControl11::SetClearColor(int r, int g, int b, int a)
{
    WDL_MutexLock lock(&mMutex);
    
    SET_COLOR_FROM_INT(mClearColor, r, g, b, a);
    
    mDirty = true;
    mDataChanged = true;
}

void
GraphControl11::SetCurveColor(int curveNum, int r, int g, int b)
{
    WDL_MutexLock lock(&mMutex);
    
    if (curveNum >= mNumCurves)
        return;
    
    SET_COLOR_FROM_INT(mCurves[curveNum]->mColor, r, g, b, 255);
    
    mDirty = true;
    mDataChanged = true;
}

void
GraphControl11::SetCurveAlpha(int curveNum, BL_GUI_FLOAT alpha)
{
    WDL_MutexLock lock(&mMutex);
    
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->mAlpha = alpha;
    
    mDirty = true;
    mDataChanged = true;
}

void
GraphControl11::SetCurveLineWidth(int curveNum, BL_GUI_FLOAT lineWidth)
{
    WDL_MutexLock lock(&mMutex);
    
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->mLineWidth = lineWidth;
    
    mDirty = true;
    mDataChanged = true;
}

void
GraphControl11::SetCurveBevel(int curveNum, bool bevelFlag)
{
    WDL_MutexLock lock(&mMutex);
    
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->mBevelFlag = bevelFlag;
    
    mDirty = true;
    mDataChanged = true;
}

void
GraphControl11::SetCurveSmooth(int curveNum, bool flag)
{
    WDL_MutexLock lock(&mMutex);
    
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->mDoSmooth = flag;
    
    mDirty = true;
    mDataChanged = true;
}

void
GraphControl11::SetCurveFill(int curveNum, bool flag, BL_GUI_FLOAT originY)
{
    WDL_MutexLock lock(&mMutex);
    
    if (curveNum >= mNumCurves)
        return;

    mCurves[curveNum]->mCurveFill = flag;
    mCurves[curveNum]->mCurveFillOriginY = originY;
    
    mDirty = true;
    mDataChanged = true;
}

void
GraphControl11::SetCurveFillColor(int curveNum, int r, int g, int b)
{
    WDL_MutexLock lock(&mMutex);
    
    if (curveNum >= mNumCurves)
        return;
    
    SET_COLOR_FROM_INT(mCurves[curveNum]->mFillColor, r, g, b, 255);
    mCurves[curveNum]->mFillColorSet = true;
    
    mDirty = true;
    mDataChanged = true;
}

void
GraphControl11::SetCurveFillAlpha(int curveNum, BL_GUI_FLOAT alpha)
{
    WDL_MutexLock lock(&mMutex);
    
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->mFillAlpha = alpha;
    
    mDirty = true;
    mDataChanged = true;
}

void
GraphControl11::SetCurveFillAlphaUp(int curveNum, BL_GUI_FLOAT alpha)
{
    WDL_MutexLock lock(&mMutex);
    
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->mFillAlphaUpFlag = true;
    mCurves[curveNum]->mFillAlphaUp = alpha;
    
    mDirty = true;
    mDataChanged = true;
}

void
GraphControl11::SetCurvePointSize(int curveNum, BL_GUI_FLOAT pointSize)
{
    WDL_MutexLock lock(&mMutex);
    
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->mPointSize = pointSize;
    
    mDirty = true;
    mDataChanged = true;
}

void
GraphControl11::SetCurvePointOverlay(int curveNum, bool flag)
{
    WDL_MutexLock lock(&mMutex);
    
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->mPointOverlay = flag;
    
    mDirty = true;
    mDataChanged = true;
}

void
GraphControl11::SetCurveWeightMultAlpha(int curveNum, bool flag)
{
    WDL_MutexLock lock(&mMutex);
    
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->mWeightMultAlpha = flag;
    
    mDirty = true;
    mDataChanged = true;
}

void
GraphControl11::SetCurveWeightTargetColor(int curveNum, int color[4])
{
    WDL_MutexLock lock(&mMutex);
    
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->SetWeightTargetColor(color);
    
    mDirty = true;
    mDataChanged = true;
}

void
GraphControl11::SetCurveXScale(int curveNum, bool dbFlag,
                               BL_GUI_FLOAT minX, BL_GUI_FLOAT maxX)
{
    WDL_MutexLock lock(&mMutex);
    
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->SetXScale(dbFlag, minX, maxX);
    
    mDirty = true;
    mDataChanged = true;
}

void
GraphControl11::SetCurvePointStyle(int curveNum, bool pointFlag,
                                   bool pointsAsLinesPolar,
                                   bool pointsAsLines)
{
    WDL_MutexLock lock(&mMutex);
    
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->mPointStyle = pointFlag;
    mCurves[curveNum]->mPointsAsLinesPolar = pointsAsLinesPolar;
    mCurves[curveNum]->mPointsAsLines = pointsAsLines;
    
    mDirty = true;
    mDataChanged = true;
}

void
GraphControl11::SetCurveValuesPoint(int curveNum,
                                   const WDL_TypedBuf<BL_GUI_FLOAT> &xValues,
                                   const WDL_TypedBuf<BL_GUI_FLOAT> &yValues)
{
    WDL_MutexLock lock(&mMutex);
    
    if (curveNum >= mNumCurves)
        return;
    
    GraphCurve4 *curve = mCurves[curveNum];
    curve->mPointStyle = true;
    
    curve->ClearValues();
    
    if (curveNum >= mNumCurves)
        return;
    
    int width = this->mRECT.W();
    int height = this->mRECT.H();
    
    for (int i = 0; i < xValues.GetSize(); i++)
    {
        BL_GUI_FLOAT x = xValues.Get()[i];
        
        if (i >= yValues.GetSize())
            // Avoids a crash
            continue;
        
        BL_GUI_FLOAT y = yValues.Get()[i];
        
#if !OPTIM_CONVERT_XY
        x = ConvertX(curve, x, width);
        y = ConvertY(curve, y, height);
#endif
        
        curve->mXValues.Get()[i] = x;
        curve->mYValues.Get()[i] = y;
    }
    
#if OPTIM_CONVERT_XY
    ConvertX(curve, &curve->mXValues, width);
    ConvertY(curve, &curve->mYValues, height);
#endif
    
    mDirty = true;
    mDataChanged = true;
}

void
GraphControl11::SetCurveValuesPointEx(int curveNum,
                                      const WDL_TypedBuf<BL_GUI_FLOAT> &xValues,
                                      const WDL_TypedBuf<BL_GUI_FLOAT> &yValues,
                                      bool singleScale, bool scaleX, bool centerFlag)
{
    WDL_MutexLock lock(&mMutex);
    
    if (curveNum >= mNumCurves)
        return;
    
    GraphCurve4 *curve = mCurves[curveNum];
    curve->mPointStyle = true;
    
    curve->ClearValues();
    
    if (curveNum >= mNumCurves)
        return;
    
    int width = this->mRECT.W();
    int height = this->mRECT.H();
    
    for (int i = 0; i < xValues.GetSize(); i++)
    {
        BL_GUI_FLOAT x = xValues.Get()[i];
            
        if (i >= yValues.GetSize())
            // Avoids a crash
            continue;
            
        BL_GUI_FLOAT y = yValues.Get()[i];
        
#if !OPTIM_CONVERT_XY
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
#endif
        
        curve->mXValues.Get()[i] = x;
        curve->mYValues.Get()[i] = y;
    }
    
#if OPTIM_CONVERT_XY
    if (!singleScale)
    {
        ConvertX(curve, &curve->mXValues, width);
        ConvertY(curve, &curve->mYValues, height);
    }
    else
    {
        if (scaleX)
        {
            ConvertX(curve, &curve->mXValues, width);
            ConvertY(curve, &curve->mYValues, width);
            
            // TODO: test this
            if (centerFlag)
            {
                int offset = (width - height)*0.5;
                for (int i = 0; i < curve->mXValues.GetSize(); i++)
                {
                    curve->mXValues.Get()[i] -= offset;
                }
            }
        }
        else
        {
            ConvertX(curve, &curve->mXValues, height);
            ConvertY(curve, &curve->mYValues, height);
            
            if (centerFlag)
            {
                int offset = (width - height)*0.5;
                for (int i = 0; i < curve->mXValues.GetSize(); i++)
                {
                    curve->mXValues.Get()[i] += offset;
                }
            }
        }
    }
#endif
    
    mDirty = true;
    mDataChanged = true;
}

void
GraphControl11::SetCurveValuesPointWeight(int curveNum,
                                         const WDL_TypedBuf<BL_GUI_FLOAT> &xValues,
                                         const WDL_TypedBuf<BL_GUI_FLOAT> &yValues,
                                         const WDL_TypedBuf<BL_GUI_FLOAT> &weights)
{
    WDL_MutexLock lock(&mMutex);
    
    if (curveNum >= mNumCurves)
        return;
    
    GraphCurve4 *curve = mCurves[curveNum];
    curve->mPointStyle = true;
    
    curve->ClearValues();
    
    if (curveNum >= mNumCurves)
        return;
    
    int width = this->mRECT.W();
    int height = this->mRECT.H();

    for (int i = 0; i < xValues.GetSize(); i++)
    {
        BL_GUI_FLOAT x = xValues.Get()[i];
        BL_GUI_FLOAT y = yValues.Get()[i];
        
#if !OPTIM_CONVERT_XY
        x = ConvertX(curve, x, width);
        y = ConvertY(curve, y, height);
#endif
        
        curve->mXValues.Get()[i] = x;
        curve->mYValues.Get()[i] = y;
    }
    
#if OPTIM_CONVERT_XY
    ConvertX(curve, &curve->mXValues, width);
    ConvertY(curve, &curve->mYValues, height);
#endif
    
    curve->mWeights = weights;
    
    mDirty = true;
    mDataChanged = true;
}

void
GraphControl11::SetCurveColorWeight(int curveNum,
                                   const WDL_TypedBuf<BL_GUI_FLOAT> &colorWeights)
{
    WDL_MutexLock lock(&mMutex);
    
    if (curveNum >= mNumCurves)
        return;
    
    GraphCurve4 *curve = mCurves[curveNum];
    
    curve->mWeights = colorWeights;
}

void
GraphControl11::SetCurveSingleValueH(int curveNum, bool flag)
{
    WDL_MutexLock lock(&mMutex);
    
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->mSingleValueH = flag;
    
    mDirty = true;
    mDataChanged = true;
}

void
GraphControl11::SetCurveSingleValueV(int curveNum, bool flag)
{
    WDL_MutexLock lock(&mMutex);
    
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->mSingleValueV = flag;
    
    mDirty = true;
    mDataChanged = true;
}

void
GraphControl11::SetCurveOptimSameColor(int curveNum, bool flag)
{
    WDL_MutexLock lock(&mMutex);
    
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->mOptimSameColor = flag;
    
    mDirty = true;
    mDataChanged = true;
}

void
GraphControl11::DrawText(NVGcontext *vg,
                         BL_GUI_FLOAT x, BL_GUI_FLOAT y,
                         BL_GUI_FLOAT graphWidth, BL_GUI_FLOAT graphHeight,
                         BL_GUI_FLOAT fontSize,
                         const char *text, int color[4],
                         int halign, int valign, BL_GUI_FLOAT fontSizeCoeff)
{
    // static method -> no mutex!
    
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
GraphControl11::SetSpectrogram(BLSpectrogram4 *spectro,
                               BL_GUI_FLOAT left, BL_GUI_FLOAT top, BL_GUI_FLOAT right,
                               BL_GUI_FLOAT bottom)
{
    WDL_MutexLock lock(&mMutex);
    
    // #bl-iplug2: set mVg every time, and not at the beginning
    // here, we have many chances that mVg is NULL.
    // And do this for spectrogramdisplayscroll, image display and so on...
    
    if (mSpectrogramDisplay != NULL)
        delete mSpectrogramDisplay;
    
    mSpectrogramDisplay = new SpectrogramDisplay(mVg);
    mSpectrogramDisplay->SetSpectrogram(spectro,
                                        left, top, right, bottom);
}

SpectrogramDisplay *
GraphControl11::GetSpectrogramDisplay()
{
    return mSpectrogramDisplay;
}

void
GraphControl11::SetSpectrogramScroll(BLSpectrogram4 *spectro,
                                     BL_GUI_FLOAT left, BL_GUI_FLOAT top, BL_GUI_FLOAT right, BL_GUI_FLOAT bottom)
{
    WDL_MutexLock lock(&mMutex);
    
    if (mSpectrogramDisplayScroll != NULL)
        delete mSpectrogramDisplayScroll;
    
    mSpectrogramDisplayScroll = new SpectrogramDisplayScroll(mPlug, mVg);
    mSpectrogramDisplayScroll->SetSpectrogram(spectro,
                                              left, top, right, bottom);
}

SpectrogramDisplayScroll *
GraphControl11::GetSpectrogramDisplayScroll()
{
    return mSpectrogramDisplayScroll;
}

void
GraphControl11::SetSpectrogramScroll2(BLSpectrogram4 *spectro,
                                     BL_GUI_FLOAT left, BL_GUI_FLOAT top, BL_GUI_FLOAT right, BL_GUI_FLOAT bottom)
{
    WDL_MutexLock lock(&mMutex);
    
    if (mSpectrogramDisplayScroll2 != NULL)
        delete mSpectrogramDisplayScroll2;
    
    mSpectrogramDisplayScroll2 = new SpectrogramDisplayScroll2(mPlug, mVg);
    mSpectrogramDisplayScroll2->SetSpectrogram(spectro,
                                              left, top, right, bottom);
}

SpectrogramDisplayScroll2 *
GraphControl11::GetSpectrogramDisplayScroll2()
{
    return mSpectrogramDisplayScroll2;
}

void
GraphControl11::UpdateSpectrogram(bool updateData, bool updateFullData)
{
    WDL_MutexLock lock(&mMutex);
    
    if (mSpectrogramDisplay != NULL)
        mSpectrogramDisplay->UpdateSpectrogram(updateData, updateFullData);
    
    if (mSpectrogramDisplayScroll != NULL)
        mSpectrogramDisplayScroll->UpdateSpectrogram(updateData);
    
    if (mSpectrogramDisplayScroll2 != NULL)
        mSpectrogramDisplayScroll2->UpdateSpectrogram(updateData);
    
    mDirty = true;
    mDataChanged = true;
}

void
GraphControl11::UpdateSpectrogramColormap(bool updateData)
{
    WDL_MutexLock lock(&mMutex);
    
    if (mSpectrogramDisplayScroll != NULL)
        mSpectrogramDisplayScroll->UpdateColormap(updateData);
    
    if (mSpectrogramDisplayScroll2 != NULL)
        mSpectrogramDisplayScroll2->UpdateColormap(updateData);
    
    mDirty = true;
    mDataChanged = true;
}

void
GraphControl11::AddCustomDrawer(GraphCustomDrawer *customDrawer)
{
    WDL_MutexLock lock(&mMutex);
    
    mCustomDrawers.push_back(customDrawer);
}

void
GraphControl11::CustomDrawersPreDraw()
{
    WDL_MutexLock lock(&mMutex);
    
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
GraphControl11::CustomDrawersPostDraw()
{
    WDL_MutexLock lock(&mMutex);
    
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
GraphControl11::DrawSeparatorY0()
{
    WDL_MutexLock lock(&mMutex);
    
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
GraphControl11::AddCustomControl(GraphCustomControl *customControl)
{
    WDL_MutexLock lock(&mMutex);
    
    mCustomControls.push_back(customControl);
}

void
GraphControl11::OnMouseDown(float x, float y, const IMouseMod &mod)
{
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
GraphControl11::OnMouseUp(float x, float y, const IMouseMod &mod)
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
GraphControl11::OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod &mod)
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

void/*bool*/
GraphControl11::OnMouseDblClick(float x, float y, const IMouseMod &mod)
{
    /*bool dblClickDone =*/ IControl::OnMouseDblClick(x, y, mod);
    
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
    
    //return true;
}

void
GraphControl11::OnMouseWheel(float x, float y, const IMouseMod &mod, float d)
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
GraphControl11::OnKeyDown(float x, float y, const IKeyPress& key)
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

void
GraphControl11::OnMouseOver(float x, float y, const IMouseMod &mod)
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
GraphControl11::OnMouseOut()
{
    IControl::OnMouseOut();
    
    for (int i = 0; i < mCustomControls.size(); i++)
    {
        GraphCustomControl *control = mCustomControls[i];
        control->OnMouseOut();
    }
}

void
GraphControl11::DBG_PrintCoords(int x, int y)
{
    WDL_MutexLock lock(&mMutex);
    
    int width = this->mRECT.W();
    int height = this->mRECT.H();
    
    BL_GUI_FLOAT tX = ((BL_GUI_FLOAT)x)/width;
    BL_GUI_FLOAT tY = ((BL_GUI_FLOAT)y)/height;
    
    tX = (tX - mBounds[0])/(mBounds[2] - mBounds[0]);
    tY = (tY - mBounds[1])/(mBounds[3] - mBounds[1]);
    
    fprintf(stderr, "(%g, %g)\n", tX, 1.0 - tY);
}

void
GraphControl11::SetBackgroundImage(IBitmap bmp)
{
    WDL_MutexLock lock(&mMutex);
    
    mBgImage = bmp;
    
    mDirty = true;
    mDataChanged = true;
}

void
GraphControl11::SetOverlayImage(IBitmap bmp)
{
    WDL_MutexLock lock(&mMutex);
    
    mOverlayImage = bmp;
    
    mDirty = true;
    mDataChanged = true;
}

void
GraphControl11::SetDisablePointOffsetHack(bool flag)
{
    WDL_MutexLock lock(&mMutex);
    
    mDisablePointOffsetHack = flag;
}

void
GraphControl11::SetRecreateWhiteImageHack(bool flag)
{
    WDL_MutexLock lock(&mMutex);
    
    mRecreateWhiteImageHack = flag;
}

void
GraphControl11::CreateImageDisplay(BL_GUI_FLOAT left, BL_GUI_FLOAT top,
                                   BL_GUI_FLOAT right, BL_GUI_FLOAT bottom,
                                   ImageDisplay::Mode mode)
{
    WDL_MutexLock lock(&mMutex);
    
    if (mImageDisplay != NULL)
        delete mImageDisplay;
    
    mImageDisplay = new ImageDisplay(mVg, mode);
    mImageDisplay->SetBounds(left, top, right, bottom);
}

ImageDisplay *
GraphControl11::GetImageDisplay()
{
    return mImageDisplay;
}

void
GraphControl11::SetGraphTimeAxis(GraphTimeAxis4 *timeAxis)
{
    mGraphTimeAxis = timeAxis;
}

void
GraphControl11::SetDataChanged()
{
    WDL_MutexLock lock(&mMutex);
    
    mDataChanged = true;
    
    if (mIsEnabled) // new
        mDirty = true;
}

void
GraphControl11::SetDirty(bool triggerAction, int valIdx)
{
    WDL_MutexLock lock(&mMutex);
    
    // Set dirty only if the graph is enabled.
    // (otherwise we have the risk to try to draw a graph that is disabled).
    if (mIsEnabled)
    {
        IControl::SetDirty(triggerAction, valIdx);
    }
}

void
GraphControl11::DisplayCurveDescriptions()
{
    WDL_MutexLock lock(&mMutex);
    
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
GraphControl11::AddAxis(GraphAxis *axis, char *data[][2], int numData, int axisColor[4],
                       int axisLabelColor[4],
                       BL_GUI_FLOAT minVal, BL_GUI_FLOAT maxVal, int axisLabelOverlayColor[4],
                       int axisLinesOverlayColor[4])
{
    WDL_MutexLock lock(&mMutex);
    
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
    
    mDirty = true;
    mDataChanged = true;
}

void
GraphControl11::SetCurveDescription(int curveNum, const char *description, int descrColor[4])
{
    WDL_MutexLock lock(&mMutex);
    
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->SetDescription(description, descrColor);
    
    mDirty = true;
    mDataChanged = true;
}

void
GraphControl11::SetCurveLimitToBounds(int curveNum, bool flag)
{
    WDL_MutexLock lock(&mMutex);
    
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->SetLimitToBounds(flag);
    
    mDirty = true;
    mDataChanged = true;
}

void
GraphControl11::ResetCurve(int curveNum, BL_GUI_FLOAT val)
{
    WDL_MutexLock lock(&mMutex);
    
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->FillAllXValues(mMinX, mMaxX);
    mCurves[curveNum]->FillAllYValues(val);
    
    mDirty = true;
    mDataChanged = true;
}

void
GraphControl11::SetCurveYScale(int curveNum, bool dbFlag,
                               BL_GUI_FLOAT minY, BL_GUI_FLOAT maxY)
{
    WDL_MutexLock lock(&mMutex);
    
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->SetYScale(dbFlag, minY, maxY);
    
    mDirty = true;
    mDataChanged = true;
}

void
GraphControl11::CurveFillAllValues(int curveNum, BL_GUI_FLOAT val)
{
    WDL_MutexLock lock(&mMutex);
    
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->FillAllXValues(mMinX, mMaxX);
    mCurves[curveNum]->FillAllYValues(val);
    
    mDirty = true;
    mDataChanged = true;
}

void
GraphControl11::SetCurveValues(int curveNum, const WDL_TypedBuf<BL_GUI_FLOAT> *values)
{
    WDL_MutexLock lock(&mMutex);
    
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->SetYValues(values, mMinX, mMaxX);
    
    mDirty = true;
    mDataChanged = true;
}

void
GraphControl11::SetCurveValues2(int curveNum, const WDL_TypedBuf<BL_GUI_FLOAT> *values)
{
    WDL_MutexLock lock(&mMutex);
    
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
    mDataChanged = true;
}

void
GraphControl11::SetCurveValues3(int curveNum, const WDL_TypedBuf<BL_GUI_FLOAT> *values)
{
    WDL_MutexLock lock(&mMutex);
    
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
    mDataChanged = true;
}

void
GraphControl11::SetCurveValuesDecimateSimple(int curveNum,
                                             const WDL_TypedBuf<BL_GUI_FLOAT> *values)
{
    WDL_MutexLock lock(&mMutex);
    
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
    mDataChanged = true;
}


void
GraphControl11::SetCurveValuesDecimate(int curveNum,
                                      const WDL_TypedBuf<BL_GUI_FLOAT> *values,
                                      bool isWaveSignal)
{
    WDL_MutexLock lock(&mMutex);
    
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
    mDataChanged = true;
}

void
GraphControl11::SetCurveValuesDecimate2(int curveNum,
                                        const WDL_TypedBuf<BL_GUI_FLOAT> *values,
                                        bool isWaveSignal)
{
    WDL_MutexLock lock(&mMutex);
    
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
        BLUtilsDecim::DecimateSamples(&decimValues, *values, decFactor);
    else
        BLUtilsDecim::DecimateValues(&decimValues, *values, decFactor);
    
    for (int i = 0; i < decimValues.GetSize(); i++)
    {
        BL_GUI_FLOAT t = ((BL_GUI_FLOAT)i)/(decimValues.GetSize() - 1);
        
        BL_GUI_FLOAT y = decimValues.Get()[i];
        
        SetCurveValue(curveNum, t, y);
    }
    
    mDirty = true;
    mDataChanged = true;
}

void
GraphControl11::SetCurveValuesDecimate3(int curveNum,
                                        const WDL_TypedBuf<BL_GUI_FLOAT> *values,
                                        bool isWaveSignal)
{
    WDL_MutexLock lock(&mMutex);
    
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
        BLUtilsDecim::DecimateSamples2(&decimValues, *values, decFactor);
    else
        BLUtilsDecim::DecimateValues(&decimValues, *values, decFactor);
    
    for (int i = 0; i < decimValues.GetSize(); i++)
    {
        BL_GUI_FLOAT t = ((BL_GUI_FLOAT)i)/(decimValues.GetSize() - 1);
        
        BL_GUI_FLOAT y = decimValues.Get()[i];
        
        SetCurveValue(curveNum, t, y);
    }
    
    mDirty = true;
    mDataChanged = true;
}

void
GraphControl11::SetCurveValuesXDbDecimate(int curveNum,
                                          const WDL_TypedBuf<BL_GUI_FLOAT> *values,
                                          int nativeBufferSize, BL_GUI_FLOAT sampleRate,
                                          BL_GUI_FLOAT decimFactor)
{
    WDL_MutexLock lock(&mMutex);
    
    int bufferSize = BLUtilsPlug::PlugComputeBufferSize(nativeBufferSize, sampleRate);
    BL_GUI_FLOAT hzPerBin = sampleRate/bufferSize;
    
    BL_GUI_FLOAT minHzValue;
    BL_GUI_FLOAT maxHzValue;
    BLUtils::GetMinMaxFreqAxisValues(&minHzValue, &maxHzValue,
                                   bufferSize, sampleRate);
    
    WDL_TypedBuf<BL_GUI_FLOAT> logSignal;
    BLUtils::FreqsToDbNorm(&logSignal, *values, hzPerBin,
                         minHzValue, maxHzValue);
    
    WDL_TypedBuf<BL_GUI_FLOAT> decimSignal;
    BLUtilsDecim::DecimateValues(&decimSignal, logSignal, decimFactor);
    
    for (int i = 0; i < decimSignal.GetSize(); i++)
    {
        BL_GUI_FLOAT t = ((BL_GUI_FLOAT)i) / (decimSignal.GetSize() - 1);
        
        SetCurveValue(curveNum, t, decimSignal.Get()[i]);
    }
}

void
GraphControl11::SetCurveValuesXDbDecimateDb(int curveNum,
                                            const WDL_TypedBuf<BL_GUI_FLOAT> *values,
                                           int nativeBufferSize, BL_GUI_FLOAT sampleRate,
                                           BL_GUI_FLOAT decimFactor, BL_GUI_FLOAT minValueDb)
{
    WDL_MutexLock lock(&mMutex);
    
    int bufferSize = BLUtilsPlug::PlugComputeBufferSize(nativeBufferSize, sampleRate);
    BL_GUI_FLOAT hzPerBin = sampleRate/bufferSize;
    
    BL_GUI_FLOAT minHzValue;
    BL_GUI_FLOAT maxHzValue;
    BLUtils::GetMinMaxFreqAxisValues(&minHzValue, &maxHzValue,
                                   bufferSize, sampleRate);
    
    WDL_TypedBuf<BL_GUI_FLOAT> logSignal;
    BLUtils::FreqsToDbNorm(&logSignal, *values, hzPerBin,
                         minHzValue, maxHzValue);
    
    WDL_TypedBuf<BL_GUI_FLOAT> decimSignal;
    BLUtilsDecim::DecimateValuesDb(&decimSignal, logSignal, decimFactor, minValueDb);
    
    for (int i = 0; i < decimSignal.GetSize(); i++)
    {
        BL_GUI_FLOAT t = ((BL_GUI_FLOAT)i) / (decimSignal.GetSize() - 1);
        
        SetCurveValue(curveNum, t, decimSignal.Get()[i]);
    }
}

void
GraphControl11::SetCurveValue(int curveNum, BL_GUI_FLOAT t, BL_GUI_FLOAT val)
{
    WDL_MutexLock lock(&mMutex);
    
    if (curveNum >= mNumCurves)
        return;
    
    GraphCurve4 *curve = mCurves[curveNum];
    
    // Normalize, then adapt to the graph
    int width = this->mRECT.W();
    int height = this->mRECT.H();
    
    BL_GUI_FLOAT x = t;
    
    if (x < GRAPH_VALUE_UNDEFINED) // for float
    {
        if (mXdBScale)
            x = BLUtils::NormalizedXTodB(x, mMinX, mMaxX);
        
        // X should be already normalize in input
    
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
    mDataChanged = true;
}

void
GraphControl11::SetCurveSingleValueH(int curveNum, BL_GUI_FLOAT val)
{
    WDL_MutexLock lock(&mMutex);
    
    if (curveNum >= mNumCurves)
        return;
    
    SetCurveValue(curveNum, 0.0, val);
    
    mDirty = true;
    mDataChanged = true;
}

void
GraphControl11::SetCurveSingleValueV(int curveNum, BL_GUI_FLOAT val)
{
    WDL_MutexLock lock(&mMutex);
    
    if (curveNum >= mNumCurves)
        return;
    
    SetCurveValue(curveNum, 0.0, val);
    
    mDirty = true;
    mDataChanged = true;
}

void
GraphControl11::PushCurveValue(int curveNum, BL_GUI_FLOAT val)
{
    WDL_MutexLock lock(&mMutex);
    
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
    mDataChanged = true;
}

void
GraphControl11::DrawAxis(bool lineLabelFlag)
{
    WDL_MutexLock lock(&mMutex);
    
    if (mHAxis != NULL)
        DrawAxis(mHAxis, true, lineLabelFlag);
    
    if (mVAxis != NULL)
        DrawAxis(mVAxis, false, lineLabelFlag);
}

void
GraphControl11::DrawAxis(GraphAxis *axis, bool horizontal, bool lineLabelFlag)
{
    WDL_MutexLock lock(&mMutex);
    
    nvgSave(mVg);
    nvgStrokeWidth(mVg, 1.0);
    
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
        const GraphAxisData &data = axis->mValues[i];
        
        BL_GUI_FLOAT t = data.mT;
        const char *text = data.mText.c_str();

        if (horizontal)
        {
            BL_GUI_FLOAT textOffset = FONT_SIZE*0.2;
            
            if (axis->mXdBScale)
                t = BLUtils::NormalizedXTodB(t, mMinX, mMaxX);
            
            BL_GUI_FLOAT x = t*width;
            if ((i > 0) && (i < axis->mValues.size() - 1))
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
                    
                    DrawText(x,
                             textOffset + axis->mOffsetY*height,
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
                    DrawText(x + textOffset,
                             textOffset + axis->mOffsetY*height, FONT_SIZE,
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
                    DrawText(x - textOffset,
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
            
            // Re-added dB normalization for Ghost
            if (!mCurves.empty())
            {
                if (mCurves[0]->mYdBScale)
                    t = BLUtils::NormalizedXTodB(t, mCurves[0]->mMinY, mCurves[0]->mMaxY);
            }
            
            t = ConvertToBoundsY(t);
            
            BL_GUI_FLOAT y = t*height;
            
            // Hack
            // See Transient, with it 2 vertical axis
            y += axis->mOffset;
            
            if ((i > 0) && (i < axis->mValues.size() - 1))
                // First and last: don't draw axis line
            {
                if (lineLabelFlag)
                {
                    BL_GUI_FLOAT x0 = 0.0;
                    BL_GUI_FLOAT x1 = width;
                
                    BL_GUI_FLOAT yf = y;
#if GRAPH_CONTROL_FLIP_Y
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
                        DrawText(textOffset + axis->mOffsetX + OVERLAY_OFFSET,
                                 y + OVERLAY_OFFSET,
                                 FONT_SIZE, text, axis->mLabelOverlayColor,
                                 align | NVG_ALIGN_MIDDLE, NVG_ALIGN_BOTTOM,
                                 axis->mFontSizeCoeff);
                    }
                    
                    DrawText(textOffset + axis->mOffsetX,
                             y, FONT_SIZE, text,
                             axis->mLabelColor,
                             align | NVG_ALIGN_MIDDLE,
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
                    
                    DrawText(textOffset + axis->mOffsetX,
                             y + FONT_SIZE*0.75, FONT_SIZE, text,
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
                    
                    DrawText(textOffset + axis->mOffsetX,
                             y - FONT_SIZE*1.5, FONT_SIZE, text,
                             axis->mLabelColor, NVG_ALIGN_LEFT, NVG_ALIGN_BOTTOM,
                             axis->mFontSizeCoeff);
                }
            }
        }
    }
    
    nvgRestore(mVg);
}

void
GraphControl11::DrawCurves()
{
    WDL_MutexLock lock(&mMutex);
    
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
GraphControl11::DrawLineCurve(GraphCurve4 *curve)
{
    WDL_MutexLock lock(&mMutex);
    
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
        
        if (x >= GRAPH_VALUE_UNDEFINED)
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
        if (y >= GRAPH_VALUE_UNDEFINED)
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
    
    mDirty = true;
    
#if CURVE_DEBUG
    fprintf(stderr, "GraphControl11::DrawLineCurve - num points: %d\n", numPointsDrawn);
#endif
}

#if !FILL_CURVE_HACK
        // Bug with direct rendering
        // It seems we have no stencil, and we can only render convex polygons
void
GraphControl11::DrawFillCurve(GraphCurve4 *curve)
{
    WDL_MutexLock lock(&mMutex);
    
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
        
        if (x >= GRAPH_VALUE_UNDEFINED)
            continue;
        
        if (y >= GRAPH_VALUE_UNDEFINED)
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
    
    mDirty = true;
}
#endif

#if FILL_CURVE_HACK
// Due to a bug, we render only convex polygons when filling curves
// So we separate in rectangles
void
GraphControl11::DrawFillCurve(GraphCurve4 *curve)
{
    WDL_MutexLock lock(&mMutex);
    
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
        if (x >= GRAPH_VALUE_UNDEFINED)
            continue;
        
        BL_GUI_FLOAT y = curve->mYValues.Get()[i];
        if (y >= GRAPH_VALUE_UNDEFINED)
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
            if (x >= GRAPH_VALUE_UNDEFINED)
                continue;
            
            BL_GUI_FLOAT y = curve->mYValues.Get()[i];
            if (y >= GRAPH_VALUE_UNDEFINED)
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
    
    mDirty = true;
    
#if CURVE_DEBUG
    fprintf(stderr, "GraphControl11::DrawFillCurve - num points: %d\n", numPointsDrawn);
#endif
}
#endif

void
GraphControl11::DrawLineCurveSVH(GraphCurve4 *curve)
{
    WDL_MutexLock lock(&mMutex);
    
    if (curve->mYValues.GetSize() == 0)
        return;
    
    BL_GUI_FLOAT val = curve->mYValues.Get()[0];
    if (val >= GRAPH_VALUE_UNDEFINED)
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
    
    mDirty = true;
}

void
GraphControl11::DrawFillCurveSVH(GraphCurve4 *curve)
{
    WDL_MutexLock lock(&mMutex);
    
    if (curve->mYValues.GetSize() == 0)
        return;
    
    BL_GUI_FLOAT val = curve->mYValues.Get()[0];
    if (val >= GRAPH_VALUE_UNDEFINED)
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
    
    mDirty = true;
}

void
GraphControl11::DrawLineCurveSVV(GraphCurve4 *curve)
{
    WDL_MutexLock lock(&mMutex);
    
    // Finally, take the Y value
    // We will have to care about the curve Y scale !
    if (curve->mYValues.GetSize() == 0)
        return;
    
    int width = this->mRECT.W();
    int height = this->mRECT.H();
    
    BL_GUI_FLOAT val = curve->mYValues.Get()[0];
    if (val >= GRAPH_VALUE_UNDEFINED)
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
    
    mDirty = true;
}

// Fill right
// (only, for the moment)
void
GraphControl11::DrawFillCurveSVV(GraphCurve4 *curve)
{
    WDL_MutexLock lock(&mMutex);
    
    // Finally, take the Y value
    // We will have to care about the curve Y scale !
    if (curve->mYValues.GetSize() == 0)
        return;
    
    int width = this->mRECT.W();
    int height = this->mRECT.H();
    
    BL_GUI_FLOAT val = curve->mYValues.Get()[0];
    if (val >= GRAPH_VALUE_UNDEFINED)
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
    
    mDirty = true;
}

// Optimized !
// Optimization2: nvgQuad
void
GraphControl11::DrawPointCurve(GraphCurve4 *curve)
{
    WDL_MutexLock lock(&mMutex);
    
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
        if (x >= GRAPH_VALUE_UNDEFINED)
            continue;
        
        BL_GUI_FLOAT y = curve->mYValues.Get()[i];
        if (y >= GRAPH_VALUE_UNDEFINED)
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
    
    mDirty = true;
}

void
GraphControl11::DrawPointCurveOptimSameColor(GraphCurve4 *curve)
{
    WDL_MutexLock lock(&mMutex);
    
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
        if (x >= GRAPH_VALUE_UNDEFINED)
            break;;
        
        BL_GUI_FLOAT y = curve->mYValues.Get()[i];
        if (y >= GRAPH_VALUE_UNDEFINED)
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
    
    mDirty = true;
}

// TEST (to display lines instead of points)
void
GraphControl11::DrawPointCurveLinesPolar(GraphCurve4 *curve)
{
    WDL_MutexLock lock(&mMutex);
    
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
        if (x >= GRAPH_VALUE_UNDEFINED)
            continue;
        
        BL_GUI_FLOAT y = curve->mYValues.Get()[i];
        if (y >= GRAPH_VALUE_UNDEFINED)
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
    
    mDirty = true;
}

void
GraphControl11::DrawPointCurveLinesPolarWeights(GraphCurve4 *curve)
{
    WDL_MutexLock lock(&mMutex);
    
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
        if (x >= GRAPH_VALUE_UNDEFINED)
            continue;
        
        BL_GUI_FLOAT y = curve->mYValues.Get()[i];
        if (y >= GRAPH_VALUE_UNDEFINED)
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
    
    mDirty = true;
}

void
GraphControl11::DrawPointCurveLinesPolarFill(GraphCurve4 *curve)
{
    WDL_MutexLock lock(&mMutex);
    
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
        if (x >= GRAPH_VALUE_UNDEFINED)
            continue;
        
        BL_GUI_FLOAT y = curve->mYValues.Get()[i];
        if (y >= GRAPH_VALUE_UNDEFINED)
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
        
        //nvgStroke(mVg);
        
        lastValidIdx = i;
    }
    
    nvgRestore(mVg);
    
    mDirty = true;
}

void
GraphControl11::DrawPointCurveLines(GraphCurve4 *curve)
{
    WDL_MutexLock lock(&mMutex);
    
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
        if (x >= GRAPH_VALUE_UNDEFINED)
            continue;
        
        BL_GUI_FLOAT y = curve->mYValues.Get()[i];
        if (y >= GRAPH_VALUE_UNDEFINED)
            continue;
        
        BL_GUI_FLOAT yf = y;
#if GRAPH_CONTROL_FLIP_Y
        yf = height - yf;
#endif
        
        nvgLineTo(mVg, x, yf);
    }
    
    nvgStroke(mVg);
    
    nvgRestore(mVg);
    
    mDirty = true;
}

void
GraphControl11::DrawPointCurveLinesWeights(GraphCurve4 *curve)
{
    WDL_MutexLock lock(&mMutex);
    
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
        if (x >= GRAPH_VALUE_UNDEFINED)
            continue;
        
        BL_GUI_FLOAT y = curve->mYValues.Get()[i];
        if (y >= GRAPH_VALUE_UNDEFINED)
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
    
    mDirty = true;
}

// doesn't work well (for non convex polygons)
void
GraphControl11::DrawPointCurveLinesFill(GraphCurve4 *curve)
{
    WDL_MutexLock lock(&mMutex);
    
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
        if (x >= GRAPH_VALUE_UNDEFINED)
            continue;
        
        BL_GUI_FLOAT y = curve->mYValues.Get()[i];
        if (y >= GRAPH_VALUE_UNDEFINED)
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
    
    mDirty = true;
}

void
GraphControl11::AutoAdjust()
{
    WDL_MutexLock lock(&mMutex);
    
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
    
    mDirty = true;
    mDataChanged = true;
}

BL_GUI_FLOAT
GraphControl11::MillisToPoints(long long int elapsed, int sampleRate, int numSamplesPoint)
{
    BL_GUI_FLOAT numSamples = (((BL_GUI_FLOAT)elapsed)/1000.0)*sampleRate;
    
    BL_GUI_FLOAT numPoints = numSamples/numSamplesPoint;
    
    return numPoints;
}

#if 0 // iPlug2 / Windows
void
GraphControl11::InitFont(const char *fontPath)
{
    WDL_MutexLock lock(&mMutex);
    
#ifndef WIN32
    nvgCreateFont(mVg, GRAPH_FONT, fontPath);

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

	nvgCreateFontMem(mVg, GRAPH_FONT, data, ndata, 1);

	mFontInitialized = true;
#endif
}
#endif

void
GraphControl11::DrawText(BL_GUI_FLOAT x, BL_GUI_FLOAT y, BL_GUI_FLOAT fontSize,
                        const char *text, int color[4],
                        int halign, int valign, BL_GUI_FLOAT fontSizeCoeff)
{
    WDL_MutexLock lock(&mMutex);
    
    int width = this->mRECT.W();
    int height = this->mRECT.H();
    
    DrawText(mVg, x, y, width, height,
             fontSize, text, color,
             halign, valign, fontSizeCoeff);
}

bool
GraphControl11::NeedUpdateGUI()
{
    WDL_MutexLock lock(&mMutex);
    
    if (mSpectrogramDisplay != NULL)
    {
        bool needUpdate = mSpectrogramDisplay->NeedUpdateSpectrogram();
        if (needUpdate)
            return true;
    }
    
    if (mSpectrogramDisplayScroll != NULL)
    {
        bool needUpdate = mSpectrogramDisplayScroll->NeedUpdateSpectrogram();
        if (needUpdate)
            return true;
    }
    
    if (mSpectrogramDisplayScroll2 != NULL)
    {
        bool needUpdate = mSpectrogramDisplayScroll2->NeedUpdateSpectrogram();
        if (needUpdate)
            return true;
    }
    
    if (mImageDisplay != NULL)
    {
        bool needUpdate = mImageDisplay->NeedUpdateImage();
        if (needUpdate)
            return true;
    }

    return false;
}

#if !USE_FBO || (!defined IGRAPHICS_GL)
void
GraphControl11::Draw(IGraphics &graphics)
{
    DoDraw(graphics);
}
#endif

// From IGraphics/Tests/TestGLControl.h
#if USE_FBO && (defined IGRAPHICS_GL)
void
GraphControl11::Draw(IGraphics &graphics)
{
    WDL_MutexLock lock(&mMutex);
    
    // Keep a reference, for deleting FBO
    mGraphics = &graphics;
    
    SetVg(graphics);
    
    // Checked: if we fall here, the graph is sure to have mIsEnabled = true!
    if (!mIsEnabled)
        return;
    
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
    
        // Debug
        //graphics.DrawDottedRect(COLOR_GREEN, mRECT);
        //graphics.FillRect(mMouseIsOver ? COLOR_TRANSLUCENT : COLOR_TRANSPARENT, mRECT);
    
        nvgEndFrame(mVg);
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &mInitialFBO);
    
        nvgBindFramebuffer(mFBO);
        nvgBeginFrame(mVg, w, h, 1.0f);
        GLint vp[4];
        glGetIntegerv(GL_VIEWPORT, vp);
    
        glViewport(0, 0, w, h);
    
        glScissor(0, 0, w, h);
        glClearColor(0.f, 0.f, 0.f, 0.f);
        //glClearColor(1.f, 0.f, 0.f, 1.f); // Debug red
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    
        DoDraw(graphics);
    
        // BUG here
        //glViewport(vp[0], vp[1], vp[2], vp[3]);
    
        nvgEndFrame(mVg);
    
        // BUG fixed here!
        glViewport(vp[0], vp[1], vp[2], vp[3]);
    
        glBindFramebuffer(GL_FRAMEBUFFER, mInitialFBO);
    
        float graphicsWidth = graphics.WindowWidth();
        float graphicsHeight = graphics.WindowHeight();
        nvgBeginFrame(mVg, graphicsWidth, graphicsHeight, 1.0f);
    }
    
    APIBitmap apibmp {mFBO->image, w, h, 1, 1.};
    IBitmap bmp {&apibmp, 1, false};
    
    //graphics.DrawFittedBitmap(bmp, mRECT);
    graphics.DrawBitmap(bmp, mRECT);
    
#if USE_FBO
    mVg = NULL;
#endif
}
#endif

void
GraphControl11::DoDraw(IGraphics &graphics)
{
    WDL_MutexLock lock(&mMutex);
    
    SetVg(graphics);
    
    // Checked: if we fall here, the graph is sur to have mIsEnabled = true!
    if (!mIsEnabled)
    {
        return;
    }
    
    nvgSave(mVg);
    
    //nvgResetTransform(mVg);
    
    nvgReset(mVg);
    
//#if !USE_FBO
#if !USE_FBO || (!defined IGRAPHICS_GL)
    // #bl-iplug2
    // Be sure to draw only in the graph.
    // So when using IPlug2, we won't draw on the other GUI componenets.
    // NOTE: This is required since we commented glClear() in IPlug2.
    nvgScissor(mVg, this->mRECT.L, this->mRECT.T,
               this->mRECT.W(), this->mRECT.H());
    
    nvgTranslate(mVg, this->mRECT.L, this->mRECT.T);
#endif
    
    // Update first, before displaying
    if (mSpectrogramDisplay != NULL)
    {
        bool updated = mSpectrogramDisplay->DoUpdateSpectrogram();
        
        if (updated)
        {
            mDirty = true;
        }
    }
   
    //
    if (mSpectrogramDisplayScroll != NULL)
    {
        bool updated = mSpectrogramDisplayScroll->DoUpdateSpectrogram();
        if (updated)
        {
            mDirty = true;
        }
    }
    
    // 2
    if (mSpectrogramDisplayScroll2 != NULL)
    {
        bool updated = mSpectrogramDisplayScroll2->DoUpdateSpectrogram();
        if (updated)
        {
            mDirty = true;
        }
    }
    
    //
    if (mImageDisplay != NULL)
    {
        bool updated = mImageDisplay->DoUpdateImage();
        
        if (updated)
        {
            mDirty = true;
        }
    }
    
    int width = this->mRECT.W();
    int height = this->mRECT.H();
    
    // nvgBeginFrame() ?
    
    DrawBackgroundImage(graphics);
    
    if (mAutoAdjustFlag)
    {
        AutoAdjust();
    }
    
    CustomDrawersPreDraw();
    
    if (mSpectrogramDisplay != NULL)
        mSpectrogramDisplay->DrawSpectrogram(width, height);
    
    if (mSpectrogramDisplayScroll != NULL)
        mSpectrogramDisplayScroll->DrawSpectrogram(width, height);
    
    if (mSpectrogramDisplayScroll2 != NULL)
        mSpectrogramDisplayScroll2->DrawSpectrogram(width, height);
    
    // Update the time axis, so we are very accurate at each Draw() call
    if (mGraphTimeAxis != NULL)
        mGraphTimeAxis->Update();
    
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

    CustomDrawersPostDraw();
    
    DrawSeparatorY0();
   
    DrawOverlayImage(graphics);
    
    nvgRestore(mVg);
    
    mDirty = false;
   
#if !USE_FBO
    mVg = NULL;
#endif
}

BL_GUI_FLOAT
GraphControl11::ConvertX(GraphCurve4 *curve, BL_GUI_FLOAT val, BL_GUI_FLOAT width)
{
    WDL_MutexLock lock(&mMutex);
    
    BL_GUI_FLOAT x = val;
    if (x < GRAPH_VALUE_UNDEFINED)
    {
        if (curve->mXdBScale)
        {
            if (val > 0.0)
                // Avoid -INF values
                x = BLUtils::NormalizedYTodB(x, curve->mMinX, curve->mMaxX);
        }
        else
            x = (x - curve->mMinX)/(curve->mMaxX - curve->mMinX);
        
        x = x * width;
    }
    
    return x;
}

BL_GUI_FLOAT
GraphControl11::ConvertY(GraphCurve4 *curve, BL_GUI_FLOAT val, BL_GUI_FLOAT height)
{
    WDL_MutexLock lock(&mMutex);
    
    BL_GUI_FLOAT y = val;
    if (y < GRAPH_VALUE_UNDEFINED)
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

// Optimized versions
void
GraphControl11::ConvertX(GraphCurve4 *curve, WDL_TypedBuf<BL_GUI_FLOAT> *vals,
                         BL_GUI_FLOAT width)
{
    WDL_MutexLock lock(&mMutex);
    
    BL_GUI_FLOAT xCoeff = width/(curve->mMaxX - curve->mMinX);
    
    for (int i = 0; i < vals->GetSize(); i++)
    {
        BL_GUI_FLOAT x = vals->Get()[i];
        if (x < GRAPH_VALUE_UNDEFINED)
        {
            if (curve->mXdBScale)
            {
                if (x > 0.0)
                {
                    // Avoid -INF values
                    //x = BLUtils::NormalizedYTodB(x, curve->mMinX, curve->mMaxX);
                
                    if (std::fabs(x) < EPS)
                        x = curve->mMinX;
                    else
                        x = BLUtils::AmpToDB(x);
                
                        x = (x - curve->mMinX)*xCoeff;
                }
            }
            else
            {
                x = (x - curve->mMinX)*xCoeff;
            }
        }
        
        vals->Get()[i] = x;
    }
}

void
GraphControl11::ConvertY(GraphCurve4 *curve, WDL_TypedBuf<BL_GUI_FLOAT> *vals,
                         BL_GUI_FLOAT height)
{
    WDL_MutexLock lock(&mMutex);
    
    BL_GUI_FLOAT yCoeff =
            mAutoAdjustFactor * mYScaleFactor*height/(curve->mMaxY - curve->mMinY);
    
    for (int i = 0; i < vals->GetSize(); i++)
    {
        BL_GUI_FLOAT y = vals->Get()[i];
        if (y < GRAPH_VALUE_UNDEFINED)
        {
            if (curve->mYdBScale)
            {
                if (y > 0.0)
                {
                    // Avoid -INF values
                    //x = BLUtils::NormalizedYTodB(x, curve->mMinX, curve->mMaxX);
                    
                    if (std::fabs(y) < EPS)
                        y = curve->mMinY;
                    else
                        y = BLUtils::AmpToDB(y);
                    
                    y = (y - curve->mMinY)*yCoeff;
                }
            }
            else
            {
                y = (y - curve->mMinY)*yCoeff;
            }
        }
        
        vals->Get()[i] = y;
    }
}

void
GraphControl11::SetCurveDrawStyle(GraphCurve4 *curve)
{
    WDL_MutexLock lock(&mMutex);
    
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
    
    mDirty = true;
}

void
GraphControl11::SetCurveDrawStyleWeight(GraphCurve4 *curve, BL_GUI_FLOAT weight)
{
    WDL_MutexLock lock(&mMutex);
    
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
}

BL_GUI_FLOAT
GraphControl11::ConvertToBoundsX(BL_GUI_FLOAT t)
{
    WDL_MutexLock lock(&mMutex);
    
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
GraphControl11::ConvertToBoundsY(BL_GUI_FLOAT t)
{
    WDL_MutexLock lock(&mMutex);
    
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
GraphControl11::DrawBackgroundImage(IGraphics &graphics)
{
    WDL_MutexLock lock(&mMutex);
    
    if (mBgImage.GetAPIBitmap() == NULL)
        return;
    
    nvgSave(mVg);
    nvgTranslate(mVg, -this->mRECT.L, -this->mRECT.T);
    
    IRECT bounds = GetRECT();
    graphics.DrawBitmap(mBgImage, bounds, 0, 0);
    
    nvgRestore(mVg);
    
    //mDirty = true;
}

void
GraphControl11::DrawOverlayImage(IGraphics &graphics)
{
    WDL_MutexLock lock(&mMutex);
    
    if (mOverlayImage.GetAPIBitmap() == NULL)
        return;
    
    nvgSave(mVg); // new
    nvgTranslate(mVg, -this->mRECT.L, -this->mRECT.T);
    
    IRECT bounds = GetRECT();
    graphics.DrawBitmap(mOverlayImage, bounds, 0, 0);
    
    nvgRestore(mVg);
}

bool
GraphControl11::IsCurveUndefined(const WDL_TypedBuf<BL_GUI_FLOAT> &x,
                                 const WDL_TypedBuf<BL_GUI_FLOAT> &y,
                                 int minNumValues)
{
    WDL_MutexLock lock(&mMutex);
    
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

void
GraphControl11::SetVg(IGraphics &graphics)
{
    WDL_MutexLock lock(&mMutex);
    
    mVg = (NVGcontext *)graphics.GetDrawContext();
    
    if (mSpectrogramDisplay != NULL)
        mSpectrogramDisplay->SetNvgContext(mVg);
    if (mSpectrogramDisplayScroll != NULL)
        mSpectrogramDisplayScroll->SetNvgContext(mVg);
    if (mSpectrogramDisplayScroll2 != NULL)
        mSpectrogramDisplayScroll2->SetNvgContext(mVg);
    if (mImageDisplay != NULL)
        mImageDisplay->SetNvgContext(mVg);
}

#endif // IGRAPHICS_NANOVG
