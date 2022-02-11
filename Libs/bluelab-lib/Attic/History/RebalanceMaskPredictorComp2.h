//
//  RebalanceMaskPredictorComp2.h
//  BL-Rebalance
//
//  Created by applematuer on 5/17/20.
//
//

#ifndef __BL_Rebalance__RebalanceMaskPredictorComp2__
#define __BL_Rebalance__RebalanceMaskPredictorComp2__

#include <deque>
using namespace std;

#include <FftProcessObj16.h>

#include <DNNModel.h>

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
class SoftMaskingNComp;
class RebalanceMaskPredictorComp2 : public MultichannelProcess
{
public:
    RebalanceMaskPredictorComp2(int bufferSize,
                               BL_FLOAT overlapping, BL_FLOAT oversampling,
                               BL_FLOAT sampleRate,
                               IGraphics *graphics);
    
    virtual ~RebalanceMaskPredictorComp2();
    
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
    
    static void ColumnsToBuffer(WDL_TypedBuf<BL_FLOAT> *buf,
                                const deque<WDL_TypedBuf<BL_FLOAT> > &cols);
    
    static void ColumnsToBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *buf,
                                const deque<WDL_TypedBuf<WDL_FFT_COMPLEX> > &cols);
    
protected:    
    static void UpsamplePredictedMask(WDL_TypedBuf<BL_FLOAT> *ioBuf,
                                      BL_FLOAT sampleRate);
    
    friend class RebalanceTestPredictObj;
    friend class RebalanceTestPredictObj2;
    //static void PredictMask(WDL_TypedBuf<BL_FLOAT> *result,
    //                        WDL_TypedBuf<BL_FLOAT> *mask,
    //                        const WDL_TypedBuf<BL_FLOAT> &mixBufHisto,
    //                        DNNModel *model,
    //                        BL_FLOAT sampleRate);
    
    static void PredictMask(WDL_TypedBuf<BL_FLOAT> *mask,
                            const WDL_TypedBuf<BL_FLOAT> &mixBufHisto,
                            DNNModel *model,
                            BL_FLOAT sampleRate);
    
    //void ComputeLineMask(WDL_TypedBuf<BL_FLOAT> *maskResult,
    //                     const WDL_TypedBuf<BL_FLOAT> &maskSource);
    
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
    
    //void ComputeMasks(WDL_TypedBuf<BL_FLOAT> *maskVocal,
    //                  WDL_TypedBuf<BL_FLOAT> *maskBass,
    //                  WDL_TypedBuf<BL_FLOAT> *maskDrums,
    //                  WDL_TypedBuf<BL_FLOAT> *maskOther,
    //                  const WDL_TypedBuf<BL_FLOAT> &mixBufHisto);
    
    void ComputeMasksComp(WDL_TypedBuf<WDL_FFT_COMPLEX> *maskVocal,
                          WDL_TypedBuf<WDL_FFT_COMPLEX> *maskBass,
                          WDL_TypedBuf<WDL_FFT_COMPLEX> *maskDrums,
                          WDL_TypedBuf<WDL_FFT_COMPLEX> *maskOther,
                          const WDL_TypedBuf<BL_FLOAT> &mixBufHisto);

    friend class RebalanceTestPredictObj;
    friend class RebalanceTestPredictObj2;
    friend class RebalanceTestPredictObj3;
    static void CreateModel(const char *modelFileName,
                            const char *resourcePath,
                            DNNModel **model);
    
    
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
    DNNModel *mModelVocal;
    DNNModel *mModelBass;
    DNNModel *mModelDrums;
    DNNModel *mModelOther;
    
    deque<WDL_TypedBuf<BL_FLOAT> > mMixCols;
    
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
};

#endif /* defined(__BL_Rebalance__RebalanceMaskPredictorComp2__) */
