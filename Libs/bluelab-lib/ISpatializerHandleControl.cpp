#include <BLUtilsMath.h>

#include "ISpatializerHandleControl.h"

ISpatializerHandleControl::
ISpatializerHandleControl(const IRECT& bounds,
                          float minAngle, float maxAngle,
                          bool reverseY,
                          const IBitmap& handleBitmap,
                          int paramIdx)
: IControl(bounds, paramIdx)
{
    mHandleBitmap = handleBitmap;

    mMinAngle = minAngle;
    mMaxAngle = maxAngle;

    mPrevX = 0.0;
    mPrevY = 0.0;

    mReverseY = reverseY;
}

ISpatializerHandleControl::~ISpatializerHandleControl() {}

void
ISpatializerHandleControl::Draw(IGraphics& g)
{
    float val = GetValue();

    float x;
    float y;
    ParamToPixels(val, &x, &y);

    int w = mHandleBitmap.W();
    int h = mHandleBitmap.H();

    IRECT handleRect(x, y, x + w, y + h);
    
    IBlend blend = GetBlend();
    g.DrawBitmap(mHandleBitmap, handleRect, 0, &blend);
}

void
ISpatializerHandleControl::OnMouseDown(float x, float y, const IMouseMod& mod)
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

  mPrevX = x;
  mPrevY = y;
}


void
ISpatializerHandleControl::OnMouseDrag(float x, float y, float dX, float dY,
                                       const IMouseMod& mod)
{
    // For sensitivity
    if (mod.S) // Shift pressed => move slower
    {
        //#define PRECISION 0.25
#define PRECISION 0.125
    
        float newX = mPrevX + PRECISION*dX;
        float newY = mPrevY + PRECISION*dY;
        
        x = newX;
        y = newY;
    }

    //float deltaX = x - mPrevX;
    float deltaY = y - mPrevY;

    // Manage well when hitting/releasing shift witing the same drag action
    mPrevX = x;
    mPrevY = y;

    PixelsToParam(&deltaY);
        
    SetValue(deltaY);
    
    SetDirty(true);
}
  
void
ISpatializerHandleControl::PixelsToParam(float *y)
{
#define SPEED_COEFF 1.0 //2.0 0.5
    
    float normValue = GetValue();
    float value = mMinAngle + normValue*(mMaxAngle - mMinAngle);

    if (mReverseY)
        *y = -*y;
        
    value += *y*SPEED_COEFF;

    float normValue2 = (value - mMinAngle)/(mMaxAngle - mMinAngle);

    *y = normValue2;

    // Bounds
    if (*y < 0.0)
        *y = 0.0;
    if (*y > 1.0)
        *y = 1.0;
}

void
ISpatializerHandleControl::ParamToPixels(float val, float *x, float *y)
{
    float angleDeg = val*(mMaxAngle - mMinAngle) + mMinAngle;
    float angleRad = angleDeg*M_PI/180.0;

    float handleW = mHandleBitmap.W();
    float handleH = mHandleBitmap.H();

    float centerX = mRECT.L + mRECT.W()*0.5;
    float centerY = mRECT.T + mRECT.H()*0.5;

    float w = mRECT.W();
    //float h = mRECT.H();

    float rad = w*0.5;

    rad -= handleW;
    
    // We need the upper right corner
    *x = centerX + rad*cos(angleRad) - handleW*0.5;
    *y = centerY + rad*sin(angleRad) - handleH*0.5;
}
