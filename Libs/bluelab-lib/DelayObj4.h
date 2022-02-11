//
//  DelayObj4.h
//  BL-Precedence
//
//  Created by applematuer on 3/12/19.
//
//

#ifndef __BL_Precedence__DelayObj4__
#define __BL_Precedence__DelayObj4__

#include <BLTypes.h>

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
// - naive implementation, with BL_FLOAT value for delay
// GOOD (and makes a sort of Doppler effect)
// Little performances problem...

// IMPROV: DelayObj4
// use cyclic buffer to optimize
// (optimizes a lot with long delay lines)
class DelayObj4
{
public:
    DelayObj4(BL_FLOAT delay);
    
    DelayObj4(const DelayObj4 &other);
    
    virtual ~DelayObj4();
    
    void Reset();
    
    void SetDelay(BL_FLOAT delay);
    
    BL_FLOAT ProcessSample(BL_FLOAT sample);

    void ProcessSamples(WDL_TypedBuf<BL_FLOAT> *samples);
    
protected:
    BL_FLOAT mDelay;
    
    WDL_TypedBuf<BL_FLOAT> mDelayLine;
    
    long mReadPos;
    long mWritePos;
};

#endif /* defined(__BL_Precedence__DelayObj4__) */
