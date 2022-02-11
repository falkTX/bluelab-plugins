//
//  DelayLinePhaseShift2.h
//  BL-Bat
//
//  Created by applematuer on 12/14/19.
//
//

#ifndef __BL_Bat__DelayLinePhaseShift2__
#define __BL_Bat__DelayLinePhaseShift2__

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

#include "../../WDL/fft.h"

class DelayLinePhaseShift2
{
public:
    DelayLinePhaseShift2(BL_FLOAT sampleRate, int bufferSize, int binNum,
                        BL_FLOAT maxDelay, BL_FLOAT delay);
    
    virtual ~DelayLinePhaseShift2();
    
    void Reset(BL_FLOAT sampleRate);
    
    WDL_FFT_COMPLEX ProcessSample(WDL_FFT_COMPLEX sample);
    
    static void DBG_Test(int bufferSize, BL_FLOAT sampleRate,
                         const WDL_TypedBuf<WDL_FFT_COMPLEX> &samples);
    
protected:
    void Init();
    
    BL_FLOAT mSampleRate;
    int mBufferSize;
    
    int mBinNum;
    
    // Delay in seconds
    // Delay is in [-maxDelay/2, maxDelay/2], and so can be negative
    BL_FLOAT mDelay;
    BL_FLOAT mMaxDelay;
    
    BL_FLOAT mDPhase;
    
    // Friend for optimization
    friend class SourceLocalisationSystem3;
    WDL_FFT_COMPLEX mPhaseShiftComplex;
};

#endif /* defined(__BL_Bat__DelayLinePhaseShift2__) */
