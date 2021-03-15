//
//  IGUIResizeButtonControl.cpp
//  BL-GhostViewer-macOS
//
//  Created by applematuer on 10/29/20.
//
//

#include <BLUtils.h>
#include <BLUtilsMath.h>

#include "IGUIResizeButtonControl.h"

IGUIResizeButtonControl::IGUIResizeButtonControl(ResizeGUIPluginInterface *plug,
                                                 float x, float y,
                                                 const IBitmap &bitmap,
                                                 int paramIdx,
                                                 int guiSizeIdx,
                                                 EBlend blend)
: IRolloverButtonControl(x, y, bitmap, paramIdx, false, blend)
{
    mPlug = plug;
    
    mGuiSizeIdx = guiSizeIdx;
}

IGUIResizeButtonControl::IGUIResizeButtonControl(ResizeGUIPluginInterface *plug,
                                                 float x, float y,
                                                 const IBitmap &bitmap,
                                                 int guiSizeIdx,
                                                 EBlend blend)
: IRolloverButtonControl(x, y, bitmap, kNoParameter, false, blend)
{
    mPlug = plug;
    
    mGuiSizeIdx = guiSizeIdx;
}

IGUIResizeButtonControl::~IGUIResizeButtonControl() {}

void
IGUIResizeButtonControl::OnMouseDown(float x, float y, const IMouseMod &mod)
{
    // Disable Alt + click on gui resize buttons
    // FIX: that made the graph disappear
    if (mod.A)
        ((IMouseMod &)mod).A = false;
    
    double prevValue = GetValue();
    
    IRolloverButtonControl::OnMouseDown(x, y, mod);
    
    // Check to not resize to identity if we re-clicked on the button already active
    if (std::fabs(GetValue() - prevValue) > BL_EPS)
    {
        mPlug->ApplyGUIResize(mGuiSizeIdx);
    }
}
