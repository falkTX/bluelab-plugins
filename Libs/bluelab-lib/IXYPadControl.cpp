#include "IXYPadControl.h"

IXYPadControl::IXYPadControl(const IRECT& bounds,
                             const std::initializer_list<int>& params,
                             const IBitmap& trackBitmap,
                             const IBitmap& handleBitmap,
                             float borderSize)
: IControl(bounds, params)
{
    mTrackBitmap = trackBitmap;
    mHandleBitmap = handleBitmap;

    mBorderSize = borderSize;
    
    mMouseDown = false;
}

IXYPadControl::~IXYPadControl() {}

void
IXYPadControl::Draw(IGraphics& g)
{
    DrawTrack(g);
    DrawHandle(g);
}

void
IXYPadControl::OnMouseDown(float x, float y, const IMouseMod& mod)
{
    mMouseDown = true;

    OnMouseDrag(x, y, 0., 0., mod);
}

void
IXYPadControl::OnMouseUp(float x, float y, const IMouseMod& mod)
{
    mMouseDown = false;
    SetDirty(true);
}

void
IXYPadControl::OnMouseDrag(float x, float y, float dX, float dY,
                           const IMouseMod& mod)
{
    mRECT.Constrain(x, y);
    
    float xn = x;
    float yn = y;
    PixelsToParams(&xn, &yn);
    
    SetValue(xn, 0);
    SetValue(yn, 1);
    SetDirty(true);
}
  
void
IXYPadControl::DrawTrack(IGraphics& g)
{
    IBlend blend = GetBlend();
    g.DrawBitmap(mTrackBitmap,
                 GetRECT().GetCentredInside(IRECT(0, 0, mTrackBitmap)),
                 0, &blend);
}

void
IXYPadControl::DrawHandle(IGraphics& g)
{
    float xn = GetValue(0);
    float yn = GetValue(1);

    yn = 1.0 - yn;
    
    float x = xn;
    float y = yn;
    ParamsToPixels(&x, &y);

    int w = mHandleBitmap.W();
    int h = mHandleBitmap.H();

    IRECT handleRect(x, y, x + w, y + h);
    
    IBlend blend = GetBlend();
    g.DrawBitmap(mHandleBitmap, handleRect, 0, &blend);
}

void
IXYPadControl::PixelsToParams(float *x, float *y)
{
    float w = mHandleBitmap.W();
    float h = mHandleBitmap.H();

    *x = (*x - (mRECT.L + w/2 + mBorderSize)) /
        (mRECT.W() - w - mBorderSize*2.0);
    *y = 1.f - ((*y - (mRECT.T + h/2 + mBorderSize)) /
                (mRECT.H() - h - mBorderSize*2.0));

    // Bounds
    if (*x < 0.0)
        *x = 0.0;
    if (*x > 1.0)
        *x = 1.0;

    if (*y < 0.0)
        *y = 0.0;
    if (*y > 1.0)
        *y = 1.0;
}

void
IXYPadControl::ParamsToPixels(float *x, float *y)
{
    float w = mHandleBitmap.W();
    float h = mHandleBitmap.H();
    
    *x = mRECT.L + mBorderSize + (*x)*(mRECT.W() - w - mBorderSize*2.0);
    *y = mRECT.T + mBorderSize + (*y)*(mRECT.H() - h - mBorderSize*2.0);
}
