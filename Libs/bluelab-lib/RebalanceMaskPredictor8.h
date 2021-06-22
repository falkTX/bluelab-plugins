//
//  RebalanceMaskPredictor8.h
//  BL-Rebalance
//
//  Created by applematuer on 5/17/20.
//
//

#ifndef __BL_Rebalance__RebalanceMaskPredictor8__
#define __BL_Rebalance__RebalanceMaskPredictor8__

//#include <deque>
//using namespace std;
#include <bl_queue.h>

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
// RebalanceMaskPredictor8: from RebalanceMaskPredictorComp7
class RebalanceMaskStack2;
class MelScale;
class Scale;
class RebalanceMaskPredictor8 : public MultichannelProcess
{
public:
    RebalanceMaskPredictor8(int bufferSize,
                            BL_FLOAT overlapping,
                            BL_FLOAT sampleRate,
                            int numSpectroCols,
                            const IPluginBase &plug,
                            IGraphics& graphics);
    
    virtual ~RebalanceMaskPredictor8();
    
    void Reset();
    
    void Reset(int bufferSize, int overlapping,
               int oversampling, BL_FLOAT sampleRate);
    
    void ProcessInputFft(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > *ioFftSamples,
                         const vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > *scBuffer);
    
    // Get the masks
    bool IsMaskAvailable();
    void GetMask(int index, WDL_TypedBuf<BL_FLOAT> *mask);
    
    //
    void SetPredictModuloNum(int moduloNum);
    
    static void ColumnsToBuffer(WDL_TypedBuf<BL_FLOAT> *buf,
                                //const deque<WDL_TypedBuf<BL_FLOAT> > &cols);
                                const bl_queue<WDL_TypedBuf<BL_FLOAT> > &cols);
    
    //
    int GetHistoryIndex();
    int GetLatency();

    void SetModelNum(int modelNum);
    
protected:
    void ComputeMasks(WDL_TypedBuf<BL_FLOAT> masks[NUM_STEM_SOURCES],
                      const WDL_TypedBuf<BL_FLOAT> &mixBufHisto);
        
    void ComputeLineMask(WDL_TypedBuf<BL_FLOAT> *maskResult,
                         const WDL_TypedBuf<BL_FLOAT> &maskSource,
                         int numFreqs);
    
    void ComputeLineMasks(WDL_TypedBuf<BL_FLOAT> masksResult[NUM_STEM_SOURCES],
                          const WDL_TypedBuf<BL_FLOAT> masksSource[NUM_STEM_SOURCES],
                          int numFreqs);
    //
    void UpdateCurrentMasksAdd(const vector<WDL_TypedBuf<BL_FLOAT> > &newMasks);
    void UpdateCurrentMasksScroll();
    
    //
    static void CreateModel(const char *modelFileName,
                            const char *resourcePath,
                            DNNModel2 **model);
    
    void InitMixCols();

    void DownsampleHzToMel(WDL_TypedBuf<BL_FLOAT> *ioMagns);
    void UpsampleMelToHz(WDL_TypedBuf<BL_FLOAT> *ioMagns);
    
    //
    int mBufferSize;
    BL_FLOAT mOverlapping;
    BL_FLOAT mSampleRate;
    
    // Masks: vocal, bass, drums, other
    WDL_TypedBuf<BL_FLOAT> mMasks[NUM_STEM_SOURCES];
    
    // DNNs
    DNNModel2 *mModels[NUM_MODELS];
    int mModelNum;
    
    //deque<WDL_TypedBuf<BL_FLOAT> > mMixCols;
    bl_queue<WDL_TypedBuf<BL_FLOAT> > mMixCols;
    
    // Do not compute masks at each step
    int mMaskPredictStepNum;
    vector<WDL_TypedBuf<BL_FLOAT> > mCurrentMasks;
    
    RebalanceMaskStack2 *mMaskStacks[NUM_STEM_SOURCES];
    
    // Don't predict every mask, but re-use previous predictions and scroll
    bool mDontPredictEveryStep;
    int mPredictModulo;
    
    int mNumSpectroCols;
    
    // First filter method
    MelScale *mMelScale;

    // Second filter method
    Scale *mScale;

private:
    // Tmp buffers
    vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > mTmpBuf0;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf1;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf2;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf3;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf4;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf5[NUM_STEM_SOURCES];
    WDL_TypedBuf<BL_FLOAT> mTmpBuf6[NUM_STEM_SOURCES];
    vector<WDL_TypedBuf<BL_FLOAT> > mTmpBuf7;
    vector<WDL_TypedBuf<BL_FLOAT> > mTmpBuf8;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf9;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf10;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf11;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf12;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf13;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf14;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf15;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf16;
};

#endif /* defined(__BL_Rebalance__RebalanceMaskPredictor8__) */
