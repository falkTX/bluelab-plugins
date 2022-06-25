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
 
#ifndef IBL_SWITCH_CONTROL_H
#define IBL_SWITCH_CONTROL_H

#include <IControl.h>
#include <IControls.h>

#include "IPlug_include_in_plug_hdr.h"

using namespace iplug;
using namespace iplug::igraphics;

class IBLSwitchControl : public IBSwitchControl
{
 public:
    IBLSwitchControl(float x, float y, const IBitmap& bitmap,
                     int paramIdx = kNoParameter);

    IBLSwitchControl(const IRECT& bounds, const IBitmap& bitmap,
                     int paramIdx = kNoParameter);

    // Can we toggle off the button when clicking on it?
    void SetClickToggleOff(bool flag);
        
    virtual ~IBLSwitchControl() {}
    void Draw(IGraphics& g) override { IBSwitchControl::Draw(g); }
    void OnRescale() override { IBSwitchControl::OnRescale(); }
    void OnMouseDown(float x, float y, const IMouseMod& mod) override;

 protected:
    bool mClickToggleOff;
};

#endif /* SWITCH_CONTROL_H */
