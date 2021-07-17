//
//  SecureRestarter.h
//  Denoiser
//
//  Created by Apple m'a Tuer on 06/05/17.
//
//

#ifndef __Denoiser__SecureRestarter__
#define __Denoiser__SecureRestarter__

#include <vector>
using namespace std;

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"
//#include "../../WDL/IPlug/Containers.h"

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
    
    void Process(BL_FLOAT *buf0, BL_FLOAT *buf1, int nFrames);

    void Process(WDL_TypedBuf<BL_FLOAT> *buf0, WDL_TypedBuf<BL_FLOAT> *buf1);
    
    void Process(vector<WDL_TypedBuf<BL_FLOAT> > &bufs);
    
protected:
    bool mFirstTime;
    
    int mPrevNumSamplesToFade;
    WDL_TypedBuf<BL_FLOAT> mHanning;
};

#endif /* defined(__Denoiser__SecureRestarter__) */
