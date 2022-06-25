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
//  StereoVizProcess3.cpp
//  BL-PitchShift
//
//  Created by Pan on 20/04/18.
//
//

#ifdef IGRAPHICS_NANOVG

#include <BLUtils.h>
#include <BLUtilsComp.h>

#include <DebugGraph.h>

#include <WavesRender.h>

#include "WavesProcess.h"


WavesProcess::WavesProcess(int bufferSize,
                           BL_FLOAT overlapping, BL_FLOAT oversampling,
                           BL_FLOAT sampleRate)
: ProcessObj(bufferSize)
{
    //mBufferSize = bufferSize;
    mOverlapping = overlapping;
    //mOversampling = oversampling;
    mFreqRes = oversampling;
    
    mSampleRate = sampleRate;
    
    mWavesRender = NULL;
}

WavesProcess::~WavesProcess() {}

void
WavesProcess::Reset()
{
    Reset(mBufferSize, mOverlapping, mFreqRes/*mOversampling*/, mSampleRate);
}

void
WavesProcess::Reset(int bufferSize, int overlapping, int oversampling,
                    BL_FLOAT sampleRate)
{
    mBufferSize = bufferSize;
    
    mOverlapping = overlapping;
    //mOversampling = oversampling;
    mFreqRes = oversampling;
    
    mSampleRate = sampleRate;
    
    mValues.Resize(0);
}

void
WavesProcess::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                               const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer)

{
    //WDL_TypedBuf<WDL_FFT_COMPLEX> fftSamples = *ioBuffer;
    WDL_TypedBuf<WDL_FFT_COMPLEX> &fftSamples = mTmpBuf0;
    
    // Take half of the complexes
    //BLUtils::TakeHalf(&fftSamples);
    BLUtils::TakeHalf(*ioBuffer, &fftSamples);
    
    WDL_TypedBuf<BL_FLOAT> &magns = mTmpBuf1;
    BLUtilsComp::ComplexToMagn(&magns, fftSamples);
    
    if (mWavesRender != NULL)
    {
        mWavesRender->AddMagns(magns);
    }
}

void
WavesProcess::SetWavesRender(WavesRender *wavesRender)
{
    mWavesRender = wavesRender;
}

#endif // IGRAPHICS_NANOVG
