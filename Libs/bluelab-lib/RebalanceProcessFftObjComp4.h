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

class RebalanceMaskPredictorComp7;
class SoftMaskingNComp;
class Scale;
class BLSpectrogram4;
class SpectrogramDisplayScroll3;
class RebalanceProcessFftObjComp4 : public ProcessObj
{
public:
    RebalanceProcessFftObjComp4(int bufferSize, BL_FLOAT sampleRate,
                                RebalanceMaskPredictorComp7 *maskPred,
                                int numInputCols,
                                int softMaskHistoSize);
    
    virtual ~RebalanceProcessFftObjComp4();
    
    void Reset(int bufferSize, int oversampling, int freqRes, BL_FLOAT sampleRate);
    void Reset();

    BLSpectrogram4 *GetSpectrogram();
    void SetSpectrogramDisplay(SpectrogramDisplayScroll3 *spectroDisplay);
    
    virtual void ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                                  const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer);
    
    //
    void SetMode(RebalanceMode mode);
    
    // Global precision (previous soft/hard)
    void SetMasksContrast(BL_FLOAT contrast);
    
    // Soft masks
    void SetUseSoftMasks(bool flag);
    
    void SetVocal(BL_FLOAT vocal);
    void SetBass(BL_FLOAT bass);
    void SetDrums(BL_FLOAT drums);
    void SetOther(BL_FLOAT other);
    
protected:
    void AddSpectrogramLine(const WDL_TypedBuf<BL_FLOAT> &magns,
                            const WDL_TypedBuf<BL_FLOAT> &phases);
    
    void ComputeMix(WDL_TypedBuf<WDL_FFT_COMPLEX> *dataResult,
                    const WDL_TypedBuf<WDL_FFT_COMPLEX> &dataMix);
    
    void ApplySoftMasks(WDL_TypedBuf<WDL_FFT_COMPLEX> masksResult[NUM_STEM_SOURCES],
                        const WDL_TypedBuf<BL_FLOAT> masksSource[NUM_STEM_SOURCES]);

    void CompDiv(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > *estim,
                 const WDL_TypedBuf<WDL_FFT_COMPLEX> &mix);
    
    //
    void ResetSamplesHistory();
    void ResetMixColsComp();
    
    // Mix
    void ApplyMix(WDL_TypedBuf<WDL_FFT_COMPLEX> masks[NUM_STEM_SOURCES]);
    void ApplyMix(WDL_FFT_COMPLEX masks[NUM_STEM_SOURCES]);
    
    void ApplyMask(const WDL_TypedBuf<WDL_FFT_COMPLEX> &inData,
                   WDL_TypedBuf<WDL_FFT_COMPLEX> *outData,
                   const WDL_TypedBuf<WDL_FFT_COMPLEX> masks[NUM_STEM_SOURCES]);
    
    // Post normalization
    void NormalizeMasks(WDL_TypedBuf<WDL_FFT_COMPLEX> masks[NUM_STEM_SOURCES]);
    void NormalizeMaskVals(WDL_FFT_COMPLEX maskVals[NUM_STEM_SOURCES]);

    //
    BL_FLOAT mSampleRate;
    
    BLSpectrogram4 *mSpectrogram;
    SpectrogramDisplayScroll3 *mSpectroDisplay;
    
    //
    RebalanceMaskPredictorComp7 *mMaskPred;
    
    RebalanceMode mMode;
    
    int mNumInputCols;
    
    // Keep the history of input data
    // So we can get exactly the same correponding the the correct location of the mask
    deque<WDL_TypedBuf<WDL_FFT_COMPLEX> > mSamplesHistory;
    
    // Soft masks
    bool mUseSoftMasks;
    
    SoftMaskingNComp *mSoftMasking;
    
    deque<WDL_TypedBuf<WDL_FFT_COMPLEX> > mMixColsComp;
    
    // Mix parameters
    BL_FLOAT mMixes[NUM_STEM_SOURCES];

    Scale *mScale;
};

#endif /* defined(__BL_Rebalance__RebalanceProcessFftObjComp4__) */
