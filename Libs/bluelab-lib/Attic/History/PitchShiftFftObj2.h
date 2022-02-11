//
//  PitchShiftFftObj2.h
//  BL-PitchShift
//
//  Created by Pan on 16/04/18.
//
//

#ifndef __BL_PitchShift__PitchShiftFftObj2__
#define __BL_PitchShift__PitchShiftFftObj2__

#include "FftObj.h"
#include "FreqAdjustObj3.h"

class PitchShift;

class PitchShiftFftObj2 : public FftObj
{
public:
    PitchShiftFftObj2(int bufferSize, int oversampling, int freqRes,
                      BL_FLOAT sampleRate);
    
    virtual ~PitchShiftFftObj2();
    
    void ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                          const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer = NULL);
    
    void Reset(int oversampling, int freqRes);
    
    void Reset(int oversampling, int freqRes, BL_FLOAT sampleRate);
    
    void SetPitchFactor(BL_FLOAT factor);
    
#if 0 // EXPE (not used)
    void SavePhasesState();
    void RestorePhasesState();
#endif
    
protected:
    // Use Smb
    void Convert(WDL_TypedBuf<BL_FLOAT> *ioMagns, WDL_TypedBuf<BL_FLOAT> *ioPhases);
    
#if 0
    void FillMissingFreqs(WDL_TypedBuf<BL_FLOAT> *ioFreqs);
#endif
    
    BL_FLOAT mSampleRate;
    BL_FLOAT mFactor;
    
    // For smoothing
    BL_FLOAT mFactors[2];
    
    FreqAdjustObj3 mFreqObj;
    
#if 0 //EXPE
    FreqAdjustObj3 mSaveStateFreqObj;
#endif
};

#endif /* defined(__BL_PitchShift__PitchShiftFftObj2__) */
