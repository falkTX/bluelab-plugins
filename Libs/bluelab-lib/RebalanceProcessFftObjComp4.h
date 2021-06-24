//
//  RebalanceProcessFftObjComp4.h
//  BL-Rebalance
//
//  Created by applematuer on 5/17/20.
//
//

#ifndef __BL_Rebalance__RebalanceProcessFftObjComp4__
#define __BL_Rebalance__RebalanceProcessFftObjComp4__

#include "IPlug_include_in_plug_hdr.h"

#include "../../WDL/fft.h"

#include <bl_queue.h>
#include <FftProcessObj16.h>

// RebalanceProcessFftObj
//
// Apply the masks to the current channels
//
// RebalanceProcessFftObjComp: from RebalanceProcessFftObj
// - use RebalanceMaskPredictorComp
//
// RebalanceProcessFftObjComp2:
// for RebalanceMaskPredictorComp3
//
// RebalanceProcessFftObjComp3: for ResampProcessObj
// (define target sample rate, more corret for any source sample rate,
// more correct than just downsacaling the mask)

//class RebalanceMaskPredictorComp7;
//class SoftMaskingNComp;
class RebalanceMaskPredictor8;
class RebalanceMaskProcessor;
class Scale;
class SoftMaskingComp4;
class BLSpectrogram4;
class SpectrogramDisplayScroll4;
class RebalanceProcessFftObjComp4 : public ProcessObj
{
public:
    RebalanceProcessFftObjComp4(int bufferSize, int oversampling,
                                BL_FLOAT sampleRate,
                                RebalanceMaskPredictor8 *maskPred,
                                int numInputCols,
                                int softMaskHistoSize);
    
    virtual ~RebalanceProcessFftObjComp4();
    
    void Reset(int bufferSize, int oversampling, int freqRes, BL_FLOAT sampleRate);
    void Reset();

    BLSpectrogram4 *GetSpectrogram();
    void SetSpectrogramDisplay(SpectrogramDisplayScroll4 *spectroDisplay);
    
    virtual void ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                                  const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer);
    
    void SetVocal(BL_FLOAT vocal);
    void SetBass(BL_FLOAT bass);
    void SetDrums(BL_FLOAT drums);
    void SetOther(BL_FLOAT other);

    void SetVocalSensitivity(BL_FLOAT vocal);
    void SetBassSensitivity(BL_FLOAT bass);
    void SetDrumsSensitivity(BL_FLOAT drums);
    void SetOtherSensitivity(BL_FLOAT other);
    
    // Global precision (previous soft/hard)
    void SetContrast(BL_FLOAT contrast);

    int GetLatency();

    void RecomputeSpectrogram(bool recomputeMasks = false);
    
protected:
    void AddSpectrogramLine(const WDL_TypedBuf<BL_FLOAT> &magns,
                            const WDL_TypedBuf<BL_FLOAT> &phases);
    
    void ResetSamplesHistory();
    void ResetMixColsComp();
    void ResetRawRawHistory();
    
    void ApplyMask(const WDL_TypedBuf<WDL_FFT_COMPLEX> &inData,
                   WDL_TypedBuf<WDL_FFT_COMPLEX> *outData,
                   const WDL_TypedBuf<BL_FLOAT> &masks);

    void ApplySoftMasking(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioData,
                          const WDL_TypedBuf<BL_FLOAT> &mask);
    
    void ComputeInverseDB(WDL_TypedBuf<BL_FLOAT> *magns);

    void ComputeResult(const WDL_TypedBuf<WDL_FFT_COMPLEX> &mixBuffer,
                       const WDL_TypedBuf<BL_FLOAT> masks[NUM_STEM_SOURCES],
                       WDL_TypedBuf<WDL_FFT_COMPLEX> *result,
                       WDL_TypedBuf<BL_FLOAT> *resMagns,
                       WDL_TypedBuf<BL_FLOAT> *resPhases);

    int ComputeSpectroNumCols();
    
    //
    BL_FLOAT mSampleRate;
    
    BLSpectrogram4 *mSpectrogram;
    SpectrogramDisplayScroll4 *mSpectroDisplay;
    
    //
    RebalanceMaskPredictor8 *mMaskPred;
    RebalanceMaskProcessor *mMaskProcessor;
    
    int mNumInputCols;
    
    // Keep the history of input data
    // So we can get exactly the same corresponding the the
    // correct location of the mask
    //deque<WDL_TypedBuf<WDL_FFT_COMPLEX> > mSamplesHistory;
    bl_queue<WDL_TypedBuf<WDL_FFT_COMPLEX> > mSamplesHistory;
    
    SoftMaskingComp4 *mSoftMasking;
    
    bl_queue<WDL_TypedBuf<WDL_FFT_COMPLEX> > mMixColsComp;

    Scale *mScale;

    // Keep masks history, so when chaging parameters, all the spectrogram changes
    bl_queue<WDL_TypedBuf<BL_FLOAT> > mMasksHistory[NUM_STEM_SOURCES];
    bl_queue<WDL_TypedBuf<WDL_FFT_COMPLEX> > mSignalHistory;

    // For recomputing spectrogram when also mask changes
    bl_queue<WDL_TypedBuf<WDL_FFT_COMPLEX> > mRawSignalHistory;
    
private:
    // Tmp buffers
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf0;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf1;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf2;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf3[NUM_STEM_SOURCES];
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf4;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf5;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf6;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf7;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf8;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf9;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf10;
    vector<WDL_TypedBuf<BL_FLOAT> > mTmpBuf11;
    vector<WDL_TypedBuf<BL_FLOAT> > mTmpBuf12;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf13;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf14;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf15;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf16;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf17;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf18[NUM_STEM_SOURCES];
    WDL_TypedBuf<BL_FLOAT> mTmpBuf19;
    vector<WDL_TypedBuf<WDL_FFT_COMPLEX> *> mTmpBuf20;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf21;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf22;
    bl_queue<WDL_TypedBuf<WDL_FFT_COMPLEX> > mTmpBuf23;
};

#endif /* defined(__BL_Rebalance__RebalanceProcessFftObjComp4__) */
