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
