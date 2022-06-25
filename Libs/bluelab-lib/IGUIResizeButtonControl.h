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
//  IGUIResizeButtonControl.hpp
//  BL-GhostViewer-macOS
//
//  Created by applematuer on 10/29/20.
//
//

#ifndef IGUIResizeButtonControl_h
#define IGUIResizeButtonControl_h

#include <IRolloverButtonControl.h>
#include <IPlugConstants.h>
#include <ResizeGUIPluginInterface.h>

using namespace iplug;
using namespace iplug::igraphics;

// Must create a specific class
// (otherwise, if used parameter change, that should not be
// the right thread, and it crashes)
//
// Version 2 => with rollover
//
// From IGUIResizeButtonControl (iPlug1)
class IGUIResizeButtonControl : public IRolloverButtonControl
{
public:
    IGUIResizeButtonControl(ResizeGUIPluginInterface *plug,
                            float x, float y,
                            const IBitmap &bitmap,
                            int paramIdx,
                            int guiSizeIdx,
                            EBlend blend = EBlend::Default);
    
    IGUIResizeButtonControl(ResizeGUIPluginInterface *plug,
                            float x, float y,
                            const IBitmap &bitmap,
                            int guiSizeIdx,
                            EBlend blend = EBlend::Default);
    
    virtual ~IGUIResizeButtonControl();
    
    virtual void OnMouseDown(float x, float y, const IMouseMod &mMod) override;

    virtual void OnMouseUp(float x, float y, const IMouseMod& mod) override;
        
    // Disable double click
    // It was not consistent to use double click on a series of gui resize buttons
    // And made a bug: no button hilighted, and if we reclicked, no graph anymore
    void OnMouseDblClick(float x, float y, const IMouseMod &mod) override {}
    
protected:
    ResizeGUIPluginInterface *mPlug;
    
    int mGuiSizeIdx;
};

#endif /* IGUIResizeButtonControl_h */
