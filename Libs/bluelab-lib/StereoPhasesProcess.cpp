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
//  StereoPhasesProcess.cpp
//  BL-PitchShift
//
//  Created by Pan on 20/04/18.
//
//

#include <BLUtils.h>
#include <BLUtilsComp.h>
#include <BLUtilsFft.h>

#include "StereoPhasesProcess.h"

StereoPhasesProcess::StereoPhasesProcess(int bufferSize)
: mPhasesDiff(bufferSize)
{
    mIsActive = true;
}

StereoPhasesProcess::~StereoPhasesProcess() {}

void
StereoPhasesProcess::Reset()
{
    mPhasesDiff.Reset();
}

void
StereoPhasesProcess::Reset(int bufferSize, int overlapping, int oversampling,
                           BL_FLOAT sampleRate)
{
    mPhasesDiff.Reset(bufferSize, overlapping, oversampling, sampleRate);
}

void
StereoPhasesProcess::SetActive(bool flag)
{
    mIsActive = flag;
}

void
StereoPhasesProcess::ProcessInputFft(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > *ioFftSamples,
                                     const vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > *scBuffer)
{
    if (!mIsActive)
        return;
    
    if (ioFftSamples->size() < 2)
        return;
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> fftSamples[2] = { *(*ioFftSamples)[0], *(*ioFftSamples)[1] };
    
    // Take half of the complexes
    BLUtils::TakeHalf(&fftSamples[0]);
    BLUtils::TakeHalf(&fftSamples[1]);
    
    WDL_TypedBuf<BL_FLOAT> magns[2];
    WDL_TypedBuf<BL_FLOAT> phases[2];
    
    BLUtilsComp::ComplexToMagnPhase(&magns[0], &phases[0], fftSamples[0]);
    BLUtilsComp::ComplexToMagnPhase(&magns[1], &phases[1], fftSamples[1]);
    
    mPhasesDiff.Capture(magns[0], phases[0], magns[1], phases[1]);
    
    // No need to convert back to complex
    // because we haven't made modification
    // ...
}

void
StereoPhasesProcess::ProcessResultFft(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > *ioFftSamples,
                                      const vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > *scBuffer)
{
    if (!mIsActive)
        return;
    
    if (ioFftSamples->size() < 2)
        return;
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> *fftSamples[2] =
                { (*ioFftSamples)[0], (*ioFftSamples)[1] };
    
    // Take half of the complexes
    BLUtils::TakeHalf(fftSamples[0]);
    BLUtils::TakeHalf(fftSamples[1]);
    
    // To magns and phases
    WDL_TypedBuf<BL_FLOAT> magns[2];
    WDL_TypedBuf<BL_FLOAT> phases[2];
    
    BLUtilsComp::ComplexToMagnPhase(&magns[0], &phases[0], *fftSamples[0]);
    BLUtilsComp::ComplexToMagnPhase(&magns[1], &phases[1], *fftSamples[1]);
    
    // Process
    mPhasesDiff.ApplyPhasesDiff(&phases[0], &phases[1]);
    
    BLUtilsComp::MagnPhaseToComplex(fftSamples[0], magns[0], phases[0]);
    BLUtilsComp::MagnPhaseToComplex(fftSamples[1], magns[1], phases[1]);
    
    // Fill second half
    fftSamples[0]->Resize(fftSamples[0]->GetSize()*2);
    BLUtilsFft::FillSecondFftHalf(fftSamples[0]);
    
    fftSamples[1]->Resize(fftSamples[1]->GetSize()*2);
    BLUtilsFft::FillSecondFftHalf(fftSamples[1]);
}
