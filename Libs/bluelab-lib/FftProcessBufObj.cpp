#include <BLUtils.h>
#include <BLUtilsComp.h>

#include "FftProcessBufObj.h"

FftProcessBufObj::FftProcessBufObj(int bufferSize, int oversampling, int freqRes,
                                   BL_FLOAT sampleRate)
: ProcessObj(bufferSize)
{
    ProcessObj::Reset(bufferSize, oversampling, freqRes, sampleRate);
}

FftProcessBufObj::~FftProcessBufObj() {}

void
FftProcessBufObj::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                                   const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer)
{
    mCurrentBuf = *ioBuffer;
  
    BLUtils::TakeHalf(&mCurrentBuf);
}

void
FftProcessBufObj::GetComplexBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer)
{
    *ioBuffer = mCurrentBuf;
}

void
FftProcessBufObj::GetBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer)
{
    ioBuffer->Resize(mCurrentBuf.GetSize());
    for (int i = 0; i < mCurrentBuf.GetSize(); i++)
        ioBuffer->Get()[i] = COMP_MAGN(mCurrentBuf.Get()[i]);
}
