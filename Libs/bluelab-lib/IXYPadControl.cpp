#include "IXYPadControl.h"

IXYPadControl::IXYPadControl(const IRECT& bounds,
                             const std::initializer_list<int>& params,
                             const IBitmap& trackBitmap,
                             const IBitmap& handleBitmap,
                             float borderSize, bool reverseY)
: IControl(bounds, params)
{
    mTrackBitmap = trackBitmap;
    mHandleBitmap = handleBitmap;

    mBorderSize = borderSize;

    mReverseY = reverseY;
    
    mMouseDown = false;

    mOffsetX = 0.0;
    mOffsetY = 0.0;

    mPrevX = 0.0;
    mPrevY = 0.0;
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
#ifndef __linux__
  if (mod.A)
  {
    SetValueToDefault(GetValIdxForPos(x, y));

    return;
  }
#else
  // On Linux, Alt+click does nothing (at least on my xubuntu)
  // So use Ctrl-click instead to reset parameters
  if (mod.C)
  {
    SetValueToDefault(GetValIdxForPos(x, y));

    return;
  }
#endif
        
    mMouseDown = true;

    mPrevX = x;
    mPrevY = y;
    
    // Check if we clicked exactly on the handle
    // In this case, do not make the handle jump
    /*bool mouseOnHandle = */MouseOnHandle(x, y, &mOffsetX, &mOffsetY);
    
    // Direct jump
    OnMouseDrag(x, y, 0., 0., mod);
}

void
IXYPadControl::OnMouseUp(float x, float y, const IMouseMod& mod)
{
    mMouseDown = false;

    mOffsetX = 0.0;
    mOffsetY = 0.0;
    
    SetDirty(true);
}

void
IXYPadControl::OnMouseDrag(float x, float y, float dX, float dY,
                           const IMouseMod& mod)
{
    // For sensitivity
    if (mod.S) // Shift pressed => move slower
    {
#define PRECISION 0.25
    
        float newX = mPrevX + PRECISION*dX;
        float newY = mPrevY + PRECISION*dY;

        mPrevX = newX;
        mPrevY = newY;
        
        x = newX;
        y = newY;
    }

    // Manage well when hitting/releasing shift witing the same drag action
    mPrevX = x;
    mPrevY = y;
        
    // For dragging from click on the handle
    x -= mOffsetX;
    y -= mOffsetY;
    
    // Original code
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

    if (!mReverseY)
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

    if (mReverseY)
        *y = 1.0 - *y;
    
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

// Used to avoid handle jumps when clicking directly on it
// (In this case, we want to drag smoothly from the current position
// to a nearby position, without any jump)
bool
IXYPadControl::MouseOnHandle(float mx, float my,
                             float *offsetX, float *offsetY)
{
    float xn = GetValue(0);
    float yn = GetValue(1);

    if (!mReverseY)
        yn = 1.0 - yn;
    
    float x = xn;
    float y = yn;
    ParamsToPixels(&x, &y);

    int w = mHandleBitmap.W();
    int h = mHandleBitmap.H();

    IRECT handleRect(x, y, x + w, y + h);

    bool onHandle = handleRect.Contains(mx, my);

    *offsetX = 0.0;
    *offsetY = 0.0;
    if (onHandle)
    {
        *offsetX = mx - (x + w/2);
        *offsetY = my - (y + h/2);
    }
    
    return onHandle;
}
