//
//  Graph.cpp
//  Transient
//
//  Created by Apple m'a Tuer on 03/09/17.
//
//

#include <math.h>

#include "../../WDL/IPlug/IPlugBase.h"

#include "Utils.h"
#include "GraphControl.h"


GraphControl::GraphControl(IPlugBase *pPlug, IRECT pR,
                           int numCurves, int numCurveValue)
: IControl(pPlug, pR),
  mAutoAdjustParamSmoother(1.0, 0.9),
  mNumCurves(numCurves)
{
    for (int i = 0; i < mNumCurves; i++)
    {
        GraphCurve *curve = new GraphCurve(numCurveValue);
        
        mCurves.push_back(curve);
    }
    
    mAutoAdjustFlag = false;
    mAutoAdjustFactor = 1.0;
    
    mYScaleFactor = 1.0;
    
    mXdBScale = false;
    mMinXdB = 0.0;
    mMaxXdB = 1.0;
    
    // Cairo
    mSurface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, this->mRECT.W(), this->mRECT.H());
    mCr = cairo_create(mSurface);
    
    SET_COLOR_FROM_INT(mClearColor, 0, 0, 0, 255);
    
    mDirty = true;
}
    
GraphControl::~GraphControl()
{
    cairo_destroy(mCr);
    cairo_surface_destroy(mSurface);
    
    for (int i = 0; i < mNumCurves; i++)
        delete mCurves[i];
}

bool
GraphControl::IsDirty()
{
    return mDirty;
}

bool
GraphControl::Draw(IGraphics *pGraphics)
{
    // Must lock otherwise with DATA_STEP, curve will jerk
    IPlugBase::IMutexLock lock(mPlug);
    
    Clear();
    
    if (mAutoAdjustFlag)
    {
        AutoAdjust();
    }
    
    DrawCurves();
    
    cairo_surface_flush(mSurface);
    
    // And we render
    unsigned int *data = (unsigned int*)cairo_image_surface_get_data(mSurface);
    
    SwapChannels(data, this->mRECT.W()*this->mRECT.H());
    
    // This is the important part where you bind the cairo data to LICE
    LICE_WrapperBitmap wrapperBitmap(data, this->mRECT.W(), this->mRECT.H(),
                                    this->mRECT.W(), false);
    
    // And we render
    IBitmap result(&wrapperBitmap, wrapperBitmap.getWidth(), wrapperBitmap.getHeight());
    return pGraphics->DrawBitmap(&result, &this->mRECT);
}

void
GraphControl::SetXdBScale(bool flag, double minXdB, double maxXdB)
{
    mXdBScale = flag;
    
    mMinXdB = minXdB;
    mMaxXdB = maxXdB;
}

void
GraphControl::SetAutoAdjust(bool flag, double smoothCoeff)
{
    mAutoAdjustFlag = flag;
    
    mAutoAdjustParamSmoother.SetSmoothCoeff(smoothCoeff);
    
    mDirty = true;
}

void
GraphControl::SetYScaleFactor(double factor)
{
    mYScaleFactor = factor;
    
    mDirty = true;
}

void
GraphControl::SetClearColor(int r, int g, int b, int a)
{
    SET_COLOR_FROM_INT(mClearColor, r, g, b, a);
    
    mDirty = true;
}

void
GraphControl::SetCurveColor(int curveNum, int r, int g, int b)
{
    if (curveNum >= mNumCurves)
        return;
    
    SET_COLOR_FROM_INT(mCurves[curveNum]->mColor, r, g, b, 255);
    
    mDirty = true;
}

void
GraphControl::SetCurveAlpha(int curveNum, double alpha)
{
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->mAlpha = alpha;
    
    mDirty = true;
}


void
GraphControl::SetCurveLineWidth(int curveNum, double lineWidth)
{
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->mLineWidth = lineWidth;
    
    mDirty = true;
}

void
GraphControl::SetCurveSmooth(int curveNum, bool flag)
{
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->mDoSmooth = flag;
    
    mDirty = true;
}


void
GraphControl::SetCurveFill(int curveNum, bool flag)
{
    if (curveNum >= mNumCurves)
        return;

    mCurves[curveNum]->mCurveFill = flag;
    
    mDirty = true;
}

void
GraphControl::SetCurveFillAlpha(int curveNum, double alpha)
{
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->mFillAlpha = alpha;
    
    mDirty = true;
}

void
GraphControl::SetCurveSingleValue(int curveNum, bool flag)
{
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->mSingleValue = flag;
    
    mDirty = true;
}

void
GraphControl::Clear()
{    
    cairo_save(mCr);
    cairo_set_source_rgb(mCr, mClearColor[0], mClearColor[1], mClearColor[2]);
    
    cairo_paint(mCr);
    cairo_restore(mCr);
    
    mDirty = true;
}

void
GraphControl::SetCurveYdBScale(int curveNum, bool flag, double minYdB, double maxYdB)
{
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->SetYdBScale(flag, minYdB, maxYdB);
}

void
GraphControl::SetCurveValues(int curveNum, const WDL_TypedBuf<double> *values)
{
    // Must lock otherwise with DATA_STEP, curve will jerk
    IPlugBase::IMutexLock lock(mPlug);
    
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->SetValues(values);
    
    mDirty = true;
}

void
GraphControl::SetCurveValue(int curveNum, double t, double val)
{
    // Must lock otherwise with DATA_STEP, curve will jerk
    IPlugBase::IMutexLock lock(mPlug);
    
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->SetValue(t, val);
    
    mDirty = true;
}

void
GraphControl::PushCurveValue(int curveNum, double val)
{
    // Must lock otherwise with DATA_STEP, curve will jerk
    IPlugBase::IMutexLock lock(mPlug);
    
    if (curveNum >= mNumCurves)
        return;
    
    mCurves[curveNum]->PushValue(val);
    
    mDirty = true;
}

void
GraphControl::DrawCurves()
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
GraphControl::DrawLineCurve(const WDL_TypedBuf<double> *points, CurveColor color,
                            double alpha, double lineWidth,
                            GraphCurve *curve)
{
    int width = this->mRECT.W();
    int height = this->mRECT.H();
    
    cairo_save(mCr);
    
    cairo_set_operator(mCr, CAIRO_OPERATOR_OVER);
    cairo_set_source_rgba(mCr, color[0], color[1], color[2], alpha);
    
    cairo_set_line_width(mCr, lineWidth);
    
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
        
        double y = (1.0 - val*mAutoAdjustFactor*mYScaleFactor) * height;
        
        if (i == 0)
            cairo_move_to(mCr, x, y);
        
        cairo_line_to(mCr, x, y);
    }
    
    cairo_stroke(mCr);
    
    cairo_restore(mCr);
    
    mDirty = true;
}

void
GraphControl::DrawFillCurve(const WDL_TypedBuf<double> *points, CurveColor color, double alpha,
                            GraphCurve *curve)
{
    int width = this->mRECT.W();
    int height = this->mRECT.H();
    
    cairo_save(mCr);
    
    cairo_set_operator(mCr, CAIRO_OPERATOR_OVER);
    cairo_set_source_rgba(mCr, color[0], color[1], color[2], alpha);
    
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
        
        double y = (1.0 - val*mAutoAdjustFactor*mYScaleFactor) * height;
        
        if (i == 0)
        {
            x0 = x;
            cairo_move_to(mCr, x0, height - 1);
        }
        
        cairo_line_to(mCr, x, y);
        
        if (i >= points->GetSize() - 1)
            // Close
        {
            cairo_line_to(mCr, x, height - 1);
            cairo_line_to(mCr, x0, height -1);
        }
    }
    
    cairo_fill(mCr);
    
    cairo_restore(mCr);
    
    mDirty = true;
}

void
GraphControl::DrawLineCurveSV(double val, CurveColor color, double alpha, double lineWidth,
                              GraphCurve *curve)
{
    int width = this->mRECT.W();
    int height = this->mRECT.H();
    
    cairo_save(mCr);
    
    cairo_set_operator(mCr, CAIRO_OPERATOR_OVER);
    cairo_set_source_rgba(mCr, color[0], color[1], color[2], alpha);
    
    cairo_set_line_width(mCr, lineWidth);
    
    double x0 = 0;
    double x1 = width;
    
    //if (curve->mYdBScale)
    //    val = Utils::NormalizedYTodB(val, curve->mMinYdB, curve->mMaxYdB);
    
    //val *= mYScaleFactor;
    
    double y = (1.0 - val*mAutoAdjustFactor)*height;
    
    cairo_move_to(mCr, x0, y);
        
    cairo_line_to(mCr, x1, y);
    
    cairo_stroke(mCr);
    
    cairo_restore(mCr);
    
    mDirty = true;
}

void
GraphControl::DrawFillCurveSV(double val, CurveColor color, double alpha,
                              GraphCurve *curve)
{
    int width = this->mRECT.W();
    int height = this->mRECT.H();
    
    cairo_save(mCr);
    
    cairo_set_operator(mCr, CAIRO_OPERATOR_OVER);
    cairo_set_source_rgba(mCr, color[0], color[1], color[2], alpha);
    
    //if (curve->mYdBScale)
    //    val = Utils::NormalizedYTodB(val, curve->mMinYdB, curve->mMaxYdB);
    
    // val *= *YScaleFactor;
    
    double x0 = 0;
    double x1 = width;
    double y0 = height;
    double y1 = (1.0 - val*mAutoAdjustFactor)*height;
    
    cairo_move_to(mCr, x0, y0);

    cairo_line_to(mCr, x1, y0);
    cairo_line_to(mCr, x1, y1);
    cairo_line_to(mCr, x0, y1);
    cairo_line_to(mCr, x0, y0);
    
    cairo_fill(mCr);
    
    cairo_restore(mCr);
    
    mDirty = true;
}

void
GraphControl::AutoAdjust()
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
GraphControl::SwapChannels(unsigned int *data, int numPixels)
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
