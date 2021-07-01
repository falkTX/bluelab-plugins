//
//  RebalanceProcessor2.hpp
//  BL-Rebalance-macOS
//
//  Created by applematuer on 10/14/20.
//
//

#ifndef RebalanceProcessor2_hpp
#define RebalanceProcessor2_hpp

#include <BLTypes.h>
#include <ResampProcessObj.h>
#include <Rebalance_defs.h>

// From RebalanceProcessor
// - manages spectrogram
class FftProcessObj16;
class RebalanceDumpFftObj2;
class RebalanceProcessFftObjComp4;
class RebalanceMaskPredictor8;
class RebalanceProcessor2 : public ResampProcessObj
{
public:
    RebalanceProcessor2(BL_FLOAT sampleRate, BL_FLOAT targetSampleRate,
                       int bufferSize, int targetBufferSize,
                       int overlapping,
                       int numSpectroCols);
    
    virtual ~RebalanceProcessor2();
    
    void InitDetect(const IPluginBase &plug,
                    IGraphics& graphics);
    
    void InitDump(int dumpOverlap);
    
    // Must be called at least once
    void Reset(BL_FLOAT sampleRate);
    void Reset(BL_FLOAT sampleRate, int blockSize);
    
    int GetLatency();
    
    // Predict
    //
    void SetVocal(BL_FLOAT vocal);
    void SetBass(BL_FLOAT bass);
    void SetDrums(BL_FLOAT drums);
    void SetOther(BL_FLOAT other);
    
    void SetMasksContrast(BL_FLOAT contrast);
    
    void SetVocalSensitivity(BL_FLOAT vocalSensitivity);
    void SetBassSensitivity(BL_FLOAT bassSensitivity);
    void SetDrumsSensitivity(BL_FLOAT drumsSensitivity);
    void SetOtherSensitivity(BL_FLOAT otherSensitivity);
    
    // Dump
    //
    bool HasEnoughDumpData();
    
    void GetSpectroData(WDL_TypedBuf<BL_FLOAT> data[REBALANCE_NUM_SPECTRO_COLS]);
    void GetStereoData(WDL_TypedBuf<BL_FLOAT> data[REBALANCE_NUM_SPECTRO_COLS]);

    BLSpectrogram4 *GetSpectrogram();
    void SetSpectrogramDisplay(SpectrogramDisplayScroll4 *spectroDisplay);
    
    // Debug
    void SetDbgThreshold(BL_FLOAT thrs);

    void RecomputeSpectrogram(bool recomputeMasks = false);

    void SetModelNum(int modelNum);

    // Default is 3, (i.e 32)
    // 0 works best (do predict every step)
    void SetPredictModuloNum(int modNum);
    
protected:
    bool
    ProcessSamplesBuffers(vector<WDL_TypedBuf<BL_FLOAT> > *ioBuffers,
                          vector<WDL_TypedBuf<BL_FLOAT> > *ioResampBuffers) override;
    
    //
    int mBufferSize;
    int mTargetBufferSize;
    int mOverlapping;
    
    int mBlockSize;
    
    int mNumSpectroCols;
    
    FftProcessObj16 *mNativeFftObj;
    FftProcessObj16 *mTargetFftObj;
    
    RebalanceDumpFftObj2 *mDumpObj;
    
    RebalanceMaskPredictor8 *mMaskPred;
    
    RebalanceProcessFftObjComp4 *mDetectProcessObjs[2];

private:
    // Tmp buffers
    vector<WDL_TypedBuf<BL_FLOAT> > mTmpBuf0;
    vector<WDL_TypedBuf<BL_FLOAT> > mTmpBuf1;
    vector<WDL_TypedBuf<BL_FLOAT> > mTmpBuf2;
    vector<WDL_TypedBuf<BL_FLOAT> > mTmpBuf3;
    vector<WDL_TypedBuf<BL_FLOAT> > mTmpBuf4;
    vector<WDL_TypedBuf<BL_FLOAT> > mTmpBuf5;
};

#endif /* RebalanceProcessor2_hpp */
