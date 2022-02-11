//
//  DelayObj.h
//  BL-Precedence
//
//  Created by applematuer on 3/12/19.
//
//

#ifndef __BL_Precedence__DelayObj__
#define __BL_Precedence__DelayObj__

#include "IPlug_include_in_plug_hdr.h"

// Delay line obj
// Manages clicks:
// - when changing the delay when playing
// - when restarting after a reset

// IMPROV:
// - set and get sample by sample, to manage correctly independently of block size
// - manages better when resizing the delay line (insert and remove more correclty)
// NOTE: makes clicks (cyclic buffer), maybe not well debugged

class DelayObj2
{
public:
    DelayObj2(int delay);
    
    virtual ~DelayObj2();
    
    void Reset();
    
    void SetDelay(int delay);
    
    BL_FLOAT ProcessSample(BL_FLOAT sample);

protected:
    int mDelay;
    
    long mWriteAddress;
    
    WDL_TypedBuf<BL_FLOAT> mDelayLine;
};

#endif /* defined(__BL_Precedence__DelayObj2__) */
