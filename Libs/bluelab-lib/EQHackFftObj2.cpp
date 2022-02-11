#include <FftProcessBufObj.h>

#include <BLUtils.h>
#include <BLUtilsComp.h>
#include <BLUtilsMath.h>

#include <BLTypes.h>

#include "EQHackFftObj2.h"

EQHackFftObj2::EQHackFftObj2(int bufferSize, int oversampling,
                             int freqRes, BL_FLOAT sampleRate)
: ProcessObj(bufferSize)
  //mMode(EQHackPluginInterface::LEARN)
{
    ProcessObj::Reset(bufferSize, oversampling, freqRes, sampleRate);
}

EQHackFftObj2::~EQHackFftObj2() {}

void
EQHackFftObj2::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer0,
                                const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer0)
{
    mDiffBuffer.Resize(ioBuffer0->GetSize()/2);
    BLUtils::FillAllValue(&mDiffBuffer, (BL_FLOAT)1.0);
    
    if (scBuffer0 == NULL)
        return;
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> &ioBuffer = mTmpBuf0;
    BLUtils::TakeHalf(*ioBuffer0, &ioBuffer);

    WDL_TypedBuf<WDL_FFT_COMPLEX> &scBuffer = mTmpBuf1;
    BLUtils::TakeHalf(*scBuffer0, &scBuffer);

    for (int i = 0; i < ioBuffer.GetSize(); i++)
    {
        const WDL_FFT_COMPLEX &sig = ioBuffer.Get()[i];
        BL_FLOAT sigMagn = COMP_MAGN(sig);

        const WDL_FFT_COMPLEX &sc = scBuffer.Get()[i];
        BL_FLOAT scMagn = COMP_MAGN(sc);

        BL_FLOAT diff = 1.0;
        if (sigMagn > BL_EPS)
            diff = scMagn/sigMagn;

        mDiffBuffer.Get()[i] = diff;
    }
    
    // No need to resynth
}

void
EQHackFftObj2::GetDiffBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer)
{
    *ioBuffer = mDiffBuffer;
}

/*void
  EQHackFftObj2::SetMode(EQHackPluginInterface::Mode mode)
  {
  mMode = mode;
  }*/
