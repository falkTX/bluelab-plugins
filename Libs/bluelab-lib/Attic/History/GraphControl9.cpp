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
#include "GLContext.h"
#include "GraphControl9.h"
#include "BLSpectrogram3.h"

#define FILL_CURVE_HACK 1

#define CURVE_DEBUG 0

// Good, but misses some maxima
#define CURVE_OPTIM      0
#define CURVE_OPTIM_THRS 2048

// Spectrogram
#define GLSL_COLORMAP 1

#define USE_ZOOM_NVG_TRANSFORM 1

#define USE_SPECTRO_NEAREST 0

GraphControl9::GraphControl9(IPlugBase *pPlug, IRECT pR, int paramIdx,
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
    mGLInitialized = false;
    
    mFontPath.Set(fontPath);

    // Spectrogram
    mSpectrogram = NULL;
    mNvgSpectroImage = 0;
    mMustUpdateSpectrogram = false;
    mMustUpdateSpectrogramData = false;
    mNvgColormapImage = 0;
    
    mMustUpdateSpectrogramFullData = false;
    mNvgSpectroFullImage = 0;
    
    mShowSpectrogram = false;
    
    mSpectroCenterPos = 0.5;
    mSpectroAbsTranslation = 0.0;
    
    mSpectroMinX = 0.0;
    mSpectroMaxX = 1.0;
    
    mSpectroAbsMinX = 0.0;
    mSpectroAbsMaxX = 1.0;
    
    mSpectrogramGain = 1.0;
    
    mSpectrogramAlpha = 1.0;
    
    // No need anymore since this is static
    InitGL();
    
    InitNanoVg();
    
    mLiceFb = NULL;
    
#if PROFILE_GRAPH
    mDebugCount = 0;
#endif
}

GraphControl9::~GraphControl9()
{
    for (int i = 0; i < mNumCurves; i++)
        delete mCurves[i];
    
    if (mHAxis != NULL)
        delete mHAxis;
    
    if (mVAxis != NULL)
        delete mVAxis;

    if (mNvgSpectroImage != 0)
        nvgDeleteImage(mVg, mNvgSpectroImage);
    
    if (mNvgColormapImage != 0)
        nvgDeleteImage(mVg, mNvgColormapImage);

    if (mNvgSpectroFullImage != 0)
        nvgDeleteImage(mVg, mNvgSpectroFullImage);
    
#if 1 // AbletonLive need these two lines
    // Otherwise when destroying the plugin in AbletonLive,
    // there are many graphical bugs (huge white rectangles)
    // and the software becomes unusable
    GLContext *context = GLContext::Get();
    context->Unbind();
#endif
    
    ExitNanoVg();
    
    if (mLiceFb != NULL)
        delete mLiceFb;
    
    // No need anymore since this is static
    ExitGL();
}

void
GraphControl9::Resize(int numCurveValues)
{
    mNumCurveValues = numCurveValues;
    
    for (int i = 0; i < mNumCurves; i++)
	{
		GraphCurve4 *curve = mCurves[i];
        
		curve->ResetNumValues(mNumCurveValues);
	}
}

int
GraphControl9::GetNumCurveValues()
{
    return mNumCurveValues;
}

#if 0 // Commented for StereoWidthProcess::DebugDrawer
bool
GraphControl9::IsDirty()
{
    return mDirty;
}

void
GraphControl9::SetDirty(bool flag)
{
    mDirty = flag;;
}
#endif

const LICE_IBitmap *
GraphControl9::DrawGL()
{
#if PROFILE_GRAPH
    mDebugTimer.Start();
#endif
    
    IPlugBase::IMutexLock lock(mPlug);
    
    //
    GLContext *context = GLContext::Get();
    context->Bind();
    
    DrawGraph();
    
    mDirty = false;
    
#if PROFILE_GRAPH
    mDebugTimer.Stop();
    
    if (mDebugCount++ % 100 == 0)
    {
        long t = mDebugTimer.Get();
        mDebugTimer.Reset();
        
        fprintf(stderr, "GraphControl9 - profile: %ld\n", t);
        
        char debugMessage[512];
        sprintf(debugMessage, "GraphControl9 - profile: %ld", t);
        Debug::AppendMessage("graph-profile.txt", debugMessage);
    }
#endif
    
    return mLiceFb;
}

void
GraphControl9::AddHAxis(char *data[][2], int numData, int axisColor[4], int axisLabelColor[4])
{
    mHAxis = new GraphAxis();
    mHAxis->mOffset = 0.0;
    
    AddAxis(mHAxis, data, numData, axisColor, axisLabelColor, mMinX, mMaxX);
}

void
GraphControl9::ReplaceHAxis(char *data[][2], int numData)
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
GraphControl9::AddVAxis(char *data[][2], int numData, int axisColor[4], int axisLabelColor[4], BL_FLOAT offset)
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
GraphControl9::AddVAxis(char *data[][2], int numData, int axisColor[4], int axisLabelColor[4],
                        bool dbFlag, BL_FLOAT minY, BL_FLOAT maxY,
                        BL_FLOAT offset)
{
    mVAxis = new GraphAxis();
    
    mVAxis->mOffset = offset;
    
    AddAxis(mVAxis, data, numData, axisColor, axisLabelColor, minY, maxY);
}

void
GraphControl9::SetXScale(bool dbFlag, BL_FLOAT minX, BL_FLOAT maxX)
{
    mXdBScale = dbFlag;
    
    mMinX = minX;
    mMaxX = maxX;
}

void
GraphControl9::SetAutoAdjust(bool flag, BL_FLOAT smoothCoeff)
{
    mAutoAdjustFlag = flag;
    
    mAutoAdjustParamSmoother.SetSmoothCoeff(smoothCoeff);
    
    mDirty = true;
}

void
GraphControl9::SetYScaleFactor(BL_FLOAT factor)
{
    mYScaleFactor = factor;
    
    mDirty = true;
}

void
GraphControl9::SetClearColor(int r, int g, int b, int a)
{
    SET_COLOR_FROM_INT(mClearColor, r, g, b, a);
    
    mDirty = true;
}

void
GraphControl9::SetCurveColor(int curveNum, int r, int g, int b)
{
    if (curveNum >= mNumCurves)
        return;
    
    SET_COLOR_FROM_INT(mCurves[curveNum]->mColor, r, g, b, 255);
    
    mDirty = true;
}

void
GraphControl9::SetCurveAlpha(int curveNum, BL_FLOAT alpha)
{
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->mAlpha = alpha;
    
    mDirty = true;
}

void
GraphControl9::SetCurveLineWidth(int curveNum, BL_FLOAT lineWidth)
{
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->mLineWidth = lineWidth;
    
    mDirty = true;
}

void
GraphControl9::SetCurveSmooth(int curveNum, bool flag)
{
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->mDoSmooth = flag;
    
    mDirty = true;
}

void
GraphControl9::SetCurveFill(int curveNum, bool flag)
{
    if (curveNum >= mNumCurves)
        return;

    mCurves[curveNum]->mCurveFill = flag;
    
    mDirty = true;
}

void
GraphControl9::SetCurveFillAlpha(int curveNum, BL_FLOAT alpha)
{
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->mFillAlpha = alpha;
    
    mDirty = true;
}

void
GraphControl9::SetCurveFillAlphaUp(int curveNum, BL_FLOAT alpha)
{
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->mFillAlphaUp = alpha;
    
    mDirty = true;
}


void
GraphControl9::SetCurvePointSize(int curveNum, BL_FLOAT pointSize)
{
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->mPointSize = pointSize;
    
    mDirty = true;
}

void
GraphControl9::SetCurveXScale(int curveNum, bool dbFlag, BL_FLOAT minX, BL_FLOAT maxX)
{
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->SetXScale(dbFlag, minX, maxX);
    
    mDirty = true;
}

void
GraphControl9::SetCurvePointStyle(int curveNum, bool flag, bool pointsAsLines)
{
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->mPointStyle = flag;
    mCurves[curveNum]->mPointsAsLines = pointsAsLines;
    
    mDirty = true;
}

void
GraphControl9::SetCurveValuesPoint(int curveNum,
                                   const WDL_TypedBuf<BL_FLOAT> &xValues,
                                   const WDL_TypedBuf<BL_FLOAT> &yValues)
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
        BL_FLOAT x = xValues.Get()[i];
        BL_FLOAT y = yValues.Get()[i];
        
        x = ConvertX(curve, x, width);
        y = ConvertY(curve, y, height);
        
        curve->mXValues.Get()[i] = x;
        curve->mYValues.Get()[i] = y;
        //SetCurveValuePoint(curveNum, x, y);
    }
    
    mDirty = true;
}

void
GraphControl9::SetCurveValuesPointWeight(int curveNum,
                                         const WDL_TypedBuf<BL_FLOAT> &xValues,
                                         const WDL_TypedBuf<BL_FLOAT> &yValues,
                                         const WDL_TypedBuf<BL_FLOAT> &weights)
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
            BL_FLOAT x = xValues.Get()[i];
            BL_FLOAT y = yValues.Get()[i];
            
            x = ConvertX(curve, x, width);
            y = ConvertY(curve, y, height);
            
            curve->mXValues.Get()[i] = x;
            curve->mYValues.Get()[i] = y;
            //SetCurveValuePoint(curveNum, x, y);
        }
    
    curve->mWeights = weights;
    
    mDirty = true;
}

void
GraphControl9::SetCurveColorWeight(int curveNum,
                                   const WDL_TypedBuf<BL_FLOAT> &colorWeights)
{
    // Must lock otherwise with DATA_STEP, curve will jerk
    IPlugBase::IMutexLock lock(mPlug); // ??
    
    if (curveNum >= mNumCurves)
        return;
    
    GraphCurve4 *curve = mCurves[curveNum];
    
    curve->mWeights = colorWeights;
}

void
GraphControl9::SetCurveSingleValueH(int curveNum, bool flag)
{
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->mSingleValueH = flag;
    
    mDirty = true;
}

void
GraphControl9::SetCurveSingleValueV(int curveNum, bool flag)
{
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->mSingleValueV = flag;
    
    mDirty = true;
}

void
GraphControl9::DrawText(NVGcontext *vg, BL_FLOAT x, BL_FLOAT y, BL_FLOAT fontSize,
                        const char *text, int color[4],
                        int halign, int valign)
{
    nvgSave(vg);
    
    nvgFontSize(vg, fontSize);
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
GraphControl9::SetSpectrogram(BLSpectrogram3 *spectro,
                              BL_FLOAT left, BL_FLOAT top, BL_FLOAT right, BL_FLOAT bottom)
{
    mSpectrogram = spectro;
    
    mSpectrogramBounds[0] = left;
    mSpectrogramBounds[1] = top;
    mSpectrogramBounds[2] = right;
    mSpectrogramBounds[3] = bottom;
    
    mShowSpectrogram = true;
    
    // Be sure to create the texture image in the right thread
    UpdateSpectrogram(true, true);
}

void
GraphControl9::ShowSpectrogram(bool flag)
{
    mShowSpectrogram = flag;
}

void
GraphControl9::UpdateSpectrogram(bool updateData, bool updateFullData)
{
    mMustUpdateSpectrogram = true;
    
    if (!mMustUpdateSpectrogramData)
        mMustUpdateSpectrogramData = updateData;
    
    if (!mMustUpdateSpectrogramFullData)
        mMustUpdateSpectrogramFullData = updateFullData;
    
    mDirty = true;
}

void
GraphControl9::ResetSpectrogramTransform()
{
    mSpectroMinX = 0.0;
    mSpectroMaxX = 1.0;
    
    mSpectroAbsMinX = 0.0;
    mSpectroAbsMaxX = 1.0;
    
    mSpectroCenterPos = 0.5;
    mSpectroAbsTranslation = 0.0;
}

void
GraphControl9::SetSpectrogramZoom(BL_FLOAT zoomX)
{
    BL_FLOAT norm = (mSpectroCenterPos - mSpectroMinX)/(mSpectroMaxX - mSpectroMinX);
    
    mSpectroMinX = mSpectroCenterPos - norm*zoomX;
    mSpectroMaxX = mSpectroCenterPos + (1.0 - norm)*zoomX;
}

void
GraphControl9::SetSpectrogramAbsZoom(BL_FLOAT zoomX)
{
    BL_FLOAT norm = (mSpectroCenterPos - mSpectroAbsMinX)/(mSpectroAbsMaxX - mSpectroAbsMinX);
    
    mSpectroAbsMinX = mSpectroCenterPos - norm*zoomX;
    mSpectroAbsMaxX = mSpectroCenterPos + (1.0 - norm)*zoomX;
}

void
GraphControl9::SetSpectrogramCenterPos(BL_FLOAT centerPos)
{
    mSpectroCenterPos = centerPos;
}

bool
GraphControl9::SetSpectrogramTranslation(BL_FLOAT tX)
{
    BL_FLOAT dX = tX - mSpectroAbsTranslation;
    
    if (((mSpectroAbsMinX > 1.0) && (dX > 0.0)) ||
        ((mSpectroAbsMaxX < 0.0) && (dX < 0.0)))
        // Do not translate if the spectrogram gets out of view
        return false;
    
    mSpectroAbsTranslation = tX;
    
    mSpectroMinX += dX;
    mSpectroMaxX += dX;
    
    mSpectroAbsMinX += dX;
    mSpectroAbsMaxX += dX;
    
    return true;
}

void
GraphControl9::GetSpectrogramVisibleNormBounds(BL_FLOAT *minX, BL_FLOAT *maxX)
{
    if (mSpectroAbsMinX < 0.0)
        *minX = -mSpectroAbsMinX/(mSpectroAbsMaxX - mSpectroAbsMinX);
    else
        *minX = 0.0;
    
    if (mSpectroAbsMaxX < 1.0)
        *maxX = 1.0;
    else
        *maxX = 1.0 - (mSpectroAbsMaxX - 1.0)/(mSpectroAbsMaxX - mSpectroAbsMinX);
}

void
GraphControl9::GetSpectrogramVisibleNormBounds2(BL_FLOAT *minX, BL_FLOAT *maxX)
{
    *minX = -mSpectroAbsMinX/(mSpectroAbsMaxX - mSpectroAbsMinX);
    *maxX = 1.0 - (mSpectroAbsMaxX - 1.0)/(mSpectroAbsMaxX - mSpectroAbsMinX);
}


void
GraphControl9::ResetSpectrogramZoomAndTrans()
{
    mSpectroMinX = 0.0;
    mSpectroMaxX = 1.0;
}

void
GraphControl9::SetSpectrogramAlpha(BL_FLOAT alpha)
{
    mSpectrogramAlpha = alpha;
}

void
GraphControl9::AddCustomDrawer(GraphCustomDrawer *customDrawer)
{
    mCustomDrawers.push_back(customDrawer);
}

void
GraphControl9::CustomDrawersPreDraw()
{
    int width = this->mRECT.W();
    int height = this->mRECT.H();
    
    for (int i = 0; i < mCustomDrawers.size(); i++)
    {
        GraphCustomDrawer *drawer = mCustomDrawers[i];
        drawer->PreDraw(mVg, width, height);
    }
}

void
GraphControl9::CustomDrawersPostDraw()
{
    int width = this->mRECT.W();
    int height = this->mRECT.H();
    
    for (int i = 0; i < mCustomDrawers.size(); i++)
    {
        GraphCustomDrawer *drawer = mCustomDrawers[i];
        drawer->PostDraw(mVg, width, height);
    }
}

void
GraphControl9::AddCustomControl(GraphCustomControl *customControl)
{
    mCustomControls.push_back(customControl);
}

void
GraphControl9::OnMouseDown(int x, int y, IMouseMod* pMod)
{
    IControl::OnMouseDown(x, y, pMod);
    
    for (int i = 0; i < mCustomControls.size(); i++)
    {
        GraphCustomControl *control = mCustomControls[i];
        control->OnMouseDown(x, y, pMod);
    }
}

void
GraphControl9::OnMouseUp(int x, int y, IMouseMod* pMod)
{
    IControl::OnMouseUp(x, y, pMod);
    
    for (int i = 0; i < mCustomControls.size(); i++)
    {
        GraphCustomControl *control = mCustomControls[i];
        control->OnMouseUp(x, y, pMod);
    }
}

void
GraphControl9::OnMouseDrag(int x, int y, int dX, int dY, IMouseMod* pMod)
{
    IControl::OnMouseDrag(x, y, dX, dY, pMod);
    
    for (int i = 0; i < mCustomControls.size(); i++)
    {
        GraphCustomControl *control = mCustomControls[i];
        control->OnMouseDrag(x, y, dX, dY, pMod);
    }
}

void
GraphControl9::OnMouseDblClick(int x, int y, IMouseMod* pMod)
{
    IControl::OnMouseDblClick(x, y, pMod);
    
    for (int i = 0; i < mCustomControls.size(); i++)
    {
        GraphCustomControl *control = mCustomControls[i];
        control->OnMouseDblClick(x, y, pMod);
    }
}

void
GraphControl9::OnMouseWheel(int x, int y, IMouseMod* pMod, int d)
{
    IControl::OnMouseWheel(x, y, pMod, d);
    
    for (int i = 0; i < mCustomControls.size(); i++)
    {
        GraphCustomControl *control = mCustomControls[i];
        control->OnMouseWheel(x, y, pMod, d);
    }
}

bool
GraphControl9::OnKeyDown(int x, int y, int key)
{
    IControl::OnKeyDown(x, y, key);
    
    bool res = false;
    for (int i = 0; i < mCustomControls.size(); i++)
    {
        GraphCustomControl *control = mCustomControls[i];
        res = control->OnKeyDown(x, y, key);
    }
    
    return res;
}

void
GraphControl9::OnMouseOver(int x, int y, IMouseMod* pMod)
{
    IControl::OnMouseOver(x, y, pMod);
    
    for (int i = 0; i < mCustomControls.size(); i++)
    {
        GraphCustomControl *control = mCustomControls[i];
        control->OnMouseOver(x, y, pMod);
    }
}

void
GraphControl9::OnMouseOut()
{
    IControl::OnMouseOut();
    
    for (int i = 0; i < mCustomControls.size(); i++)
    {
        GraphCustomControl *control = mCustomControls[i];
        control->OnMouseOut();
    }
}

void
GraphControl9::InitGL()
{
    if (mGLInitialized)
        return;
    
    GLContext::Ref();
    GLContext *context = GLContext::Get();

    context->Bind();
    
    mGLInitialized = true;
}

void
GraphControl9::ExitGL()
{
    GLContext *context = GLContext::Get();
    context->Unbind();
    GLContext::Unref();

    mGLInitialized = false;
}

void
GraphControl9::DisplayCurveDescriptions()
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
        
        BL_FLOAT y = this->mRECT.H() - (DESCR_Y0 + descrNum*DESCR_Y_STEP);
        
        nvgSave(mVg);
        
        // Must force alpha to 1, because sometimes,
        // the plugins hide the curves, but we still
        // want to display the description
        BL_FLOAT prevAlpha = curve->mAlpha;
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
GraphControl9::AddAxis(GraphAxis *axis, char *data[][2], int numData, int axisColor[4],
                       int axisLabelColor[4],
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
GraphControl9::SetCurveDescription(int curveNum, const char *description, int descrColor[4])
{
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->SetDescription(description, descrColor);
}

void
GraphControl9::ResetCurve(int curveNum, BL_FLOAT val)
{
    IPlugBase::IMutexLock lock(mPlug);
    
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->FillAllXValues(mMinX, mMaxX);
    mCurves[curveNum]->FillAllYValues(val);
    
    mDirty = true;
}

void
GraphControl9::SetCurveYScale(int curveNum, bool dbFlag, BL_FLOAT minY, BL_FLOAT maxY)
{
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->SetYScale(dbFlag, minY, maxY);
    
    mDirty = true;
}

void
GraphControl9::CurveFillAllValues(int curveNum, BL_FLOAT val)
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
GraphControl9::SetCurveValues(int curveNum, const WDL_TypedBuf<BL_FLOAT> *values)
{
    // Must lock otherwise with DATA_STEP, curve will jerk
    IPlugBase::IMutexLock lock(mPlug);
    
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->SetYValues(values, mMinX, mMaxX);
    
    mDirty = true;
}

void
GraphControl9::SetCurveValues2(int curveNum, const WDL_TypedBuf<BL_FLOAT> *values)
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
        BL_FLOAT t = ((BL_FLOAT)i)/(values->GetSize() - 1);
        BL_FLOAT y = values->Get()[i];
        
        SetCurveValue(curveNum, t, y);
    }
    
    mDirty = true;
}

void
GraphControl9::SetCurveValuesDecimate(int curveNum,
                                      const WDL_TypedBuf<BL_FLOAT> *values,
                                      bool isWaveSignal)
{    
    // Must lock otherwise with DATA_STEP, curve will jerk
    IPlugBase::IMutexLock lock(mPlug);
    
    if (curveNum >= mNumCurves)
        return;
    
    GraphCurve4 *curve = mCurves[curveNum];
    
    int width = this->mRECT.W();
    
    BL_FLOAT prevX = -1.0;
    BL_FLOAT maxY = -1.0;
    
    if (isWaveSignal)
        maxY = (curve->mMinY + curve->mMaxY)/2.0;
        
    BL_FLOAT thrs = 1.0/GRAPHCONTROL_PIXEL_DENSITY;
    for (int i = 0; i < values->GetSize(); i++)
    {
        BL_FLOAT t = ((BL_FLOAT)i)/(values->GetSize() - 1);
        
        BL_FLOAT x = t*width;
        BL_FLOAT y = values->Get()[i];
        
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
}

void
GraphControl9::SetCurveValuesDecimate2(int curveNum,
                                      const WDL_TypedBuf<BL_FLOAT> *values,
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
    BL_FLOAT decFactor = ((BL_FLOAT)mNumCurveValues)/values->GetSize();
    
    WDL_TypedBuf<BL_FLOAT> decimValues;
    if (isWaveSignal)
        BLUtils::DecimateSamples(&decimValues, *values, decFactor);
    else
        BLUtils::DecimateValues(&decimValues, *values, decFactor);
    
    for (int i = 0; i < decimValues.GetSize(); i++)
    {
        BL_FLOAT t = ((BL_FLOAT)i)/(decimValues.GetSize() - 1);
        
        BL_FLOAT y = decimValues.Get()[i];
        
        SetCurveValue(curveNum, t, y);
    }
    
    mDirty = true;
}

void
GraphControl9::SetCurveValue(int curveNum, BL_FLOAT t, BL_FLOAT val)
{
    // Must lock otherwise we may have curve will jerk (??)
    IPlugBase::IMutexLock lock(mPlug);
    
    if (curveNum >= mNumCurves)
        return;
    
    GraphCurve4 *curve = mCurves[curveNum];
    
    // Normalize, then adapt to the graph
    int width = this->mRECT.W();
    int height = this->mRECT.H();
    
    BL_FLOAT x = t;
    
    if (x != GRAPH_VALUE_UNDEFINED)
    {
        if (mXdBScale)
            x = BLUtils::NormalizedXTodB(x, mMinX, mMaxX);
        
        // X should be already normalize in input
        //else
        //    x = (x - mMinX)/(mMaxX - mMinX);
    
        // Scale for the interface
        x = x * width;
    }
    
    BL_FLOAT y = ConvertY(curve, val, height);
                 
    curve->SetValue(t, x, y);
    
    mDirty = true;
}

#if 0
void
GraphControl9::SetCurveValuePoint(int curveNum, BL_FLOAT x, BL_FLOAT y)
{
    if (curveNum >= mNumCurves)
        return;
    
    if (curveNum >= mNumCurves)
        return;
    
    GraphCurve4 *curve = mCurves[curveNum];
    
    BL_FLOAT t = (x - curve->mMinX)/(curve->mMaxX - curve->mMinX);
    
    SetCurveValue(curveNum, t, y);
}
#endif

void
GraphControl9::SetCurveSingleValueH(int curveNum, BL_FLOAT val)
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
GraphControl9::SetCurveSingleValueV(int curveNum, BL_FLOAT val)
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
GraphControl9::PushCurveValue(int curveNum, BL_FLOAT val)
{
    IPlugBase::IMutexLock lock(mPlug);
    
    if (curveNum >= mNumCurves)
        return;
    
    int height = this->mRECT.H();
    
    GraphCurve4 *curve = mCurves[curveNum];
    val = ConvertY(curve, val, height);
    
    BL_FLOAT dummyX = 1.0;
    curve->PushValue(dummyX, val);
    
    BL_FLOAT maxXValue = this->mRECT.W();
    curve->NormalizeXValues(maxXValue);
    
    mDirty = true;
}

void
GraphControl9::DrawAxis(bool lineLabelFlag)
{
    if (mHAxis != NULL)
        DrawAxis(mHAxis, true, lineLabelFlag);
    
    if (mVAxis != NULL)
        DrawAxis(mVAxis, false, lineLabelFlag);
}

void
GraphControl9::DrawAxis(GraphAxis *axis, bool horizontal, bool lineLabelFlag)
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
            BL_FLOAT textOffset = FONT_SIZE*0.2;
            
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
                    // Draw a horizontal line
                    nvgBeginPath(mVg);
                
                    BL_FLOAT x0 = 0.0;
                    BL_FLOAT x1 = width;
                
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
GraphControl9::DrawCurves()
{
    for (int i = 0; i < mNumCurves; i++)
    {
        if (!mCurves[i]->mSingleValueH && !mCurves[i]->mSingleValueV)
        {
            if (mCurves[i]->mPointStyle)
            {
                if (!mCurves[i]->mPointsAsLines)
                    DrawPointCurve(mCurves[i]);
                else
                    DrawPointCurveLines(mCurves[i]);
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
GraphControl9::DrawLineCurve(GraphCurve4 *curve)
{
#if CURVE_DEBUG
    int numPointsDrawn = 0;
#endif
    
    nvgSave(mVg);
    
    SetCurveDrawStyle(curve);
    
    nvgBeginPath(mVg);
    
#if CURVE_OPTIM
    BL_FLOAT prevX = -1.0;
#endif
    
    bool firstPoint = true;
    for (int i = 0; i < curve->mXValues.GetSize(); i ++)
    {
        BL_FLOAT x = curve->mXValues.Get()[i];
        
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
        
        BL_FLOAT y = curve->mYValues.Get()[i];
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
    fprintf(stderr, "GraphControl9::DrawLineCurve - num points: %d\n", numPointsDrawn);
#endif
}

#if !FILL_CURVE_HACK
        // Bug with direct rendering
        // It seems we have no stencil, and we can only render convex polygons
void
GraphControl9::DrawFillCurve(GraphCurve *curve)
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
    
    BL_FLOAT x0 = 0.0;
    for (int i = 0; i < curve->mXValues.GetSize(); i ++)
    {        
        BL_FLOAT x = curve->mXValues.Get()[i];
        BL_FLOAT y = curve->mYValues.Get()[i];
        
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
GraphControl9::DrawFillCurve(GraphCurve4 *curve)
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
    
    BL_FLOAT prevX = -1.0;
    BL_FLOAT prevY = 0.0;
    for (int i = 0; i < curve->mXValues.GetSize() - 1; i ++)
    {
        BL_FLOAT x = curve->mXValues.Get()[i];
        
        if (x == GRAPH_VALUE_UNDEFINED)
            continue;
        
        BL_FLOAT y = curve->mYValues.Get()[i];
        
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
    
    // Fill upside if necessary
    if (curve->mFillAlphaUp)
        // Fill the upper area with specific alpha
    {
        int sFillColorUp[4] = { (int)(curve->mColor[0]*255), (int)(curve->mColor[1]*255),
            (int)(curve->mColor[2]*255), (int)(curve->mFillAlphaUp*255) };
        SWAP_COLOR(sFillColorUp);
        
        int height = this->mRECT.H();
        
        BL_FLOAT prevX = -1.0;
        BL_FLOAT prevY = 0.0;
        for (int i = 0; i < curve->mXValues.GetSize() - 1; i ++)
        {
            BL_FLOAT x = curve->mXValues.Get()[i];
            
            if (x == GRAPH_VALUE_UNDEFINED)
                continue;
            
            BL_FLOAT y = curve->mYValues.Get()[i];
            
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
    
#if CURVE_DEBUG
    fprintf(stderr, "GraphControl9::DrawFillCurve - num points: %d\n", numPointsDrawn);
#endif
}
#endif

void
GraphControl9::DrawLineCurveSVH(GraphCurve4 *curve)
{
    if (curve->mYValues.GetSize() == 0)
        return;
    
    BL_FLOAT val = curve->mYValues.Get()[0];
    
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
    
    BL_FLOAT x0 = 0;
    BL_FLOAT x1 = width;
    
    nvgMoveTo(mVg, x0, val);
    nvgLineTo(mVg, x1, val);
    
    nvgStroke(mVg);
    nvgRestore(mVg);
    
    mDirty = true;
}

void
GraphControl9::DrawFillCurveSVH(GraphCurve4 *curve)
{
    if (curve->mYValues.GetSize() == 0)
        return;
    
    BL_FLOAT val = curve->mYValues.Get()[0];
    
    if (val == GRAPH_VALUE_UNDEFINED)
        return;
    
    int width = this->mRECT.W();
    
    nvgSave(mVg);
    
    int sColor[4] = { (int)(curve->mColor[0]*255), (int)(curve->mColor[1]*255),
                      (int)(curve->mColor[2]*255), (int)(curve->mFillAlpha*255) };
    SWAP_COLOR(sColor);
    
    nvgStrokeColor(mVg, nvgRGBA(sColor[0], sColor[1], sColor[2], sColor[3]));
    
    nvgBeginPath(mVg);
        
    BL_FLOAT x0 = 0;
    BL_FLOAT x1 = width;
    BL_FLOAT y0 = 0;
    BL_FLOAT y1 = val;
    
    nvgMoveTo(mVg, x0, y0);
    nvgLineTo(mVg, x0, y1);
    nvgLineTo(mVg, x1, y1);
    nvgLineTo(mVg, x1, y0);
    
    nvgClosePath(mVg);
    
    nvgFillColor(mVg, nvgRGBA(sColor[0], sColor[1], sColor[2], sColor[3]));
	nvgFill(mVg);
    
    nvgStroke(mVg);
    
    // Fill upside if necessary
    if (curve->mFillAlphaUp)
        // Fill the upper area with specific alpha
    {
        int height = this->mRECT.H();
        
        int sFillColorUp[4] = { (int)(curve->mColor[0]*255), (int)(curve->mColor[1]*255),
                                (int)(curve->mColor[2]*255), (int)(curve->mFillAlphaUp*255) };
        SWAP_COLOR(sFillColorUp);
        
        nvgStrokeColor(mVg, nvgRGBA(sColor[0], sColor[1], sColor[2], sColor[3]));
        
        nvgBeginPath(mVg);
        
        BL_FLOAT x0 = 0;
        BL_FLOAT x1 = width;
        BL_FLOAT y0 = height;
        BL_FLOAT y1 = val;
        
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
}

void
GraphControl9::DrawLineCurveSVV(GraphCurve4 *curve)
{
    // Finally, take the Y value
    // We will have to care about the curve Y scale !
    if (curve->mYValues.GetSize() == 0)
        return;
    
    int width = this->mRECT.W();
    int height = this->mRECT.H();
    
    BL_FLOAT val = curve->mYValues.Get()[0];
    
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
    
    BL_FLOAT y0 = 0;
    BL_FLOAT y1 = height;
    
    BL_FLOAT x = val;
    
    nvgMoveTo(mVg, x, y0);
    nvgLineTo(mVg, x, y1);
    
    nvgStroke(mVg);
    nvgRestore(mVg);
    
    mDirty = true;
}

// Fill right
// (only, for the moment)
void
GraphControl9::DrawFillCurveSVV(GraphCurve4 *curve)
{
    // Finally, take the Y value
    // We will have to care about the curve Y scale !
    if (curve->mYValues.GetSize() == 0)
        return;
    
    int width = this->mRECT.W();
    int height = this->mRECT.H();
    
    BL_FLOAT val = curve->mYValues.Get()[0];
    
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
    
    BL_FLOAT y0 = 0;
    BL_FLOAT y1 = height;
    BL_FLOAT x0 = val;
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

#if 0 // OLD ? => TODO: check and remove
// TEST: draw lines instead of points
void
GraphControl9::DrawPointCurveLines(GraphCurve4 *curve)
{
    nvgSave(mVg);
    
    SetCurveDrawStyle(curve);
    
    int width = this->mRECT.W();
    BL_FLOAT pointSize = curve->mPointSize; ///width;
    
    nvgBeginPath(mVg);
    
    bool firstPoint = true;
    for (int i = 0; i < curve->mXValues.GetSize(); i ++)
    {
        BL_FLOAT x = curve->mXValues.Get()[i];
        
        if (x == GRAPH_VALUE_UNDEFINED)
            continue;
        
        BL_FLOAT y = curve->mYValues.Get()[i];
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
}
#endif

// Optimized !
void
GraphControl9::DrawPointCurve(GraphCurve4 *curve)
{
#define FILL_RECTS 1
    
    nvgSave(mVg);
    
    SetCurveDrawStyle(curve);
    
    BL_FLOAT pointSize = curve->mPointSize;
    
#if !FILL_RECTS
    nvgBeginPath(mVg);
#endif
    
    for (int i = 0; i < curve->mXValues.GetSize(); i ++)
    {
        BL_FLOAT x = curve->mXValues.Get()[i];
        
        if (x == GRAPH_VALUE_UNDEFINED)
            continue;
        
        BL_FLOAT y = curve->mYValues.Get()[i];
        if (y == GRAPH_VALUE_UNDEFINED)
            continue;
    
        if (curve->mWeights.GetSize() == curve->mXValues.GetSize())
        {
            BL_FLOAT weight = curve->mWeights.Get()[i];
            SetCurveDrawStyleWeight(curve, weight);
        }
        
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
}

// TEST (to display lines instead of points)
void
GraphControl9::DrawPointCurveLines(GraphCurve4 *curve)
{    
    nvgSave(mVg);
    
    SetCurveDrawStyle(curve);
    
    BL_FLOAT pointSize = curve->mPointSize;
    nvgStrokeWidth(mVg, pointSize);
    
    int width = this->mRECT.W();
    
#if !FILL_RECTS
    nvgBeginPath(mVg);
#endif
    
    for (int i = 0; i < curve->mXValues.GetSize(); i ++)
    {
        BL_FLOAT x = curve->mXValues.Get()[i];
        
        if (x == GRAPH_VALUE_UNDEFINED)
            continue;
        
        BL_FLOAT y = curve->mYValues.Get()[i];
        if (y == GRAPH_VALUE_UNDEFINED)
            continue;
        
        nvgBeginPath(mVg);
        nvgMoveTo(mVg, width/2.0, 0.0);
        nvgLineTo(mVg, x, y);
        nvgStroke(mVg);
    }
    
    nvgRestore(mVg);
    
    mDirty = true;
}

void
GraphControl9::AutoAdjust()
{
    // First, compute the maximum value of all the curves
    BL_FLOAT max = -1e16;
    for (int i = 0; i < mNumCurves; i++)
    {
        GraphCurve4 *curve = mCurves[i];
        
        for (int j = curve->mYValues.GetSize(); j >= 0; j--)
        {
            BL_FLOAT val = curve->mYValues.Get()[j];
            
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
GraphControl9::MillisToPoints(long long int elapsed, int sampleRate, int numSamplesPoint)
{
    BL_FLOAT numSamples = (((BL_FLOAT)elapsed)/1000.0)*sampleRate;
    
    BL_FLOAT numPoints = numSamples/numSamplesPoint;
    
    return numPoints;
}

void
GraphControl9::InitFont(const char *fontPath)
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
GraphControl9::DrawText(BL_FLOAT x, BL_FLOAT y, BL_FLOAT fontSize,
                        const char *text, int color[4],
                        int halign, int valign)
{
    DrawText(mVg, x, y, fontSize, text, color, halign, valign);
}

void
GraphControl9::DrawGraph()
{
    IPlugBase::IMutexLock lock(mPlug);
    
    // Update first, before displaying
    if (mMustUpdateSpectrogram)
    {
        DoUpdateSpectrogram();
        
        mMustUpdateSpectrogram = false;
        mMustUpdateSpectrogramData = false;
        mMustUpdateSpectrogramFullData = false;
    }
    
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

    // Valgrind: error here ?
    
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
    
    CustomDrawersPreDraw();
    
    // Draw spectrogram first
    nvgSave(mVg);
    
    // New: set colormap only in the spectrogram state
#if GLSL_COLORMAP
    nvgSetColormap(mVg, mNvgColormapImage);
#endif
    
    DrawSpectrogram();
    nvgRestore(mVg);
    
    DrawAxis(true);
    
    nvgSave(mVg);
    
    DrawCurves();
    
    nvgRestore(mVg);
    
    DrawAxis(false);
    
    DisplayCurveDescriptions();
    
    //DrawSpectrogram();
    
    CustomDrawersPostDraw();
    
    nvgEndFrame(mVg);
}

BL_FLOAT
GraphControl9::ConvertX(GraphCurve4 *curve, BL_FLOAT val, BL_FLOAT width)
{
    BL_FLOAT x = val;
    if (x != GRAPH_VALUE_UNDEFINED)
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

BL_FLOAT
GraphControl9::ConvertY(GraphCurve4 *curve, BL_FLOAT val, BL_FLOAT height)
{
    BL_FLOAT y = val;
    if (y != GRAPH_VALUE_UNDEFINED)
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
GraphControl9::SetCurveDrawStyle(GraphCurve4 *curve)
{
    nvgStrokeWidth(mVg, curve->mLineWidth);
    
    int sColor[4] = { (int)(curve->mColor[0]*255), (int)(curve->mColor[1]*255),
        (int)(curve->mColor[2]*255), (int)(curve->mAlpha*255) };
    SWAP_COLOR(sColor);

    nvgStrokeColor(mVg, nvgRGBA(sColor[0], sColor[1], sColor[2], sColor[3]));
    
    int sFillColor[4] = { (int)(curve->mColor[0]*255), (int)(curve->mColor[1]*255),
        (int)(curve->mColor[2]*255), (int)(curve->mFillAlpha*255) };
    SWAP_COLOR(sFillColor);
    
    nvgFillColor(mVg, nvgRGBA(sFillColor[0], sFillColor[1], sFillColor[2], sFillColor[3]));
}

void
GraphControl9::SetCurveDrawStyleWeight(GraphCurve4 *curve, BL_FLOAT weight)
{
    //nvgStrokeWidth(mVg, curve->mLineWidth);
    
    int sColor[4] = { (int)(curve->mColor[0]*255*weight), (int)(curve->mColor[1]*255*weight),
                      (int)(curve->mColor[2]*255*weight), (int)(curve->mAlpha*255*weight) };
    for (int i = 0; i < 4; i++)
    {
        if (sColor[i] < 0)
            sColor[i] = 0;
        
        if (sColor[i] > 255)
            sColor[i] = 255;
    }
    SWAP_COLOR(sColor);
    
    nvgStrokeColor(mVg, nvgRGBA(sColor[0], sColor[1], sColor[2], sColor[3]));
    
    int sFillColor[4] = { (int)(curve->mColor[0]*255*weight), (int)(curve->mColor[1]*255*weight),
                          (int)(curve->mColor[2]*255*weight), (int)(curve->mFillAlpha*255*weight) };
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

void
GraphControl9::DoUpdateSpectrogram()
{    
    //int w = mSpectrogram->GetMaxNumCols();
    int w = mSpectrogram->GetNumCols();
    int h = mSpectrogram->GetHeight();
    
#if 1 // Avoid white image when there is no data
    if (w == 0)
    {
        w = 1;
        int imageSize = w*h*4;
        
        mSpectroImageData.Resize(imageSize);
        memset(mSpectroImageData.Get(), 0, imageSize);

        // Spectrogram image
        if (mNvgSpectroImage != 0)
            nvgDeleteImage(mVg, mNvgSpectroImage);
        
        mNvgSpectroImage = nvgCreateImageRGBA(mVg,
                                              w, h,
#if USE_SPECTRO_NEAREST
                                              NVG_IMAGE_NEAREST |
#endif

#if GLSL_COLORMAP
                                              NVG_IMAGE_ONE_FLOAT_FORMAT
#endif
                                              ,
                                              mSpectroImageData.Get());
        // Spectrogram full image
        if (mNvgSpectroFullImage != 0)
            nvgDeleteImage(mVg, mNvgSpectroFullImage);
        
        mNvgSpectroFullImage = nvgCreateImageRGBA(mVg,
                                                  w, h,
#if USE_SPECTRO_NEAREST
                                                  NVG_IMAGE_NEAREST |
#endif
                                              
#if GLSL_COLORMAP
                                                  NVG_IMAGE_ONE_FLOAT_FORMAT
#endif
                                                  ,
                                                  mSpectroImageData.Get());
        
        mDirty = true;
        
        return;
    }
#endif
    
   int imageSize = w*h*4;
   if (mMustUpdateSpectrogramData)
   {
       if (mSpectroImageData.GetSize() != imageSize)
       {
           mSpectroImageData.Resize(imageSize);
            
           memset(mSpectroImageData.Get(), 0, imageSize);
           mSpectrogram->GetImageDataFloat(w, h, mSpectroImageData.Get());
           
           // Spectrogram image
           if (mNvgSpectroImage != 0)
               nvgDeleteImage(mVg, mNvgSpectroImage);
            
            mNvgSpectroImage = nvgCreateImageRGBA(mVg,
                                                  w, h,
#if USE_SPECTRO_NEAREST
                                                  NVG_IMAGE_NEAREST |
#endif
                                                  
#if GLSL_COLORMAP
                                                  NVG_IMAGE_ONE_FLOAT_FORMAT
#endif
                                                  ,
                                                  mSpectroImageData.Get());

           if (mMustUpdateSpectrogramFullData)
           {
               // Spectrogram full image
               if (mNvgSpectroFullImage != 0)
                   nvgDeleteImage(mVg, mNvgSpectroFullImage);
           
               mNvgSpectroFullImage = nvgCreateImageRGBA(mVg,
                                                         w, h,
#if USE_SPECTRO_NEAREST
                                                         NVG_IMAGE_NEAREST |
#endif
                                                 
#if GLSL_COLORMAP
                                                         NVG_IMAGE_ONE_FLOAT_FORMAT
#endif
                                                         ,
                                                         mSpectroImageData.Get());
           }
        } else
        {
            memset(mSpectroImageData.Get(), 0, imageSize);
            mSpectrogram->GetImageDataFloat(w, h, mSpectroImageData.Get());
            
            // Spectrogram image
            nvgUpdateImage(mVg, mNvgSpectroImage, mSpectroImageData.Get());

            // Spectrogram full image
            if (mMustUpdateSpectrogramFullData)
            {
                nvgUpdateImage(mVg, mNvgSpectroFullImage, mSpectroImageData.Get());
            }
        }
    }
    
    // Colormap
    WDL_TypedBuf<unsigned int> colorMapData;
    mSpectrogram->GetColormapImageDataRGBA(&colorMapData);
    
    if (colorMapData.GetSize() != mColormapImageData.GetSize())
    {
        mColormapImageData = colorMapData;
        
        if (mNvgColormapImage != 0)
            nvgDeleteImage(mVg, mNvgColormapImage);
        
        mNvgColormapImage = nvgCreateImageRGBA(mVg,
                                               mColormapImageData.GetSize(), 1, NVG_IMAGE_NEAREST /*0*/,
                                               (unsigned char *)mColormapImageData.Get());
    } else
    {
        mColormapImageData = colorMapData;
        
        nvgUpdateImage(mVg, mNvgColormapImage, (unsigned char *)mColormapImageData.Get());
    }
    
    mDirty = true;
}

void
GraphControl9::DrawSpectrogram()
{
    if (!mShowSpectrogram)
        return;
    
    int width = this->mRECT.W();
    int height = this->mRECT.H();
    
#define DEBUG_DISPLAY_FG 1
#define DEBUG_DISPLAY_BG 0
    
    nvgSave(mVg);
    
#if DEBUG_DISPLAY_BG
    //
    // Spectrogram full image
    //
    // Display the full image first, so it will be in background
    
    NVGpaint fullImgPaint = nvgImagePattern(mVg, 0.0, 0.0, width, height,
                                            0.0, mNvgSpectroFullImage, mSpectrogramAlpha);
    
#if USE_ZOOM_NVG_TRANSFORM
    nvgResetTransform(mVg);
    
    BL_FLOAT absZoom = mSpectroAbsMaxX - mSpectroAbsMinX;
    
    nvgTranslate(mVg, mSpectroAbsMinX*width, 0.0);
    nvgScale(mVg, absZoom, 1.0);
#endif
    
    nvgBeginPath(mVg);
    
    // Corner (x, y) => bottom-left
    nvgRect(mVg,
            mSpectrogramBounds[0]*width,
            mSpectrogramBounds[1]*height,
            mSpectrogramBounds[2]*width,
            mSpectrogramBounds[3]*height);
    
    nvgFillPaint(mVg, fullImgPaint);
    nvgFill(mVg);
    
#if USE_ZOOM_NVG_TRANSFORM
    nvgResetTransform(mVg); //
#endif

#endif

#if DEBUG_DISPLAY_FG
    //
    // Spectrogram image
    //

    // Display the rightmost par in case of zoom
    NVGpaint imgPaint = nvgImagePattern(mVg, 0.0, 0.0, width, height,
                                        0.0, mNvgSpectroImage, mSpectrogramAlpha);
#endif
    
    
#if USE_ZOOM_NVG_TRANSFORM
    nvgResetTransform(mVg);
    
    BL_FLOAT zoom = mSpectroMaxX - mSpectroMinX;
    
    nvgTranslate(mVg, mSpectroMinX*width, 0.0);
    nvgScale(mVg, zoom, 1.0);
#endif
    
    nvgBeginPath(mVg);
    nvgRect(mVg,
            mSpectrogramBounds[0]*width,
            mSpectrogramBounds[1]*height,
            mSpectrogramBounds[2]*width,
            mSpectrogramBounds[3]*height);

            
    nvgFillPaint(mVg, imgPaint);
    nvgFill(mVg);
    
#if USE_ZOOM_NVG_TRANSFORM
    nvgResetTransform(mVg); //
#endif
    
    nvgRestore(mVg);
}

void
GraphControl9::InitNanoVg()
{
    // NVG_NIKO_ANTIALIAS_SKIP_FRINGES :
    // For Spectrogram, colormap, and display one image over the other
    // => The borders of the image were black
    mVg = nvgCreateGL2(NVG_ANTIALIAS
                       | NVG_NIKO_ANTIALIAS_SKIP_FRINGES
                       | NVG_STENCIL_STROKES
                       //| NVG_DEBUG
                       );
	if (mVg == NULL)
		return;
    
    InitFont(mFontPath.Get());
}

void
GraphControl9::ExitNanoVg()
{
    nvgDeleteGL2(mVg);
}

