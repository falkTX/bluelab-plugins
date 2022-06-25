/* Copyright (C) 2022 Nicolas Dittlo <deadlab.plugins@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this software; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
 
#include <BLUtils.h>
#include <BLUtilsComp.h>

#include "SpectralDiffObj.h"


SpectralDiffObj::SpectralDiffObj(int bufferSize, int oversampling, int freqRes,
                                 BL_FLOAT sampleRate)
: ProcessObj(bufferSize)
{
    ProcessObj::Reset(bufferSize, oversampling, freqRes, sampleRate);

    mCurveChanged[0] = false;
    mCurveChanged[1] = false;
    mCurveChanged[2] = false;
}

SpectralDiffObj::~SpectralDiffObj() {}

#if 0 // OLD
void
SpectralDiffObj::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                                  const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer)
{
    // Signal
    WDL_TypedBuf<BL_FLOAT> signalMagns;
    WDL_TypedBuf<BL_FLOAT> signalPhases;
    BLUtils::ComplexToMagnPhase(&signalMagns, &signalPhases, *ioBuffer);
  
    // Side chain in input
    mOutSignal0 = signalMagns;
  
    // Sc signal
    WDL_TypedBuf<WDL_FFT_COMPLEX> scFftSamples;
    scFftSamples.Resize(mBufferSize);
  
    WDL_TypedBuf<BL_FLOAT> scMagns;
    WDL_TypedBuf<BL_FLOAT> scPhases;
    if ((scBuffer != NULL) && (scBuffer->GetSize() == mBufferSize))
    {
        // We have sidechain
        BLUtils::ComplexToMagnPhase(&scMagns, &scPhases, *scBuffer);
    
        mOutSignal1 = scMagns;
    }
  
    WDL_TypedBuf<BL_FLOAT> resultMagns;
  
    if (signalMagns.GetSize() >= mBufferSize/2)
    {
        resultMagns.Resize(mBufferSize/2);
        for (int i = 0; i < mBufferSize/2; i++)
        {
            BL_FLOAT signalMagn = signalMagns.Get()[i];
    
            BL_FLOAT newMagn = signalMagn;
            if (scMagns.GetSize() == mBufferSize)
                // We have sidechain
            {
                BL_FLOAT scMagn = scMagns.Get()[i];
    
                newMagn = scMagn - signalMagn;
                newMagn = fabs(newMagn);
      
                if (newMagn > 1.0)
                    newMagn = 1.0;
            }
    
            resultMagns.Get()[i] = newMagn;
        }
    }
  
    WDL_TypedBuf<BL_FLOAT> resultMagnsSym;
    BLUtils::MakeSymmetry(&resultMagnsSym, resultMagns);
  
    // Result
    mOutDiff = resultMagns;
  
#if !NO_SOUND_OUTPUT
    BLUtils::MagnPhaseToComplex(ioBuffer, resultMagnsSym, signalPhases);
#endif
}
#endif

// NEW
void
SpectralDiffObj::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                                  const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer)
{
    WDL_TypedBuf<WDL_FFT_COMPLEX> &halfBuffer = mTmpBuf0;
    
    //BLUtils::TakeHalf(&halfBuffer);
    BLUtils::TakeHalf(*ioBuffer, &halfBuffer);
    
    // Signal
    WDL_TypedBuf<BL_FLOAT> &signalMagns = mTmpBuf1;
    WDL_TypedBuf<BL_FLOAT> &signalPhases = mTmpBuf2;
    BLUtilsComp::ComplexToMagnPhase(&signalMagns, &signalPhases, halfBuffer);
    
    // Side chain in input
    mOutSignal0 = signalMagns;
    
    // Sc signal
    WDL_TypedBuf<WDL_FFT_COMPLEX> &halfScBuffer = mTmpBuf3;
    
    //BLUtils::TakeHalf(&halfScBuffer);
    if (scBuffer != NULL)
        BLUtils::TakeHalf(*scBuffer, &halfScBuffer);
    
    WDL_TypedBuf<BL_FLOAT> &scMagns = mTmpBuf4;
    WDL_TypedBuf<BL_FLOAT> &scPhases = mTmpBuf5;
    if (scBuffer != NULL)
    {
        // We have sidechain
        BLUtilsComp::ComplexToMagnPhase(&scMagns, &scPhases, halfScBuffer);
        
        mOutSignal1 = scMagns;
    }
    
    WDL_TypedBuf<BL_FLOAT> &resultMagns = mTmpBuf6;
    resultMagns.Resize(signalMagns.GetSize());

    for (int i = 0; i < resultMagns.GetSize(); i++)
    {
        BL_FLOAT signalMagn = signalMagns.Get()[i];
        BL_FLOAT newMagn = signalMagn;
        
        if (scMagns.GetSize() == resultMagns.GetSize())
        // We have sidechain
        {
            BL_FLOAT scMagn = scMagns.Get()[i];
                
            newMagn = scMagn - signalMagn;
            newMagn = fabs(newMagn);
                
            if (newMagn > 1.0)
                newMagn = 1.0;
        }
            
        resultMagns.Get()[i] = newMagn;
    }
    
    // Result
    mOutDiff = resultMagns;

    mCurveChanged[0] = true;
    mCurveChanged[1] = true;
    mCurveChanged[2] = true;
    
#if !NO_SOUND_OUTPUT
    BLUtils::MagnPhaseToComplex(ioBuffer, resultMagnsSym, signalPhases);
    BLUtils::FillSecondHalf(ioBuffer);
#endif
}

bool
SpectralDiffObj::GetSignal0BufferSpect(WDL_TypedBuf<BL_FLOAT> *ioBuffer)
{
    bool res = mCurveChanged[0];
    
    *ioBuffer = mOutSignal0;

    mCurveChanged[0] = false;

    return res;
}

bool
SpectralDiffObj::GetSignal1BufferSpect(WDL_TypedBuf<BL_FLOAT> *ioBuffer)
{
    bool res = mCurveChanged[1];
    
    *ioBuffer = mOutSignal1;

    mCurveChanged[1] = false;

    return res;
}

bool
SpectralDiffObj::GetDiffSpect(WDL_TypedBuf<BL_FLOAT> *ioBuffer)
{
    bool res = mCurveChanged[2];
    
    *ioBuffer = mOutDiff;

    mCurveChanged[2] = false;

    return res;
}
