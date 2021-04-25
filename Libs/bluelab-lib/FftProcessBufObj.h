#ifndef FFT_PROCESS_BUF_OBJ_H
#define FFT_PROCESS_BUF_OBJ_H

#include <FftProcessObj16.h>

#include "IGraphics_include_in_plug_hdr.h"

class FftProcessBufObj : public ProcessObj
{
public:
  FftProcessBufObj(int bufferSize, int oversampling,
                   int freqRes, BL_FLOAT sampleRate);
  
  virtual ~FftProcessBufObj();
  
  void ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                        const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer = NULL);
  
  void GetComplexBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer);
  
  void GetBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer);
  
private:
  WDL_TypedBuf<WDL_FFT_COMPLEX> mCurrentBuf;
};

#endif
