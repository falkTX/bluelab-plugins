/* Copyright (C) 2022 Nicolas Dittlo <deadlab.plugins@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this software; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
 
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

    bool altChecked = false;
#ifndef __linux__
    // Alt is more logical on Windows for resetting
    // But on linux, Alt is used to grab and move the window
    altChecked = pMod.A;
#endif
    
    if (pMod.C || pMod.Cmd || altChecked)
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
