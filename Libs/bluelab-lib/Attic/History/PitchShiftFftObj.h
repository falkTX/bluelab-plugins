//
//  PitchShiftFftObj.h
//  BL-PitchShift
//
//  Created by Pan on 16/04/18.
//
//

#ifndef __BL_PitchShift__PitchShiftFftObj__
#define __BL_PitchShift__PitchShiftFftObj__

#include "FftProcessObj16.h"
#include "FreqAdjustObj3.h"
#include "FifoDecimator.h"

class PitchShift;

class PitchShiftFftObj : public FftProcessObj16
{
public:
    PitchShiftFftObj(int bufferSize, int oversampling, int freqRes,
                     BL_FLOAT sampleRate,
                     int maxNumPoints, BL_FLOAT decimFactor);
    
    virtual ~PitchShiftFftObj();
    
    void PreProcessSamplesBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer);
    
    void ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                          const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer = NULL);
    
    void PostProcessSamplesBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer);
    
    void GetInput(WDL_TypedBuf<BL_FLOAT> *outInput);
    
    void GetOutput(WDL_TypedBuf<BL_FLOAT> *outOutput);
    
    void Reset(int oversampling, int freqRes);
    
    //void ResetPhases();
    
    // For special uses
    //void SetPrevPhases(const WDL_TypedBuf<BL_FLOAT> &phases);
    
    // For special uses
    void SavePhasesState();
    void RestorePhasesState();
    
    void SetPitchFactor(BL_FLOAT factor);
    
    // DEBUG
    void SetPhase(BL_FLOAT phase);
    
    void SetShift(BL_FLOAT shift);
    
    void SetCorrectEnvelope1(bool flag);
    void SetEnvelopeAutoAlign(bool flag);
    
    void SetCorrectEnvelope2(bool flag);
    
    //
    void SetMagnsUseLinerp(bool flag);
    void SetMagnsFillHoles(bool flag);
    
    //
    void SetUseFreqAdjust(bool flag);
    void SetUseSoftFft1(bool flag);
    void SetUseSoftFft2(bool flag);
    
protected:
    void Convert(WDL_TypedBuf<BL_FLOAT> *ioMagns, WDL_TypedBuf<BL_FLOAT> *ioPhases);
    
    void SmbConvert(WDL_TypedBuf<BL_FLOAT> *ioMagns, WDL_TypedBuf<BL_FLOAT> *ioPhases);
    void SmbFillMissingFreqs(WDL_TypedBuf<BL_FLOAT> *ioFreqs);
    
    BL_FLOAT mSampleRate;
    
    BL_FLOAT mFactor;
    
    BL_FLOAT mPhase;
    BL_FLOAT mPhaseSum;
    WDL_TypedBuf<BL_FLOAT> mPhaseSums;
    
    BL_FLOAT mShift;
    BL_FLOAT mShiftSum;
    
    // For smoothing
    BL_FLOAT mFactors[2];
    int mFftBufferNum;
    
    WDL_TypedBuf<BL_FLOAT> mCurrentBuf;
    
    FreqAdjustObj3 mFreqObj;
    
    FreqAdjustObj3 mSaveStateFreqObj;
    
    // DEBUG
    bool mMagnsUseLinerp;
    bool mMagnsFillHoles;
    
    bool mUseFreqAdjust;
    bool mUseSoftFft1;
    bool mUseSoftFft2;
    
    FifoDecimator mInput;
    FifoDecimator mOutput;
    long mBufferCount;
    
private:
    WDL_TypedBuf<BL_FLOAT> mFreqVector;
    WDL_TypedBuf<BL_FLOAT> mWellTemperedIntervals;
    WDL_TypedBuf<BL_FLOAT> mShiftedInterval;
};

#endif /* defined(__BL_PitchShift__PitchShiftFftObj__) */
