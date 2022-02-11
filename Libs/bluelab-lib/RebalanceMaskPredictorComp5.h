//
//  RebalanceMaskPredictorComp5.h
//  BL-Rebalance
//
//  Created by applematuer on 5/17/20.
//
//

#ifndef __BL_Rebalance__RebalanceMaskPredictorComp5__
#define __BL_Rebalance__RebalanceMaskPredictorComp5__

#include <deque>
using namespace std;

#include <FftProcessObj16.h>

#include <DNNModelMc.h>

// Include for defines
#include <Rebalance_defs.h>

#include "IPlug_include_in_plug_hdr.h"

#if FORCE_SAMPLE_RATE
#include "../../WDL/resample.h"
#endif

#include "../../WDL/fft.h"

// 4 models, 1 model by quality
#define NUM_MODELS 1 //4

using namespace iplug;

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
// RebalanceMaskPredictorComp4: from RebalanceMaskPredictorComp3
// - code clean => removed unused methods
//
// RebalanceMaskPredictorComp5: for Leonardo Pepino method
//
class RebalanceMaskStack2;
class RebalanceMaskPredictorComp5 : public MultichannelProcess
{
public:
#if 0
    enum Quality
    {
        QUALITY_0_0 = 0, // Real-time, 63% CPU
        QUALITY_1_0,     // Real-time, 73% CPU
        QUALITY_1_1,     // Real-time, 87% CPU
        QUALITY_1_3,     // Offline, 8s
        QUALITY_2_3,     // Offline, bounce 23s
        QUALITY_3_3      // Offline, bounce 1mn20 (140s)
    };
#endif
    
    RebalanceMaskPredictorComp5(int bufferSize,
                               BL_FLOAT overlapping, BL_FLOAT oversampling,
                               BL_FLOAT sampleRate,
                               const IPluginBase &plug);
    
    virtual ~RebalanceMaskPredictorComp5();
    
    void Reset() override;
    
    void Reset(int bufferSize, int overlapping,
               int oversampling, BL_FLOAT sampleRate) override;
    
#if FORCE_SAMPLE_RATE
    void ProcessInputSamplesPre(vector<WDL_TypedBuf<BL_FLOAT> * > *ioSamples,
                                const vector<WDL_TypedBuf<BL_FLOAT> > *scBuffer);
#endif
    
    void ProcessInputFft(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > *ioFftSamples,
                         const vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > *scBuffer);
    
    // Get the masks
    void GetMask(int index, WDL_TypedBuf<BL_FLOAT> *mask);
    
    static void Downsample(WDL_TypedBuf<BL_FLOAT> *ioBuf, BL_FLOAT sampleRate);
    static void Upsample(WDL_TypedBuf<BL_FLOAT> *ioBuf, BL_FLOAT sampleRate);
    
#if FORCE_SAMPLE_RATE
    void SetPlugSampleRate(BL_FLOAT sampleRate);
#endif
    
    // NEW: since darknet
    void SetModelNum(int modelNum);
    
    //
    void SetPredictModuloNum(int moduloNum);
    
#if 0
    void SetQuality(Quality quality);
#endif
    
    static void ColumnsToBuffer(WDL_TypedBuf<BL_FLOAT> *buf,
                                const deque<WDL_TypedBuf<BL_FLOAT> > &cols);
    
    static void ColumnsToBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *buf,
                                const deque<WDL_TypedBuf<WDL_FFT_COMPLEX> > &cols);
    
    //
    
    void SetVocalSensitivity(BL_FLOAT vocalSensitivity);
    void SetBassSensitivity(BL_FLOAT bassSensitivity);
    void SetDrumsSensitivity(BL_FLOAT drumsSensitivity);
    void SetOtherSensitivity(BL_FLOAT otherSensitivity);
    
    int GetHistoryIndex();
    int GetLatency();
    
    // Masks contrast, relative one to each other (previous soft/hard)
    void SetMasksContrast(BL_FLOAT contrast);
    
protected:    
    static void UpsamplePredictedMask(WDL_TypedBuf<BL_FLOAT> *ioBuf,
                                      BL_FLOAT sampleRate, int numCols);
    
    void ComputeLineMask(WDL_TypedBuf<BL_FLOAT> *maskResult,
                         const WDL_TypedBuf<BL_FLOAT> &maskSource,
                         int numFreqs);
    
    void ComputeLineMasks(WDL_TypedBuf<BL_FLOAT> masksResult[4],
                          const WDL_TypedBuf<BL_FLOAT> masksSource[4],
                          int numFreqs);
    
    void ComputeMasks(WDL_TypedBuf<BL_FLOAT> masks[4],
                      const WDL_TypedBuf<BL_FLOAT> &mixBufHisto);

    //
    void UpdateCurrentMasksAdd(const vector<WDL_TypedBuf<BL_FLOAT> > &newMasks);
    void UpdateCurrentMasksScroll();

    //
    
    // Sensivity
    void ApplySensitivityHard(BL_FLOAT masks[4]);
    void ApplySensitivitySoft(BL_FLOAT masks[4]);
    void ApplySensitivity(WDL_TypedBuf<BL_FLOAT> masks[4]);
    
    //
    
    void NormalizeMasks(WDL_TypedBuf<BL_FLOAT> masks[4]);
    
    void ApplyMasksContrast(WDL_TypedBuf<BL_FLOAT> masks[4]);
    
    //
    friend class RebalanceTestPredictObj;
    friend class RebalanceTestPredictObj2;
    friend class RebalanceTestPredictObj3;
    static void CreateModel(const char *modelFileName,
                            const char *resourcePath,
                            DNNModelMc **model);
    
    
#if FORCE_SAMPLE_RATE
    void InitResamplers();
#endif
    
    void InitMixCols();

    //
    int mBufferSize;
    BL_FLOAT mOverlapping;
    BL_FLOAT mOversampling;
    BL_FLOAT mSampleRate;
    
    // Masks: vocal, bass, drums, other
    WDL_TypedBuf<BL_FLOAT> mMasks[4];
    
    // DNNs
    int mModelNum;
    DNNModelMc *mModels[NUM_MODELS];
    
    deque<WDL_TypedBuf<BL_FLOAT> > mMixCols;
    
#if FORCE_SAMPLE_RATE
    WDL_Resampler mResamplers[2];
    
    BL_FLOAT mRemainingSamples[2];
    
    // In FftObj, we stay in 44100Hz
    // But we keep track of the result plugin frequency
    BL_FLOAT mPlugSampleRate;
#endif
    
    // Do not compute masks at each step
    int mMaskPredictStepNum;
    vector<WDL_TypedBuf<BL_FLOAT> > mCurrentMasks;
    
    // Parameters
    BL_FLOAT mSensitivities[4];
    
    // Masks contract, relative one to the othersn
    struct MaskContrastStruct
    {
        int mMaskId;
        BL_FLOAT mValue;
        
        static bool ValueSmaller(const MaskContrastStruct m0, const MaskContrastStruct &m1)
        {
            return m0.mValue < m1.mValue;
        }
    };
    
    BL_FLOAT mMasksContrast;
    
    RebalanceMaskStack2 *mMaskStacks[4];
    
    // Don't predict every mask, but re-use previous predictions and scroll
    bool mDontPredictEveryStep;
    int mPredictModulo;
};

#endif /* defined(__BL_Rebalance__RebalanceMaskPredictorComp5__) */
