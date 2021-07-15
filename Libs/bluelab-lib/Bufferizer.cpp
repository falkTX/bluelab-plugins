#include <BLUtils.h>

#include "Bufferizer.h"

Bufferizer::Bufferizer(int bufferSize)
{
  mBufferSize = bufferSize;
}

Bufferizer::~Bufferizer() {}

void
Bufferizer::AddValues(const WDL_TypedBuf<BL_FLOAT> &values)
{
  mValues.Add(values.Get(), values.GetSize());
}

bool
Bufferizer::GetBuffer(WDL_TypedBuf<BL_FLOAT> *buffer)
{
  if (mValues.GetSize() < mBufferSize)
    return false;

  //buffer->Add(mValues.Get(), mBufferSize);
  BLUtils::CopyBuf(buffer, mValues.Get(), mBufferSize);
  
  BLUtils::ConsumeLeft(&mValues, mBufferSize);
  
  return true;
}
