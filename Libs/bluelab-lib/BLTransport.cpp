#include <BLUtils.h>
#include <BLDebug.h>

#include <ParamSmoother2.h>

#include "BLTransport.h"

#define DEBUG_DUMP 0 //1

BLTransport::BLTransport(BL_FLOAT sampleRate)
{
    // Avoid jump when restarting playback
    mIsTransportPlaying = false;
    mIsMonitorOn = false;

    mStartTransportTimeStampTotal = -1.0;
    mStartTransportTimeStampLoop = -1.0;
    mNow = -1.0;

    mDAWStartTransportValueSecLoop = 0.0;
    mDAWCurrentTransportValueSecLoop = 0.0;

    mDAWTransportValueSecTotal = 0.0;

    mSoftResynchEnabled = false;
    
    // Hak delay value
#define DELAY_MS 2.0 //1.0
    mResynchOffsetSecLoop = 0.0;
    mDiffSmootherLoop = new ParamSmoother2(sampleRate, 0.0, DELAY_MS);

    mResynchOffsetSecTotal = 0.0;
    mDiffSmootherTotal = new ParamSmoother2(sampleRate, 0.0, DELAY_MS);

    mIsBypassed = false;
}

BLTransport::~BLTransport()
{
    delete mDiffSmootherLoop;
    delete mDiffSmootherTotal;
}

void
BLTransport::SetSoftResynchEnabled(bool flag)
{
    mSoftResynchEnabled = flag;
}

void
BLTransport::Reset()
{
    // Reset only if the spectrogram is moving
    // FIX: GhostViewer: play, stop, then change the spectro speed
    // => there was a jump in the spectrogram
    //if (!mIsTransportPlaying && !mIsMonitorOn)
    //    return;
    
    mStartTransportTimeStampTotal = BLUtils::GetTimeMillisF();
    mStartTransportTimeStampLoop = BLUtils::GetTimeMillisF();

    mDAWStartTransportValueSecLoop = mDAWCurrentTransportValueSecLoop;

    mDAWTransportValueSecTotal = 0.0;
        
    mResynchOffsetSecLoop = 0.0;
    mDiffSmootherLoop->ResetToTargetValue(0.0);

    mResynchOffsetSecTotal = 0.0;
    mDiffSmootherTotal->ResetToTargetValue(0.0);
}

void
BLTransport::Reset(BL_FLOAT sampleRate)
{
    mDiffSmootherLoop->Reset(sampleRate);
    mDiffSmootherTotal->Reset(sampleRate);

    Reset();
}

bool
BLTransport::SetTransportPlaying(bool transportPlaying,
                                 bool monitorOn,
                                 BL_FLOAT dawTransportValueSec)
{
    if (mIsBypassed)
        return false;
    
    bool transportJustStarted = ((transportPlaying && !mIsTransportPlaying) ||
                                 (monitorOn && !mIsMonitorOn));

    bool transportJustStopped = ((mIsTransportPlaying || mIsMonitorOn) &&
                                 (!transportPlaying && !monitorOn));
    
    bool loopDetected = (dawTransportValueSec < mDAWCurrentTransportValueSecLoop);

    if (transportJustStarted)
        // Play just started  
    {
#if DEBUG_DUMP
    BLDebug::ResetFile("real.txt");
    BLDebug::ResetFile("smooth.txt");
    BLDebug::ResetFile("total.txt");
#endif
    
        mStartTransportTimeStampTotal = BLUtils::GetTimeMillisF();
        mStartTransportTimeStampLoop = BLUtils::GetTimeMillisF();
        
        mDAWStartTransportValueSecLoop = dawTransportValueSec;

        mDAWTransportValueSecTotal = 0.0;
        
        mResynchOffsetSecLoop = 0.0;
        mDiffSmootherLoop->ResetToTargetValue(0.0);

        mResynchOffsetSecTotal = 0.0;
        mDiffSmootherTotal->ResetToTargetValue(0.0);

        transportJustStarted = true;
    }

    if (loopDetected)
    {
        mStartTransportTimeStampLoop = BLUtils::GetTimeMillisF();
        mDAWStartTransportValueSecLoop = dawTransportValueSec;

        mResynchOffsetSecLoop = 0.0;
        mDiffSmootherLoop->ResetToTargetValue(0.0);
    }
    
    mIsTransportPlaying = transportPlaying;
    mIsMonitorOn = monitorOn;

    if (!transportJustStarted && !loopDetected)
    {
        mDAWTransportValueSecTotal +=
            dawTransportValueSec - mDAWCurrentTransportValueSecLoop;
    }
        
    mDAWCurrentTransportValueSecLoop = dawTransportValueSec;
    
    return transportJustStarted;
}

void
BLTransport::SetBypassed(bool flag)
{
    mIsBypassed = flag;

    if (mIsBypassed)
    {
        mIsTransportPlaying = false;
        mIsMonitorOn = false;
    }
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
    if (mIsBypassed)
        return;
    
    if (mIsTransportPlaying || mIsMonitorOn)
        // Enabled the possibility to fine adjust the transport
        // after play stopping (soft resynch).
        // FIX: GhostViewer: play, stop => there was a small jump of
        // ths spectrogram just after having stopped
    {
        mNow = BLUtils::GetTimeMillisF();

        if (mSoftResynchEnabled)
            SoftResynch();
    }
}

// Change only the transport value when start playing or reseting a loop
// Otherwise process smoothly
void
BLTransport::SetDAWTransportValueSec(BL_FLOAT dawTransportValueSec)
{
    if (mIsBypassed)
        return;
    
    bool loopDetected = false;
    if (dawTransportValueSec < mDAWCurrentTransportValueSecLoop)
        // Just restarted a loop
    {
        mStartTransportTimeStampLoop = BLUtils::GetTimeMillisF();
        
        mDAWStartTransportValueSecLoop = dawTransportValueSec;

        mResynchOffsetSecLoop = 0.0;
        mDiffSmootherLoop->ResetToTargetValue(0.0);

        loopDetected = true;
    }

    if (!loopDetected)
    {
        mDAWTransportValueSecTotal +=
            dawTransportValueSec - mDAWCurrentTransportValueSecLoop;
    }
    
    mDAWCurrentTransportValueSecLoop = dawTransportValueSec;
}

BL_FLOAT
BLTransport::GetTransportElapsedSecTotal()
{
    if ((mStartTransportTimeStampTotal < 0.0) ||
        (mNow < 0.0))
        return -1;

    double result =
        (mNow - mStartTransportTimeStampTotal)*0.001 + mResynchOffsetSecTotal;

#if DEBUG_DUMP // 0
    BLDebug::AppendValue("real.txt", mDAWCurrentTransportValueSecLoop);
    BLDebug::AppendValue("total.txt", result);

    BL_FLOAT loop = GetTransportValueSecLoop();
    BLDebug::AppendValue("smooth.txt", loop);
#endif
    
    return result;
}

BL_FLOAT
BLTransport::GetTransportElapsedSecLoop()
{
    if ((mStartTransportTimeStampLoop < 0.0) ||
        (mNow < 0.0))
        return -1;

    double elapsed =
        (mNow - mStartTransportTimeStampLoop)*0.001 + mResynchOffsetSecLoop;

    return elapsed;
}

BL_FLOAT
BLTransport::GetTransportValueSecLoop()
{
    BL_FLOAT elapsed = GetTransportElapsedSecLoop();

    if (elapsed < 0.0)
        elapsed = 0.0;

    BL_FLOAT result = mDAWStartTransportValueSecLoop + elapsed;

#if 0 //DEBUG_DUMP
    BLDebug::AppendValue("real.txt", mDAWCurrentTransportValueSecLoop);
    BLDebug::AppendValue("smooth.txt", result);

    BL_FLOAT total = GetTransportElapsedSecTotal();
    BLDebug::AppendValue("total.txt", total);
#endif
    
    return result;
}

void
BLTransport::HardResynch()
{
    // Loop
    mResynchOffsetSecLoop = 0.0;
    BL_FLOAT estimTransportLoop =
        mDAWStartTransportValueSecLoop + GetTransportElapsedSecLoop();
    BL_FLOAT diffLoop = mDAWCurrentTransportValueSecLoop - estimTransportLoop;
    mResynchOffsetSecLoop = diffLoop;

    // Total
    mResynchOffsetSecTotal = 0.0;
    BL_FLOAT diffTotal = mDAWTransportValueSecTotal - GetTransportElapsedSecTotal();
    mResynchOffsetSecTotal = diffTotal;
}

void
BLTransport::SoftResynch()
{
    if (mIsTransportPlaying)
        // Update only if the real transport value changes
    {
        // Loop
        mResynchOffsetSecLoop = 0.0;
        BL_FLOAT estimTransportLoop =
            mDAWStartTransportValueSecLoop + GetTransportElapsedSecLoop();
        BL_FLOAT diffLoop = mDAWCurrentTransportValueSecLoop - estimTransportLoop;
        mDiffSmootherLoop->SetTargetValue(diffLoop);
        BL_FLOAT diffSmoothLoop = mDiffSmootherLoop->Process();
        mResynchOffsetSecLoop = diffSmoothLoop;

        // Total
        mResynchOffsetSecTotal = 0.0;
        BL_FLOAT diffTotal =
            mDAWTransportValueSecTotal - GetTransportElapsedSecTotal();
        mDiffSmootherTotal->SetTargetValue(diffTotal);
        BL_FLOAT diffSmoothTotal = mDiffSmootherTotal->Process();
        mResynchOffsetSecTotal = diffSmoothTotal;
    }
}
