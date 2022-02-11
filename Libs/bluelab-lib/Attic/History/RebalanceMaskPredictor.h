//
//  RebalanceMaskPredictor.h
//  BL-Rebalance
//
//  Created by applematuer on 5/17/20.
//
//

#ifndef __BL_Rebalance__RebalanceMaskPredictor__
#define __BL_Rebalance__RebalanceMaskPredictor__

#include <deque>
using namespace std;

#include <FftProcessObj16.h>
#include <DNNModel.h>

// Include for defines
#include <Rebalance_defs.h>

#include "IPlug_include_in_plug_hdr.h"

#if FORCE_SAMPLE_RATE
#include "../../WDL/resample.h"
#endif

using namespace iplug::igraphics;

//MaskPredictor
//
// Predict a single mask even with several channels
// (1 mask for 2 stereo channels)
//
class SoftMaskingN;
class RebalanceMaskPredictor : public MultichannelProcess
{
public:
    RebalanceMaskPredictor(int bufferSize,
                           BL_FLOAT overlapping, BL_FLOAT oversampling,
                           BL_FLOAT sampleRate,
                           IGraphics *graphics);
    
    virtual ~RebalanceMaskPredictor();
    
    void Reset();
    
    void Reset(int bufferSize, int overlapping, int oversampling, BL_FLOAT sampleRate);
    
#if FORCE_SAMPLE_RATE
    void ProcessInputSamplesPre(vector<WDL_TypedBuf<BL_FLOAT> * > *ioSamples,
                                const vector<WDL_TypedBuf<BL_FLOAT> > *scBuffer);
#endif
    
    void ProcessInputFft(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > *ioFftSamples,
                         const vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > *scBuffer);
    
    // Get the masks
    void GetMaskVocal(WDL_TypedBuf<BL_FLOAT> *maskVocal);
    
    void GetMaskBass(WDL_TypedBuf<BL_FLOAT> *maskBass);
    
    void GetMaskDrums(WDL_TypedBuf<BL_FLOAT> *maskDrums);
    
    void GetMaskOther(WDL_TypedBuf<BL_FLOAT> *maskOther);
    
    
    static void Downsample(WDL_TypedBuf<BL_FLOAT> *ioBuf, BL_FLOAT sampleRate);
    
    static void Upsample(WDL_TypedBuf<BL_FLOAT> *ioBuf, BL_FLOAT sampleRate);
    
#if FORCE_SAMPLE_RATE
    void SetPlugSampleRate(BL_FLOAT sampleRate);
#endif
    
#if USE_SOFT_MASK_N
    void SetUseSoftMasks(bool flag);
#endif
    
protected:
#ifdef WIN32
    bool LoadModelWin(IGraphics *pGraphics, int rcId,
                      std::unique_ptr<pt::Model> *model);
#endif
    
    void UpsamplePredictedMask(WDL_TypedBuf<BL_FLOAT> *ioBuf);
    
    void PredictMask(WDL_TypedBuf<BL_FLOAT> *result,
                     const WDL_TypedBuf<BL_FLOAT> &mixBufHisto,
                     DNNModel *model);
    
    void ComputeLineMask(WDL_TypedBuf<BL_FLOAT> *maskResult,
                         const WDL_TypedBuf<BL_FLOAT> &maskSource);
    
#if USE_SOFT_MASK_N
    void ComputeLineMask2(WDL_TypedBuf<BL_FLOAT> *maskResult,
                          const WDL_TypedBuf<BL_FLOAT> &maskSource);
    
    void ComputeLineMasksSoft(WDL_TypedBuf<BL_FLOAT> *maskVocal,
                              const WDL_TypedBuf<BL_FLOAT> &maskVocalFull,
                              WDL_TypedBuf<BL_FLOAT> *maskBass,
                              const WDL_TypedBuf<BL_FLOAT> &maskBassFull,
                              WDL_TypedBuf<BL_FLOAT> *maskDrums,
                              const WDL_TypedBuf<BL_FLOAT> &maskDrumsFull,
                              WDL_TypedBuf<BL_FLOAT> *maskOther,
                              const WDL_TypedBuf<BL_FLOAT> &maskOtherFull);
#endif
    
    static void ColumnsToBuffer(WDL_TypedBuf<BL_FLOAT> *buf,
                                const deque<WDL_TypedBuf<BL_FLOAT> > &cols);
    
    void ComputeMasks(WDL_TypedBuf<BL_FLOAT> *maskVocal,
                      WDL_TypedBuf<BL_FLOAT> *maskBass,
                      WDL_TypedBuf<BL_FLOAT> *maskDrums,
                      WDL_TypedBuf<BL_FLOAT> *maskOther,
                      const WDL_TypedBuf<BL_FLOAT> &mixBufHisto);
    
#if FORCE_SAMPLE_RATE
    void InitResamplers();
#endif
    
    int mBufferSize;
    BL_FLOAT mOverlapping;
    BL_FLOAT mOversampling;
    BL_FLOAT mSampleRate;
    
    // Masks
    WDL_TypedBuf<BL_FLOAT> mMaskVocal;
    WDL_TypedBuf<BL_FLOAT> mMaskBass;
    WDL_TypedBuf<BL_FLOAT> mMaskDrums;
    WDL_TypedBuf<BL_FLOAT> mMaskOther;
    
    // DNNs
    std::unique_ptr<pt::Model> mModelVocal;
    std::unique_ptr<pt::Model> mModelBass;
    std::unique_ptr<pt::Model> mModelDrums;
    std::unique_ptr<pt::Model> mModelOther;
    
    deque<WDL_TypedBuf<BL_FLOAT> > mMixCols;
    
#if FORCE_SAMPLE_RATE
    WDL_Resampler mResamplers[2];
    
    BL_FLOAT mRemainingSamples[2];
    
    // In FftObj, we stay in 44100Hz
    // But we keep track of the result plugin frequency
    BL_FLOAT mPlugSampleRate;
#endif
    
#if USE_SOFT_MASK_N
    SoftMaskingN *mSoftMasking;
    WDL_TypedBuf<BL_FLOAT> mCurrentMagns;
    bool mUseSoftMasks;
#endif
};

#endif /* defined(__BL_Rebalance__RebalanceMaskPredictor__) */
