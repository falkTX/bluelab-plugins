#ifndef GHOST_CUSTOM_KEY_CONTROL_H
#define GHOST_CUSTOM_KEY_CONTROL_H

#include <GhostPluginInterface.h>

#include "IPlug_include_in_plug_hdr.h"
#include "IControl.h"

using namespace iplug;
using namespace iplug::igraphics;

//class GhostCustomKeyControl : public KeyCustomControl
class GhostCustomKeyControl : public IControl
{
public:
    GhostCustomKeyControl(GhostPluginInterface *plug, const IRECT &bounds);
    
    virtual ~GhostCustomKeyControl() {}
    
    virtual bool OnKeyDown(float x, float y, const IKeyPress &key) override;
    
    virtual void Draw(IGraphics& g) override {};
    
protected:
    GhostPluginInterface *mPlug;
};

#endif
