//
//  PitchShiftFftObj3.h
//  BL-PitchShift
//
//  Created by Pan on 16/04/18.
//
//

#ifndef __BL_PitchShift__PitchShiftPrusaFftObj__
#define __BL_PitchShift__PitchShiftPrusaFftObj__

#include <FftProcessObj16.h>

class PitchShift;
class PitchShiftPrusaFftObj : public ProcessObj
{
public:
    PitchShiftPrusaFftObj(int bufferSize, int oversampling, int freqRes,
                          BL_FLOAT sampleRate);
    
    virtual ~PitchShiftPrusaFftObj();
    
    void ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                          const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer);
    
    void Reset(int bufferSize, int oversampling, int freqRes, BL_FLOAT sampleRate);
    
    void Reset();
    
    void SetFactor(BL_FLOAT factor);
    
protected:
    void Convert(WDL_TypedBuf<BL_FLOAT> *magns,
                 WDL_TypedBuf<BL_FLOAT> *phases,
                 BL_FLOAT factor);
    
    void ResetPitchShift();
        
    //
    BL_FLOAT mFactor;

private:
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf6;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf0;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf1;
};

#endif /* defined(__BL_PitchShift__PitchShiftPrusaFftObj__) */
