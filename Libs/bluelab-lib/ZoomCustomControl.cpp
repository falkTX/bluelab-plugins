#include "ZoomCustomControl.h"

#define WHEEL_ZOOM_STEP 0.025
#define DRAG_WHEEL_COEFF 0.2

ZoomCustomControl::ZoomCustomControl(ZoomListener *listener)
{
    mListener = listener;
    
    mStartDrag[0] = 0;
    mStartDrag[1] = 0;
    
    mPrevMouseY = 0.0;
}
    
ZoomCustomControl::~ZoomCustomControl() {}
    
void
ZoomCustomControl::OnMouseDown(float x, float y, const IMouseMod &pMod)
{
    mPrevMouseY = y;
   
    if (pMod.C || pMod.Cmd)
        // Command pressed (or Control on Windows)
    {
        mListener->ResetZoom();
        
        return;
    }
   
    mStartDrag[0] = x;
    mStartDrag[1] = y;
}        

void
ZoomCustomControl::OnMouseDrag(float x, float y, float dX, float dY,
                               const IMouseMod &pMod)
{    
#if 0 // On Linux, drag + alt moves the window
    if (pMod.A)
#endif
        // Alt-drag => zoom
    {
        BL_FLOAT dY0 = y - mPrevMouseY;
        mPrevMouseY = y;
        
        dY0 *= -1.0;
        dY0 *= DRAG_WHEEL_COEFF;
        
        OnMouseWheel(mStartDrag[0], mStartDrag[1], pMod, dY0);
    }
}
    
void
ZoomCustomControl::OnMouseWheel(float x, float y, const IMouseMod &pMod, float d)
{
    BL_FLOAT zoomChange = 1.0 + d*WHEEL_ZOOM_STEP;
    mListener->UpdateZoom(zoomChange);
}
