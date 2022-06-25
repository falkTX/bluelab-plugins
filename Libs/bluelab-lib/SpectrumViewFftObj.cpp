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
 
//
//  SpectrumViewFftObj.cpp
//  BL-SpectrumView
//
//  Created by applematuer on 7/8/19.
//
//

#include <BLUtils.h>
#include <BLUtilsComp.h>

#include "SpectrumViewFftObj.h"


SpectrumViewFftObj::SpectrumViewFftObj(int bufferSize, int oversampling, int freqRes)
: ProcessObj(bufferSize) {}

SpectrumViewFftObj::~SpectrumViewFftObj() {}

void
SpectrumViewFftObj::Reset(int bufferSize, int oversampling,
                          int freqRes, BL_FLOAT sampleRate)
{
    ProcessObj::Reset(bufferSize, oversampling, freqRes, sampleRate);
    
    //
    mSignalBuf.Resize(0);
}

void
SpectrumViewFftObj::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer0,
                                     const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer)
{
    //BLUtils::TakeHalf(ioBuffer);
    WDL_TypedBuf<WDL_FFT_COMPLEX> &ioBuffer = mTmpBuf0;
    BLUtils::TakeHalf(*ioBuffer0, &ioBuffer);
    
    WDL_TypedBuf<BL_FLOAT> &sigMagns = mTmpBuf1;
    WDL_TypedBuf<BL_FLOAT> &phases = mTmpBuf2;
    BLUtilsComp::ComplexToMagnPhase(&sigMagns, &phases, ioBuffer);
    
    mSignalBuf = sigMagns;
}

void
SpectrumViewFftObj::GetSignalBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer)
{
    *ioBuffer = mSignalBuf;
}
