#include <stdio.h>

#include <BLUtils.h>

#include <BLDebug.h>

#include "IPlug_include_in_plug_hdr.h"

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
    mLinkedId = -1;
    
    mWasAlive = false;
    mZombieAge = 0;
    
    mAge = 0;
    
    mCookie = 0.0;
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
    mLinkedId = other.mLinkedId;
    
    mWasAlive = other.mWasAlive;
    mZombieAge = other.mZombieAge;
    
    mAge = other.mAge;
    
    mCookie = other.mCookie;

    // QIFFT
    mBinIdxF = other.mBinIdxF;
    mAlpha0 = other.mAlpha0;
    mBeta0 = other.mBeta0;
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

void
Partial::DBG_PrintPartials(const vector<Partial> &partials)
{
    fprintf(stderr, "-------------------\n");
    for (int i = 0; i < partials.size(); i++)
    {
        const Partial &p = partials[i];
        
        fprintf(stderr, "id: %ld state: %d amp: %g freq: %g\n",
                p.mId, p.mState, p.mAmp, p.mFreq);
    }
}

void
Partial::DBG_DumpPartials(const char *fileName,
                          const vector<Partial> &partials, int size)
{
#define MIN_AMP_DB -120.0
    
    WDL_TypedBuf<BL_FLOAT> data;
    data.Resize(size);
    BLUtils::FillAllZero(&data);
    
    for (int i = 0; i < partials.size(); i++)
    {
        const Partial &p = partials[i];
        
        int idx = p.mFreq*size;
        if (idx >= size)
            continue;

        BL_FLOAT amp = (p.mAmp - MIN_AMP_DB)/(0.0 - MIN_AMP_DB);
        data.Get()[idx] = amp; //p.mAmp;
    }

    BLDebug::DumpData(fileName, data);
}
