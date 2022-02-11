//
//  Graph.cpp
//  Transient
//
//  Created by Apple m'a Tuer on 03/09/17.
//
//

#include <math.h>

#include "Graph.h"

#define FILL_CURVE_ALPHA 0.125


GraphCurve::GraphCurve()
{
    mPrevValue = 0.0;
    mPrevValueSet = false;
    
    mCurveFill = false;
    mFillAlpha = FILL_CURVE_ALPHA;
    
    mSingleValue = false;
    mSqrtScale = false;
}

GraphCurve::~GraphCurve() {}

Graph::Graph(IBitmapControl *bitmapControl, IBitmap *bitmap)
{
    mGraphControl = bitmapControl;
    mBitmap = (LICE_MemBitmap *)bitmap->mData;
    
    mSaveBitmap = new LICE_MemBitmap();
    mSaveBitmap->resize(mBitmap->getWidth(), mBitmap->getHeight());
    BlitBitmap(mBitmap, mSaveBitmap);
    
    mBackBitmap = new LICE_MemBitmap();
    mBackBitmap->resize(mBitmap->getWidth(), mBitmap->getHeight());
    BlitBitmap(mBitmap, mBackBitmap);
    
    mClearColor = LICE_RGBA(0, 0, 0, 255);
}
    
Graph::~Graph()
{
    delete mSaveBitmap;
    delete mBackBitmap;
}

void
Graph::SetClearColor(int r, int g, int b, int a)
{
    mClearColor = LICE_RGBA(r, g, b, a);
}

void
Graph::SetCurveColor(int curveNum, int r, int g, int b, int a)
{
    if (curveNum >= MAX_NUM_CURVES)
        return;
    
    mCurves[curveNum].mColor = LICE_RGBA(r, g, b, a);
}

void
Graph::SetCurveSmooth(int curveNum, bool flag)
{
    if (curveNum >= MAX_NUM_CURVES)
        return;
    
    mCurves[curveNum].mDoSmooth = flag;
}


void
Graph::SetCurveFill(int curveNum, bool flag)
{
    if (curveNum >= MAX_NUM_CURVES)
        return;

    mCurves[curveNum].mCurveFill = flag;
}

void
Graph::SetCurveFillAlpha(int curveNum, double alpha)
{
    if (curveNum >= MAX_NUM_CURVES)
        return;
    
    mCurves[curveNum].mFillAlpha = alpha;
}

void
Graph::SetCurveSingleValue(int curveNum, bool flag)
{
    if (curveNum >= MAX_NUM_CURVES)
        return;
    
    mCurves[curveNum].mSingleValue = flag;
}

void
Graph::SetCurveSqrtScale(int curveNum, bool flag)
{
    if (curveNum >= MAX_NUM_CURVES)
        return;

    mCurves[curveNum].mSqrtScale = flag;
}

void
Graph::Update()
{
    BlitBitmap(mBackBitmap, mBitmap);
    mGraphControl->SetDirty();
}

void
Graph::Clear()
{
    LICE_Clear(mBackBitmap, mClearColor);
}

void
Graph::SaveBitmap()
{
    BlitBitmap(mBackBitmap, mSaveBitmap);
}

void
Graph::RestoreBitmap()
{
    BlitBitmap(mSaveBitmap, mBackBitmap);
}

void
Graph::AddPointValue(double val, int r, int g, int b)
{
    int y = (int)((1.0 - val)*mBackBitmap->getHeight());
    int x = mBackBitmap->getWidth() - 1;
    
    LICE_PutPixel(mBackBitmap, x, y,
                  LICE_RGBA(r, g, b, 255), 1.0, LICE_BLIT_MODE_COPY);
}

void
Graph::AddLineValue(int curveNum, double val)
{
    if (curveNum >= MAX_NUM_CURVES)
        return;
    
    if (mCurves[curveNum].mSqrtScale)
    {
        val = sqrt(val);
    }
    
    // Smooth
    if (mCurves[curveNum].mDoSmooth)
    {
        mCurves[curveNum].mParamSmoother.SetNewValue(val);
        mCurves[curveNum].mParamSmoother.Update();
        val = mCurves[curveNum].mParamSmoother.GetCurrentValue();
    }
    
    if (!mCurves[curveNum].mPrevValueSet)
    {
        mCurves[curveNum].mPrevValue = val;
        mCurves[curveNum].mPrevValueSet = true;
    }
    
    if (mCurves[curveNum].mSingleValue)
    {
        if (mCurves[curveNum].mCurveFill)
            FillCurveSV(val, mCurves[curveNum].mColor, mCurves[curveNum].mFillAlpha);
        
        DrawCurveSV(val, mCurves[curveNum].mPrevValue, mCurves[curveNum].mColor);
    }
    
    if (mCurves[curveNum].mCurveFill)
        FillCurve(val, mCurves[curveNum].mColor, mCurves[curveNum].mFillAlpha);
    
    DrawCurve(val, mCurves[curveNum].mPrevValue, mCurves[curveNum].mColor);
    
    mCurves[curveNum].mPrevValue = val;
}

void
Graph::ScrollLeft()
{
    RECT srcrect;
    srcrect.left = 1;
    srcrect.right = mBackBitmap->getWidth();
    srcrect.top = 0;
    srcrect.bottom = mBackBitmap->getHeight();
    
    LICE_Blit(mBackBitmap, mBackBitmap, 0, 0,
              &srcrect, 1.0, LICE_BLIT_MODE_COPY);
    
    int x = mBackBitmap->getWidth() - 1;
    
    // Set the new line black
    for (int i = 0; i < mBackBitmap->getHeight(); i++)
    {
        LICE_PutPixel(mBackBitmap, x, i,
                      mClearColor, 1.0, LICE_BLIT_MODE_COPY);
    }
}

void
Graph::BlitBitmap(LICE_MemBitmap *srcBitmap, LICE_MemBitmap *dstBitmap)
{
    RECT srcrect;
    srcrect.left = 0;
    srcrect.right = srcBitmap->getWidth();
    srcrect.top = 0;
    srcrect.bottom = srcBitmap->getHeight();
    
    LICE_Blit(dstBitmap, srcBitmap, 0, 0,
              &srcrect, 1.0, LICE_BLIT_MODE_COPY);
}

void
Graph::DrawCurve(double val, double prevValue, LICE_pixel color)
{
    float x0 = mBackBitmap->getWidth() - 2;
    float y0 = (1.0 - prevValue)*mBackBitmap->getHeight();
    
    float x1 = mBackBitmap->getWidth() - 1;
    float y1 = (1.0 - val)*mBackBitmap->getHeight();
    
    LICE_Line(mBackBitmap, x0, y0, x1, y1, color);
}

void
Graph::FillCurve(double val, LICE_pixel color, double alpha)
{
    int x = mBackBitmap->getWidth() - 1;
    int y = (int)((1.0 - val)*mBackBitmap->getHeight());
    
    for (int j = y; j < mBackBitmap->getHeight(); j++)
    {
        LICE_PutPixel(mBackBitmap, x, j,
                      color, alpha, LICE_BLIT_MODE_COPY);
    }
}

void
Graph::DrawCurveSV(double val, double prevValue, LICE_pixel color)
{
    int x0 = 0;
    
    int y0 = (int)((1.0 - val)*mBackBitmap->getHeight());
    int y1 = y0 + 1;
    if (y1 >= mBackBitmap->getHeight())
        y1 = mBackBitmap->getHeight() - 1;
    int height = y1 - y0;
    
    LICE_FillRect(mBackBitmap, x0, y0, mBackBitmap->getWidth(), height,
                  color, 1.0);
}

void
Graph::FillCurveSV(double val, LICE_pixel color, double alpha)
{
    int x0 = 0;
    int y0 = (int)((1.0 - val)*mBackBitmap->getHeight());
    
    int height = mBackBitmap->getHeight() - y0;
    
    LICE_FillRect(mBackBitmap, x0, y0, mBackBitmap->getWidth(), height,
                  color, alpha);
}
