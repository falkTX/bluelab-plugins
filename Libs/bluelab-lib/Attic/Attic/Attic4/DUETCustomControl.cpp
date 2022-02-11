//
//  DUETCustomControl.cpp
//  BL-Panogram
//
//  Created by applematuer on 10/21/19.
//
//

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
DUETCustomControl::OnMouseDown(int x, int y, IMouseMod* pMod)
{
    if (pMod->Cmd)
        // Command pressed (or Control on Windows)
    {
        // disable pick cursor
        mPlug->SetPickCursorActive(false);
        
        return;
    }
        
    mPlug->SetPickCursor(x, y);
}

void
DUETCustomControl::OnMouseDrag(int x, int y, int dX, int dY, IMouseMod* pMod)
{
    mPlug->SetPickCursor(x, y);
}

bool
DUETCustomControl::OnKeyDown(int x, int y, int key, IMouseMod* pMod)
{
    if (pMod->A)
        mPlug->SetInvertPickSelection(true);
    else
        mPlug->SetInvertPickSelection(false);
    
    return true;
}
