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
//  WavesPluginInterface.h
//  UST-macOS
//
//  Created by applematuer on 9/26/20.
//
//

#ifndef ResizeGUIPluginInterface_h
#define ResizeGUIPluginInterface_h

#include "IPlug_include_in_plug_hdr.h"

using namespace iplug;
using namespace iplug::igraphics;

class IGUIResizeButtonControl;
class GraphControl11;

class ResizeGUIPluginInterface
{
public:
    ResizeGUIPluginInterface(Plugin *plug);
    virtual ~ResizeGUIPluginInterface();

    virtual void PreResizeGUI(int guiSizeIdx,
                              int *outNewGUIWidth,
                              int *outNewGUIHeight) = 0;
    
    void ApplyGUIResize(int guiSizeIdx);

protected:
    void GUIResizeParamChange(int paramNum,
                              int params[], IGUIResizeButtonControl *buttons[],
                              int numParams);
    
    void GUIResizeComputeOffsets(int defaultGUIWidth, int defaultGUIHeight,
                                 int newGUIWidth, int newGUIHeight,
                                 int *offsetX, int *offsetY);
    
    //
    Plugin *mPlug;
    
    // Avoid launching a new gui resize while a first one is in progress
    bool mIsResizingGUI;
};

#endif /* ResizeGUIPluginInterface_h */
