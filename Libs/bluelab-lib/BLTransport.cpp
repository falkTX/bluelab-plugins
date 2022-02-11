#include <BLUtils.h>
#include <BLDebug.h>

#include <ParamSmoother2.h>

#include <TransportListener.h>

#include "BLTransport.h"

// Reset if transport (from the host) made a very long jump
#define MAX_TRANSPORT_JUMP_SEC 10.0

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
    // 2.0 is too slow, we have black lines, and re-center spectro is too slow
#define DELAY_MS 0.5 //2.0 //1.0
    mResynchOffsetSecLoop = 0.0;
    mDiffSmootherLoop = new ParamSmoother2(sampleRate, 0.0, DELAY_MS);

    mResynchOffsetSecTotal = 0.0;
    mDiffSmootherTotal = new ParamSmoother2(sampleRate, 0.0, DELAY_MS);

    mIsBypassed = false;

#if USE_AUTO_HARD_RESYNCH
    mHardResynchThreshold = BL_INF;
#endif

    mListener = NULL;

    mUseLegacyLock = false;
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

/*bool*/
void
BLTransport::SetTransportPlaying(bool transportPlaying,
                                 bool monitorOn,
                                 BL_FLOAT dawTransportValueSec,
                                 int blockSize)
{
    if (mUseLegacyLock)
    {
        SetTransportPlayingLF(transportPlaying, monitorOn,
                              dawTransportValueSec, blockSize);
        return;
    }
    
    Command &cmd = mTmpBuf1;

    cmd.mType = Command::SET_TRANSPORT_PLAYING;
    cmd.mIsPlaying = transportPlaying;
    cmd.mMonitorEnabled = monitorOn;
    cmd.mBlockSize = blockSize;
    
    cmd.mTransportTime = dawTransportValueSec;

    mLockFreeQueues[0].push(cmd);
}

bool
BLTransport::SetTransportPlayingLF(bool transportPlaying,
                                   bool monitorOn,
                                   BL_FLOAT dawTransportValueSec,
                                   int blockSize)
{
    if (mIsBypassed)
        return false;

    bool transportJustStarted = ((transportPlaying && !mIsTransportPlaying) ||
                                 (monitorOn && !mIsMonitorOn));

#if 0
    bool transportJustStopped = ((mIsTransportPlaying || mIsMonitorOn) &&
                                 (!transportPlaying && !monitorOn));
#endif
    
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

    if (dawTransportValueSec >
        mDAWCurrentTransportValueSecLoop + MAX_TRANSPORT_JUMP_SEC)
    {
        // Transport has done a very long jump
        // Steps: play, click very far later on the time line
        // Clear smoothers and so on to avoid a long smooth and slow scrolling
        // to the new transport position
        SetupTransportJustStarted(dawTransportValueSec);
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

    if (mListener != NULL)
    {
        if (transportJustStarted)
            mListener->TransportPlayingChanged();
    }
            
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

void
BLTransport::SetDAWTransportValueSec(BL_FLOAT dawTransportValueSec)
{
    if (mUseLegacyLock)
    {
        SetDAWTransportValueSecLF(dawTransportValueSec);
            
        return;
    }
    
    Command &cmd = mTmpBuf2;

    cmd.mType = Command::SET_DAW_TRANSPORT_VALUE;
    
    cmd.mTransportTime = dawTransportValueSec;

    mLockFreeQueues[0].push(cmd);
}
    
// Change only the transport value when start playing or reseting a loop
// Otherwise process smoothly
//
// NOTE: for the moment it is only called for resetting the transport to 0
void
BLTransport::SetDAWTransportValueSecLF(BL_FLOAT dawTransportValueSec)
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

    if (dawTransportValueSec >
        mDAWCurrentTransportValueSecLoop + MAX_TRANSPORT_JUMP_SEC)
    {
        // Transport has done a very long jump
        // Steps: play, click very far later on the time line
        // Clear smoothers and so on to avoid a long smooth and slow scrolling
        // to the new transport position
        SetupTransportJustStarted(dawTransportValueSec);
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
    // If we auto hard resynch, do as if we just start playing 
    if (std::fabs(mResynchOffsetSecTotal) < mHardResynchThreshold)
        return false;
#endif
    
    SetupTransportJustStarted(mDAWCurrentTransportValueSecLoop);
    
    return true;
}

void
BLTransport::SetListener(TransportListener *listener)
{
    mListener = listener;
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

void
BLTransport::PushData()
{
    if (mUseLegacyLock)
        return;
    
    mLockFreeQueues[1].push(mLockFreeQueues[0]);
    mLockFreeQueues[0].clear();
}

void
BLTransport::PullData()
{
    if (mUseLegacyLock)
        return;
    
    mLockFreeQueues[2].push(mLockFreeQueues[1]);
    mLockFreeQueues[1].clear();
}

void
BLTransport::ApplyData()
{
    if (mUseLegacyLock)
        return;
    
    for (int i = 0; i < mLockFreeQueues[2].size(); i++)
    {
        Command &cmd = mTmpBuf0;
        mLockFreeQueues[2].get(i, cmd);

        if (cmd.mType == Command::SET_TRANSPORT_PLAYING)
        {
            SetTransportPlayingLF(cmd.mIsPlaying, cmd.mMonitorEnabled,
                                  cmd.mTransportTime, cmd.mBlockSize);
        }
        else if (cmd.mType == Command::SET_DAW_TRANSPORT_VALUE)
        {
            SetDAWTransportValueSecLF(cmd.mTransportTime);
        }
    }

    mLockFreeQueues[2].clear();
}

void
BLTransport::SetUseLegacyLock(bool flag)
{
    mUseLegacyLock = flag;
}
