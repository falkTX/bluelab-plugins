//
//  SpectrumViewFftObj.h
//  BL-SpectrumView
//
//  Created by applematuer on 7/8/19.
//
//

#ifndef __BL_SpectrumView__SpectrumViewFftObj__
#define __BL_SpectrumView__SpectrumViewFftObj__

#include <FftProcessObj16.h>

// SpectrumViewFftObj
class SpectrumViewFftObj : public ProcessObj
{
public:
    SpectrumViewFftObj(int bufferSize, int oversampling, int freqRes);
    
    virtual ~SpectrumViewFftObj();
    
    void Reset(int bufferSize, int oversampling,
               int freqRes, BL_FLOAT sampleRate) override;
    
    void
    ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                     const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer = NULL) override;
    
    void GetSignalBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer);
    
protected:
    WDL_TypedBuf<BL_FLOAT> mSignalBuf;

private:
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf0;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf1;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf2;
};

#endif /* defined(__BL_SpectrumView__SpectrumViewFftObj__) */
