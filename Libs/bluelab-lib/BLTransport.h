#ifndef BL_TRANSPORT_H
#define BL_TRANSPORT_H

#include <BLTypes.h>

// Get current accurate transport value at any time.
// Because the daw transport value can be very hacked.
// Well suitable for smooth animations depending on transport

class BLTransport
{
 public:
    BLTransport();
    virtual ~BLTransport();

    void Reset();
    
    // From daw
    // Return true if dependent object update is needed
    bool SetTransportPlaying(bool transportPlaying, bool monitorOn = false,
                             BL_FLOAT transportTime = -1.0);
    bool IsTransportPlaying();
    
    // Update the current smooth transport value
    void Update();

    // Real transport value from daw
    void SetDAWTransportValueSec(BL_FLOAT transportTime);
    
    // Smooth value
    // Total time elapsed since start playing
    BL_FLOAT GetTransportElapsedSecTotal();
    // Time stamp elapsed since the last loop
    BL_FLOAT GetTransportElapsedSecLoop();
    // Get smoothed transport value, based on DAW transport value 
    BL_FLOAT GetTransportValueSecLoop();
    
 protected:
    bool mIsTransportPlaying;
    bool mIsMonitorOn;

    // Total, do not manage transport looping 
    double mStartTransportTimeStampTotal;
    // Manage transport looping
    double mStartTransportTimeStampLoop;
    
    double mNow;

    BL_FLOAT mDAWTransportValueSec;
};

#endif
