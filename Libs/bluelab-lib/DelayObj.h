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
class DelayObj
{
public:
    DelayObj(int maxDelaySamples, BL_FLOAT sampleRate);
    
    virtual ~DelayObj();
    
    void Reset(BL_FLOAT sampleRate);
    
    void SetDelay(int delay);
    
    void AddSamples(const WDL_TypedBuf<BL_FLOAT> &inBuf);

    void GetSamples(const WDL_TypedBuf<BL_FLOAT> &inBuf,
                    WDL_TypedBuf<BL_FLOAT> *outBuf);
    
    void ConsumeBuffer();

protected:
    int mMaxDelaySamples;
    
    BL_FLOAT mSampleRate;
    
    int mDelay;
    int mPrevDelay;
    
    bool mPrevReset;
    
    WDL_TypedBuf<BL_FLOAT> mDelayLine;
    WDL_TypedBuf<BL_FLOAT> mPrevDelayLine;
};

#endif /* defined(__BL_Precedence__DelayObj__) */
