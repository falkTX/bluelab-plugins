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
