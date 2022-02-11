#include <GhostPluginInterface.h>

#include "GhostFileDropControl.h"

GhostFileDropControl::GhostFileDropControl(GhostPluginInterface* pPlug,
                                           const IRECT& bounds)
: IControl(bounds)
{
    mPlug = pPlug;
    
    // Avoid locking all mouse interactions over the graph
    //SetInteractionDisabled(true);
    //mIgnoreMouse = true;
    
    mDisabled = true;
    mMouseOverWhenDisabled = true;
}

void
//GhostFilesDropControl::OnFilesDropped(const char *fileNames)
GhostFileDropControl::OnDrop(const char *fileNames)
{
    // Assume there is only one file
    //((GhostPluginInterface *)mPlug)->OpenFile(fileNames);
    mPlug->OpenFile(fileNames);
}
