#include <BLUtils.h>

#include "BLTransport.h"

BLTransport::BLTransport()
{
    // Avoid jump when restarting playback
    mIsTransportPlaying = false;
    mIsMonitorOn = false;

    mStartTransportTimeStamp = -1.0;
    mNow = -1.0;
}

BLTransport::~BLTransport() {}

void
BLTransport::Reset()
{
    mStartTransportTimeStamp = BLUtils::GetTimeMillisF();
}

bool
BLTransport::SetTransportPlaying(bool transportPlaying,
                                 bool monitorOn)
{
    bool result = false;
    
    if ((transportPlaying != mIsTransportPlaying) ||
        (monitorOn != mIsMonitorOn))
        
    {
        if (transportPlaying || monitorOn)
        {
            mStartTransportTimeStamp = BLUtils::GetTimeMillisF();

            result = true;
        }
    }

    mIsTransportPlaying = transportPlaying;
    mIsMonitorOn = monitorOn;

    return result;
}

bool
BLTransport::IsTransportPlaying()
{
    bool isPlaying = (mIsTransportPlaying || mIsMonitorOn);

    return isPlaying;
}

void
BLTransport::Update()
{
    mNow = BLUtils::GetTimeMillisF();
}
    
BL_FLOAT
BLTransport::GetTransportValueSec()
{
    if ((mStartTransportTimeStamp < 0.0) ||
        (mNow < 0.0))
        return -1;

    double elapsed = (mNow - mStartTransportTimeStamp)*0.001;

    return elapsed;
}


