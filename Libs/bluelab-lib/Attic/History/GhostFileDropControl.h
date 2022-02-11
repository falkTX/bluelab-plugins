#ifndef GHOST_FILE_DROP_CONTROL_H
#define GHOST_FILE_DROP_CONTROL_H

#include "IPlug_include_in_plug_hdr.h"

#include <GhostPluginInterface.h>

#include "IControl.h"

using namespace iplug;
using namespace iplug::igraphics;

//class GhostFilesDropControl : public IFilesDropControl
class GhostFileDropControl : public IControl
{
public:
    //GhostFilesDropControl(IPlugBase* pPlug)
    //: IFilesDropControl(pPlug) {}
    GhostFileDropControl(GhostPluginInterface* pPlug, const IRECT& bounds);
    
    virtual ~GhostFileDropControl() {}
    
    void Draw(IGraphics& g) override {}
    
    //void OnFilesDropped(const char *fileNames);
    void OnDrop(const char *fileNames) override;
    
protected:
    GhostPluginInterface *mPlug;
};

#endif
