#include "Partial.h"

// Kalman
// "How much do we expect to our measurement vary"
//
// 200Hz (previously tested with 50Hz)
#define PT5_KF_E_MEA 0.01 // 200.0Hz
#define PT5_KF_E_EST PT5_KF_E_MEA

// "usually a small number between 0.001 and 1"
//
// If too low: predicted values move too slowly
// If too high: predicted values go straight
//
// 0.01: "oohoo" => fails when "EEAAooaa"
#define PT5_KF_Q 5.0 //2.0 // Was 1.0


unsigned long Partial::mCurrentId = 0;

Partial::Partial()
: mKf(PT5_KF_E_MEA, PT5_KF_E_EST, PT5_KF_Q)
{
    mPeakIndex = 0;
    mLeftIndex = 0;
    mRightIndex = 0;
    
    mFreq = 0.0;
    mAmp = 0.0;
    
    mPhase = 0.0;
    
    mState = ALIVE;
    
    mId = -1;
    
    mWasAlive = false;
    mZombieAge = 0;
    
    mAge = 0;
    
    mCookie = 0.0;
    
    // Kalman
    mPredictedFreq = 0.0;
}

    
Partial::Partial(const Partial &other)
: mKf(other.mKf)
{
    mPeakIndex = other.mPeakIndex;
    mLeftIndex = other.mLeftIndex;
    mRightIndex = other.mRightIndex;
    
    mFreq = other.mFreq;
    mAmp = other.mAmp;
    
    mPhase = other.mPhase;
        
    mState = other.mState;;
        
    mId = other.mId;
    
    mWasAlive = other.mWasAlive;
    mZombieAge = other.mZombieAge;
    
    mAge = other.mAge;
    
    mCookie = other.mCookie;
    
    // Kalman
    mPredictedFreq = other.mPredictedFreq;
}

Partial::~Partial() {}

void
Partial::GenNewId()
{
    mId = mCurrentId++;
}
    
bool
Partial::FreqLess(const Partial &p1, const Partial &p2)
{
    return (p1.mFreq < p2.mFreq);
}

bool
Partial::AmpLess(const Partial &p1, const Partial &p2)
{
    return (p1.mAmp < p2.mAmp);
}

bool
Partial::IdLess(const Partial &p1, const Partial &p2)
{
    return (p1.mId < p2.mId);
}

bool
Partial::CookieLess(const Partial &p1, const Partial &p2)
{
    return (p1.mCookie < p2.mCookie);
}
