#ifndef BL_TRANSPORT_H
#define BL_TRANSPORT_H

#include <BLTypes.h>

// Not working well; because when we start transport,
// there is a longer delay
// => so it jitters when starting play
#define USE_AUTO_HARD_RESYNCH 0

// Get current accurate transport value at any time.
// Because the daw transport value can be very hacked.
// Well suitable for smooth animations depending on transport
class ParamSmoother2;
class TransportListener;
class BLTransport
{
 public:
    BLTransport(BL_FLOAT sampleRate);
    virtual ~BLTransport();

    void SetSoftResynchEnabled(bool flag);
    
#if USE_AUTO_HARD_RESYNCH
    void SetAutoHardResynchThreshold(BL_FLOAT threshold = 0.1);
#endif
    
    void Reset();
    void Reset(BL_FLOAT sampleRate);
    
    // From daw
    // Return true if dependent object update is needed
    bool SetTransportPlaying(bool transportPlaying, bool monitorOn,
                             BL_FLOAT dawTransportValueSec,
                             int blockSize);
    bool IsTransportPlaying();
    
    void SetBypassed(bool flag);
    
    // Update the current smooth transport value
    void Update();
    
    // Real transport value from daw
    void SetDAWTransportValueSec(BL_FLOAT dawTransportValueSec);
    
    // Smooth value
    // Total time elapsed since start playing
    BL_FLOAT GetTransportElapsedSecTotal();
    // Time stamp elapsed since the last loop
    BL_FLOAT GetTransportElapsedSecLoop();
    // Get smoothed transport value, based on DAW transport value 
    BL_FLOAT GetTransportValueSecLoop();
    
    // Reset the estimated transport value
    // Resynch directly to the DAW transport value
    bool HardResynch();

    void SetListener(TransportListener *listener);
    
protected:
    // Resynch progressively the estimated transport value
    // to the host transport value
    // This method must be called continuously
    void SoftResynch();

    // Update current time stamp
    void UpdateNow();
    
    void SetupTransportJustStarted(BL_FLOAT dawTransportValueSec);

    //
    BL_FLOAT mSampleRate;
    
    bool mIsTransportPlaying;
    bool mIsMonitorOn;

    bool mIsBypassed;

    // NOTE: use double, otherwise we have drift due to float imprecision
    
    // Total, do not manage transport looping 
    double mStartTransportTimeStampTotal;
    // Manage transport looping
    double mStartTransportTimeStampLoop;
    
    double mNow;

    // DAW transport time when start playing, or re-starting a loop
    double mDAWStartTransportValueSecLoop;
    
    // DAW current transport time
    // NOTE: Not used for the moment
    double mDAWCurrentTransportValueSecLoop;

    // DAW transport time since start playing (accumulated)
    double mDAWTransportValueSecTotal;
    
    // Used for loop
    //

    bool mSoftResynchEnabled;
        
    // Offset used for hard or soft resynch
    double mResynchOffsetSecLoop;

    // Soft resynth
    ParamSmoother2 *mDiffSmootherLoop;

    // Used for total
    
    // Offset used for hard or soft resynch
    double mResynchOffsetSecTotal;

    // Soft resynth
    ParamSmoother2 *mDiffSmootherTotal;

#if USE_AUTO_HARD_RESYNCH
    BL_FLOAT mAutoHardResynchThreshold;
#endif

    // Later, add more if necessary
    TransportListener *mListener;
};

#endif
