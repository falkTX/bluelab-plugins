#include <BLUtils.h>
#include <BLDebug.h>

#include "BLTransport.h"

#define DEBUG_DUMP 0 //1

BLTransport::BLTransport()
{
    // Avoid jump when restarting playback
    mIsTransportPlaying = false;
    mIsMonitorOn = false;

    mStartTransportTimeStampTotal = -1.0;
    mStartTransportTimeStampLoop = -1.0;
    mNow = -1.0;

    mDAWStartTransportValueSecLoop = 0.0;
    mDAWCurrentTransportValueSecLoop = 0.0;
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
                                 BL_FLOAT dawTransportValueSec)
{    
    bool result = false;
    
    if ((transportPlaying && !mIsTransportPlaying) ||
        (monitorOn && !mIsMonitorOn))
        // Play just started  
    {
#ifdef DEBUG_DUMP
    BLDebug::ResetFile("real.txt");
    BLDebug::ResetFile("smooth.txt");
#endif
    
        mStartTransportTimeStampTotal = BLUtils::GetTimeMillisF();
        mStartTransportTimeStampLoop = BLUtils::GetTimeMillisF();
        
        mDAWStartTransportValueSecLoop = dawTransportValueSec;
        
        result = true;
    }

    mIsTransportPlaying = transportPlaying;
    mIsMonitorOn = monitorOn;

    mDAWCurrentTransportValueSecLoop = dawTransportValueSec;
    
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
BLTransport::SetDAWTransportValueSec(BL_FLOAT dawTransportValueSec)
{
    if (dawTransportValueSec <= mDAWStartTransportValueSecLoop)
    {
        mStartTransportTimeStampLoop = BLUtils::GetTimeMillisF();
        
        mDAWStartTransportValueSecLoop = dawTransportValueSec;
    }

    mDAWCurrentTransportValueSecLoop = dawTransportValueSec;
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

    BL_FLOAT result = mDAWStartTransportValueSecLoop + elapsed;

#ifdef DEBUG_DUMP
    BLDebug::AppendValue("real.txt", mDAWStartTransportValueSecLoop);
    BLDebug::AppendValue("smooth.txt", result);
#endif
    
    return result;
}
