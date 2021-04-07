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
    bool SetTransportPlaying(bool transportPlaying, bool monitorOn = false);
    bool IsTransportPlaying();
    
    // Update the current smooth transport value
    void Update();
    
    // Smooth value
    BL_FLOAT GetTransportValueSec();
    
 protected:
    bool mIsTransportPlaying;
    bool mIsMonitorOn;

    double mStartTransportTimeStamp;
    double mNow;
};

#endif
