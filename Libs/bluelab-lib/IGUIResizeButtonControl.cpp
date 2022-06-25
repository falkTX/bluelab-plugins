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
: IRolloverButtonControl(x, y, bitmap, paramIdx, false,
                         false, false, blend)
{
    mPlug = plug;
    
    mGuiSizeIdx = guiSizeIdx;
}

IGUIResizeButtonControl::IGUIResizeButtonControl(ResizeGUIPluginInterface *plug,
                                                 float x, float y,
                                                 const IBitmap &bitmap,
                                                 int guiSizeIdx,
                                                 EBlend blend)
: IRolloverButtonControl(x, y, bitmap, kNoParameter, false,
                         false, false, blend)
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

    // Do nothing if the button is already toggled on
    // (otherwise, it would deactive the button)
    if (prevValue > 0.5)
        return;
    
    IRolloverButtonControl::OnMouseDown(x, y, mod);

#if 0 // No need; since the param change will do it automatically
      // NOTE: will avoid to call ApplyGUIResize() 2 times
    
    // Check to not resize to identity if we re-clicked on the button already active
    if (std::fabs(GetValue() - prevValue) > BL_EPS)
    {
        mPlug->ApplyGUIResize(mGuiSizeIdx);
    }
#endif
}

void
IGUIResizeButtonControl::OnMouseUp(float x, float y, const IMouseMod& mod)
{
    double prevValue = GetValue();

    // Do nothing if the button is already toggled on
    // (otherwise, it would deactive the button)
    if (prevValue > 0.5)
        return;

    IRolloverButtonControl::OnMouseUp(x, y, mod);
}
