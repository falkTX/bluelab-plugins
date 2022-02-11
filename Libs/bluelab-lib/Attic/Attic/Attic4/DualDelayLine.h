//
//  DualDelayLine.h
//  BL-Bat
//
//  Created by applematuer on 12/14/19.
//
//

#ifndef __BL_Bat__DualDelayLine__
#define __BL_Bat__DualDelayLine__

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

#include "../../WDL/fft.h"

// From DelayObj4
//
class DualDelayLine
{
public:
    DualDelayLine(BL_FLOAT sampleRate, BL_FLOAT maxDelay, BL_FLOAT delay);
    
    virtual ~DualDelayLine();
    
    void Reset(BL_FLOAT sampleRate);
    
    WDL_FFT_COMPLEX ProcessSample(WDL_FFT_COMPLEX sample);
    
protected:
    void Init();
    
    BL_FLOAT mSampleRate;
    
    // Delay in seconds
    // Delay is in [-maxDelay/2, maxDelay/2], and so can be negative
    BL_FLOAT mDelay;
    BL_FLOAT mMaxDelay;
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> mDelayLine;
    
    long mReadPos;
    long mWritePos;
};

#endif /* defined(__BL_Bat__DualDelayLine__) */
