#include <BLUtils.h>

#include "PlugBypassDetector.h"

PlugBypassDetector::PlugBypassDetector(int delayMs)
{
    mDelayMs = delayMs;
    mIsPlaying = false;

    // Update time stamps
    TouchFromAudioThread();
    TouchFromIdleThread();
}

PlugBypassDetector::~PlugBypassDetector() {}

void
PlugBypassDetector::TouchFromAudioThread()
{
    mPrevAudioTouchTimeStamp = BLUtils::GetTimeMillis();
}

void
PlugBypassDetector::SetTransportPlaying(bool flag)
{
    mIsPlaying = flag;
}

void
PlugBypassDetector::TouchFromIdleThread()
{
    mPrevIdleTouchTimeStamp = BLUtils::GetTimeMillis();
}

bool
PlugBypassDetector::PlugIsBypassed()
{
    // Do not detect bypass if transport is not playing at all
    if (!mIsPlaying)
        return false;

    // By sure to manage long idle calls
    // to avoid detecting false positives
    // NOTE: deosn't work well
    if (mPrevIdleTouchTimeStamp < mPrevAudioTouchTimeStamp)
        return false;
    
    long int now = BLUtils::GetTimeMillis();
    if (now - mPrevAudioTouchTimeStamp > mDelayMs)
        return true;

    return false;
}
