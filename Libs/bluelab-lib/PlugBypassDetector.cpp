#include <BLUtils.h>

#include "PlugBypassDetector.h"

PlugBypassDetector::PlugBypassDetector(int delayMs)
{
    mDelayMs = delayMs;
}

PlugBypassDetector::~PlugBypassDetector() {}

void
PlugBypassDetector::Touch()
{
    mPrevTouchTime = BLUtils::GetTimeMillis();
}

bool
PlugBypassDetector::PlugIsBypassed()
{
    long int millis = BLUtils::GetTimeMillis();
    if (millis - mPrevTouchTime > mDelayMs)
        return true;

    return false;
}
