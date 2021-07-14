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
