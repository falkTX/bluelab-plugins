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

#include "Utils.h"
#include "GLContext2.h"
#include "GraphControl4.h"
#include "Debug.h"

#define FONT_SIZE 14.0

#ifdef WIN32
#define SWAP_COLOR(__RGBA__)
#endif

#ifdef __APPLE__
// Convert from RGBA to ABGR
#define SWAP_COLOR(__RGBA__) \
{ int tmp[4] = { __RGBA__[0], __RGBA__[1], __RGBA__[2], __RGBA__[3] }; \
__RGBA__[0] = tmp[2]; \
__RGBA__[1] = tmp[1]; \
__RGBA__[2] = tmp[0]; \
__RGBA__[3] = tmp[3]; }
#endif


GraphControl4::GraphControl4(IPlugBase *pPlug, IRECT pR,
	int numCurves, int numCurveValues,
	const char *fontPath)
	: IControl(pPlug, pR),
	mFontInitialized(false),
	mAutoAdjustParamSmoother(1.0, 0.9),
	mNumCurves(numCurves),
	mNumCurveValues(numCurveValues),
	mHAxis(NULL),
	mVAxis(NULL)
{
	for (int i = 0; i < mNumCurves; i++)
	{
		GraphCurve *curve = new GraphCurve(numCurveValues);

		mCurves.push_back(curve);
	}

	mAutoAdjustFlag = false;
	mAutoAdjustFactor = 1.0;

	mYScaleFactor = 1.0;

	mXdBScale = false;
	mMinXdB = 0.1;
	mMaxXdB = 1.0;
    
    SetClearColor(0, 0, 0, 255);
    
    mDirty = true;
    
    // GL and nanovg
    mGLInitialized = false;
    mFontPath.Set(fontPath);
    
    InitGL();
    InitNanoVg();
    
    mLiceFb = NULL;
    
#if PROFILE_GRAPH
    mDebugCount = 0;
#endif
}

GraphControl4::~GraphControl4()
{
    for (int i = 0; i < mNumCurves; i++)
        delete mCurves[i];
    
    if (mHAxis != NULL)
        delete mHAxis;
    
    if (mVAxis != NULL)
        delete mVAxis;
    
    ExitNanoVg();
    
    if (mLiceFb != NULL)
        delete mLiceFb;
    
    ExitGL();
}

bool
GraphControl4::IsDirty()
{
    return mDirty;
}

bool
GraphControl4::Draw(IGraphics *pGraphics)
{
#if PROFILE_GRAPH
    mDebugTimer.Start();
#endif
    
    IPlugBase::IMutexLock lock(mPlug);
    
    GLContext2 *context = GLContext2::Get();
    context->Bind();
    
    DrawGL();
    
    // Hardware accelerated blit
    LICE_SysBitmap *graphicsBmp = pGraphics->GetDrawBitmap();
    HDC graphicsDC = graphicsBmp->getDC();
    
    HDC fbDC = mLiceFb->getDC();
    
    IRECT *r = GetRECT();
    
    BitBlt(graphicsDC, r->L, r->T, r->W(), r->H(), fbDC, 0, 0, LICE_EXT_BLIT_ACCEL);
    
    // Unbind OpenGL context
    context->Unbind();
    
    mDirty = false;
    
#if PROFILE_GRAPH
    mDebugTimer.Stop();
    
    if (mDebugCount++ % 100 == 0)
    {
        long t = mDebugTimer.Get();
        mDebugTimer.Reset();
        
        char message[1024];
        sprintf(message, "Draw(x): %ld\n", t);
        Debug::DumpMessage("GraphProfile.txt", message);
    }
#endif

    return true;
}

void
GraphControl4::AddHAxis(char *data[][2], int numData, int axisColor[4], int axisLabelColor[4])
{
    mHAxis = new GraphAxis();
    mHAxis->mOffset = 0.0;
    
    AddAxis(mHAxis, data, numData, axisColor, axisLabelColor, mMinXdB, mMaxXdB);
}

void
GraphControl4::AddVAxis(char *data[][2], int numData, int axisColor[4], int axisLabelColor[4], double offset)
{
    mVAxis = new GraphAxis();
    
    mVAxis->mOffset = offset;
    
    // Retreive the Y db scale
    double minYdB = -40.0;
    double maxYdB = 40.0;
    
    if (!mCurves.empty())
        // Get from the first curve
    {
        minYdB = mCurves[0]->mMinYdB;
        maxYdB = mCurves[0]->mMaxYdB;
    }
    
    AddAxis(mVAxis, data, numData, axisColor, axisLabelColor, minYdB, maxYdB);
}

void
GraphControl4::SetXdBScale(bool flag, double minXdB, double maxXdB)
{
    mXdBScale = flag;
    
    mMinXdB = minXdB;
    mMaxXdB = maxXdB;
}

void
GraphControl4::SetAutoAdjust(bool flag, double smoothCoeff)
{
    mAutoAdjustFlag = flag;
    
    mAutoAdjustParamSmoother.SetSmoothCoeff(smoothCoeff);
    
    mDirty = true;
}

void
GraphControl4::SetYScaleFactor(double factor)
{
    mYScaleFactor = factor;
    
    mDirty = true;
}

void
GraphControl4::SetClearColor(int r, int g, int b, int a)
{
    SET_COLOR_FROM_INT(mClearColor, r, g, b, a);
    
    mDirty = true;
}

void
GraphControl4::SetCurveColor(int curveNum, int r, int g, int b)
{
    if (curveNum >= mNumCurves)
        return;
    
    SET_COLOR_FROM_INT(mCurves[curveNum]->mColor, r, g, b, 255);
    
    mDirty = true;
}

void
GraphControl4::SetCurveAlpha(int curveNum, double alpha)
{
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->mAlpha = alpha;
    
    mDirty = true;
}

void
GraphControl4::SetCurveLineWidth(int curveNum, double lineWidth)
{
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->mLineWidth = lineWidth;
    
    mDirty = true;
}

void
GraphControl4::SetCurveSmooth(int curveNum, bool flag)
{
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->mDoSmooth = flag;
    
    mDirty = true;
}

void
GraphControl4::SetCurveFill(int curveNum, bool flag)
{
    if (curveNum >= mNumCurves)
        return;

    mCurves[curveNum]->mCurveFill = flag;
    
    mDirty = true;
}

void
GraphControl4::SetCurveFillAlpha(int curveNum, double alpha)
{
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->mFillAlpha = alpha;
    
    mDirty = true;
}

void
GraphControl4::SetCurveSingleValue(int curveNum, bool flag)
{
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->mSingleValue = flag;
    
    mDirty = true;
}

void
GraphControl4::AddAxis(GraphAxis *axis, char *data[][2], int numData, int axisColor[4], int axisLabelColor[4],
                       double mindB, double maxdB)
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
    }
    
    // Copy data
    for (int i = 0; i < numData; i++)
    {
        char *cData[2] = { data[i][0], data[i][1] };
        
        double t = atof(cData[0]);
        
        string text(cData[1]);
        
        // Error here, if we add an Y axis, we must not use mMinXdB
        GraphAxisData aData;
        aData.mT = (t - mindB)/(maxdB - mindB);
        aData.mText = text;
        
        axis->mValues.push_back(aData);
    }
}

void
GraphControl4::ResetCurve(int curveNum, double val)
{
    IPlugBase::IMutexLock lock(mPlug);
    
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->FillAllValues(val);
    
    mDirty = true;
}

void
GraphControl4::SetCurveYdBScale(int curveNum, bool flag, double minYdB, double maxYdB)
{
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->SetYdBScale(flag, minYdB, maxYdB);
    
    mDirty = true;
}

void
GraphControl4::SetCurveValues(int curveNum, const WDL_TypedBuf<double> *values)
{
    // Must lock otherwise with DATA_STEP, curve will jerk
    IPlugBase::IMutexLock lock(mPlug);
    
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->SetValues(values);
    
    mDirty = true;
}

void
GraphControl4::SetCurveValue(int curveNum, double t, double val)
{
    // Must lock otherwise we may have curve will jerk (??)
    IPlugBase::IMutexLock lock(mPlug);
    
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->SetValue(t, val);
    
    mDirty = true;
}

void
GraphControl4::SetCurveSingleValue(int curveNum, double val)
{
    // Must lock otherwise with DATA_STEP, curve will jerk
    IPlugBase::IMutexLock lock(mPlug);
    
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->SetValue(0.0, val);
    
    mDirty = true;
}

void
GraphControl4::PushCurveValue(int curveNum, double val)
{
    IPlugBase::IMutexLock lock(mPlug);
    
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->PushValue(val);
    mDirty = true;
}

void
GraphControl4::DrawAxis(bool lineLabelFlag)
{
    if (mHAxis != NULL)
        DrawAxis(mHAxis, true, lineLabelFlag);
    
    if (mVAxis != NULL)
        DrawAxis(mVAxis, false, lineLabelFlag);
}

void
GraphControl4::DrawAxis(GraphAxis *axis, bool horizontal, bool lineLabelFlag)
{
    nvgSave(mVg);
    nvgStrokeWidth(mVg, 1.0);
    
    int axisColor[4] = { axis->mColor[0], axis->mColor[1], axis->mColor[2], axis->mColor[3] };
    SWAP_COLOR(axisColor);
    
    nvgStrokeColor(mVg, nvgRGBA(axisColor[0], axisColor[1], axisColor[2], axisColor[3]));
    
    int width = this->mRECT.W();
    int height = this->mRECT.H();
    
    for (int i = 0; i < axis->mValues.size(); i++)
    {
        const GraphAxisData &data = axis->mValues[i];
        
        double t = data.mT;
        const char *text = data.mText.c_str();

        if (horizontal)
        {
            double textOffset = FONT_SIZE*0.2;
            
            t = Utils::NormalizedXTodB(t, mMinXdB, mMaxXdB);
            
            double x = t*width;
        
            if ((i > 0) && (i < axis->mValues.size() - 1))
                // First and last: don't draw axis line
            {
                if (lineLabelFlag)
                {
                    // Draw a vertical line
                    nvgBeginPath(mVg);

                    double y0 = 0.0;
                    double y1 = height;
        
                    nvgMoveTo(mVg, x, y0);
                    nvgLineTo(mVg, x, y1);
    
                    nvgStroke(mVg);
                }
                else
                    DrawText(x, textOffset, FONT_SIZE, text, axis->mLabelColor, NVG_ALIGN_CENTER);
            }
     
            if (!lineLabelFlag)
            {
                if (i == 0)
                    // First text: aligne left
                    DrawText(x + textOffset, textOffset, FONT_SIZE, text, axis->mLabelColor, NVG_ALIGN_LEFT);
        
                if (i == axis->mValues.size() - 1)
                    // Last text: aligne right
                    DrawText(x - textOffset, textOffset, FONT_SIZE, text, axis->mLabelColor, NVG_ALIGN_RIGHT);
            }
        }
        else
            // Vertical
        {
            double textOffset = FONT_SIZE*0.2;
            
#if 0
            // Retreive the Y db scale
            double minYdB = -40.0;
            double maxYdB = 40.0;
            
            if (!mCurves.empty())
                // Get from the first curve
            {
                minYdB = mCurves[0]->mMinYdB;
                maxYdB = mCurves[0]->mMaxYdB;
            }
#endif
            // Do not need to nomalize on the Y axis, because we want to display
            // the dB linearly (this is the curve values that are displayed exponentially).
            //t = Utils::NormalizedYTodB3(t, mMinXdB, mMaxXdB);
            
            double y = t*height;
            
            // Hack
            // See Transient, with it 2 vertical axis
            y += axis->mOffset;
            
            if ((i > 0) && (i < axis->mValues.size() - 1))
                // First and last: don't draw axis line
            {
                if (lineLabelFlag)
                {
                    // HERE DEBUG HERE !!!!
                    
                    // Draw a horizontal line
                    nvgBeginPath(mVg);
                
                    double x0 = 0.0;
                    double x1 = width;
                
                    nvgMoveTo(mVg, x0, y);
                    nvgLineTo(mVg, x1, y);
                
                    nvgStroke(mVg);

                }
                else
                    DrawText(textOffset, y, FONT_SIZE, text, axis->mLabelColor, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
            }
            
            if (!lineLabelFlag)
            {
                if (i == 0)
                    // First text: aligne "top"
                    DrawText(textOffset, y + FONT_SIZE*0.75, FONT_SIZE, text, axis->mLabelColor, NVG_ALIGN_LEFT);
            
                if (i == axis->mValues.size() - 1)
                    // Last text: aligne "bottom"
                    DrawText(textOffset, y - FONT_SIZE*1.5, FONT_SIZE, text, axis->mLabelColor, NVG_ALIGN_LEFT);
            }
        }
    }
    
    nvgRestore(mVg);
}

void
GraphControl4::DrawCurves()
{
    for (int i = 0; i < mNumCurves; i++)
    {
        if (!mCurves[i]->mSingleValue)
        {
            if (mCurves[i]->mCurveFill)
            {
                DrawFillCurve(&mCurves[i]->mValues,
                               mCurves[i]->mColor, mCurves[i]->mFillAlpha,
                               mCurves[i]->mAlpha, mCurves[i]->mLineWidth, mCurves[i]);
            }
            else
            {
                DrawLineCurve(&mCurves[i]->mValues,
                              mCurves[i]->mColor, mCurves[i]->mAlpha, mCurves[i]->mLineWidth, mCurves[i]);
            }
        }
        else
        {
            if (!mCurves[i]->mValues.GetSize() == 0)
            {
                if (mCurves[i]->mCurveFill)
                {
                    DrawFillCurveSV(mCurves[i]->mValues.Get()[0],
                                    mCurves[i]->mColor, mCurves[i]->mFillAlpha, mCurves[i]);
                }
            
                DrawLineCurveSV(mCurves[i]->mValues.Get()[0],
                                mCurves[i]->mColor, mCurves[i]->mAlpha,
                                mCurves[i]->mLineWidth, mCurves[i]);
            }
        }
    }
}

void
GraphControl4::DrawLineCurve(const WDL_TypedBuf<double> *points, CurveColor color,
                            double alpha, double lineWidth,
                            GraphCurve *curve)
{
    int width = this->mRECT.W();
    int height = this->mRECT.H();
    
    nvgSave(mVg);
    nvgStrokeWidth(mVg, lineWidth);
    
    int sColor[4] = { (int)(color[0]*255), (int)(color[1]*255),
                      (int)(color[2]*255), (int)(alpha*255) };
    SWAP_COLOR(sColor);
    
    nvgStrokeColor(mVg, nvgRGBA(sColor[0], sColor[1], sColor[2], sColor[3]));
    nvgBeginPath(mVg);
    
    for (int i = 0; i < points->GetSize(); i ++)
    {
        // Normalize
        double x = ((double)i)/points->GetSize();
        
        if (mXdBScale)
            x = Utils::NormalizedXTodB(x, mMinXdB, mMaxXdB);
        
        // Scale for the interface
        x = x * width;
        
        double val = points->Get()[i];
        
        if (curve->mYdBScale)
            val = Utils::NormalizedYTodB(val, curve->mMinYdB, curve->mMaxYdB);
        
        double y = val * mAutoAdjustFactor * mYScaleFactor * height;
        
        if (i == 0)
            nvgMoveTo(mVg, x, y);
        
        nvgLineTo(mVg, x, y);
    }
    
    nvgStroke(mVg);
    nvgRestore(mVg);
    
    mDirty = true;
}

void
GraphControl4::DrawFillCurve(const WDL_TypedBuf<double> *points, CurveColor color, double fillAlpha,
                             double strokeAlpha, double lineWidth, GraphCurve *curve)
{
    // Offset used to draw the closing of the curve outside the viewport
    // Because we draw both stroke and fill at the same time
#define OFFSET lineWidth
    
    int width = this->mRECT.W();
    int height = this->mRECT.H();
    
    nvgSave(mVg);
    
    int sFillColor[4] = { (int)(color[0]*255), (int)(color[1]*255),
                      (int)(color[2]*255), (int)(fillAlpha*255) };
    SWAP_COLOR(sFillColor);
    
    int sStrokeColor[4] = { (int)(color[0]*255), (int)(color[1]*255),
        (int)(color[2]*255), (int)(strokeAlpha*255) };
    SWAP_COLOR(sStrokeColor);
    
    nvgBeginPath(mVg);
    
    double x0 = 0.0;
    for (int i = 0; i < points->GetSize(); i ++)
    {        
        // Normalize
        double x = ((double)i)/points->GetSize();
        
        if (mXdBScale)
            x = Utils::NormalizedXTodB(x, mMinXdB, mMaxXdB);
        
        // Scale for the interface
        x = x * width;
        
        double val = points->Get()[i];
        if (curve->mYdBScale)
            val = Utils::NormalizedYTodB(val, curve->mMinYdB, curve->mMaxYdB);
        
        double y = val * mAutoAdjustFactor * mYScaleFactor * height;
        
        if (i == 0)
        {
            x0 = x;
            
            nvgMoveTo(mVg, x0 - OFFSET, 0 - OFFSET);
            nvgLineTo(mVg, x - OFFSET, y);
        }
        
        nvgLineTo(mVg, x, y);
        
        if (i >= points->GetSize() - 1)
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
}

void
GraphControl4::DrawLineCurveSV(double val, CurveColor color, double alpha, double lineWidth,
                              GraphCurve *curve)
{
    int width = this->mRECT.W();
    int height = this->mRECT.H();
    
    nvgSave(mVg);
    nvgStrokeWidth(mVg, lineWidth);
    
    int sColor[4] = { (int)(color[0]*255), (int)(color[1]*255),
                      (int)(color[2]*255), (int)(alpha*255) };
    SWAP_COLOR(sColor);
    
    nvgStrokeColor(mVg, nvgRGBA(sColor[0], sColor[1], sColor[2], sColor[3]));
    
    nvgBeginPath(mVg);
    
    double x0 = 0;
    double x1 = width;
    
    if (curve->mYdBScale)
        val = Utils::NormalizedYTodB(val, curve->mMinYdB, curve->mMaxYdB);

    double y = val * mAutoAdjustFactor * height;
    
    nvgMoveTo(mVg, x0, y);
    nvgLineTo(mVg, x1, y);
    
    nvgStroke(mVg);
    nvgRestore(mVg);
    
    mDirty = true;
}

void
GraphControl4::DrawFillCurveSV(double val, CurveColor color, double alpha,
                              GraphCurve *curve)
{
    int width = this->mRECT.W();
    int height = this->mRECT.H();
    
    nvgSave(mVg);
    
    int sColor[4] = { (int)(color[0]*255), (int)(color[1]*255),
                      (int)(color[2]*255), (int)(alpha*255) };
    SWAP_COLOR(sColor);
    
    nvgStrokeColor(mVg, nvgRGBA(sColor[0], sColor[1], sColor[2], sColor[3]));
    
    nvgBeginPath(mVg);
    
    if (curve->mYdBScale)
        val = Utils::NormalizedYTodB(val, curve->mMinYdB, curve->mMaxYdB);
    
    double x0 = 0;
    double x1 = width;
    double y0 = 0;
    double y1 = val * mAutoAdjustFactor * height;
    
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
}

void
GraphControl4::AutoAdjust()
{
    // First, compute the maximul value of all the curves
    double max = -1e16;
    for (int i = 0; i < mNumCurves; i++)
    {
        GraphCurve *curve = mCurves[i];
        
        for (int j = curve->mValues.GetSize(); j >= 0; j--)
        {
            double val = curve->mValues.Get()[j];
            
            if (val > max)
                max = val;
        }
    }
    
    // Compute the scale factor
    
    // keep a margin, do not scale to the maximum
    double margin = 0.25;
    double factor = 1.0;
    
    if (max > 0.0)
         factor = (1.0 - margin)/max;
    
    // Then smooth the factor
    mAutoAdjustParamSmoother.SetNewValue(factor);
    mAutoAdjustParamSmoother.Update();
    factor = mAutoAdjustParamSmoother.GetCurrentValue();
    
    mAutoAdjustFactor = factor;
}

double
GraphControl4::MillisToPoints(long long int elapsed, int sampleRate, int numSamplesPoint)
{
    double numSamples = (((double)elapsed)/1000.0)*sampleRate;
    
    double numPoints = numSamples/numSamplesPoint;
    
    return numPoints;
}

void
GraphControl4::InitFont(const char *fontPath)
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
GraphControl4::DrawText(double x, double y, double fontSize,
                        const char *text, int color[4],
                        int halign)
{
    nvgSave(mVg);
    
    nvgFontSize(mVg, fontSize);
	nvgFontFace(mVg, "font");
    nvgFontBlur(mVg, 0);
	nvgTextAlign(mVg, halign | NVG_ALIGN_BOTTOM);
    
    int sColor[4] = { color[0], color[1], color[2], color[3] };
    SWAP_COLOR(sColor);
    
    nvgFillColor(mVg, nvgRGBA(sColor[0], sColor[1], sColor[2], sColor[3]));
    
	nvgText(mVg, x, y, text, NULL);
    
    nvgRestore(mVg);
}

void
GraphControl4::DrawGL()
{
    IPlugBase::IMutexLock lock(mPlug);
    
	// On Windows, we need lazy evaluation, because we need HInstance and IGraphics
	if (!mFontInitialized)
		InitFont(NULL);
    
    if (mLiceFb == NULL)
    {
// Take care of the following macro in Lice ! : DISABLE_LICE_EXTENSIONS
        mLiceFb = new LICE_GL_SysBitmap(0, 0);
        
        IRECT *r = GetRECT();
        mLiceFb->resize(r->W(), r->H());
    }

    // Bind the fbo
    //unsigned int fbo = ((LICE_GL_SysBitmap *)mLiceFb)->getFBO();
    //glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    
    int width = this->mRECT.W();
    int height = this->mRECT.H();
    
    // Set pixel ratio to 1.0
    // Otherwise, fonts will be blurry,
    // and lines too I guess...
    nvgBeginFrame(mVg, width, height, 1.0);
    
    // And clear with Lice, because BitBlt needs it.
    unsigned char clearColor[4] = { (int)(mClearColor[0]*255), (int)(mClearColor[1]*255),
                                    (int)(mClearColor[2]*255), (int)(mClearColor[3]*255) };
        
    // Warning, the colors are ARGB
    LICE_Clear(mLiceFb, *((LICE_pixel *)&clearColor));
    
    if (mAutoAdjustFlag)
    {
        AutoAdjust();
    }
    
    nvgBeginPath(mVg);
    
    int sFillColor[4] = { (int)(mCurves[0]->mColor[0]*255), 255/*(int)(mCurves[0]->mColor[1]*255)*/,
        (int)(mCurves[0]->mColor[2]*255), 255/*(int)(mCurves[0]->mFillAlpha*255)*/ };
    SWAP_COLOR(sFillColor);
    
#define COEFF 200.0
    
#if 1
    nvgMoveTo(mVg, 0.25*COEFF, 0.25*COEFF);
    nvgLineTo(mVg, 0.4*COEFF, 0.4*COEFF);
    nvgLineTo(mVg, 0.5*COEFF, 0.25*COEFF);
    nvgLineTo(mVg, 0.5*COEFF, 0.5*COEFF);
    nvgLineTo(mVg, 0.25*COEFF, 0.5*COEFF);
    nvgLineTo(mVg, 0.25*COEFF, 0.25*COEFF);
#endif
    
    // Reversed
#if 0
    nvgMoveTo(mVg, 0.25*COEFF, 0.25*COEFF);
    nvgLineTo(mVg, 0.25*COEFF, 0.25*COEFF);
    nvgLineTo(mVg, 0.25*COEFF, 0.5*COEFF);
    nvgLineTo(mVg, 0.5*COEFF, 0.5*COEFF);
    nvgLineTo(mVg, 0.5*COEFF, 0.25*COEFF);
    nvgLineTo(mVg, 0.4*COEFF, 0.4*COEFF);
#endif
    
    nvgClosePath(mVg);
    
    nvgFillColor(mVg, nvgRGBA(sFillColor[0], sFillColor[1], sFillColor[2], sFillColor[3]));
	nvgFill(mVg);
    
    nvgStrokeWidth(mVg, 2.0);
    nvgStrokeColor(mVg, nvgRGBA(sFillColor[1], sFillColor[0], sFillColor[2], sFillColor[3]));
    nvgStroke(mVg);
    
#if 0
    DrawAxis(true);
    
    nvgSave(mVg);
    
    DrawCurves();
    
    nvgRestore(mVg);
    
    DrawAxis(false);
#endif
    
    nvgEndFrame(mVg);
}

void
GraphControl4::InitGL()
{
    if (mGLInitialized)
        return;
    
    // "Init" GL
    GLContext2::Catch();
    
    GLContext2 *context = GLContext2::Get();
    context->Bind();

    mGLInitialized = true;
}

void
GraphControl4::ExitGL()
{
    GLContext2 *context = GLContext2::Get();
    
    context->Unbind();
    
    GLContext2::Release();
}

void
GraphControl4::InitNanoVg()
{
    GLContext2 *context = GLContext2::Get();
    context->Bind();
    
	mVg = nvgCreateGL2(NVG_ANTIALIAS | NVG_STENCIL_STROKES);
	if (mVg == NULL)
		return;
    
    InitFont(mFontPath.Get());
}

void
GraphControl4::ExitNanoVg()
{
    GLContext2 *context = GLContext2::Get();
    
	context->Bind();
    
    nvgDeleteGL2(mVg);
}
