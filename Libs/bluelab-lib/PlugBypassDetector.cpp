#include <BLUtils.h>

#include "PlugBypassDetector.h"

PlugBypassDetector::PlugBypassDetector(int delayMs)
{
    mDelayMs = delayMs;
    mIsPlaying = false;
}

PlugBypassDetector::~PlugBypassDetector() {}

void
PlugBypassDetector::Touch()
{
    mPrevTouchTime = BLUtils::GetTimeMillis();
}

void
PlugBypassDetector::SetTransportPlaying(bool flag)
{
    mIsPlaying = flag;
}

bool
PlugBypassDetector::PlugIsBypassed()
{
    // Do not detect bypass if transport is not playing at all
    if (!mIsPlaying)
        return false;
    
    long int millis = BLUtils::GetTimeMillis();
    if (millis - mPrevTouchTime > mDelayMs)
        return true;

    return false;
}
