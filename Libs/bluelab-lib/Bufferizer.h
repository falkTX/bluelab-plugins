#ifndef BUFFERIZER_H
#define BUFFERIZER_H

#include <BLTypes.h>

#include "IPlug_include_in_plug_hdr.h"

// Class Bufferizer, to fill the graph correctly whatever nFrames
class Bufferizer
{
public:
  Bufferizer(int bufferSize);
  
  virtual ~Bufferizer();

  void Reset();
    
  void AddValues(const WDL_TypedBuf<BL_FLOAT> &values);
  
  bool GetBuffer(WDL_TypedBuf<BL_FLOAT> *buffer);
  
protected:
  int mBufferSize;
  
  WDL_TypedBuf<BL_FLOAT> mValues;
};


#endif
