//
//  SecureRestarter.h
//  Denoiser
//
//  Created by Apple m'a Tuer on 06/05/17.
//
//

#ifndef __Denoiser__SecureRestarter__
#define __Denoiser__SecureRestarter__

#include "../../WDL/IPlug/Containers.h"

// Logic X has a bug: when it restarts after stop,
// sometimes it provides full volume directly, making a sound crack.
// So we need to make a smooth transition by hand,
// each time we restart the play.
// We use an half Hanning on inputs for that, as Logic does (when it doesn't bug).
class SecureRestarter
{
public:
    SecureRestarter();
    
    virtual ~SecureRestarter();
    
    void Reset();
    
    void Process(double *buf0, double *buf1, int nFrames);

protected:
    bool mFirstTime;
    
    int mPrevNFrames;
    WDL_TypedBuf<double> mHanning;
};

#endif /* defined(__Denoiser__SecureRestarter__) */
