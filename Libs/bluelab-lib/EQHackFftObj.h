#ifndef EQHACK_FFT_OBJ_H
#define EQHACK_FFT_OBJ_H

#include <FftProcessObj16.h>

#include <EQHackPluginInterface.h> // For Mode

#include "IGraphics_include_in_plug_hdr.h"

class FftProcessBufObj;
class EQHackFftObj : public ProcessObj
{
public:
  EQHackFftObj(int bufferSize, int oversampling, int freqRes,
               BL_FLOAT sampleRate,
               FftProcessBufObj *eqSource);
  
  virtual ~EQHackFftObj();
  
  void ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                        const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer = NULL);

  void GetBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer);
  
  void SetMode(EQHackPluginInterface::Mode mode);
  
  void SetLearnCurve(const WDL_TypedBuf<BL_FLOAT> *learnCurve);
  
private:
  FftProcessBufObj *mEQSource;
  
  WDL_TypedBuf<BL_FLOAT> mCurrentBuf;
  
  EQHackPluginInterface::Mode mMode;
  
  WDL_TypedBuf<BL_FLOAT> mLearnCurve;
};

#endif
