//
//  RebalanceProcessFftObj.h
//  BL-Rebalance
//
//  Created by applematuer on 5/17/20.
//
//

#ifndef __BL_Rebalance__RebalanceProcessFftObj__
#define __BL_Rebalance__RebalanceProcessFftObj__

#include "IPlug_include_in_plug_hdr.h"
#include "../../WDL/fft.h"

#include <FftProcessObj16.h>

// Include for defines
//#include <Rebalance_defs.h>
#include <Rebalance.h>

#if FORCE_SAMPLE_RATE
#include "../../WDL/resample.h"
#endif

// RebalanceProcessFftObj
//
// Apply the masks to the current channels
//
class RebalanceMaskPredictor;
class RebalanceProcessFftObj : public ProcessObj
{
public:
    RebalanceProcessFftObj(int bufferSize, RebalanceMaskPredictor *maskPred);
    
    virtual ~RebalanceProcessFftObj();
    
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
    
    void SetMode(Rebalance::Mode mode);
    
    // Global precision (previous soft/hard)
    void SetPrecision(BL_FLOAT precision);
    
    // Apply on values
    void ApplySensitivity(BL_FLOAT *maskVocal, BL_FLOAT *maskBass,
                          BL_FLOAT *maskDrums, BL_FLOAT *maskOther);
    
    void DBG_SetSpectrogramDump(bool flag);
    
#if FORCE_SAMPLE_RATE
    void SetPlugSampleRate(BL_FLOAT sampleRate);
#endif
    
#if USE_SOFT_MASK_N
    void SetUseSoftMasks(bool flag);
#endif
    
protected:
    void ComputeMixSoft(WDL_TypedBuf<BL_FLOAT> *resultMagns,
                        const WDL_TypedBuf<BL_FLOAT> &magnsMix);
    
    void ComputeMixHard(WDL_TypedBuf<BL_FLOAT> *resultMagns,
                        const WDL_TypedBuf<BL_FLOAT> &magnsMix);
    
    void NormalizeMasks(WDL_TypedBuf<BL_FLOAT> *maskVocal,
                        WDL_TypedBuf<BL_FLOAT> *maskBass,
                        WDL_TypedBuf<BL_FLOAT> *maskDrums,
                        WDL_TypedBuf<BL_FLOAT> *maskOther);
    
    // Apply on whole masks
    void ApplySensitivity(WDL_TypedBuf<BL_FLOAT> *maskVocal,
                          WDL_TypedBuf<BL_FLOAT> *maskBass,
                          WDL_TypedBuf<BL_FLOAT> *maskDrums,
                          WDL_TypedBuf<BL_FLOAT> *maskOther);
    
    
    // Debug: listen to the audio sent to the dnn
    void DBG_ListenBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                          const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer);
    
    void DBG_SaveSpectrograms();
    
#if FORCE_SAMPLE_RATE
    void InitResamplers();
#endif
    
    //
    
    RebalanceMaskPredictor *mMaskPred;
    
    // Parameters
    BL_FLOAT mVocal;
    BL_FLOAT mBass;
    BL_FLOAT mDrums;
    BL_FLOAT mOther;
    
    BL_FLOAT mVocalSensitivity;
    BL_FLOAT mBassSensitivity;
    BL_FLOAT mDrumsSensitivity;
    BL_FLOAT mOtherSensitivity;
    
    // Global precision
    BL_FLOAT mPrecision;
    
    Rebalance::Mode mMode;
    
    // Debug
    bool mDbgSpectrogramDump;
    
    DbgSpectrogram *mDbgMixSpectrogram;
    DbgSpectrogram *mDbgSourceSpectrograms[4];
    
#if FORCE_SAMPLE_RATE
    WDL_Resampler mResamplerIn;
    WDL_Resampler mResamplerOut;
    
    BL_FLOAT mPlugSampleRate;
    
    // ratio of sample remaining after upsampling
    // (to adjust and avoid clicks / blank zones in the output)
    BL_FLOAT mRemainingSamples;
#endif
};

#endif /* defined(__BL_Rebalance__RebalanceProcessFftObj__) */
