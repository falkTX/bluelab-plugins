//
//  DUETCustomControl.cpp
//  BL-Panogram
//
//  Created by applematuer on 10/21/19.
//
//

#ifdef IGRAPHICS_NANOVG

#include "DUETCustomControl.h"

DUETCustomControl::DUETCustomControl(DUETPlugInterface *plug)
{
    mPlug = plug;
    
    Reset();
}

void
DUETCustomControl::Reset()
{
    mPlug->SetPickCursorActive(false);
}

void
DUETCustomControl::OnMouseDown(float x, float y, const IMouseMod &mod)
{
    if (mod.Cmd)
        // Command pressed (or Control on Windows)
    {
        // disable pick cursor
        mPlug->SetPickCursorActive(false);
        
        return;
    }
        
    mPlug->SetPickCursor(x, y);
}

void
DUETCustomControl::OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod &mod)
{
    mPlug->SetPickCursor(x, y);
}

bool
DUETCustomControl::OnKeyDown(float x, float y, const IKeyPress& key)
{
    if (key.A)
        mPlug->SetInvertPickSelection(true);
    else
        mPlug->SetInvertPickSelection(false);
    
    return true;
}

#endif // IGRAPHICS_NANOVG
