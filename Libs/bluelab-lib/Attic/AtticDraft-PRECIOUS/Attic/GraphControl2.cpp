//
//  Graph.cpp
//  Transient
//
//  Created by Apple m'a Tuer on 03/09/17.
//
//

#include <math.h>

#define NANOVG_GLEW

#include <stdio.h>
#ifdef NANOVG_GLEW
#  include <GL/glew.h>
#endif
#define GLFW_INCLUDE_GLEXT
#include <GLFW/glfw3.h>
#include "nanovg.h"
#define NANOVG_GL2_IMPLEMENTATION

#include "nanovg_gl.h"
#include "nanovg_gl_utils.h"

#include "../../WDL/IPlug/IPlugBase.h"

#include "Utils.h"
#include "GraphControl2.h"


#define  FONT_SIZE 14.0


GraphControl2::GraphControl2(IPlugBase *pPlug, IRECT pR,
                            int numCurves, int numCurveValues,
                            const char *fontPath)
: IControl(pPlug, pR),
  mAutoAdjustParamSmoother(1.0, 0.9),
  mNumCurves(numCurves),
  mNumCurveValues(numCurveValues)
  //mSynchronizedGraph(NULL)
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
    
    InitGL();
    
    mVg = nvgCreateGL2(NVG_ANTIALIAS | NVG_STENCIL_STROKES);
    mFb = nvgluCreateFramebuffer(mVg, this->mRECT.W(), this->mRECT.H(), NVG_IMAGE_NEAREST);
    
    mPixels = (unsigned char *)malloc(this->mRECT.W()*this->mRECT.H()*4*sizeof(unsigned char));
    
    SET_COLOR_FROM_INT(mClearColor, 0, 0, 0, 255);
    
    InitFont(fontPath);
    
    mTranslateX = 0.0;
    
    mDirty = true;
}
    
GraphControl2::~GraphControl2()
{
    nvgluDeleteFramebuffer(mFb);
    nvgDeleteGL2(mVg);
    
    free(mPixels);
    
    DestroyContext();
    
    for (int i = 0; i < mNumCurves; i++)
        delete mCurves[i];
}

bool
GraphControl2::IsDirty()
{
    return mDirty;
}

bool
GraphControl2::Draw(IGraphics *pGraphics)
{
    //fprintf(stderr, "draw\n");
    
    //IPlugBase::IMutexLock lock(mPlug);
 
    // Synchronize two graphs
    //if ((mSynchronizedGraph != NULL) && !mSynchronizedGraph->IsDirty())
    //    return false;
    
    //MutexLock thisLock(this);
    //MutexLock graphLock(mSynchronizedGraph);
    
    SetContext();
    
    nvgluBindFramebuffer(mFb);
    
    int fboWidth;
    int fboHeight;
    
    nvgImageSize(mVg, mFb->image, &fboWidth, &fboHeight);
    
    glViewport(0, 0, fboWidth, fboHeight);
    
    Clear();
    
    int rectWidth = this->GetRECT()->W();
    int rectHeight = this->GetRECT()->H();
    
    // Set pixel ratio to 1.0
    // Otherwise, fonts will be blurry,
    // and lines too I guess...
    nvgBeginFrame(mVg, rectWidth, rectHeight, 1.0);
    
    if (mAutoAdjustFlag)
    {
        AutoAdjust();
    }
    
    DrawAxis();
    
    
    if (mSampleRate > 0)
    {
        unsigned long int elapsed = mTimer.Get();
        mTimer.Reset();
    
        double points = MillisToPoints(elapsed, mSampleRate, mNumPointSamples);
    
        fprintf(stderr, "elapsed: %g\n", points);
    }
    
    //double trans = (1.0/mCurves[0]->mValues.GetSize());
    //trans *= width;
    
    //int width = this->mRECT.W();
    
    // One pixel
    //double trans = 1.0/width;
    double onePoint = 1.0/((double)mNumCurveValues);
    double trans = onePoint;
    
//TODO: count points, then points mess, then transalation
    //while (mTranslateX > 0.0)
    //    mTranslateX -= trans;
    
    //while (mTranslateX < 0.0)
    //    mTranslateX += trans;
    
    //fprintf(stderr, "transx: %g\n", mTranslateX);
    
    nvgTranslate(mVg, mTranslateX, 0.0);
    
    DrawCurves();
    
    nvgEndFrame(mVg);
    
    glReadPixels(0, 0, this->mRECT.W(), this->mRECT.H(), GL_BGRA, GL_UNSIGNED_BYTE, (GLubyte *)mPixels);
    
    nvgluBindFramebuffer(NULL);
    
    SwapChannels((unsigned int *)mPixels, this->mRECT.W()*this->mRECT.H());
    
    // This is the important part where you bind the cairo data to LICE
    LICE_WrapperBitmap wrapperBitmap((LICE_pixel *)mPixels, this->mRECT.W(), this->mRECT.H(),
                                     this->mRECT.W(), false);
    
    // And we render
    IBitmap result(&wrapperBitmap, wrapperBitmap.getWidth(), wrapperBitmap.getHeight());
    
    
    RestoreContext();
    
    return pGraphics->DrawBitmap(&result, &this->mRECT);
}

void
GraphControl2::SynchrnonizeDraw(GraphControl2 *other)
{
   // mSynchronizedGraph = other;
}

void
GraphControl2::AddHAxis(char *data[][2], int numData, int axisColor[4], int axisLabelColor[4])
{
    // Copy color
    for (int i = 0; i < 4; i++)
    {
        mAxis.mColor[i] = axisColor[i];
        mAxis.mLabelColor[i] = axisLabelColor[i];
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
        
        mAxis.mValues.push_back(aData);
    }
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
GraphControl2::Clear()
{    
    glClearColor(mClearColor[0], mClearColor[1], mClearColor[1], mClearColor[3]);
    glClear(GL_COLOR_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
    
    mDirty = true;
}

void
GraphControl2::ResetCurve(int curveNum, double val)
{
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
    // Must lock otherwise with DATA_STEP, curve will jerk
    IPlugBase::IMutexLock lock(mPlug);
    
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->SetValue(t, val);
    
    mDirty = true;
}

void
GraphControl2::PushCurveValue(int curveNum, double val)
{
    // Must lock otherwise with DATA_STEP, curve will jerk
    //IPlugBase::IMutexLock lock(mPlug);
    
    //fprintf(stderr, "push point\n");
    
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->PushValue(val);
    
    if (curveNum == 0)
    {
        //int width = this->mRECT.W();
        
        //double onePix = 1.0/width;
        double onePoint = 1.0/((double)mNumCurveValues);
        
        double trans = onePoint;//*onePix;
        
        mTranslateX += trans;
    }
    
    mDirty = true;
}

void
GraphControl2::DrawAxis()
{
    nvgSave(mVg);
    nvgStrokeWidth(mVg, 1.0);
    nvgStrokeColor(mVg,
                   nvgRGBA(mAxis.mColor[0], mAxis.mColor[1],
                           mAxis.mColor[2], mAxis.mColor[3]));
    
    int width = this->mRECT.W();
    int height = this->mRECT.H();
    
    double textOffset = FONT_SIZE*0.2;
    
    for (int i = 0; i < mAxis.mValues.size(); i++)
    {
        const GraphAxisData &data = mAxis.mValues[i];
        
        double t = data.mT;
        const char *text = data.mText.c_str();
        
        t = Utils::NormalizedXTodB(t, mMinXdB, mMaxXdB);

        double x = t*width;
        
        if ((i > 0) && (i < mAxis.mValues.size() - 1))
            // First and last: don't draw axis line
        {
            // Draw a vertical line
            nvgBeginPath(mVg);

            double y0 = 0.0;
            double y1 = height;
        
            nvgMoveTo(mVg, x, y0);
            nvgLineTo(mVg, x, y1);
    
            nvgStroke(mVg);
            
            DrawText(x, textOffset, FONT_SIZE, text, mAxis.mLabelColor, NVG_ALIGN_CENTER);
        }
     
        if (i == 0)
            // First text: aligne left
            DrawText(x + textOffset, textOffset, FONT_SIZE, text, mAxis.mLabelColor, NVG_ALIGN_LEFT);
        
        if (i == mAxis.mValues.size() - 1)
            // Last text: aligne right
            DrawText(x - textOffset, textOffset, FONT_SIZE, text, mAxis.mLabelColor, NVG_ALIGN_RIGHT);
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

void
GraphControl2::InitFont(const char *fontPath)
{
    nvgCreateFont(mVg, "font", fontPath);
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

#if 0
void
GraphControl2::DebugPrintAxisValues(int numValues)
{
    for (int i = 0; i < numValues; i++)
    {
        double x = ((double)i)/(numValues - 1);
        
        double xdB = Utils::NormalizedXTodBInv(x, mMinXdB, mMaxXdB);
        
        fprintf(stderr, "%g %g  ", x, xdB);
    }
    fprintf(stderr, "\n");
}
#endif

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

void
GraphControl2::InitGL()
{
    if (!glfwInit())
		return;
    
#ifndef _WIN32 // don't require this on win32, and works with more cards
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif
	//glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, 1);
    
#ifdef DEMO_MSAA
	glfwWindowHint(GLFW_SAMPLES, 4);
#endif
	    
    CreateContext();
    
#ifdef NANOVG_GLEW
	if(glewInit() != GLEW_OK)
		return;
	
	// GLEW generates GL error because it calls glGetString(GL_EXTENSIONS), we'll consume it here.
	glGetError();
#endif
}

#ifdef __APPLE__ 
long
GraphControl2::CreateContext()
{
    const GLubyte *glstring;
    
    GLint npix;
    CGLPixelFormatObj PixelFormat;
    
    const CGLPixelFormatAttribute attributes[] =
    {
        //kCGLPFAOffScreen,
        //      kCGLPFAColorSize, (CGLPixelFormatAttribute)8,
        //      kCGLPFADepthSize, (CGLPixelFormatAttribute)16,
        kCGLPFAAccelerated, (CGLPixelFormatAttribute)0
    };
    
    // Create context if none exists
    CGLChoosePixelFormat(attributes, &PixelFormat, &npix);
    
    if (PixelFormat == NULL)
    {
        return 0;
    }
    
    CGLCreateContext(PixelFormat, NULL, (CGLContextObj *)&mGLContext);
    
    if (mGLContext == NULL)
    {
        return 0;
    }
    
    // Set the current context
    
    if (SetContext())
        return 0;
    
    // Check OpenGL functionality:
    glstring = glGetString(GL_EXTENSIONS);
    
    if(!gluCheckExtension((const GLubyte *)"GL_EXT_framebuffer_object", glstring))
    {
        return 0;
    }
    
    return 0;
}

void
GraphControl2::DestroyContext()
{
    if (mGLContext != NULL)
        CGLDestroyContext((CGLContextObj)mGLContext);
}

long
GraphControl2::SetContext()
{
    // Set the current context
    if (CGLSetCurrentContext((CGLContextObj)mGLContext))
    {
        return 1;
    }
    
    return 0;
}

void
GraphControl2::RestoreContext()
{
    CGLSetCurrentContext(NULL);
}
#else
NOT IMPLEMETED ON WINDOWS...
#endif
