//
//  AnticlickObj.h
//  Denoiser
//
//  Created by Apple m'a Tuer on 06/05/17.
//
//

#ifndef __Denoiser__AnticlickObj__
#define __Denoiser__AnticlickObj__

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
class AnticlickObj
{
public:
    enum Direction
    {
        OFF_TO_ON,
        ON_TO_OFF
    };
    
    AnticlickObj();
    
    virtual ~AnticlickObj();
    
    void Reset(Direction dir);
    
    bool MustProcessOnSignal();
    
    void SetOffSignal(const vector<WDL_TypedBuf<BL_FLOAT> > &bufs);
    void SetOnSignal(const vector<WDL_TypedBuf<BL_FLOAT> > &bufs);
    
    void Process(vector<WDL_TypedBuf<BL_FLOAT> > *bufs);
    
protected:
    vector<WDL_TypedBuf<BL_FLOAT> > mOnSignal;
    vector<WDL_TypedBuf<BL_FLOAT> > mOffSignal;
    
    bool mNeedProcess;
    
    Direction mDirection;
};

#endif /* defined(__Denoiser__AnticlickObj__) */
