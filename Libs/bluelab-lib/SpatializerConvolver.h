#ifndef SPATIALIZER_COVOLVER_H
#define SPATIALIZER_COVOLVER_H

#include <FftConvolver6.h>
#include <BufProcessObjSmooth2.h>

#include "IPlug_include_in_plug_hdr.h"

class SpatializerConvolver : public FftConvolver6,
    public BufProcessObj2
{
public:
  SpatializerConvolver(int bufferSize);
  
  virtual ~SpatializerConvolver() {}
  
  void Reset() override;
  
  void Reset(int oversampling, int freqRes);
  
  void SetBufferSize(int bufferSize) override;

  void Flush() override;
  
  void SetParameter(void *param) override;
  
  void *GetParameter() override;
  
  bool Process(BL_FLOAT *input, BL_FLOAT *output, int nFrames) override;
  
protected:
  WDL_TypedBuf<BL_FLOAT> *mResponse;
};

#endif
