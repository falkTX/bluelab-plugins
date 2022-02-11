#include <UpTime.h>

#include "GhostPluginInterface.h"

GhostPluginInterface::GhostPluginInterface()
{
    mPrevUpTime = UpTime::GetUpTime();
}

GhostPluginInterface::~GhostPluginInterface() {}

// For Protools
bool
GhostPluginInterface::PlaybackWasRestarted(unsigned long long delay)
{
    unsigned long long currentUpTime = UpTime::GetUpTime();
    
    bool result = false;
    if (currentUpTime - mPrevUpTime > delay)
        result = true;
    
    mPrevUpTime = currentUpTime;
    
    return result;
}


