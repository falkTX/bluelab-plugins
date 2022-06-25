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
 
#ifndef VUMETER_CONTROL_H
#define VUMETER_CONTROL_H

#include <IGraphics.h>
#include <IControl.h>
#include <IGraphicsStructs.h>
#include <IPlugConstants.h>

using namespace iplug;
using namespace iplug::igraphics;

// Bitmap vumeter
// Mouse interaction disabled
class VumeterControl : public IBKnobControl
{
 public:
    VumeterControl(float x, float y, const IBitmap& bitmap, int paramIdx,
                   EDirection direction = EDirection::Vertical,
                   double gearing = DEFAULT_GEARING)
    : IBKnobControl(x, y, bitmap, paramIdx, direction, gearing) {}
    
    VumeterControl(const IRECT& bounds, const IBitmap& bitmap,
                   int paramIdx, EDirection direction = EDirection::Vertical,
                   double gearing = DEFAULT_GEARING)
    : IBKnobControl(bounds, bitmap, paramIdx, direction, gearing) {}
  
    virtual ~VumeterControl() {}

    // Disable all interactions
    // But do not call SetDisabled()
    // So we can have tooltips on such a Vumeter
    
    virtual void OnMouseDown(float x, float y, const IMouseMod& mod) override {}
    virtual void OnMouseUp(float x, float y, const IMouseMod& mod) override {}
    virtual void OnMouseDrag(float x, float y, float dX, float dY,
                             const IMouseMod& mod) override {}
    virtual void OnMouseDblClick(float x, float y, const IMouseMod& mod) override {}
    virtual void OnMouseWheel(float x, float y, const IMouseMod& mod,
                              float d) override {};
    virtual bool OnKeyDown(float x, float y,
                           const IKeyPress& key) override { return false; }
    virtual bool OnKeyUp(float x, float y,
                         const IKeyPress& key) override { return false; }
    virtual void OnMouseOver(float x, float y, const IMouseMod& mod) override {}
    virtual void OnMouseOut() override {}
};

#endif
