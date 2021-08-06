#ifndef PARTIAL_H
#define PARTIAL_H

#include <SimpleKalmanFilter.h>

#include <BLTypes.h>

class Partial
{
 public:
    enum State
    {
        ALIVE,
        ZOMBIE,
        DEAD
    };
        
    Partial();
        
    Partial(const Partial &other);
        
    virtual ~Partial();
        
    void GenNewId();
        
    //
    static bool FreqLess(const Partial &p1, const Partial &p2);
        
    static bool AmpLess(const Partial &p1, const Partial &p2);
        
    static bool IdLess(const Partial &p1, const Partial &p2);
        
    static bool CookieLess(const Partial &p1, const Partial &p2);
        
 public:
    int mPeakIndex;
    int mLeftIndex;
    int mRightIndex;
        
    // When detecting and filtering, mFreq and mAmp are "scaled and normalized"
    // After processing, we can compute the real frequencies in Hz and amp in dB.
    BL_FLOAT mFreq;
    union{
        // Inside PartialTracker6
        BL_FLOAT mAmp;
            
        // After, outside PartialTracker6, if external classes need amp in dB
        // Need to call DenormPartials() then PartialsAmpToAmpDB()
        BL_FLOAT mAmpDB;
    };
    BL_FLOAT mPhase;
        
    long mId;
        
    enum State mState;
        
    bool mWasAlive;
    long mZombieAge;
        
    long mAge;
        
    // All-purpose field
    BL_FLOAT mCookie;
        
    SimpleKalmanFilter mKf;
    BL_FLOAT mPredictedFreq;

    // QIFFT
    BL_FLOAT mBinIdxF;
    //BL_FLOAT mAmp;
    //BL_FLOAT mPhase;
    BL_FLOAT mAlpha0;
    BL_FLOAT mBeta0;
        
 protected:
    static unsigned long mCurrentId;
};

#endif
