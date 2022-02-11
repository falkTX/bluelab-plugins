//
//  RebalanceMaskPredictorComp3.h
//  BL-Rebalance
//
//  Created by applematuer on 5/17/20.
//
//

#ifndef __BL_Rebalance__RebalanceMaskPredictorComp3__
#define __BL_Rebalance__RebalanceMaskPredictorComp3__

#include <deque>
using namespace std;

#include <FftProcessObj16.h>

#include <DNNModelMc.h>

// Include for defines
#include <Rebalance_defs.h>

#if FORCE_SAMPLE_RATE
#include "../../WDL/resample.h"
#endif

#include "../../WDL/fft.h"


//MaskPredictor
//
// Predict a single mask even with several channels
// (1 mask for 2 stereo channels)
//
// RebalanceMaskPredictorComp: from RebalanceMaskPredictor
// - use complex number masks
//
// RebalanceMaskPredictorComp2: for darknet models
// RebalanceMaskPredictorComp3: for darknet multichannel models (4 masks at once)
//
class SoftMaskingNComp;
class RebalanceMaskPredictorComp3 : public MultichannelProcess
{
public:
    RebalanceMaskPredictorComp3(int bufferSize,
                               BL_FLOAT overlapping, BL_FLOAT oversampling,
                               BL_FLOAT sampleRate,
                               IGraphics *graphics);
    
    virtual ~RebalanceMaskPredictorComp3();
    
    void Reset();
    
    void Reset(int bufferSize, int overlapping, int oversampling, BL_FLOAT sampleRate);
    
#if FORCE_SAMPLE_RATE
    void ProcessInputSamplesPre(vector<WDL_TypedBuf<BL_FLOAT> * > *ioSamples,
                                const vector<WDL_TypedBuf<BL_FLOAT> > *scBuffer);
#endif
    
    void ProcessInputFft(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > *ioFftSamples,
                         const vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > *scBuffer);
    
    // Get the masks
    void GetMaskVocal(WDL_TypedBuf<WDL_FFT_COMPLEX> *maskVocal);
    void GetMaskBass(WDL_TypedBuf<WDL_FFT_COMPLEX> *maskBass);
    void GetMaskDrums(WDL_TypedBuf<WDL_FFT_COMPLEX> *maskDrums);
    void GetMaskOther(WDL_TypedBuf<WDL_FFT_COMPLEX> *maskOther);
    
    static void Downsample(WDL_TypedBuf<BL_FLOAT> *ioBuf, BL_FLOAT sampleRate);
    
    static void Upsample(WDL_TypedBuf<BL_FLOAT> *ioBuf, BL_FLOAT sampleRate);
    
#if FORCE_SAMPLE_RATE
    void SetPlugSampleRate(BL_FLOAT sampleRate);
#endif
    
#if USE_SOFT_MASK_N
    void SetUseSoftMasks(bool flag);
#endif
    
    void SetModelNum(int modelNum);
    
    void AddInputSamples(const WDL_TypedBuf<WDL_FFT_COMPLEX> &samples);
    void GetCurrentSamples(WDL_TypedBuf<WDL_FFT_COMPLEX> *samples);
    
    static void ColumnsToBuffer(WDL_TypedBuf<BL_FLOAT> *buf,
                                const deque<WDL_TypedBuf<BL_FLOAT> > &cols);
    
    static void ColumnsToBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *buf,
                                const deque<WDL_TypedBuf<WDL_FFT_COMPLEX> > &cols);
    
protected:    
    static void UpsamplePredictedMask(WDL_TypedBuf<BL_FLOAT> *ioBuf,
                                      BL_FLOAT sampleRate);
    
    friend class RebalanceTestPredictObj;
    friend class RebalanceTestPredictObj2;
    friend class RebalanceProcessFftObjComp2;
    
    static void PredictMasks(vector<WDL_TypedBuf<BL_FLOAT> > *masks,
                             const WDL_TypedBuf<BL_FLOAT> &mixBufHisto,
                             DNNModelMc *model,
                             BL_FLOAT sampleRate);
    
    void ComputeLineMaskComp(WDL_TypedBuf<WDL_FFT_COMPLEX> *maskResult,
                             const WDL_TypedBuf<WDL_FFT_COMPLEX> &maskSource);
    
    void ComputeLineMaskComp2(WDL_TypedBuf<WDL_FFT_COMPLEX> *maskResult,
                              const WDL_TypedBuf<BL_FLOAT> &maskSource);
    
#if USE_SOFT_MASK_N
    void ComputeLineMask2(WDL_TypedBuf<BL_FLOAT> *maskResult,
                          const WDL_TypedBuf<BL_FLOAT> &maskSource);
    
    void ComputeLineMasksSoft(WDL_TypedBuf<WDL_FFT_COMPLEX> *maskVocal,
                              const WDL_TypedBuf<BL_FLOAT> &maskVocalFull,
                              WDL_TypedBuf<WDL_FFT_COMPLEX> *maskBass,
                              const WDL_TypedBuf<BL_FLOAT> &maskBassFull,
                              WDL_TypedBuf<WDL_FFT_COMPLEX> *maskDrums,
                              const WDL_TypedBuf<BL_FLOAT> &maskDrumsFull,
                              WDL_TypedBuf<WDL_FFT_COMPLEX> *maskOther,
                              const WDL_TypedBuf<BL_FLOAT> &maskOtherFull);
#endif
    
    void ComputeMasksComp(WDL_TypedBuf<WDL_FFT_COMPLEX> *maskVocal,
                          WDL_TypedBuf<WDL_FFT_COMPLEX> *maskBass,
                          WDL_TypedBuf<WDL_FFT_COMPLEX> *maskDrums,
                          WDL_TypedBuf<WDL_FFT_COMPLEX> *maskOther,
                          const WDL_TypedBuf<BL_FLOAT> &mixBufHisto);

    //
    void UpdateCurrentMasksAdd(const vector<WDL_TypedBuf<BL_FLOAT> > &newMasks);
    void UpdateCurrentMasksScroll();

    
    friend class RebalanceTestPredictObj;
    friend class RebalanceTestPredictObj2;
    friend class RebalanceTestPredictObj3;
    static void CreateModel(const char *modelFileName,
                            const char *resourcePath,
                            DNNModelMc **model);
    
    
#if FORCE_SAMPLE_RATE
    void InitResamplers();
#endif
    
    // If NUM_OUTPUT_COLS is greater than 1, we must
    // take all the column, and reduce them to only 1 colum
    static void ReduceMaskCols(WDL_TypedBuf<BL_FLOAT> *ioMask);

    
    int mBufferSize;
    BL_FLOAT mOverlapping;
    BL_FLOAT mOversampling;
    BL_FLOAT mSampleRate;
    
    // Masks
    WDL_TypedBuf<WDL_FFT_COMPLEX> mMaskVocal;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mMaskBass;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mMaskDrums;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mMaskOther;
    
    // DNNs
    int mModelNum;
    DNNModelMc *mModels[4];
    
    deque<WDL_TypedBuf<BL_FLOAT> > mMixCols;
    
    deque<WDL_TypedBuf<WDL_FFT_COMPLEX> > mSamplesHistory;
    
#if FORCE_SAMPLE_RATE
    WDL_Resampler mResamplers[2];
    
    BL_FLOAT mRemainingSamples[2];
    
    // In FftObj, we stay in 44100Hz
    // But we keep track of the result plugin frequency
    BL_FLOAT mPlugSampleRate;
#endif
    
#if USE_SOFT_MASK_N
    SoftMaskingNComp *mSoftMasking;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mCurrentData;
    bool mUseSoftMasks;
#endif
    
    // Do not compute masks at each step
    int mMaskPredictStepNum;
    vector<WDL_TypedBuf<BL_FLOAT> > mCurrentMasks;
};

#endif /* defined(__BL_Rebalance__RebalanceMaskPredictorComp3__) */
