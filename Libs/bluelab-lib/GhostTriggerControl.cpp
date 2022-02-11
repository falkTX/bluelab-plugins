#include <GhostPluginInterface.h>

#include "GhostTriggerControl.h"

void
GhostTriggerControl::OnGUIIdle()
{
    ((GhostPluginInterface *)mPlug)->CheckRecomputeData();
}
