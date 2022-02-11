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
