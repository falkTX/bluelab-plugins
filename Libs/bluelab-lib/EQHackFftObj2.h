#ifndef EQHACK_FFT_OBJ2_H
#define EQHACK_FFT_OBJ2_H

#include <FftProcessObj16.h>

#include <EQHackPluginInterface.h> // For Mode

#include "IGraphics_include_in_plug_hdr.h"

class FftProcessBufObj;
class EQHackFftObj2 : public ProcessObj
{
public:
    EQHackFftObj2(int bufferSize, int oversampling,
                  int freqRes, BL_FLOAT sampleRate);
  
    virtual ~EQHackFftObj2();
  
    void ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                          const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer = NULL);

    //void SetMode(EQHackPluginInterface::Mode mode);
    
    void GetDiffBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer);
  
protected:
    //EQHackPluginInterface::Mode mMode;
  
    WDL_TypedBuf<BL_FLOAT> mDiffBuffer;

private:
    // Tmp buffers
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf0;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf1;
};

#endif
