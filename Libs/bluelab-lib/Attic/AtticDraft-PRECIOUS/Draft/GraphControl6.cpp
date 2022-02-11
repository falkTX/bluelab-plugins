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

// ADDED NIKO
#include <GL/glew.h>

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
//#include "GLContext2.h"
#include "GLContext.h"
#include "GraphControl6.h"
#include "Debug.h"

#define FILL_CURVE_HACK 1

#define CURVE_DEBUG 0

// Good, but misses some maxima
#define CURVE_OPTIM      0
#define CURVE_OPTIM_THRS 2048

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

GraphControl6::GraphControl6(IPlugBase *pPlug, IRECT pR, int paramIdx,
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

	mXdBScale = false;
	//mMinX = 0.1;
    mMinX = 0.0;
	mMaxX = 1.0;
    
    SetClearColor(0, 0, 0, 255);
    
    mDirty = true;
    
    // No need since this is static
    //mGLInitialized = false;
    
    mFontPath.Set(fontPath);

    // TEST NIKO
#if 0
    // No need anymore since this is static
    InitGL();
#endif
    
    mGLContext = NULL;
    
    InitNanoVg();
    
    mLiceFb = NULL;
    
#if PROFILE_GRAPH
    mDebugCount = 0;
#endif
}

GraphControl6::~GraphControl6()
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
    
    // TEST NIKO
#if 0
    // No need anymore since this is static
    ExitGL();
#endif
    
    DestroyCGLContext();
}

void
GraphControl6::ResetNumCurveValues(int numCurveValues)
{
    mNumCurveValues = numCurveValues;
    
    for (int i = 0; i < mNumCurves; i++)
	{
		GraphCurve4 *curve = mCurves[i];
        
		curve->ResetNumValues(mNumCurveValues);
	}
}

int
GraphControl6::GetNumCurveValues()
{
    return mNumCurveValues;
}

bool
GraphControl6::IsDirty()
{
    return mDirty;
}

const LICE_IBitmap *
GraphControl6::DrawGL()
{
#if PROFILE_GRAPH
    mDebugTimer.Start();
#endif
    
    IPlugBase::IMutexLock lock(mPlug);
    
#if 0 // TEST NIKO ...
    //
    //GLContext2 *context = GLContext2::Get();
    GLContext *context = GLContext::Get();
    context->Bind();
#endif
    
    //nvgluBindFramebuffer(mFb);
    
    BindCGLContext();
    
    DrawGraph();
    
    UnBindCGLContext();
    
    mDirty = false;
    
#if PROFILE_GRAPH
    mDebugTimer.Stop();
    
    if (mDebugCount++ % 100 == 0)
    {
        long t = mDebugTimer.Get();
        mDebugTimer.Reset();
        
        fprintf(stderr, "GraphControl6 - profile: %ld\n", t);
    }
#endif
    
    //nvgluBindFramebuffer(NULL);
    
    return mLiceFb;
}

void
GraphControl6::AddHAxis(char *data[][2], int numData, int axisColor[4], int axisLabelColor[4])
{
    mHAxis = new GraphAxis();
    mHAxis->mOffset = 0.0;
    
    AddAxis(mHAxis, data, numData, axisColor, axisLabelColor, mMinX, mMaxX);
}

void
GraphControl6::ReplaceHAxis(char *data[][2], int numData)
{
    int axisColor[4];
    int axisLabelColor[4];
    
    if (mHAxis != NULL)
    {
        for (int i = 0; i < 4; i++)
            axisColor[i] = mHAxis->mColor[i];
        
        for (int i = 0; i < 4; i++)
            axisLabelColor[i] = mHAxis->mLabelColor[i];
        
        delete mHAxis;
    }
    
    AddHAxis(data, numData, axisColor, axisLabelColor);
}

void
GraphControl6::AddVAxis(char *data[][2], int numData, int axisColor[4], int axisLabelColor[4], double offset)
{
    mVAxis = new GraphAxis();
    
    mVAxis->mOffset = offset;
    
    // Retreive the Y db scale
    double minY = -40.0;
    double maxY = 40.0;
    
    if (!mCurves.empty())
        // Get from the first curve
    {
        minY = mCurves[0]->mMinY;
        maxY = mCurves[0]->mMaxY;
    }
    
    AddAxis(mVAxis, data, numData, axisColor, axisLabelColor, minY, maxY);
}

void
GraphControl6::SetXScale(bool dbFlag, double minX, double maxX)
{
    mXdBScale = dbFlag;
    
    mMinX = minX;
    mMaxX = maxX;
}

void
GraphControl6::SetAutoAdjust(bool flag, double smoothCoeff)
{
    mAutoAdjustFlag = flag;
    
    mAutoAdjustParamSmoother.SetSmoothCoeff(smoothCoeff);
    
    mDirty = true;
}

void
GraphControl6::SetYScaleFactor(double factor)
{
    mYScaleFactor = factor;
    
    mDirty = true;
}

void
GraphControl6::SetClearColor(int r, int g, int b, int a)
{
    SET_COLOR_FROM_INT(mClearColor, r, g, b, a);
    
    mDirty = true;
}

void
GraphControl6::SetCurveColor(int curveNum, int r, int g, int b)
{
    if (curveNum >= mNumCurves)
        return;
    
    SET_COLOR_FROM_INT(mCurves[curveNum]->mColor, r, g, b, 255);
    
    mDirty = true;
}

void
GraphControl6::SetCurveAlpha(int curveNum, double alpha)
{
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->mAlpha = alpha;
    
    mDirty = true;
}

void
GraphControl6::SetCurveLineWidth(int curveNum, double lineWidth)
{
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->mLineWidth = lineWidth;
    
    mDirty = true;
}

void
GraphControl6::SetCurveSmooth(int curveNum, bool flag)
{
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->mDoSmooth = flag;
    
    mDirty = true;
}

void
GraphControl6::SetCurveFill(int curveNum, bool flag)
{
    if (curveNum >= mNumCurves)
        return;

    mCurves[curveNum]->mCurveFill = flag;
    
    mDirty = true;
}

void
GraphControl6::SetCurveFillAlpha(int curveNum, double alpha)
{
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->mFillAlpha = alpha;
    
    mDirty = true;
}

void
GraphControl6::SetCurveSingleValueH(int curveNum, bool flag)
{
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->mSingleValueH = flag;
    
    mDirty = true;
}

void
GraphControl6::SetCurveSingleValueV(int curveNum, bool flag)
{
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->mSingleValueV = flag;
    
    mDirty = true;
}


void
GraphControl6::InitGL()
{
#if 0 // TEST ...
    // TEST NIKO
    //if (mGLInitialized)
    //    return;
    
#if 0 // ORIGIN
    GLContext2::Catch();
    GLContext2 *context = GLContext2::Get();
#else
    GLContext::Ref();
    GLContext *context = GLContext::Get();
#endif

    context->Bind();
    
    //mGLInitialized = true;
    
#endif
}

void
GraphControl6::ExitGL()
{
#if 0 // TEST NIKO
    
#if 0 // ORIGIN
    GLContext2 *context = GLContext2::Get();
    context->Unbind();
    GLContext2::Release();
#else
    GLContext *context = GLContext::Get();
    context->Unbind();
    GLContext::Unref();
#endif
    
    //mGLInitialized = false;
#endif
}

void
GraphControl6::DisplayCurveDescriptions()
{
#define DESCR_X 40.0
#define DESCR_Y0 10.0
    
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
        
        double y = this->mRECT.H() - (DESCR_Y0 + descrNum*DESCR_Y_STEP);
        
        nvgSave(mVg);
        
        // Must force alpha to 1, because sometimes,
        // the plugins hide the curves, but we still
        // want to display the description
        double prevAlpha = curve->mAlpha;
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
GraphControl6::AddAxis(GraphAxis *axis, char *data[][2], int numData, int axisColor[4],
                       int axisLabelColor[4],
                       double minVal, double maxVal)
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
        aData.mT = (t - minVal)/(maxVal - minVal);
        aData.mText = text;
        
        axis->mValues.push_back(aData);
    }
}

void
GraphControl6::SetCurveDescription(int curveNum, const char *description, int descrColor[4])
{
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->SetDescription(description, descrColor);
}

void
GraphControl6::ResetCurve(int curveNum, double val)
{
    IPlugBase::IMutexLock lock(mPlug);
    
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->FillAllXValues(mMinX, mMaxX);
    mCurves[curveNum]->FillAllYValues(val);
    
    mDirty = true;
}

void
GraphControl6::SetCurveYScale(int curveNum, bool dbFlag, double minY, double maxY)
{
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->SetYScale(dbFlag, minY, maxY);
    
    mDirty = true;
}

void
GraphControl6::CurveFillAllValues(int curveNum, double val)
{
    // Must lock otherwise with DATA_STEP, curve will jerk
    IPlugBase::IMutexLock lock(mPlug);
    
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->FillAllXValues(mMinX, mMaxX);
    mCurves[curveNum]->FillAllYValues(val);
    
    mDirty = true;
}

void
GraphControl6::SetCurveValues(int curveNum, const WDL_TypedBuf<double> *values)
{
    // Must lock otherwise with DATA_STEP, curve will jerk
    IPlugBase::IMutexLock lock(mPlug);
    
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->SetYValues(values, mMinX, mMaxX);
    
    mDirty = true;
}

void
GraphControl6::SetCurveValues2(int curveNum, const WDL_TypedBuf<double> *values)
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
        double t = ((double)i)/(values->GetSize() - 1);
        double y = values->Get()[i];
        
        SetCurveValue(curveNum, t, y);
    }
    
    mDirty = true;
}

void
GraphControl6::SetCurveValuesDecimate(int curveNum,
                                      const WDL_TypedBuf<double> *values,
                                      bool isWaveSignal)
{    
    // Must lock otherwise with DATA_STEP, curve will jerk
    IPlugBase::IMutexLock lock(mPlug);
    
    if (curveNum >= mNumCurves)
        return;
    
    GraphCurve4 *curve = mCurves[curveNum];
    
    int width = this->mRECT.W();
    
    double prevX = -1.0;
    double maxY = -1.0;
    
    if (isWaveSignal)
        maxY = (curve->mMinY + curve->mMaxY)/2.0;
        
    double thrs = 1.0/GRAPHCONTROL_PIXEL_DENSITY;
    for (int i = 0; i < values->GetSize(); i++)
    {
        double t = ((double)i)/(values->GetSize() - 1);
        
        double x = t*width;
        double y = values->Get()[i];
        
        // Keep the maximum
        // (we prefer keeping the maxima, and not discard them)
        if (!isWaveSignal)
        {
            if (fabs(y) > maxY)
                maxY = y;
        }
        else
        {
            if (fabs(y) > fabs(maxY))
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
}

void
GraphControl6::SetCurveValue(int curveNum, double t, double val)
{
    // Must lock otherwise we may have curve will jerk (??)
    IPlugBase::IMutexLock lock(mPlug);
    
    if (curveNum >= mNumCurves)
        return;
    
    GraphCurve4 *curve = mCurves[curveNum];
    
    // Normalize, then adapt to the graph
    int width = this->mRECT.W();
    int height = this->mRECT.H();
    
    double x = t;
    
    if (x != GRAPH_VALUE_UNDEFINED)
    {
        if (mXdBScale)
            x = Utils::NormalizedXTodB(x, mMinX, mMaxX);
        
        // X should be already normalize in input
        //else
        //    x = (x - mMinX)/(mMaxX - mMinX);
    
        // Scale for the interface
        x = x * width;
    }
    
    double y = ConvertY(curve, val, height);
                 
    curve->SetValue(t, x, y);
    
    mDirty = true;
}

void
GraphControl6::SetCurveSingleValueH(int curveNum, double val)
{
    // Must lock otherwise with DATA_STEP, curve will jerk
    IPlugBase::IMutexLock lock(mPlug);
    
    if (curveNum >= mNumCurves)
        return;
    
    //mCurves[curveNum]->SetValue(0.0, 0.0, val);
    SetCurveValue(curveNum, 0.0, val);
    
    mDirty = true;
}

void
GraphControl6::SetCurveSingleValueV(int curveNum, double val)
{
    // Must lock otherwise with DATA_STEP, curve will jerk
    IPlugBase::IMutexLock lock(mPlug);
    
    if (curveNum >= mNumCurves)
        return;
    
    //mCurves[curveNum]->SetValue(0.0, 0.0, val);
    SetCurveValue(curveNum, 0.0, val);
    
    mDirty = true;
}

void
GraphControl6::PushCurveValue(int curveNum, double val)
{
    IPlugBase::IMutexLock lock(mPlug);
    
    if (curveNum >= mNumCurves)
        return;
    
    int height = this->mRECT.H();
    
    GraphCurve4 *curve = mCurves[curveNum];
    val = ConvertY(curve, val, height);
    
    double dummyX = 1.0;
    curve->PushValue(dummyX, val);
    
    double maxXValue = this->mRECT.W();
    curve->NormalizeXValues(maxXValue);
    
    mDirty = true;
}

void
GraphControl6::DrawAxis(bool lineLabelFlag)
{
    if (mHAxis != NULL)
        DrawAxis(mHAxis, true, lineLabelFlag);
    
    if (mVAxis != NULL)
        DrawAxis(mVAxis, false, lineLabelFlag);
}

void
GraphControl6::DrawAxis(GraphAxis *axis, bool horizontal, bool lineLabelFlag)
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
            
            if (mXdBScale)
                t = Utils::NormalizedXTodB(t, mMinX, mMaxX);
            //else
            //    t = (t - mMinX)/(mMaxX - mMinX);
            
            double x = t*width;
        
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

                    double y0 = 0.0;
                    double y1 = height;
        
                    nvgMoveTo(mVg, x, y0);
                    nvgLineTo(mVg, x, y1);
    
                    nvgStroke(mVg);
                }
                else
                    DrawText(x, textOffset, FONT_SIZE, text, axis->mLabelColor,
                             NVG_ALIGN_CENTER, NVG_ALIGN_BOTTOM);
            }
     
            if (!lineLabelFlag)
            {
                if (i == 0)
                    // First text: aligne left
                    DrawText(x + textOffset, textOffset, FONT_SIZE, text, axis->mLabelColor, NVG_ALIGN_LEFT, NVG_ALIGN_BOTTOM);
        
                if (i == axis->mValues.size() - 1)
                    // Last text: aligne right
                    DrawText(x - textOffset, textOffset, FONT_SIZE, text, axis->mLabelColor, NVG_ALIGN_RIGHT, NVG_ALIGN_BOTTOM);
            }
        }
        else
            // Vertical
        {
            double textOffset = FONT_SIZE*0.2;
            
            // Do not need to nomalize on the Y axis, because we want to display
            // the dB linearly (this is the curve values that are displayed exponentially).
            //t = Utils::NormalizedYTodB3(t, mMinXdB, mMaxXdB);
            
            // For Impulse
            //t = (t - mCurves[0]->mMinY)/(mCurves[0]->mMaxY - mCurves[0]->mMinY);
            
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
                    DrawText(textOffset, y, FONT_SIZE, text, axis->mLabelColor, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE, NVG_ALIGN_BOTTOM);
            }
            
            if (!lineLabelFlag)
            {
                if (i == 0)
                    // First text: aligne "top"
                    DrawText(textOffset, y + FONT_SIZE*0.75, FONT_SIZE, text, axis->mLabelColor, NVG_ALIGN_LEFT, NVG_ALIGN_BOTTOM);
            
                if (i == axis->mValues.size() - 1)
                    // Last text: aligne "bottom"
                    DrawText(textOffset, y - FONT_SIZE*1.5, FONT_SIZE, text, axis->mLabelColor, NVG_ALIGN_LEFT, NVG_ALIGN_BOTTOM);
            }
        }
    }
    
    nvgRestore(mVg);
}

void
GraphControl6::DrawCurves()
{
    for (int i = 0; i < mNumCurves; i++)
    {
        if (!mCurves[i]->mSingleValueH && !mCurves[i]->mSingleValueV)
        {
            if (mCurves[i]->mCurveFill)
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
GraphControl6::DrawLineCurve(GraphCurve4 *curve)
{
#if CURVE_DEBUG
    int numPointsDrawn = 0;
#endif
    
    nvgSave(mVg);
    
    SetCurveDrawStyle(curve);
    
    nvgBeginPath(mVg);
    
#if CURVE_OPTIM
    double prevX = -1.0;
#endif
    
    bool firstPoint = true;
    for (int i = 0; i < curve->mXValues.GetSize(); i ++)
    {
        double x = curve->mXValues.Get()[i];
        
        if (x == GRAPH_VALUE_UNDEFINED)
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
        
        double y = curve->mYValues.Get()[i];
        if (y == GRAPH_VALUE_UNDEFINED)
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
    
#if CURVE_DEBUG
    fprintf(stderr, "GraphControl6::DrawLineCurve - num points: %d\n", numPointsDrawn);
#endif
}

#if !FILL_CURVE_HACK
        // Bug with direct rendering
        // It seems we have no stencil, and we can only render convex polygons
void
GraphControl6::DrawFillCurve(GraphCurve *curve)
{    
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
    
    double x0 = 0.0;
    for (int i = 0; i < curve->mXValues.GetSize(); i ++)
    {        
        double x = curve->mXValues.Get()[i];
        double y = curve->mYValues.Get()[i];
        
        if (x == GRAPH_VALUE_UNDEFINED)
            continue;
        
        if (y == GRAPH_VALUE_UNDEFINED)
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
#endif

#if FILL_CURVE_HACK
// Due to a bug, we render only convex polygons when filling curves
// So we separate in rectangles
void
GraphControl6::DrawFillCurve(GraphCurve4 *curve)
{
#if CURVE_DEBUG
    int numPointsDrawn = 0;
#endif
    
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
    
    double prevX = -1.0;
    double prevY = 0.0;
    for (int i = 0; i < curve->mXValues.GetSize() - 1; i ++)
    {
        double x = curve->mXValues.Get()[i];
        
        if (x == GRAPH_VALUE_UNDEFINED)
            continue;
        
        double y = curve->mYValues.Get()[i];
        
        if (y == GRAPH_VALUE_UNDEFINED)
            continue;
        
        if (prevX < 0.0)
            // Init
        {
            prevX = x;
            prevY = y;
            
            continue;
        }
        
        // Avoid any overlap
        // The problem can bee seen using alpha
        if (x - prevX < 2.0) // More than 1.0
            continue;
        
        nvgBeginPath(mVg);
        
        nvgMoveTo(mVg, prevX, prevY);
        nvgLineTo(mVg, x, y);
        
        nvgLineTo(mVg, x, 0);
        nvgLineTo(mVg, prevX, 0);
        nvgLineTo(mVg, prevX, prevY);
        
        nvgClosePath(mVg);
        
        nvgFillColor(mVg, nvgRGBA(sFillColor[0], sFillColor[1], sFillColor[2], sFillColor[3]));
        nvgFill(mVg);
                  
        prevX = x;
        prevY = y;
    }
    
    nvgRestore(mVg);
    
    mDirty = true;
    
#if CURVE_DEBUG
    fprintf(stderr, "GraphControl6::DrawFillCurve - num points: %d\n", numPointsDrawn);
#endif
}
#endif

void
GraphControl6::DrawLineCurveSVH(GraphCurve4 *curve)
{
    if (curve->mYValues.GetSize() == 0)
        return;
    
    double val = curve->mYValues.Get()[0];
    
    if (val == GRAPH_VALUE_UNDEFINED)
        return;
    
    int width = this->mRECT.W();
    
    nvgSave(mVg);
    nvgStrokeWidth(mVg, curve->mLineWidth);
    
    int sColor[4] = { (int)(curve->mColor[0]*255), (int)(curve->mColor[1]*255),
                      (int)(curve->mColor[2]*255), (int)(curve->mAlpha*255) };
    SWAP_COLOR(sColor);
    
    nvgStrokeColor(mVg, nvgRGBA(sColor[0], sColor[1], sColor[2], sColor[3]));
    
    nvgBeginPath(mVg);
    
    double x0 = 0;
    double x1 = width;
    
    nvgMoveTo(mVg, x0, val);
    nvgLineTo(mVg, x1, val);
    
    nvgStroke(mVg);
    nvgRestore(mVg);
    
    mDirty = true;
}

void
GraphControl6::DrawFillCurveSVH(GraphCurve4 *curve)
{
    if (curve->mYValues.GetSize() == 0)
        return;
    
    double val = curve->mYValues.Get()[0];
    
    if (val == GRAPH_VALUE_UNDEFINED)
        return;
    
    int width = this->mRECT.W();
    
    nvgSave(mVg);
    
    int sColor[4] = { (int)(curve->mColor[0]*255), (int)(curve->mColor[1]*255),
                      (int)(curve->mColor[2]*255), (int)(curve->mFillAlpha*255) };
    SWAP_COLOR(sColor);
    
    nvgStrokeColor(mVg, nvgRGBA(sColor[0], sColor[1], sColor[2], sColor[3]));
    
    nvgBeginPath(mVg);
        
    double x0 = 0;
    double x1 = width;
    double y0 = 0;
    double y1 = val;
    
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
GraphControl6::DrawLineCurveSVV(GraphCurve4 *curve)
{
    // Finally, take the Y value
    // We will have to care about the curve Y scale !
    if (curve->mYValues.GetSize() == 0)
        return;
    
    int width = this->mRECT.W();
    int height = this->mRECT.H();
    
    double val = curve->mYValues.Get()[0];
    
    if (val == GRAPH_VALUE_UNDEFINED)
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
    
    double y0 = 0;
    double y1 = height;
    
    double x = val;
    
    nvgMoveTo(mVg, x, y0);
    nvgLineTo(mVg, x, y1);
    
    nvgStroke(mVg);
    nvgRestore(mVg);
    
    mDirty = true;
}

// Fill right
// (only, for the moment)
void
GraphControl6::DrawFillCurveSVV(GraphCurve4 *curve)
{
    // Finally, take the Y value
    // We will have to care about the curve Y scale !
    if (curve->mYValues.GetSize() == 0)
        return;
    
    int width = this->mRECT.W();
    int height = this->mRECT.H();
    
    double val = curve->mYValues.Get()[0];
    
    if (val == GRAPH_VALUE_UNDEFINED)
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
    
    double y0 = 0;
    double y1 = height;
    double x0 = val;
    double x1 = width;
    
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
GraphControl6::AutoAdjust()
{
    // First, compute the maximum value of all the curves
    double max = -1e16;
    for (int i = 0; i < mNumCurves; i++)
    {
        GraphCurve4 *curve = mCurves[i];
        
        for (int j = curve->mYValues.GetSize(); j >= 0; j--)
        {
            double val = curve->mYValues.Get()[j];
            
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
GraphControl6::MillisToPoints(long long int elapsed, int sampleRate, int numSamplesPoint)
{
    double numSamples = (((double)elapsed)/1000.0)*sampleRate;
    
    double numPoints = numSamples/numSamplesPoint;
    
    return numPoints;
}

void
GraphControl6::InitFont(const char *fontPath)
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
GraphControl6::DrawText(double x, double y, double fontSize,
                        const char *text, int color[4],
                        int halign, int valign)
{
    nvgSave(mVg);
    
    nvgFontSize(mVg, fontSize);
	nvgFontFace(mVg, "font");
    nvgFontBlur(mVg, 0);
	nvgTextAlign(mVg, halign | valign);
    
    int sColor[4] = { color[0], color[1], color[2], color[3] };
    SWAP_COLOR(sColor);
    
    nvgFillColor(mVg, nvgRGBA(sColor[0], sColor[1], sColor[2], sColor[3]));
    
	nvgText(mVg, x, y, text, NULL);
    
    nvgRestore(mVg);
}

void
GraphControl6::DrawGraph()
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
    
    int width = this->mRECT.W();
    int height = this->mRECT.H();
    
    // Clear with Lice, because BitBlt needs it.
    unsigned char clearColor[4] = { (unsigned char)(mClearColor[0]*255), (unsigned char)(mClearColor[1]*255),
                                    (unsigned char)(mClearColor[2]*255), (unsigned char)(mClearColor[3]*255) };
    
    // Clear the GL Bitmap and bind the FBO at the same time !
    LICE_Clear(mLiceFb, *((LICE_pixel *)&clearColor));
    
    // Set pixel ratio to 1.0
    // Otherwise, fonts will be blurry,
    // and lines too I guess...
    nvgBeginFrame(mVg, width, height, 1.0);
    
    if (mAutoAdjustFlag)
    {
        AutoAdjust();
    }
    
    DrawAxis(true);
    
    nvgSave(mVg);
    
    DrawCurves();
    
    nvgRestore(mVg);
    
    DrawAxis(false);
    
    DisplayCurveDescriptions();
    
    nvgEndFrame(mVg);
}

double
GraphControl6::ConvertY(GraphCurve4 *curve, double val, double height)
{
    double y = val;
    if (y != GRAPH_VALUE_UNDEFINED)
    {
        if (curve->mYdBScale)
        {
            if (val > 0.0)
            // Avoid -INF values
                y = Utils::NormalizedYTodB(y, curve->mMinY, curve->mMaxY);
        }
        else
            y = (y - curve->mMinY)/(curve->mMaxY - curve->mMinY);
        
        y = y * mAutoAdjustFactor * mYScaleFactor * height;
    }
    
    return y;
}

void
GraphControl6::SetCurveDrawStyle(GraphCurve4 *curve)
{
    nvgStrokeWidth(mVg, curve->mLineWidth);
    
    int sColor[4] = { (int)(curve->mColor[0]*255), (int)(curve->mColor[1]*255),
        (int)(curve->mColor[2]*255), (int)(curve->mAlpha*255) };
    SWAP_COLOR(sColor);

    nvgStrokeColor(mVg, nvgRGBA(sColor[0], sColor[1], sColor[2], sColor[3]));
}

void
GraphControl6::InitNanoVg()
{
    if (!CreateCGLContext())
        return;
    
    glewExperimental = GL_TRUE;
	if(glewInit())
        return;
    
    //GLboolean hasFbo = glewIsSupported("GL_EXT_framebuffer_object");
    //if (hasFbo != GL_TRUE)
    //    return;
    
	mVg = nvgCreateGL2(NVG_ANTIALIAS | NVG_STENCIL_STROKES);
	if (mVg == NULL)
		return;
    
    //int width = this->mRECT.W();
    //int height = this->mRECT.H();
    //int flags = 0;
    //mFb = nvgluCreateFramebuffer(mVg, width, height, flags);
    //if (mFb == NULL)
    //    return;
    
    InitFont(mFontPath.Get());
}

void
GraphControl6::ExitNanoVg()
{
    //nvgluDeleteFramebuffer(mFb);
    nvgDeleteGL2(mVg);
}

bool
GraphControl6::CreateCGLContext()
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
        return false;
    }
    
    CGLCreateContext(PixelFormat, NULL, (CGLContextObj *)&mGLContext);
    
    if (mGLContext == NULL)
    {
        return false;
    }
    
    // Set the current context
    
    if (!BindCGLContext())
        return false;
    
    // Check OpenGL functionality:
    glstring = glGetString(GL_EXTENSIONS);
    
    if(!gluCheckExtension((const GLubyte *)"GL_EXT_framebuffer_object", glstring))
    {
        return false;
    }
    
    return true;
}

void
GraphControl6::DestroyCGLContext()
{
    if (mGLContext != NULL)
        CGLDestroyContext((CGLContextObj)mGLContext);

}

bool
GraphControl6::BindCGLContext()
{
    if (CGLSetCurrentContext((CGLContextObj)mGLContext))
        return false;
    
    return true;
}

void
GraphControl6::UnBindCGLContext()
{
    CGLSetCurrentContext(NULL);
}
