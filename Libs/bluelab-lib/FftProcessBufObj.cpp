#include <BLUtils.h>
#include <BLUtilsComp.h>

#include "FftProcessBufObj.h"

FftProcessBufObj::FftProcessBufObj(int bufferSize, int oversampling, int freqRes,
                                   BL_FLOAT sampleRate)
: ProcessObj(bufferSize)
{
    ProcessObj::Reset(bufferSize, oversampling, freqRes, sampleRate);

    mMagnsBufValid = false;
}

FftProcessBufObj::~FftProcessBufObj() {}

void
FftProcessBufObj::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                                   const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer)
{
    //mCurrentBuf = *ioBuffer;
    //BLUtils::TakeHalf(&mCurrentBuf);

    BLUtils::TakeHalf(*ioBuffer, &mCurrentBuf);

    mMagnsBufValid = false;
}

void
FftProcessBufObj::GetComplexBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer)
{
    *ioBuffer = mCurrentBuf;
}

void
FftProcessBufObj::GetBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer)
{
    // Avoid recomputing magns if nothing changed
    // Useful e.g for block size = 32
    if (mMagnsBufValid)
    {
        *ioBuffer = mCurrentMagnsBuf;

        return;
    }
    
    ioBuffer->Resize(mCurrentBuf.GetSize());
    for (int i = 0; i < mCurrentBuf.GetSize(); i++)
        ioBuffer->Get()[i] = COMP_MAGN(mCurrentBuf.Get()[i]);

    // Update
    mCurrentMagnsBuf = *ioBuffer;
    mMagnsBufValid = true;
}
