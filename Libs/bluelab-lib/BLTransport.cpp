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

    mResynchOffsetSec = 0.0;
    // Hak delay value
#define DELAY_MS 2.0 //1.0
    mDiffSmoother = new ParamSmoother2(sampleRate, 0.0, DELAY_MS);
    mMustResetDiffSmoother = true;
}

BLTransport::~BLTransport()
{
    delete mDiffSmoother;
}

void
BLTransport::Reset()
{
    mStartTransportTimeStampTotal = BLUtils::GetTimeMillisF();
    mStartTransportTimeStampLoop = BLUtils::GetTimeMillisF();

    mDAWStartTransportValueSecLoop = mDAWCurrentTransportValueSecLoop;
        
    mResynchOffsetSec = 0.0;
    //mDiffSmoother->ResetToTargetValue(0.0);
    mMustResetDiffSmoother = true;
}

void
BLTransport::Reset(BL_FLOAT sampleRate)
{
    Reset();

    mDiffSmoother->Reset(sampleRate);
    mMustResetDiffSmoother = true;
}

bool
BLTransport::SetTransportPlaying(bool transportPlaying,
                                 bool monitorOn,
                                 BL_FLOAT dawTransportValueSec)
{    
    bool result = false;

    bool loopDetected = (dawTransportValueSec < mDAWCurrentTransportValueSecLoop);
    
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

        mResynchOffsetSec = 0.0;
        //mDiffSmoother->ResetToTargetValue(0.0);
        mMustResetDiffSmoother = true;
        
        result = true;
    }

    if (loopDetected)
    {
        mStartTransportTimeStampLoop = BLUtils::GetTimeMillisF();
        mDAWStartTransportValueSecLoop = dawTransportValueSec;

        mResynchOffsetSec = 0.0;
        //mDiffSmoother->ResetToTargetValue(0.0);
        mMustResetDiffSmoother = true;
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
    if (dawTransportValueSec < mDAWCurrentTransportValueSecLoop)
        // Just restarted a loop
    {
        mStartTransportTimeStampLoop = BLUtils::GetTimeMillisF();
        
        mDAWStartTransportValueSecLoop = dawTransportValueSec;

        mResynchOffsetSec = 0.0;
        //mDiffSmoother->ResetToTargetValue(0.0);
        mMustResetDiffSmoother = true;
    }

    mDAWCurrentTransportValueSecLoop = dawTransportValueSec;
}

BL_FLOAT
BLTransport::GetTransportElapsedSecTotal()
{
    if ((mStartTransportTimeStampTotal < 0.0) ||
        (mNow < 0.0))
        return -1;

    double elapsed =
        (mNow - mStartTransportTimeStampTotal)*0.001 + mResynchOffsetSec;

    return elapsed;
}

BL_FLOAT
BLTransport::GetTransportElapsedSecLoop()
{
    if ((mStartTransportTimeStampLoop < 0.0) ||
        (mNow < 0.0))
        return -1;

    double elapsed =
        (mNow - mStartTransportTimeStampLoop)*0.001 + mResynchOffsetSec;

    return elapsed;
}

BL_FLOAT
BLTransport::GetTransportValueSecLoop()
{
    BL_FLOAT elapsed = GetTransportElapsedSecLoop();

    if (elapsed < 0.0)
        elapsed = 0.0;

    BL_FLOAT result =
        mDAWStartTransportValueSecLoop + elapsed;

#ifdef DEBUG_DUMP
    BLDebug::AppendValue("real.txt", mDAWCurrentTransportValueSecLoop);
    BLDebug::AppendValue("smooth.txt", result);
#endif
    
    return result;
}

void
BLTransport::HardResynch()
{
    mResynchOffsetSec = 0.0;
    
    BL_FLOAT estimTransport =
        mDAWStartTransportValueSecLoop + GetTransportElapsedSecLoop();
    
    BL_FLOAT diff = estimTransport - mDAWCurrentTransportValueSecLoop;

    mResynchOffsetSec = -diff;
}

void
BLTransport::SoftResynch()
{
    if (mIsTransportPlaying)
        // Update only if the real transport value changes
    {   
        mResynchOffsetSec = 0.0;
        
        BL_FLOAT estimTransport =
            mDAWStartTransportValueSecLoop + GetTransportElapsedSecLoop();
    
        BL_FLOAT diff = estimTransport - mDAWCurrentTransportValueSecLoop;

        if (mMustResetDiffSmoother)
        {
            mDiffSmoother->ResetToTargetValue(diff);
            mMustResetDiffSmoother = false;
        }
        
        mDiffSmoother->SetTargetValue(diff);
        BL_FLOAT diffSmooth = mDiffSmoother->Process();
        
        mResynchOffsetSec = -diffSmooth;
    }
}
