//
//  RebalanceMaskPredictorComp7.h
//  BL-Rebalance
//
//  Created by applematuer on 5/17/20.
//
//

#ifndef __BL_Rebalance__RebalanceMaskPredictorComp7__
#define __BL_Rebalance__RebalanceMaskPredictorComp7__

#include <deque>
using namespace std;

#include <FftProcessObj16.h>

//#include <DNNModelMc.h>
#include <DNNModel2.h>

// Include for defines
#include <Rebalance_defs.h>

#include "IPlug_include_in_plug_hdr.h"

#if FORCE_SAMPLE_RATE
#include "../../WDL/resample.h"
#endif

#include "../../WDL/fft.h"

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
class MelScale;
class RebalanceMaskPredictorComp7 : public MultichannelProcess
{
public:
    RebalanceMaskPredictorComp7(int bufferSize,
                                BL_FLOAT overlapping,
                                BL_FLOAT sampleRate,
                                int numSpectroCols,
                                const IPluginBase &plug,
                                IGraphics& graphics);
    
    virtual ~RebalanceMaskPredictorComp7();
    
    void Reset();
    
    void Reset(int bufferSize, int overlapping, int oversampling, BL_FLOAT sampleRate);
    
    void ProcessInputFft(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > *ioFftSamples,
                         const vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > *scBuffer);
    
    // Get the masks
    bool IsMaskAvailable();
    void GetMask(int index, WDL_TypedBuf<BL_FLOAT> *mask);
    
    //
    void SetPredictModuloNum(int moduloNum);
    
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
    
    // DEBUG
    //void SetDbgThreshold(BL_FLOAT thrs);
    
protected:
    void ComputeLineMask(WDL_TypedBuf<BL_FLOAT> *maskResult,
                         const WDL_TypedBuf<BL_FLOAT> &maskSource,
                         int numFreqs);
    
    void ComputeLineMasks(WDL_TypedBuf<BL_FLOAT> masksResult[NUM_STEM_SOURCES],
                          const WDL_TypedBuf<BL_FLOAT> masksSource[NUM_STEM_SOURCES],
                          int numFreqs);
    
    void ComputeMasks(WDL_TypedBuf<BL_FLOAT> masks[NUM_STEM_SOURCES],
                      const WDL_TypedBuf<BL_FLOAT> &mixBufHisto);

    //
    void UpdateCurrentMasksAdd(const vector<WDL_TypedBuf<BL_FLOAT> > &newMasks);
    void UpdateCurrentMasksScroll();

    //
    
    // Sensivity
    void ApplySensitivityHard(BL_FLOAT masks[NUM_STEM_SOURCES]);
    void ApplySensitivitySoft(BL_FLOAT masks[NUM_STEM_SOURCES]);
    void ApplySensitivity(WDL_TypedBuf<BL_FLOAT> masks[NUM_STEM_SOURCES]);
    
    //
    
    void NormalizeMasks(WDL_TypedBuf<BL_FLOAT> masks[NUM_STEM_SOURCES]);
    
    void ApplyMasksContrast(WDL_TypedBuf<BL_FLOAT> masks[NUM_STEM_SOURCES]);
    
    //
    static void CreateModel(const char *modelFileName,
                            const char *resourcePath,
                            DNNModel2 **model);
                            //DNNModelMc **model);
    
    void InitMixCols();

    void DownsampleHzToMel(WDL_TypedBuf<BL_FLOAT> *ioMagns);
    void UpdsampleMelToHz(WDL_TypedBuf<BL_FLOAT> *ioMagns);
    
    //
    int mBufferSize;
    BL_FLOAT mOverlapping;
    BL_FLOAT mSampleRate;
    
    // Masks: vocal, bass, drums, other
    WDL_TypedBuf<BL_FLOAT> mMasks[NUM_STEM_SOURCES];
    
    // DNNs
    //DNNModelMc *mModel;
    DNNModel2 *mModel;
    
    deque<WDL_TypedBuf<BL_FLOAT> > mMixCols;
    
    // Do not compute masks at each step
    int mMaskPredictStepNum;
    vector<WDL_TypedBuf<BL_FLOAT> > mCurrentMasks;
    
    // Parameters
    BL_FLOAT mSensitivities[NUM_STEM_SOURCES];
    
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
    
    RebalanceMaskStack2 *mMaskStacks[NUM_STEM_SOURCES];
    
    // Don't predict every mask, but re-use previous predictions and scroll
    bool mDontPredictEveryStep;
    int mPredictModulo;
    
    int mNumSpectroCols;
    
    //
    MelScale *mMelScale;
};

#endif /* defined(__BL_Rebalance__RebalanceMaskPredictorComp7__) */
