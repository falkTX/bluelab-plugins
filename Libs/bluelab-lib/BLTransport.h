#ifndef BL_TRANSPORT_H
#define BL_TRANSPORT_H

#include <BLTypes.h>

// Get current accurate transport value at any time.
// Because the daw transport value can be very hacked.
// Well suitable for smooth animations depending on transport
class ParamSmoother2;
class BLTransport
{
 public:
    BLTransport(BL_FLOAT sampleRate);
    virtual ~BLTransport();

    void SetSoftResynchEnabled(bool flag);
    
    void Reset();
    void Reset(BL_FLOAT sampleRate);
    
    // From daw
    // Return true if dependent object update is needed
    bool SetTransportPlaying(bool transportPlaying, bool monitorOn = false,
                             BL_FLOAT dawTransportValueSec = -1.0);
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

    // Reset the extimated transport value
    // Resynch directly to the DAW transport value
    void HardResynch();
    
 protected:
    // Resynch progressively the estimated transport value
    // to the host transport value
    // This method must be called continuously
    void SoftResynch();

    //
    bool mIsTransportPlaying;
    bool mIsMonitorOn;

    bool mIsBypassed;
    
    // Total, do not manage transport looping 
    double mStartTransportTimeStampTotal;
    // Manage transport looping
    double mStartTransportTimeStampLoop;
    
    double mNow;

    // DAW transport time when start playing, or re-starting a loop
    BL_FLOAT mDAWStartTransportValueSecLoop;
    
    // DAW current transport time
    // NOTE: Not used for the moment
    BL_FLOAT mDAWCurrentTransportValueSecLoop;

    // DAW transport time since start playing (accumulated)
    BL_FLOAT mDAWTransportValueSecTotal;
    
    // Used for loop
    //

    bool mSoftResynchEnabled;
        
    // Offset used for hard or soft resynch
    BL_FLOAT mResynchOffsetSecLoop;

    // Soft resynth
    ParamSmoother2 *mDiffSmootherLoop;

    // Used for total
    
    // Offset used for hard or soft resynch
    BL_FLOAT mResynchOffsetSecTotal;

    // Soft resynth
    ParamSmoother2 *mDiffSmootherTotal;
};

#endif
