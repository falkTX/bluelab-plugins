//
//  TransientShaperObj.h
//  BL-Shaper
//
//  Created by Pan on 16/04/18.
//
//

#ifndef __BL_Shaper__TransientShaperObj__
#define __BL_Shaper__TransientShaperObj__

#include "FifoDecimator.h"

#include "FftProcessObj12.h"


// TransientShaper
class TransientShaperFftObj : public FftProcessObj12
{
public:
    TransientShaperFftObj(int bufferSize, int oversampling, int freqRes,
                          bool useAnalysisWindowing,
                          BL_FLOAT precision,
                          int maxNumPoints, BL_FLOAT decimFactor,
                          bool applyTransients = true);
    
    virtual ~TransientShaperFftObj();
    
    void Reset(int oversampling, int freqRes);
    
    void SetPrecision(BL_FLOAT precision);
    
    void SetSoftHard(BL_FLOAT softHard);
    
    void SetFreqsToTrans(bool flag);
    
    void SetAmpsToTrans(bool flag);
    
    void PreProcessSamplesBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer);
    
    void ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                          const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer = NULL);
    
    void ProcessSamplesBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer,
                              WDL_TypedBuf<BL_FLOAT> *scBuffer);
    
    void PostProcessSamplesBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer);
    
    void GetTransientness(WDL_TypedBuf<BL_FLOAT> *outTransientness);
    
    void GetInput(WDL_TypedBuf<BL_FLOAT> *outInput);
    
    void GetOutput(WDL_TypedBuf<BL_FLOAT> *outOutput);
    
    void GetCurrentTransientness(WDL_TypedBuf<BL_FLOAT> *outTransientness);
    
    void ApplyTransientness(WDL_TypedBuf<BL_FLOAT> *ioSamples,
                            const WDL_TypedBuf<BL_FLOAT> &currentTransientness);
    
protected:
    BL_FLOAT ComputeMaxTransientness();
    
    // Transientness doe not depend on the magnitude...
    // So impossible to normalize in a simple way...
    
    //void ComputeNormTransientness(WDL_TypedBuf<BL_FLOAT> *outTransientess,
    //                              const WDL_TypedBuf<BL_FLOAT> &transientness,
    //                              const WDL_TypedBuf<BL_FLOAT> &magns);
    
    BL_FLOAT mSoftHard;
    BL_FLOAT mPrecision;
    
    bool mFreqsToTrans;
    bool mAmpsToTrans;
    
    bool mDoApplyTransients;
    
    WDL_TypedBuf<BL_FLOAT> mCurrentTransientness;
    
    FifoDecimator mTransientness;
    FifoDecimator mInput;
    FifoDecimator mOutput;
    
    // For computing derivative (for amp to trans)
    WDL_TypedBuf<BL_FLOAT> mPrevPhases;
    
    long mBufferCount;
};

#endif /* defined(__BL_Shaper__TransientShaperObj__) */
