#ifndef PARTIAL_H
#define PARTIAL_H

#include <vector>
using namespace std;

#include <SimpleKalmanFilter.h>

#include <BLTypes.h>

class Partial
{
 public:
    enum State
    {
        ALIVE = 0,
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

    // Debug
    static void DBG_PrintPartials(const vector<Partial> &partials);
    static void DBG_DumpPartials(const char *fileName,
                                 const vector<Partial> &partials, int size,
                                 bool normFreq = true, bool db = true);
    
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

    // Partial id
    long mId;
    // Index of the linked partial in the current array (for associating)
    long mLinkedId;
    
    enum State mState;
        
    bool mWasAlive;
    long mZombieAge;
        
    long mAge;
        
    // All-purpose field
    BL_FLOAT mCookie;
        
    SimpleKalmanFilter mKf;

    // QIFFT
    BL_FLOAT mBinIdxF;
    BL_FLOAT mAlpha0;
    BL_FLOAT mBeta0;
    //BL_FLOAT mAmp;
    //BL_FLOAT mPhase;
    
 protected:
    static unsigned long mCurrentId;
};

#endif
