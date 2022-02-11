#ifndef MOUSE_CUSTOM_CONTROL_H
#define MOUSE_CUSTOM_CONTROL_H

#include "IPlug_include_in_plug_hdr.h"

class MouseCustomControl
{
public:
    MouseCustomControl() {}
    
    virtual ~MouseCustomControl() {}
    
    virtual void OnMouseUp(float x, float y, const IMouseMod &pMod) {}
};

#endif
