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
