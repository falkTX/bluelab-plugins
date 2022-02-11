#include <FftProcessBufObj.h>

#include <BLUtils.h>
#include <BLUtilsComp.h>
#include <BLUtilsMath.h>
#include <BLUtilsFft.h>

#include <BLTypes.h>

#include "EQHackFftObj.h"

EQHackFftObj::EQHackFftObj(int bufferSize, int oversampling, int freqRes,
                           BL_FLOAT sampleRate,
                           FftProcessBufObj *eqSource)
: ProcessObj(bufferSize),
  mEQSource(eqSource),
  mMode(EQHackPluginInterface::LEARN)
{
    ProcessObj::Reset(bufferSize, oversampling, freqRes, sampleRate);
}

EQHackFftObj::~EQHackFftObj() {}

void
EQHackFftObj::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                               const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer)
{
    BLUtils::TakeHalf(ioBuffer);
  
    WDL_TypedBuf<WDL_FFT_COMPLEX> eqBuf;
    WDL_TypedBuf<WDL_FFT_COMPLEX> result;
  
    if (mEQSource != NULL)
    {
        mEQSource->GetComplexBuffer(&eqBuf);
    
        if (eqBuf.GetSize() != ioBuffer->GetSize())
            return;
    }
  
    result.Resize(ioBuffer->GetSize());
    mCurrentBuf.Resize(ioBuffer->GetSize());
  
    for (int i = 0; i < ioBuffer->GetSize(); i++)
    {
        WDL_FFT_COMPLEX signal = ioBuffer->Get()[i];
        BL_FLOAT sigMagn = COMP_MAGN(signal);
    
        WDL_FFT_COMPLEX res = signal;
    
        // Normalize the direction (to keep the "phase")
        if (sigMagn > 0.0)
        {
            res.re /= sigMagn;
            res.im /= sigMagn;
        }
    
        // Get the gain for each frequency
    
        // NOTE: this is important to have a default gain of 1.0
        // By gain, we mean signal ratios
        // Default value of 1 make the ratio curve stable when values are really small
        // i.e, when there is almost no signal, and we don't want to process the values.
        // This makes the curve stable aroud 1 on the graph !
        BL_FLOAT newMagn = 1.0;

        // WARNING: HARD CODED EPS THRESHOLD !!!
        //#define EPS 1e-16

        // Get the learn value
        BL_FLOAT learnValue = 1.0;
    
        if (mLearnCurve.GetSize()/**2*/ == ioBuffer->GetSize())
        {
            int index = i;
            if (i >= ioBuffer->GetSize()/*/2*/)
                index = ioBuffer->GetSize() - 1 - i;
      
            learnValue = mLearnCurve.Get()[index];
        }
    
        if ((mMode == EQHackPluginInterface::LEARN) ||
            (mMode == EQHackPluginInterface::GUESS))
        {
            WDL_FFT_COMPLEX eq = eqBuf.Get()[i];
    
            BL_FLOAT eqMagn = COMP_MAGN(eq);
    
            if (mMode == EQHackPluginInterface::LEARN)
            {
                if (sigMagn > BL_EPS)
                    newMagn = eqMagn/sigMagn;
            }
            else if (mMode == EQHackPluginInterface::GUESS)
            {
                if (eqMagn > BL_EPS)
                    newMagn = sigMagn/eqMagn;
          
                if (learnValue != 0.0)
                    newMagn *= learnValue;
            }
        }
        else if (mMode == EQHackPluginInterface::APPLY)
        {
            // Add the learn curve
            if (learnValue != 0.0)
                newMagn = sigMagn*learnValue;
        }
        else if (mMode == EQHackPluginInterface::APPLY_INV)
        {
            // Add the inverse learn curve
            if (learnValue != 0.0)
                newMagn = sigMagn/learnValue;
        }
    
        // Set the new magn to both "audio" and display current buffer
        if (newMagn < 0.0)
            newMagn = 0.0;
    
        res.re *= newMagn;
        res.im *= newMagn;
    
        result.Get()[i] = res;
        mCurrentBuf.Get()[i] = COMP_MAGN(res);
    }
  
    *ioBuffer = result;
  
    BLUtils::ResizeFillZeros(ioBuffer, ioBuffer->GetSize()*2);
    BLUtilsFft::FillSecondFftHalf(ioBuffer);
}

void
EQHackFftObj::GetBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer)
{
    *ioBuffer = mCurrentBuf;
}

void
EQHackFftObj::SetMode(EQHackPluginInterface::Mode mode)
{
    mMode = mode;
}

void
EQHackFftObj::SetLearnCurve(const WDL_TypedBuf<BL_FLOAT> *learnCurve)
{
    mLearnCurve = *learnCurve;
}
