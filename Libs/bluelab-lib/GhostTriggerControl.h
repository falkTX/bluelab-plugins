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
 
#ifndef GHOST_TRIGGER_CONTROL_H
#define GHOST_TRIGGER_CONTROL_H

#include "IPlug_include_in_plug_hdr.h"
#include "IControl.h"

using namespace iplug;
using namespace iplug::igraphics;

class GhostPluginInterface;
//class GhostTriggerControl : public ITriggerControl
class GhostTriggerControl : public IControl
{
public:
    GhostTriggerControl(GhostPluginInterface *pPlug,
                        int paramIdx, const IRECT &bounds)
    : IControl(bounds) { mPlug = pPlug; }
    
    //: ITriggerControl(pPlug, paramIdx) {}
    
    virtual ~GhostTriggerControl() {}
    
    virtual void OnGUIIdle() override;
    
    virtual void Draw(IGraphics& g) override {};
    
protected:
    GhostPluginInterface *mPlug;
};

#endif
