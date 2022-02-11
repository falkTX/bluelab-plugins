//
//  RebalanceProcessFftObjComp2.h
//  BL-Rebalance
//
//  Created by applematuer on 5/17/20.
//
//

#ifndef __BL_Rebalance__RebalanceProcessFftObjComp2__
#define __BL_Rebalance__RebalanceProcessFftObjComp2__

#include "IPlug_include_in_plug_hdr.h"

#include "../../WDL/fft.h"

#include <FftProcessObj16.h>

//#include <Rebalance.h>

#if FORCE_SAMPLE_RATE
#include "../../WDL/resample.h"
#endif

// RebalanceProcessFftObj
//
// Apply the masks to the current channels
//
// RebalanceProcessFftObjComp: from RebalanceProcessFftObj
// - use RebalanceMaskPredictorComp
//
// RebalanceProcessFftObjComp2:
// for RebalanceMaskPredictorComp3

// For Unet
//#define MASK_PREDICTOR_CLASS RebalanceMaskPredictorComp4

// For method of Leonardo Pepino
#define MASK_PREDICTOR_CLASS RebalanceMaskPredictorComp5

class MASK_PREDICTOR_CLASS;
class SoftMaskingNComp;
class RebalanceProcessFftObjComp2 : public ProcessObj
{
public:
    RebalanceProcessFftObjComp2(int bufferSize,
                                MASK_PREDICTOR_CLASS *maskPred);
    
    virtual ~RebalanceProcessFftObjComp2();
    
    void Reset(int bufferSize, int oversampling, int freqRes, BL_FLOAT sampleRate);
    
    void Reset();
    
#if FORCE_SAMPLE_RATE
    void ResetResamplers();
#endif
    
    virtual void ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                                  const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer);
    
#if FORCE_SAMPLE_RATE
    void ProcessSamplesPost(WDL_TypedBuf<BL_FLOAT> *ioBuffer);
#endif
    
    void SetVocal(BL_FLOAT vocal);
    void SetBass(BL_FLOAT bass);
    void SetDrums(BL_FLOAT drums);
    void SetOther(BL_FLOAT other);
    
    void SetVocalSensitivity(BL_FLOAT vocalSensitivity);
    void SetBassSensitivity(BL_FLOAT bassSensitivity);
    void SetDrumsSensitivity(BL_FLOAT drumsSensitivity);
    void SetOtherSensitivity(BL_FLOAT otherSensitivity);
    
    void SetMode(RebalanceMode mode);
    
    // Global precision (previous soft/hard)
    void SetMasksContrast(BL_FLOAT contrast);
    
#if FORCE_SAMPLE_RATE
    void SetPlugSampleRate(BL_FLOAT sampleRate);
#endif
    
    // Soft masks
    void SetUseSoftMasks(bool flag);
    
protected:
    void ComputeMix(WDL_TypedBuf<WDL_FFT_COMPLEX> *dataResult,
                    const WDL_TypedBuf<WDL_FFT_COMPLEX> &dataMix);
    
#if FORCE_SAMPLE_RATE
    void InitResamplers();
#endif
    
    void ApplySoftMasks(WDL_TypedBuf<WDL_FFT_COMPLEX> masksResult[4],
                        const WDL_TypedBuf<BL_FLOAT> masksSource[4]);

    void CompDiv(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > *estim,
                 const WDL_TypedBuf<WDL_FFT_COMPLEX> &mix);
    
    //
    void ResetSamplesHistory();
    void ResetMixColsComp();
    
    // Mix
    void ApplyMix(WDL_TypedBuf<WDL_FFT_COMPLEX> masks[4]);
    void ApplyMix(WDL_FFT_COMPLEX masks[4]);
    
    // Post normalization
    void NormalizeMasks(WDL_TypedBuf<WDL_FFT_COMPLEX> masks[4]);
    void NormalizeMaskVals(WDL_FFT_COMPLEX maskVals[4]);
                        
    //
    MASK_PREDICTOR_CLASS *mMaskPred;
    
    RebalanceMode mMode;
    
#if FORCE_SAMPLE_RATE
    WDL_Resampler mResamplerIn;
    WDL_Resampler mResamplerOut;
    
    BL_FLOAT mPlugSampleRate;
    
    // ratio of sample remaining after upsampling
    // (to adjust and avoid clicks / blank zones in the output)
    BL_FLOAT mRemainingSamples;
#endif
    
    // Keep the history of input data
    // So we can get exactly the same correponding the the correct location of the mask
    deque<WDL_TypedBuf<WDL_FFT_COMPLEX> > mSamplesHistory;
    
    // Soft masks
    bool mUseSoftMasks;
    
    SoftMaskingNComp *mSoftMasking;
    
    deque<WDL_TypedBuf<WDL_FFT_COMPLEX> > mMixColsComp;
    
    // Mix parameters
    BL_FLOAT mMixes[4];
};

#endif /* defined(__BL_Rebalance__RebalanceProcessFftObjComp2__) */
