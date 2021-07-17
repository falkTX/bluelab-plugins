#ifndef SPECTRAL_DIFF_OBJ_H
#define SPECTRAL_DIFF_OBJ_H

#include <FftProcessObj16.h>

#include "IPlug_include_in_plug_hdr.h"

#define NO_SOUND_OUTPUT 1

// SpectralDiffObj
class SpectralDiffObj : public ProcessObj
{
 public:
    SpectralDiffObj(int bufferSize, int oversampling, int freqRes,
                    BL_FLOAT sampleRate);
    
    virtual ~SpectralDiffObj();
    
    void ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                          const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer = NULL);
    
    bool GetSignal0BufferSpect(WDL_TypedBuf<BL_FLOAT> *ioBuffer);
    bool GetSignal1BufferSpect(WDL_TypedBuf<BL_FLOAT> *ioBuffer);
    bool GetDiffSpect(WDL_TypedBuf<BL_FLOAT> *ioBuffer);
    
 protected:
    WDL_TypedBuf<BL_FLOAT> mOutSignal0;
    WDL_TypedBuf<BL_FLOAT> mOutSignal1;
    WDL_TypedBuf<BL_FLOAT> mOutDiff;

    // Optim
    bool mCurveChanged[3];
    
private:
    // Tmp buffers
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf0;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf1;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf2;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf3;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf4;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf5;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf6;
};

#endif
