//
//  TransientShaperObj2.h
//  BL-Shaper
//
//  Created by Pan on 16/04/18.
//
//

#ifndef __BL_Shaper__TransientShaperFftObj2__
#define __BL_Shaper__TransientShaperFftObj2__

#include "FftObj.h"


// TransientShaper
class TransientShaperFftObj2 : public FftObj
{
public:
    TransientShaperFftObj2(int bufferSize, int oversampling, int freqRes,
                           bool doApplyTransients,
                           int maxNumPoints = 0, BL_FLOAT decimFactor = 1.0);
    
    virtual ~TransientShaperFftObj2();
    
    void Reset(int oversampling, int freqRes);
    
    
    void SetPrecision(BL_FLOAT precision);
    
    void SetSoftHard(BL_FLOAT softHard);
    
    void SetFreqsToTrans(bool flag);
    
    void SetAmpsToTrans(bool flag);
    
    
    void ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                          const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer = NULL);
    
    void ProcessSamplesBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer,
                              WDL_TypedBuf<BL_FLOAT> *scBuffer);
    
    
    void GetCurrentTransientness(WDL_TypedBuf<BL_FLOAT> *outTransientness);
    
    void ApplyTransientness(WDL_TypedBuf<BL_FLOAT> *ioSamples,
                            const WDL_TypedBuf<BL_FLOAT> &currentTransientness);
    
    void GetTransientness(WDL_TypedBuf<BL_FLOAT> *outTransientness);
    
protected:
#if 0
    BL_FLOAT ComputeMaxTransientness();
#endif
    
    // NOTE: we can't compute a transientness normalized from the gain of the signal...
    // ... because transientness doesn't depend on the gain
    
    BL_FLOAT mSoftHard;
    BL_FLOAT mPrecision;
    
    bool mFreqsToTrans;
    bool mAmpsToTrans;
    
    bool mDoApplyTransients;
    
    WDL_TypedBuf<BL_FLOAT> mCurrentTransientness;
    
    FifoDecimator mTransientness;
    
    // For computing derivative (for amp to trans)
    WDL_TypedBuf<BL_FLOAT> mPrevPhases;
};

#endif /* defined(__BL_Shaper__TransientShaperFftObj2__) */
