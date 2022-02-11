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

#include <BLUtils.h>
#include "GLContext2.h"
#include "GraphControl5.h"

#define FILL_CURVE_HACK 1

#define CURVE_DEBUG 0

// Good, bu misses some maxima
#define CURVE_OPTIM      1
#define CURVE_OPTIM_THRS 2048
//#define CURVE_OPTIM 0

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

GraphControl5::GraphControl5(IPlugBase *pPlug, IRECT pR,
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
		GraphCurve3 *curve = new GraphCurve3(numCurveValues);

		mCurves.push_back(curve);
	}

	mAutoAdjustFlag = false;
	mAutoAdjustFactor = 1.0;

	mYScaleFactor = 1.0;

	mXdBScale = false;
	mMinX = 0.1;
	mMaxX = 1.0;
    
    SetClearColor(0, 0, 0, 255);
    
    mDirty = true;
    
    // No need since this is static
    mGLInitialized = false;
    
    mFontPath.Set(fontPath);

    // No need anymore since this is static
    InitGL();
    
    InitNanoVg();
    
    mLiceFb = NULL;
    
#if PROFILE_GRAPH
    mDebugCount = 0;
#endif
}

GraphControl5::~GraphControl5()
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
    
    // No need anymore since this is static
    ExitGL();
}

void
GraphControl5::ResetNumCurveValues(int numCurveValues)
{
    mNumCurveValues = numCurveValues;
    
    for (int i = 0; i < mNumCurves; i++)
	{
		GraphCurve3 *curve = mCurves[i];
        
		curve->ResetNumValues(mNumCurveValues);
	}
}

int
GraphControl5::GetNumCurveValues()
{
    return mNumCurveValues;
}

bool
GraphControl5::IsDirty()
{
    return mDirty;
}

#if 0
bool
GraphControl5::Draw(IGraphics *pGraphics)
{
#if PROFILE_GRAPH
    mDebugTimer.Start();
#endif
    
    IPlugBase::IMutexLock lock(mPlug);
    
    //
    GLContext2 *context = GLContext2::Get();
    context->Bind();
    
    DrawGL();
    
    IBitmap result(mLiceFb, mLiceFb->getWidth(), mLiceFb->getHeight());
    
    // NOTE OPTIL GL
    // - here, pGraphics 'mDrawBitmap' is CPU
    bool res = pGraphics->DrawBitmap(&result, &this->mRECT);
    
    // Unbind OpenGL context
    //context->Unbind();
    
    mDirty = false;
    
#if PROFILE_GRAPH
    mDebugTimer.Stop();
    
    if (mDebugCount++ % 100 == 0)
    {
        long t = mDebugTimer.Get();
        mDebugTimer.Reset();
        
#if 0
        char message[1024];
        sprintf(message, "Draw(x): %ld\n", t);
        Debug::DumpMessage("GraphProfile.txt", message);
#endif
        
        fprintf(stderr, "GraphControl5 - profile: %ld\n", t);
    }
#endif

    return res;
}
#endif

const LICE_IBitmap *
GraphControl5::DrawGL()
{
#if PROFILE_GRAPH
    mDebugTimer.Start();
#endif
    
    IPlugBase::IMutexLock lock(mPlug);
    
    //
    GLContext2 *context = GLContext2::Get();
    context->Bind();
 
    DrawGraph();
    
    mDirty = false;
    
#if PROFILE_GRAPH
    mDebugTimer.Stop();
    
    if (mDebugCount++ % 100 == 0)
    {
        long t = mDebugTimer.Get();
        mDebugTimer.Reset();
        
#if 0
        char message[1024];
        sprintf(message, "Draw(x): %ld\n", t);
        Debug::DumpMessage("GraphProfile.txt", message);
#endif
        
        fprintf(stderr, "GraphControl5 - profile: %ld\n", t);
    }
#endif
    
    return mLiceFb;
}


void
GraphControl5::AddHAxis(char *data[][2], int numData, int axisColor[4], int axisLabelColor[4])
{
    mHAxis = new GraphAxis();
    mHAxis->mOffset = 0.0;
    
    AddAxis(mHAxis, data, numData, axisColor, axisLabelColor, mMinX, mMaxX);
}

void
GraphControl5::AddVAxis(char *data[][2], int numData, int axisColor[4], int axisLabelColor[4], BL_FLOAT offset)
{
    mVAxis = new GraphAxis();
    
    mVAxis->mOffset = offset;
    
    // Retreive the Y db scale
    BL_FLOAT minY = -40.0;
    BL_FLOAT maxY = 40.0;
    
    if (!mCurves.empty())
        // Get from the first curve
    {
        minY = mCurves[0]->mMinY;
        maxY = mCurves[0]->mMaxY;
    }
    
    AddAxis(mVAxis, data, numData, axisColor, axisLabelColor, minY, maxY);
}

void
GraphControl5::SetXScale(bool dbFlag, BL_FLOAT minX, BL_FLOAT maxX)
{
    mXdBScale = dbFlag;
    
    mMinX = minX;
    mMaxX = maxX;
}

void
GraphControl5::SetAutoAdjust(bool flag, BL_FLOAT smoothCoeff)
{
    mAutoAdjustFlag = flag;
    
    mAutoAdjustParamSmoother.SetSmoothCoeff(smoothCoeff);
    
    mDirty = true;
}

void
GraphControl5::SetYScaleFactor(BL_FLOAT factor)
{
    mYScaleFactor = factor;
    
    mDirty = true;
}

void
GraphControl5::SetClearColor(int r, int g, int b, int a)
{
    SET_COLOR_FROM_INT(mClearColor, r, g, b, a);
    
    mDirty = true;
}

void
GraphControl5::SetCurveColor(int curveNum, int r, int g, int b)
{
    if (curveNum >= mNumCurves)
        return;
    
    SET_COLOR_FROM_INT(mCurves[curveNum]->mColor, r, g, b, 255);
    
    mDirty = true;
}

void
GraphControl5::SetCurveAlpha(int curveNum, BL_FLOAT alpha)
{
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->mAlpha = alpha;
    
    mDirty = true;
}

void
GraphControl5::SetCurveLineWidth(int curveNum, BL_FLOAT lineWidth)
{
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->mLineWidth = lineWidth;
    
    mDirty = true;
}

void
GraphControl5::SetCurveSmooth(int curveNum, bool flag)
{
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->mDoSmooth = flag;
    
    mDirty = true;
}

void
GraphControl5::SetCurveFill(int curveNum, bool flag)
{
    if (curveNum >= mNumCurves)
        return;

    mCurves[curveNum]->mCurveFill = flag;
    
    mDirty = true;
}

void
GraphControl5::SetCurveFillAlpha(int curveNum, BL_FLOAT alpha)
{
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->mFillAlpha = alpha;
    
    mDirty = true;
}

void
GraphControl5::SetCurveSingleValueH(int curveNum, bool flag)
{
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->mSingleValueH = flag;
    
    mDirty = true;
}

void
GraphControl5::SetCurveSingleValueV(int curveNum, bool flag)
{
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->mSingleValueV = flag;
    
    mDirty = true;
}


void
GraphControl5::InitGL()
{
    if (mGLInitialized)
        return;
    
    GLContext2::Catch();
    GLContext2 *context = GLContext2::Get();
    context->Bind();
    
    mGLInitialized = true;
}

void
GraphControl5::ExitGL()
{
    GLContext2 *context = GLContext2::Get();
    context->Unbind();
    GLContext2::Release();
    
    mGLInitialized = false;
}

void
GraphControl5::AddAxis(GraphAxis *axis, char *data[][2], int numData, int axisColor[4], int axisLabelColor[4],
                       BL_FLOAT minVal, BL_FLOAT maxVal)
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
        
        BL_FLOAT t = atof(cData[0]);
        
        string text(cData[1]);
        
        // Error here, if we add an Y axis, we must not use mMinXdB
        GraphAxisData aData;
        aData.mT = (t - minVal)/(maxVal - minVal);
        aData.mText = text;
        
        axis->mValues.push_back(aData);
    }
}

void
GraphControl5::ResetCurve(int curveNum, BL_FLOAT val)
{
    IPlugBase::IMutexLock lock(mPlug);
    
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->FillAllValues(val);
    
    mDirty = true;
}

void
GraphControl5::SetCurveYScale(int curveNum, bool dbFlag, BL_FLOAT minY, BL_FLOAT maxY)
{
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->SetYScale(dbFlag, minY, maxY);
    
    mDirty = true;
}

void
GraphControl5::CurveFillAllValues(int curveNum, BL_FLOAT val)
{
    // Must lock otherwise with DATA_STEP, curve will jerk
    IPlugBase::IMutexLock lock(mPlug);
    
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->FillAllValues(val);
    
    mDirty = true;
}

void
GraphControl5::SetCurveValues(int curveNum, const WDL_TypedBuf<BL_FLOAT> *values)
{
    // Must lock otherwise with DATA_STEP, curve will jerk
    IPlugBase::IMutexLock lock(mPlug);
    
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->SetValues(values);
    
    mDirty = true;
}

void
GraphControl5::SetCurveValue(int curveNum, BL_FLOAT t, BL_FLOAT val)
{
    // Must lock otherwise we may have curve will jerk (??)
    IPlugBase::IMutexLock lock(mPlug);
    
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->SetValue(t, val);
    
    mDirty = true;
}

void
GraphControl5::SetCurveSingleValueH(int curveNum, BL_FLOAT val)
{
    // Must lock otherwise with DATA_STEP, curve will jerk
    IPlugBase::IMutexLock lock(mPlug);
    
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->SetValue(0.0, val);
    
    mDirty = true;
}

void
GraphControl5::SetCurveSingleValueV(int curveNum, BL_FLOAT val)
{
    // Must lock otherwise with DATA_STEP, curve will jerk
    IPlugBase::IMutexLock lock(mPlug);
    
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->SetValue(0.0, val);
    
    mDirty = true;
}

void
GraphControl5::PushCurveValue(int curveNum, BL_FLOAT val)
{
    IPlugBase::IMutexLock lock(mPlug);
    
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->PushValue(val);
    mDirty = true;
}

void
GraphControl5::DrawAxis(bool lineLabelFlag)
{
    if (mHAxis != NULL)
        DrawAxis(mHAxis, true, lineLabelFlag);
    
    if (mVAxis != NULL)
        DrawAxis(mVAxis, false, lineLabelFlag);
}

void
GraphControl5::DrawAxis(GraphAxis *axis, bool horizontal, bool lineLabelFlag)
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
        
        BL_FLOAT t = data.mT;
        const char *text = data.mText.c_str();

        if (horizontal)
        {
            BL_FLOAT textOffset = FONT_SIZE*0.2;
            
            if (mXdBScale)
                t = BLUtils::NormalizedXTodB(t, mMinX, mMaxX);
            //else
            //    t = (t - mMinX)/(mMaxX - mMinX);
            
            BL_FLOAT x = t*width;
        
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

                    BL_FLOAT y0 = 0.0;
                    BL_FLOAT y1 = height;
        
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
            BL_FLOAT textOffset = FONT_SIZE*0.2;
            
#if 0
            // Retreive the Y db scale
            BL_FLOAT minYdB = -40.0;
            BL_FLOAT maxYdB = 40.0;
            
            if (!mCurves.empty())
                // Get from the first curve
            {
                minYdB = mCurves[0]->mMinYdB;
                maxYdB = mCurves[0]->mMaxYdB;
            }
#endif
            // Do not need to nomalize on the Y axis, because we want to display
            // the dB linearly (this is the curve values that are displayed exponentially).
            //t = BLUtils::NormalizedYTodB3(t, mMinXdB, mMaxXdB);
            
            // For Impulse
            //t = (t - mCurves[0]->mMinY)/(mCurves[0]->mMaxY - mCurves[0]->mMinY);
            
            BL_FLOAT y = t*height;
            
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
                
                    BL_FLOAT x0 = 0.0;
                    BL_FLOAT x1 = width;
                
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
GraphControl5::DrawCurves()
{
    for (int i = 0; i < mNumCurves; i++)
    {
        if (!mCurves[i]->mSingleValueH && !mCurves[i]->mSingleValueV)
        {
            if (mCurves[i]->mCurveFill)
            {
                DrawFillCurve(&mCurves[i]->mValues,
                               mCurves[i]->mColor, mCurves[i]->mFillAlpha,
                               mCurves[i]->mAlpha, mCurves[i]->mLineWidth, mCurves[i]);
                
#if FILL_CURVE_HACK
                DrawLineCurve(&mCurves[i]->mValues,
                              mCurves[i]->mColor, mCurves[i]->mAlpha, mCurves[i]->mLineWidth, mCurves[i]);
#endif
            }
            else
            {
                DrawLineCurve(&mCurves[i]->mValues,
                              mCurves[i]->mColor, mCurves[i]->mAlpha, mCurves[i]->mLineWidth, mCurves[i]);
            }
        }
        else
        {
            if (mCurves[i]->mSingleValueH)
            {
                if (!mCurves[i]->mValues.GetSize() == 0)
                {
                    if (mCurves[i]->mCurveFill)
                    {
                        DrawFillCurveSVH(mCurves[i]->mValues.Get()[0],
                                    mCurves[i]->mColor, mCurves[i]->mFillAlpha, mCurves[i]);
                    
#if FILL_CURVE_HACK
                        DrawLineCurveSVH(mCurves[i]->mValues.Get()[0],
                                         mCurves[i]->mColor, mCurves[i]->mAlpha,
                                         mCurves[i]->mLineWidth, mCurves[i]);
#endif
                    }
            
                    DrawLineCurveSVH(mCurves[i]->mValues.Get()[0],
                                     mCurves[i]->mColor, mCurves[i]->mAlpha,
                                     mCurves[i]->mLineWidth, mCurves[i]);
                }
            }
            else
                if (mCurves[i]->mSingleValueV)
                {
                    if (!mCurves[i]->mValues.GetSize() == 0)
                    {
                        if (mCurves[i]->mCurveFill)
                        {
                            DrawFillCurveSVV(mCurves[i]->mValues.Get()[0],
                                             mCurves[i]->mColor, mCurves[i]->mFillAlpha, mCurves[i]);
                            
#if FILL_CURVE_HACK
                            DrawLineCurveSVV(mCurves[i]->mValues.Get()[0],
                                             mCurves[i]->mColor, mCurves[i]->mAlpha,
                                             mCurves[i]->mLineWidth, mCurves[i]);
#endif
                        }
                        
                        DrawLineCurveSVV(mCurves[i]->mValues.Get()[0],
                                         mCurves[i]->mColor, mCurves[i]->mAlpha,
                                         mCurves[i]->mLineWidth, mCurves[i]);
                    }
                }
        }
    }
}

void
GraphControl5::DrawLineCurve(const WDL_TypedBuf<BL_FLOAT> *points, CurveColor color,
                            BL_FLOAT alpha, BL_FLOAT lineWidth,
                            GraphCurve3 *curve)
{
#if CURVE_DEBUG
    int numPointsDrawn = 0;
#endif
    
    int width = this->mRECT.W();
    int height = this->mRECT.H();
    
    nvgSave(mVg);
    nvgStrokeWidth(mVg, lineWidth);
    
    int sColor[4] = { (int)(color[0]*255), (int)(color[1]*255),
                      (int)(color[2]*255), (int)(alpha*255) };
    SWAP_COLOR(sColor);
    
    nvgStrokeColor(mVg, nvgRGBA(sColor[0], sColor[1], sColor[2], sColor[3]));
    nvgBeginPath(mVg);
    
#if CURVE_OPTIM
    BL_FLOAT prevX = -1.0;
#endif
    
    for (int i = 0; i < points->GetSize(); i ++)
    {
        // Normalize
        BL_FLOAT x = ((BL_FLOAT)i)/points->GetSize();
        
        if (mXdBScale)
            x = BLUtils::NormalizedXTodB(x, mMinX, mMaxX);
        //else
        //    x = (x - mMinX)/(mMaxX - mMinX);
        
        // Scale for the interface
        x = x * width;
        
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
        
        BL_FLOAT val = points->Get()[i];
        if (val == GRAPH_VALUE_UNDEFINED)
            continue;
        
        if (curve->mYdBScale)
        {
            if (val > 0.0)
                // Avoid -INF values
                val = BLUtils::NormalizedYTodB(val, curve->mMinY, curve->mMaxY);
        }
        else
        {
            val = (val - curve->mMinY)/(curve->mMaxY - curve->mMinY);
        }
        
        BL_FLOAT y = val * mAutoAdjustFactor * mYScaleFactor * height;
        
        if (i == 0)
            nvgMoveTo(mVg, x, y);
        
        nvgLineTo(mVg, x, y);
    }
    
    nvgStroke(mVg);
    nvgRestore(mVg);
    
    mDirty = true;
    
#if CURVE_DEBUG
    fprintf(stderr, "GraphControl5::DrawLineCurve - num points: %d\n", numPointsDrawn);
#endif
}

#if !FILL_CURVE_HACK
        // Bug with direct rendering
        // It seems we have no stencil, and we can only render convex polygons
void
GraphControl5::DrawFillCurve(const WDL_TypedBuf<BL_FLOAT> *points, CurveColor color, BL_FLOAT fillAlpha,
                             BL_FLOAT strokeAlpha, BL_FLOAT lineWidth, GraphCurve *curve)
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
    
    BL_FLOAT x0 = 0.0;
    for (int i = 0; i < points->GetSize(); i ++)
    {        
        // Normalize
        BL_FLOAT x = ((BL_FLOAT)i)/points->GetSize();
        
        if (mXdBScale)
            x = BLUtils::NormalizedXTodB(x, mMinX, mMaxX);
        //else
        //    x = (x - mMinX)/(mMaxX - mMinX);
        
        // Scale for the interface
        x = x * width;
        
        BL_FLOAT val = points->Get()[i];
        if (val == GRAPH_VALUE_UNDEFINED)
            continue;
        
#if CURVE_DEBUG
        numPointsDrawn++;
#endif
        
        if (curve->mYdBScale)
            val = BLUtils::NormalizedYTodB(val, curve->mMinY, curve->mMaxY);
        else
            val = (val - curve->mMinY)/(curve->mMaxY - curve->mMinY);
        
        BL_FLOAT y = val * mAutoAdjustFactor * mYScaleFactor * height;
        
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
GraphControl5::DrawFillCurve(const WDL_TypedBuf<BL_FLOAT> *points, CurveColor color, BL_FLOAT fillAlpha,
                             BL_FLOAT strokeAlpha, BL_FLOAT lineWidth, GraphCurve3 *curve)
{
#if CURVE_DEBUG
    int numPointsDrawn = 0;
#endif
    
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
    
    BL_FLOAT prevX = -1.0;
    BL_FLOAT prevY = 0.0;
    for (int i = 0; i < points->GetSize() - 1; i ++)
    {
        // Normalize
        BL_FLOAT x = ((BL_FLOAT)i)/points->GetSize();
        
        if (mXdBScale)
            x = BLUtils::NormalizedXTodB(x, mMinX, mMaxX);
        //else
        //    x = (x - mMinX)/(mMaxX - mMinX);
        
        // Scale for the interface
        x = x * width;
        
        BL_FLOAT val = points->Get()[i];
        if (val == GRAPH_VALUE_UNDEFINED)
            continue;
        
        if (curve->mYdBScale)
        {
            if (val > 0.0)
                // Avoid -INF values
                val = BLUtils::NormalizedYTodB(val, curve->mMinY, curve->mMaxY);
        }
        else
            val = (val - curve->mMinY)/(curve->mMaxY - curve->mMinY);
        
        BL_FLOAT y = val * mAutoAdjustFactor * mYScaleFactor * height;
        
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
    fprintf(stderr, "GraphControl5::DrawFillCurve - num points: %d\n", numPointsDrawn);
#endif
}
#endif

void
GraphControl5::DrawLineCurveSVH(BL_FLOAT val, CurveColor color, BL_FLOAT alpha, BL_FLOAT lineWidth,
                                GraphCurve3 *curve)
{
    if (val == GRAPH_VALUE_UNDEFINED)
        return;
    
    int width = this->mRECT.W();
    int height = this->mRECT.H();
    
    nvgSave(mVg);
    nvgStrokeWidth(mVg, lineWidth);
    
    int sColor[4] = { (int)(color[0]*255), (int)(color[1]*255),
                      (int)(color[2]*255), (int)(alpha*255) };
    SWAP_COLOR(sColor);
    
    nvgStrokeColor(mVg, nvgRGBA(sColor[0], sColor[1], sColor[2], sColor[3]));
    
    nvgBeginPath(mVg);
    
    BL_FLOAT x0 = 0;
    BL_FLOAT x1 = width;
    
    if (curve->mYdBScale)
        val = BLUtils::NormalizedYTodB(val, curve->mMinY, curve->mMaxY);
    else
        val = (val - curve->mMinY)/(curve->mMaxY - curve->mMinY);
    
    BL_FLOAT y = val * mAutoAdjustFactor * height;
    
    nvgMoveTo(mVg, x0, y);
    nvgLineTo(mVg, x1, y);
    
    nvgStroke(mVg);
    nvgRestore(mVg);
    
    mDirty = true;
}

void
GraphControl5::DrawFillCurveSVH(BL_FLOAT val, CurveColor color, BL_FLOAT alpha,
                                GraphCurve3 *curve)
{
    if (val == GRAPH_VALUE_UNDEFINED)
        return;
    
    int width = this->mRECT.W();
    int height = this->mRECT.H();
    
    nvgSave(mVg);
    
    int sColor[4] = { (int)(color[0]*255), (int)(color[1]*255),
                      (int)(color[2]*255), (int)(alpha*255) };
    SWAP_COLOR(sColor);
    
    nvgStrokeColor(mVg, nvgRGBA(sColor[0], sColor[1], sColor[2], sColor[3]));
    
    nvgBeginPath(mVg);
    
    if (curve->mYdBScale)
        val = BLUtils::NormalizedYTodB(val, curve->mMinY, curve->mMaxY);
    else
        val = (val - curve->mMinY)/(curve->mMaxY - curve->mMinY);
        
    BL_FLOAT x0 = 0;
    BL_FLOAT x1 = width;
    BL_FLOAT y0 = 0;
    BL_FLOAT y1 = val * mAutoAdjustFactor * height;
    
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
GraphControl5::DrawLineCurveSVV(BL_FLOAT val, CurveColor color, BL_FLOAT alpha, BL_FLOAT lineWidth,
                                GraphCurve3 *curve)
{
    if (val == GRAPH_VALUE_UNDEFINED)
        return;
    
    int width = this->mRECT.W();
    int height = this->mRECT.H();
    
    nvgSave(mVg);
    nvgStrokeWidth(mVg, lineWidth);
    
    int sColor[4] = { (int)(color[0]*255), (int)(color[1]*255),
        (int)(color[2]*255), (int)(alpha*255) };
    SWAP_COLOR(sColor);
    
    nvgStrokeColor(mVg, nvgRGBA(sColor[0], sColor[1], sColor[2], sColor[3]));
    
    nvgBeginPath(mVg);
    
    BL_FLOAT y0 = 0;
    BL_FLOAT y1 = height;
    
    if (mXdBScale)
        val = BLUtils::NormalizedXTodB(val, mMinX, mMaxX);
    else
        val = (val - mMinX)/(mMaxX - mMinX);
    
    BL_FLOAT x = val * width;
    
    nvgMoveTo(mVg, x, y0);
    nvgLineTo(mVg, x, y1);
    
    nvgStroke(mVg);
    nvgRestore(mVg);
    
    mDirty = true;
}

// Fill right
// (only, for the moment)
void
GraphControl5::DrawFillCurveSVV(BL_FLOAT val, CurveColor color, BL_FLOAT alpha,
                                GraphCurve3 *curve)
{
    if (val == GRAPH_VALUE_UNDEFINED)
        return;
    
    int width = this->mRECT.W();
    int height = this->mRECT.H();
    
    nvgSave(mVg);
    
    int sColor[4] = { (int)(color[0]*255), (int)(color[1]*255),
        (int)(color[2]*255), (int)(alpha*255) };
    SWAP_COLOR(sColor);
    
    nvgStrokeColor(mVg, nvgRGBA(sColor[0], sColor[1], sColor[2], sColor[3]));
    
    nvgBeginPath(mVg);
    
    if (mXdBScale)
        val = BLUtils::NormalizedXTodB(val, mMinX, mMaxX);
    else
        val = (val - mMinX)/(mMaxX - mMinX);
    
    BL_FLOAT y0 = 0;
    BL_FLOAT y1 = height;
    BL_FLOAT x0 = val * width;
    BL_FLOAT x1 = width;
    
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
GraphControl5::AutoAdjust()
{
    // First, compute the maximul value of all the curves
    BL_FLOAT max = -1e16;
    for (int i = 0; i < mNumCurves; i++)
    {
        GraphCurve3 *curve = mCurves[i];
        
        for (int j = curve->mValues.GetSize(); j >= 0; j--)
        {
            BL_FLOAT val = curve->mValues.Get()[j];
            
            if (val > max)
                max = val;
        }
    }
    
    // Compute the scale factor
    
    // keep a margin, do not scale to the maximum
    BL_FLOAT margin = 0.25;
    BL_FLOAT factor = 1.0;
    
    if (max > 0.0)
         factor = (1.0 - margin)/max;
    
    // Then smooth the factor
    mAutoAdjustParamSmoother.SetNewValue(factor);
    mAutoAdjustParamSmoother.Update();
    factor = mAutoAdjustParamSmoother.GetCurrentValue();
    
    mAutoAdjustFactor = factor;
}

BL_FLOAT
GraphControl5::MillisToPoints(long long int elapsed, int sampleRate, int numSamplesPoint)
{
    BL_FLOAT numSamples = (((BL_FLOAT)elapsed)/1000.0)*sampleRate;
    
    BL_FLOAT numPoints = numSamples/numSamplesPoint;
    
    return numPoints;
}

void
GraphControl5::InitFont(const char *fontPath)
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
GraphControl5::DrawText(BL_FLOAT x, BL_FLOAT y, BL_FLOAT fontSize,
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

static void
DebugTestFill(NVGcontext *mVg, CurveColor mColor)
{
    int sFillColor[4] = { (int)(mColor[0]*255), 255/*(int)(mCurves[0]->mColor[1]*255)*/,
        (int)(mColor[2]*255), 255/*(int)(mCurves[0]->mFillAlpha*255)*/ };
    SWAP_COLOR(sFillColor);
    
#define COEFF 200.0
    
    nvgBeginPath(mVg);
    
    nvgMoveTo(mVg, 0.25*COEFF, 0.25*COEFF);
    nvgLineTo(mVg, 0.4*COEFF, 0.4*COEFF);
    nvgLineTo(mVg, 0.5*COEFF, 0.25*COEFF);
    nvgLineTo(mVg, 0.5*COEFF, 0.5*COEFF);
    nvgLineTo(mVg, 0.25*COEFF, 0.5*COEFF);
    nvgLineTo(mVg, 0.25*COEFF, 0.25*COEFF);
    
    // Same, reversed
    /* nvgMoveTo(mVg, 0.25*COEFF, 0.25*COEFF);
    nvgLineTo(mVg, 0.25*COEFF, 0.25*COEFF);
    nvgLineTo(mVg, 0.25*COEFF, 0.5*COEFF);
    nvgLineTo(mVg, 0.5*COEFF, 0.5*COEFF);
    nvgLineTo(mVg, 0.5*COEFF, 0.25*COEFF);
    nvgLineTo(mVg, 0.4*COEFF, 0.4*COEFF);
     */
    
    nvgClosePath(mVg);
    
    nvgFillColor(mVg, nvgRGBA(sFillColor[0], sFillColor[1], sFillColor[2], sFillColor[3]));
	nvgFill(mVg);
    
    nvgStrokeWidth(mVg, 2.0);
    nvgStrokeColor(mVg, nvgRGBA(sFillColor[1], sFillColor[0], sFillColor[2], sFillColor[3]));
    nvgStroke(mVg);
}

void
GraphControl5::DrawGraph()
{
    IPlugBase::IMutexLock lock(mPlug);
    
	// On Windows, we need lazy evaluation, because we need HInstance and IGraphics
	if (!mFontInitialized)
		InitFont(NULL);
    
    if (mLiceFb == NULL)
    {
// Take care of the following macro in Lice ! : DISABLE_LICE_EXTENSIONS
        
        mLiceFb = new LICE_GL_SysBitmap(0, 0);
        //mLiceFb = new LICE_GL_MemBitmap(0, 0);
        
        IRECT *r = GetRECT();
        mLiceFb->resize(r->W(), r->H());
    }
    
    int width = this->mRECT.W();
    int height = this->mRECT.H();
    
    // Clear with Lice, because BitBlt needs it.
    unsigned char clearColor[4] = { (unsigned char)(mClearColor[0]*255), (unsigned char)(mClearColor[1]*255),
                                    (unsigned char)(mClearColor[2]*255), (unsigned char)(mClearColor[3]*255) };
    
    //glViewport(this->mRECT.L, this->mRECT.T, width, height);
    
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
    
    //DebugTestFill(mVg, mCurves[0]->mColor);
   
    DrawAxis(true);
    
    nvgSave(mVg);
    
    DrawCurves();
    
    nvgRestore(mVg);
    
    DrawAxis(false);
    
    nvgEndFrame(mVg);
}

void
GraphControl5::InitNanoVg()
{
    //GLContext2 *context = GLContext2::Get();
    //context->Bind();
    
	mVg = nvgCreateGL2(NVG_ANTIALIAS | NVG_STENCIL_STROKES);
	if (mVg == NULL)
		return;
    
    InitFont(mFontPath.Get());
}

void
GraphControl5::ExitNanoVg()
{
    //GLContext2 *context = GLContext2::Get();
    
	//context->Bind();
    
    nvgDeleteGL2(mVg);
}
