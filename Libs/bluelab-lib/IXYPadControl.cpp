#include "IXYPadControl.h"

IXYPadControl::IXYPadControl(const IRECT& bounds,
                             const std::initializer_list<int>& params,
                             const IBitmap& trackBitmap,
                             const IBitmap& handleBitmap)
: IControl(bounds, params)
{
    mTrackBitmap = trackBitmap;
    mHandleBitmap = handleBitmap;
        
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
    float xn = (x - mRECT.L) / mRECT.W();
    float yn = 1.f - ((y - mRECT.T) / mRECT.H());
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
    
    float x = mRECT.L + xn*mRECT.W();
    float y = mRECT.T + yn*mRECT.H();

    int w = mHandleBitmap.W();
    int h = mHandleBitmap.H();
    
    IRECT handleRect(x - w/2, y - h/2, x + w/2, y + h/2);
    
    IBlend blend = GetBlend();
    //g.DrawBitmap(mHandleBitmap,
    //             GetRECT().GetCentredInside(IRECT(0, 0, mHandleBitmap)),
    //             0, &blend);
    g.DrawBitmap(mHandleBitmap, handleRect, 0, &blend);
}
