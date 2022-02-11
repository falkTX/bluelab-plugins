//
//  DelayObj3.h
//  BL-Precedence
//
//  Created by applematuer on 3/12/19.
//
//

#ifndef __BL_Precedence__DelayObj3__
#define __BL_Precedence__DelayObj3__

#include "IPlug_include_in_plug_hdr.h"

// Delay line obj
// Manages clicks:
// - when changing the delay when playing
// - when restarting after a reset

// IMPROV: DelayObj2
// - set and get sample by sample, to manage correctly independently of block size
// - manages better when resizing the delay line (insert and remove more correclty)
// NOTE: makes clicks (cyclic buffer), maybe not well debugged

// IMPROV: DelayObj3
// - naive implementation, with double value for delay
// GOOD (and makes a sort of Doppler effect)
// Little performances problem...
class DelayObj3
{
public:
    DelayObj3(BL_FLOAT delay);
    
    virtual ~DelayObj3();
    
    void Reset();
    
    void SetDelay(BL_FLOAT delay);
    
    BL_FLOAT ProcessSample(BL_FLOAT sample);

protected:
    BL_FLOAT mDelay;
    
    WDL_TypedBuf<BL_FLOAT> mDelayLine;
};

#endif /* defined(__BL_Precedence__DelayObj3__) */
