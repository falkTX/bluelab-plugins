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
//  DemoModeManager.h
//  Spatializer
//
//  Created by Apple m'a Tuer on 21/04/17.
//
//

#ifndef Spatializer_DemoModeManager_h
#define Spatializer_DemoModeManager_h

#include "IPlug_include_in_plug_hdr.h"
#include "IControl.h"

using namespace iplug;
using namespace iplug::igraphics;

// If defined,  the plugins will be compiled in demo mode
//#define DEMO_MODE

// Demo Label bitmap
#define DEMO_LABEL_ID 200
#define DEMO_LABEL_FN "resources/img/demo.png"

// Do not use singleton anymore because a singleton is shared between all the plugins
// of the same class.
// Then the bitmap provided to the demo manager can be destroyed by another
// plugin that exits.
class DemoModeManager
{
public:
    DemoModeManager();
    
    virtual ~DemoModeManager();
    
    void Init(Plugin *pPlug, IGraphics *pGraphics, bool isV2 = false);
    
    bool IsDemoMode();
    
    bool MustProcess();
    
    void Process(double **outputs, int nFrames);
    
protected:
    IBitmapControl *mBitmapControl;
};

#endif
