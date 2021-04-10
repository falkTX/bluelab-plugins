#include <BLUtils.h>
#include <BLDebug.h>

#include <ParamSmoother2.h>

#include "BLTransport.h"

BLTransport::BLTransport(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    
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

#if USE_AUTO_HARD_RESYNCH
    mHardResynchThreshold = BL_INF;
#endif
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

#if USE_AUTO_HARD_RESYNCH
void
BLTransport::SetAutoHardResynchThreshold(BL_FLOAT threshold)
{
    mAutoHardResynchThreshold = threshold;
}
#endif

void
BLTransport::Reset()
{
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
    mSampleRate = sampleRate;
    
    mDiffSmootherLoop->Reset(sampleRate);
    mDiffSmootherTotal->Reset(sampleRate);

    Reset();
}

bool
BLTransport::SetTransportPlaying(bool transportPlaying,
                                 bool monitorOn,
                                 BL_FLOAT dawTransportValueSec,
                                 int blockSize)
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
        SetupTransportJustStarted(dawTransportValueSec);
        
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

#if 0 // ORIGIN: Trust the transport
    if (!transportJustStarted && !loopDetected)
    {
        mDAWTransportValueSecTotal +=
            dawTransportValueSec - mDAWCurrentTransportValueSecLoop;
    }
#endif

#if 1 // NEW: Trust the number of samples added (should be more accurate)
    if (!transportJustStarted)
    {
        double samplesElapsedSec = ((double)blockSize)/mSampleRate;
        mDAWTransportValueSecTotal += samplesElapsedSec;
    }
#endif
    
    mDAWCurrentTransportValueSecLoop = dawTransportValueSec;

#if USE_AUTO_HARD_RESYNCH
    bool hardResynch = HardResynch();
    if (hardResynch)
        transportJustStarted = true;
#endif
    
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

    UpdateNow();
    
    if (mIsTransportPlaying || mIsMonitorOn)
        // Enabled the possibility to fine adjust the transport
        // after play stopping (soft resynch).
        // FIX: GhostViewer: play, stop => there was a small jump of
        // the spectrogram just after having stopped
    {
        if (mSoftResynchEnabled)
            SoftResynch();
    }
}

// Change only the transport value when start playing or reseting a loop
// Otherwise process smoothly
//
// NOTE: for the moment it is only called for resetting the transport to 0
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
    
    return result;
}

bool
BLTransport::HardResynch()
{
#if USE_AUTO_HARD_RESYNCH
    // If we auto hard resynch, to as if we just start playing 
    if (std::fabs(mResynchOffsetSecTotal) < mHardResynchThreshold)
        return false;
#endif
    
    SetupTransportJustStarted(mDAWCurrentTransportValueSecLoop);
    
    return true;
}

void
BLTransport::SoftResynch()
{
    if (mIsTransportPlaying)
        // Update only if the real transport value changes
    {
        // Loop
        mResynchOffsetSecLoop = 0.0;
        double estimTransportLoop = mDAWStartTransportValueSecLoop +
            (mNow - mStartTransportTimeStampLoop)*0.001;
        double diffLoop = mDAWCurrentTransportValueSecLoop - estimTransportLoop;
        mDiffSmootherLoop->SetTargetValue(diffLoop);
        double diffSmoothLoop = mDiffSmootherLoop->Process();
        mResynchOffsetSecLoop = diffSmoothLoop;

        // Total
        mResynchOffsetSecTotal = 0.0;
        double diffTotal = mDAWTransportValueSecTotal -
            (mNow - mStartTransportTimeStampTotal)*0.001;
        mDiffSmootherTotal->SetTargetValue(diffTotal);
        double diffSmoothTotal = mDiffSmootherTotal->Process();
        mResynchOffsetSecTotal = diffSmoothTotal;
    }
}

void
BLTransport::UpdateNow()
{
    if (!mIsBypassed)
    {
        if (mIsTransportPlaying || mIsMonitorOn)
            mNow = BLUtils::GetTimeMillisF();
    }
}

void
BLTransport::SetupTransportJustStarted(BL_FLOAT dawTransportValueSec)
{
    mStartTransportTimeStampTotal = BLUtils::GetTimeMillisF();
    mStartTransportTimeStampLoop = BLUtils::GetTimeMillisF();
    
    mDAWStartTransportValueSecLoop = dawTransportValueSec;
    
    mDAWTransportValueSecTotal = 0.0;
    
    mResynchOffsetSecLoop = 0.0;
    mDiffSmootherLoop->ResetToTargetValue(0.0);
    
    mResynchOffsetSecTotal = 0.0;
    mDiffSmootherTotal->ResetToTargetValue(0.0);
}
