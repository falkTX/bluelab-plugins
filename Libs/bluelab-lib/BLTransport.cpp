#include <BLUtils.h>

#include "BLTransport.h"

BLTransport::BLTransport()
{
    // Avoid jump when restarting playback
    mIsTransportPlaying = false;
    mIsMonitorOn = false;

    mStartTransportTimeStampTotal = -1.0;
    mStartTransportTimeStampLoop = -1.0;
    mNow = -1.0;

    mDAWTransportValueSec = 0.0;
}

BLTransport::~BLTransport() {}

void
BLTransport::Reset()
{
    mStartTransportTimeStampTotal = BLUtils::GetTimeMillisF();
    mStartTransportTimeStampLoop = BLUtils::GetTimeMillisF();
}

bool
BLTransport::SetTransportPlaying(bool transportPlaying,
                                 bool monitorOn,
                                 BL_FLOAT transportTime)
{
    bool result = false;
    
    if ((transportPlaying && !mIsTransportPlaying) ||
        (monitorOn && !mIsMonitorOn))
        // Play just started  
    {
        mStartTransportTimeStampTotal = BLUtils::GetTimeMillisF();
        mStartTransportTimeStampLoop = BLUtils::GetTimeMillisF();
        
        mDAWTransportValueSec = transportTime;
        
        result = true;
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

// Change only the transport value when start playing or reseting a loop
// Otherwise process smoothly
void
BLTransport::SetDAWTransportValueSec(BL_FLOAT transportTime)
{
    if (transportTime <= mDAWTransportValueSec)
    {
        mStartTransportTimeStampLoop = BLUtils::GetTimeMillisF();
        
        mDAWTransportValueSec = transportTime;
    }
}

BL_FLOAT
BLTransport::GetTransportElapsedSecTotal()
{
    if ((mStartTransportTimeStampTotal < 0.0) ||
        (mNow < 0.0))
        return -1;

    double elapsed = (mNow - mStartTransportTimeStampTotal)*0.001;

    return elapsed;
}

BL_FLOAT
BLTransport::GetTransportElapsedSecLoop()
{
    if ((mStartTransportTimeStampLoop < 0.0) ||
        (mNow < 0.0))
        return -1;

    double elapsed = (mNow - mStartTransportTimeStampLoop)*0.001;

    return elapsed;
}

BL_FLOAT
BLTransport::GetTransportValueSecLoop()
{
    BL_FLOAT elapsed = GetTransportElapsedSecLoop();

    if (elapsed < 0.0)
        elapsed = 0.0;

    BL_FLOAT result = mDAWTransportValueSec + elapsed;

    return result;
}
