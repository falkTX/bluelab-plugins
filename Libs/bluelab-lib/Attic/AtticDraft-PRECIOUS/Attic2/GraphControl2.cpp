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

#include "../../WDL/IPlug/IPlugBase.h"

// Plugin resource file
#include "resource.h"

#include "Utils.h"
#include "GLContext2.h"
#include "GraphControl2.h"

#define FONT_SIZE 14.0

GraphControl2::GraphControl2(IPlugBase *pPlug, IRECT pR,
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
    GLContext2::Catch();
    
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

	// "Init" GL
    GLContext2 *context = GLContext2::Get();
    context->Bind();
    
	mVg = nvgCreateGL2(NVG_ANTIALIAS | NVG_STENCIL_STROKES);
	if (mVg == NULL)
		return;
    
    mPixels = (unsigned char *)malloc(this->mRECT.W()*this->mRECT.H()*4*sizeof(unsigned char));
    
    SET_COLOR_FROM_INT(mClearColor, 0, 0, 0, 255);
    
    InitFont(fontPath);
    
    mDirty = true;
}
    
GraphControl2::~GraphControl2()
{
    GLContext2 *context = GLContext2::Get();
    context->Unbind();
    
    nvgDeleteGL2(mVg);
    
    free(mPixels);
    
    for (int i = 0; i < mNumCurves; i++)
        delete mCurves[i];
    
    if (mHAxis != NULL)
        delete mHAxis;
    
    if (mVAxis != NULL)
        delete mVAxis;
    
    GLContext2::Release();
}

bool
GraphControl2::IsDirty()
{
    return mDirty;
}

bool
GraphControl2::Draw(IGraphics *pGraphics)
{
    IPlugBase::IMutexLock lock(mPlug);
    
	// On Windows, we need lazy evaluation, because we need HInstance and IGraphics
	if (!mFontInitialized)
		InitFont(NULL);

    GLContext2 *context = GLContext2::Get();
    context->Bind();
    
    NVGLUframebuffer *fbo = nvgluCreateFramebuffer(mVg, this->mRECT.W(), this->mRECT.H(), NVG_IMAGE_NEAREST);
    
    nvgluBindFramebuffer(fbo);
    
    int width = this->mRECT.W();
    int height = this->mRECT.H();
    
    nvgImageSize(mVg, fbo->image, &width, &height);
    
    glViewport(0, 0, width, height);
    
    int rectWidth = this->GetRECT()->W();
    int rectHeight = this->GetRECT()->H();
    
    // Set pixel ratio to 1.0
    // Otherwise, fonts will be blurry,
    // and lines too I guess...
    nvgBeginFrame(mVg, rectWidth, rectHeight, 1.0);
    
	Clear();

    if (mAutoAdjustFlag)
    {
        AutoAdjust();
    }
    
    // Debug: commented everything else
    // Note: axis only draw lines , wthout text
    // And it crackles...
    DrawAxis(true);
    
    //nvgSave(mVg);
    
    //DrawCurves();
    
    //nvgRestore(mVg);
    
    //DrawAxis(false);
    
    nvgEndFrame(mVg);
    
    glReadPixels(0, 0, this->mRECT.W(), this->mRECT.H(), GL_BGRA, GL_UNSIGNED_BYTE, (GLubyte *)mPixels);
    
    nvgluBindFramebuffer(NULL);

    nvgluDeleteFramebuffer(fbo);
    
#ifdef __APPLE__
    SwapChannels((unsigned int *)mPixels, this->mRECT.W()*this->mRECT.H());
#endif
    
    // This is the important part where you bind the cairo data to LICE
    LICE_WrapperBitmap wrapperBitmap((LICE_pixel *)mPixels, this->mRECT.W(), this->mRECT.H(),
                                     this->mRECT.W(), false);
    
    // And we render
    IBitmap result(&wrapperBitmap, wrapperBitmap.getWidth(), wrapperBitmap.getHeight());
    
    context->Unbind();
    
    mDirty = false;
    
    return pGraphics->DrawBitmap(&result, &this->mRECT);
}

void
GraphControl2::AddHAxis(char *data[][2], int numData, int axisColor[4], int axisLabelColor[4])
{
    mHAxis = new GraphAxis();
    mHAxis->mOffset = 0.0;
    
    AddAxis(mHAxis, data, numData, axisColor, axisLabelColor);
}

void
GraphControl2::AddVAxis(char *data[][2], int numData, int axisColor[4], int axisLabelColor[4], double offset)
{
    mVAxis = new GraphAxis();
    
    mVAxis->mOffset = offset;
    
    AddAxis(mVAxis, data, numData, axisColor, axisLabelColor);
}

void
GraphControl2::SetXdBScale(bool flag, double minXdB, double maxXdB)
{
    mXdBScale = flag;
    
    mMinXdB = minXdB;
    mMaxXdB = maxXdB;
}

void
GraphControl2::SetAutoAdjust(bool flag, double smoothCoeff)
{
    mAutoAdjustFlag = flag;
    
    mAutoAdjustParamSmoother.SetSmoothCoeff(smoothCoeff);
    
    mDirty = true;
}

void
GraphControl2::SetYScaleFactor(double factor)
{
    mYScaleFactor = factor;
    
    mDirty = true;
}

void
GraphControl2::SetClearColor(int r, int g, int b, int a)
{
    SET_COLOR_FROM_INT(mClearColor, r, g, b, a);
    
    mDirty = true;
}

void
GraphControl2::SetCurveColor(int curveNum, int r, int g, int b)
{
    if (curveNum >= mNumCurves)
        return;
    
    SET_COLOR_FROM_INT(mCurves[curveNum]->mColor, r, g, b, 255);
    
    mDirty = true;
}

void
GraphControl2::SetCurveAlpha(int curveNum, double alpha)
{
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->mAlpha = alpha;
    
    mDirty = true;
}


void
GraphControl2::SetCurveLineWidth(int curveNum, double lineWidth)
{
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->mLineWidth = lineWidth;
    
    mDirty = true;
}

void
GraphControl2::SetCurveSmooth(int curveNum, bool flag)
{
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->mDoSmooth = flag;
    
    mDirty = true;
}

void
GraphControl2::SetCurveFill(int curveNum, bool flag)
{
    if (curveNum >= mNumCurves)
        return;

    mCurves[curveNum]->mCurveFill = flag;
    
    mDirty = true;
}

void
GraphControl2::SetCurveFillAlpha(int curveNum, double alpha)
{
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->mFillAlpha = alpha;
    
    mDirty = true;
}

void
GraphControl2::SetCurveSingleValue(int curveNum, bool flag)
{
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->mSingleValue = flag;
    
    mDirty = true;
}

void
GraphControl2::AddAxis(GraphAxis *axis, char *data[][2], int numData, int axisColor[4], int axisLabelColor[4])
{
    // Copy color
    for (int i = 0; i < 4; i++)
    {
        axis->mColor[i] = axisColor[i];
        axis->mLabelColor[i] = axisLabelColor[i];
    }
    
    // Copy data
    for (int i = 0; i < numData; i++)
    {
        char *cData[2] = { data[i][0], data[i][1] };
        
        double t = atof(cData[0]);
        
        string text(cData[1]);
        
        GraphAxisData aData;
        aData.mT = (t - mMinXdB)/(mMaxXdB - mMinXdB);
        aData.mText = text;
        
        axis->mValues.push_back(aData);
    }
}


void
GraphControl2::Clear()
{    
    glClearColor(mClearColor[0], mClearColor[1], mClearColor[2], mClearColor[3]);
    
    glClear(GL_COLOR_BUFFER_BIT|GL_STENCIL_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    
    // mDirty = true;
}

void
GraphControl2::ResetCurve(int curveNum, double val)
{
    IPlugBase::IMutexLock lock(mPlug);
    
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->FillAllValues(val);
    
    mDirty = true;
}

void
GraphControl2::SetCurveYdBScale(int curveNum, bool flag, double minYdB, double maxYdB)
{
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->SetYdBScale(flag, minYdB, maxYdB);
    
    mDirty = true;
}

void
GraphControl2::SetCurveValues(int curveNum, const WDL_TypedBuf<double> *values)
{
    // Must lock otherwise with DATA_STEP, curve will jerk
    IPlugBase::IMutexLock lock(mPlug);
    
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->SetValues(values);
    
    mDirty = true;
}

void
GraphControl2::SetCurveValue(int curveNum, double t, double val)
{
    // Must lock otherwise we may have curve will jerk (??)
    IPlugBase::IMutexLock lock(mPlug);
    
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->SetValue(t, val);
    
    mDirty = true;
}

void
GraphControl2::SetCurveSingleValue(int curveNum, double val)
{
    // Must lock otherwise with DATA_STEP, curve will jerk
    IPlugBase::IMutexLock lock(mPlug);
    
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->SetValue(0.0, val);
    
    mDirty = true;
}


void
GraphControl2::PushCurveValue(int curveNum, double val)
{
    IPlugBase::IMutexLock lock(mPlug);
    
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->PushValue(val);
    mDirty = true;
}

void
GraphControl2::DrawAxis(bool lineLabelFlag)
{
    if (mHAxis != NULL)
        DrawAxis(mHAxis, true, lineLabelFlag);
    
    if (mVAxis != NULL)
        DrawAxis(mVAxis, false, lineLabelFlag);
}


void
GraphControl2::DrawAxis(GraphAxis *axis, bool horizontal, bool lineLabelFlag)
{
    nvgSave(mVg);
    nvgStrokeWidth(mVg, 1.0);
    nvgStrokeColor(mVg,
                   nvgRGBA(axis->mColor[0], axis->mColor[1],
                           axis->mColor[2], axis->mColor[3]));
    
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
            
            // Retreive the Y db scale
            double minYdB = -40.0;
            double maxYdB = 40.0;
            
            if (!mCurves.empty())
                // Get from the first curve
            {
                minYdB = mCurves[0]->mMinYdB;
                maxYdB = mCurves[0]->mMaxYdB;
            }
            
            t = (t - minYdB)/(maxYdB - minYdB);
            
            double y = t*height;
            
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
GraphControl2::DrawCurves()
{
    for (int i = 0; i < mNumCurves; i++)
    {
        if (!mCurves[i]->mSingleValue)
        {
            if (mCurves[i]->mCurveFill)
            {
                DrawFillCurve(&mCurves[i]->mValues,
                               mCurves[i]->mColor, mCurves[i]->mFillAlpha, mCurves[i]);
            }
            
            DrawLineCurve(&mCurves[i]->mValues,
                           mCurves[i]->mColor, mCurves[i]->mAlpha, mCurves[i]->mLineWidth, mCurves[i]);
        }
        else
        {
            if (!mCurves[i]->mValues.GetSize() == 0)
            {
                if (mCurves[i]->mCurveFill)
                {
                    DrawFillCurveSV(mCurves[i]->mValues.Get()[0], //mCurves[i]->mValues.GetSize()-1
                                    mCurves[i]->mColor, mCurves[i]->mFillAlpha, mCurves[i]);
                }
            
                DrawLineCurveSV(mCurves[i]->mValues.Get()[0], // mCurves[i]->mValues.GetSize()-1
                                mCurves[i]->mColor, mCurves[i]->mAlpha,
                                mCurves[i]->mLineWidth, mCurves[i]);
            }
        }
    }
}

void
GraphControl2::DrawLineCurve(const WDL_TypedBuf<double> *points, CurveColor color,
                            double alpha, double lineWidth,
                            GraphCurve *curve)
{
    int width = this->mRECT.W();
    int height = this->mRECT.H();
    
    nvgSave(mVg);
    nvgStrokeWidth(mVg, lineWidth);
    nvgStrokeColor(mVg, nvgRGBA(color[0]*255, color[1]*255, color[2]*255, alpha*255));
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
GraphControl2::DrawFillCurve(const WDL_TypedBuf<double> *points, CurveColor color, double alpha,
                            GraphCurve *curve)
{
    int width = this->mRECT.W();
    int height = this->mRECT.H();
    
    nvgSave(mVg);
    nvgStrokeColor(mVg, nvgRGBA(color[0]*255, color[1]*255, color[2]*255, alpha*255));
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
            
            nvgMoveTo(mVg, x0, 0);
        }
        
        nvgLineTo(mVg, x, y);
        
        if (i >= points->GetSize() - 1)
            // Close
        {
            nvgLineTo(mVg, x, 0);
            
            nvgClosePath(mVg);
        }
    }
    
    nvgFillColor(mVg, nvgRGBA(color[0]*255, color[1]*255, color[2]*255, alpha*255));
	nvgFill(mVg);
    
    nvgRestore(mVg);
    
    mDirty = true;
}

void
GraphControl2::DrawLineCurveSV(double val, CurveColor color, double alpha, double lineWidth,
                              GraphCurve *curve)
{
    int width = this->mRECT.W();
    int height = this->mRECT.H();
    
    nvgSave(mVg);
    nvgStrokeWidth(mVg, lineWidth);
    nvgStrokeColor(mVg, nvgRGBA(color[0]*255, color[1]*255, color[2]*255, alpha*255));
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
GraphControl2::DrawFillCurveSV(double val, CurveColor color, double alpha,
                              GraphCurve *curve)
{
    int width = this->mRECT.W();
    int height = this->mRECT.H();
    
    nvgSave(mVg);
    nvgStrokeColor(mVg, nvgRGBA(color[0]*255, color[1]*255, color[2]*255, alpha*255));
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
    
    nvgFillColor(mVg, nvgRGBA(color[0]*255, color[1]*255, color[2]*255, alpha*255));
	nvgFill(mVg);
    
    nvgStroke(mVg);
    nvgRestore(mVg);
    
    mDirty = true;
}

void
GraphControl2::AutoAdjust()
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
GraphControl2::MillisToPoints(long long int elapsed, int sampleRate, int numSamplesPoint)
{
    double numSamples = (((double)elapsed)/1000.0)*sampleRate;
    
    double numPoints = numSamples/numSamplesPoint;
    
    return numPoints;
}

void
GraphControl2::InitFont(const char *fontPath)
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
GraphControl2::DrawText(double x, double y, double fontSize,
                        const char *text, int color[4],
                        int halign)
{
    nvgSave(mVg);
    
    nvgFontSize(mVg, fontSize);
	nvgFontFace(mVg, "font");
    nvgFontBlur(mVg, 0);
	nvgTextAlign(mVg, halign | NVG_ALIGN_BOTTOM);
	nvgFillColor(mVg, nvgRGBA(color[0], color[1], color[2], color[3]));
	nvgText(mVg, x, y, text, NULL);
    
    nvgRestore(mVg);
}

void
GraphControl2::SwapChannels(unsigned int *data, int numPixels)
{
    for (int i = 0; i < numPixels; i++)
    {
        unsigned int srcPixel = data[i];
        
        unsigned char *srcRGBA = (unsigned char *)&srcPixel;
        
        // Swap
        unsigned char dstRGBA[4] = { srcRGBA[3], srcRGBA[2], srcRGBA[1], srcRGBA[0] };
        
        unsigned int dstPixel = *(unsigned int *)dstRGBA;
        
        data[i] = dstPixel;
    }
}
