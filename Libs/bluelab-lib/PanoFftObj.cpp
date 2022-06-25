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
//  PanoFftObj.cpp
//  BL-Pano
//
//  Created by Pan on 02/06/18.
//
//

#ifdef IGRAPHICS_NANOVG

#include <BLSpectrogram4.h>
#include <Window.h>
#include <SpectrogramDisplayScroll.h>

#include <BLUtils.h>
#include <BLUtilsComp.h>
#include <BLUtilsMath.h>

#include "PanoFftObj.h"

//
#define MIN_SMOOTH_DIVISOR 12.0
#define MAX_SMOOTH_DIVISOR 48.0


PanoFftObj::PanoFftObj(int bufferSize, int oversampling, int freqRes,
                      BL_FLOAT sampleRate)
: MultichannelProcess()
{
    mSpectrogram = new BLSpectrogram4(sampleRate, bufferSize/4, -1);
    mSpectroDisplay = NULL;
    
    MultichannelProcess::Reset(bufferSize, oversampling, freqRes, sampleRate);
    
    //
    mBufferSize = bufferSize;
    mOverlapping = oversampling;
    mFreqRes = freqRes;
    mSampleRate = sampleRate;
    
    //
    mLineCount = 0;
    
    mSharpness = 0.0;
}

PanoFftObj::~PanoFftObj()
{
    delete mSpectrogram;
}

void
PanoFftObj::ProcessInputFft(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > *ioFftSamples,
                            const vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > *scBuffer)
{
    if (ioFftSamples->size() != 2)
        return;
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> fftSamples[2];
    fftSamples[0] = *(*ioFftSamples)[0];
    fftSamples[1] = *(*ioFftSamples)[1];
    
    WDL_TypedBuf<BL_FLOAT> magns[2];
    WDL_TypedBuf<BL_FLOAT> phases[2];
    
    for (int i = 0; i < 2; i++)
    {
        BLUtils::TakeHalf(&fftSamples[i]);
        BLUtilsComp::ComplexToMagnPhase(&magns[i], &phases[i], *(*ioFftSamples)[i]);
    }
    
    WDL_TypedBuf<BL_FLOAT> panoLine;
    MagnsToPanoLine(magns, &panoLine);
    
    // Take left phases... (we don't need them)
    WDL_TypedBuf<BL_FLOAT> &phases0 = phases[0];
    AddSpectrogramLine(panoLine, phases0);
    
    mLineCount++;
}

void
PanoFftObj::Reset(int bufferSize, int oversampling, int freqRes, BL_FLOAT sampleRate)
{
    MultichannelProcess::Reset(bufferSize, oversampling, freqRes, sampleRate);
    
    mBufferSize = bufferSize;
    mOverlapping = oversampling;
    mFreqRes = freqRes;
    mSampleRate = sampleRate;
    
    // ORIGIN
    //int numCols = mBufferSize/8;
    
    // NEW
    // Prefer this, so the scroll speed won't be modified when
    // the overlapping changes
    int numCols = mBufferSize/(32/mOverlapping);
    
    // Adjust to the sample rate to avoid scrolling
    // 2 times faster when we go from 44100 to 88200
    //
    BL_FLOAT srCoeff = sampleRate/44100.0;
    srCoeff = bl_round(srCoeff);
    numCols *= srCoeff;
    
    mSpectrogram->Reset(mSampleRate, mBufferSize/4, numCols);
    
    mLineCount = 0;
    
    mOverlapLines.clear();
}

BLSpectrogram4 *
PanoFftObj::GetSpectrogram()
{
    return mSpectrogram;
}

void
PanoFftObj::SetSpectrogramDisplay(SpectrogramDisplayScroll *spectroDisplay)
{
    mSpectroDisplay = spectroDisplay;
}

void
PanoFftObj::SetSharpness(BL_FLOAT sharpness)
{
    mSharpness = sharpness;
    
    // Force re-creating the window
    mSmoothWin.Resize(0);
}

void
PanoFftObj::AddSpectrogramLine(const WDL_TypedBuf<BL_FLOAT> &magns,
                               const WDL_TypedBuf<BL_FLOAT> &phases)
{
    // Simple add
    if (mSpectroDisplay != NULL)
        mSpectroDisplay->AddSpectrogramLine(magns, phases);
    else
        mSpectrogram->AddLine(magns, phases);
}

void
PanoFftObj::MagnsToPanoLine(const WDL_TypedBuf<BL_FLOAT> magns[2],
                            WDL_TypedBuf<BL_FLOAT> *panoLine)
{
    panoLine->Resize(magns[0].GetSize());
    BLUtils::FillAllZero(panoLine);
    
    BL_FLOAT hzPerBin = mSampleRate/mBufferSize;
    
    // Do not take 0Hz!
    for (int i = 1; i < magns[0].GetSize(); i++)
    {
        // Compute pan
        BL_FLOAT l = magns[0].Get()[i];
        BL_FLOAT r = magns[1].Get()[i];
        
        BL_FLOAT angle = std::atan2(r, l);
        
        BL_FLOAT panNorm = angle/M_PI;
        panNorm = (panNorm + 0.25);
        panNorm = 1.0 - panNorm;
        
        // With 2, display goes outside of view with extreme pans
#define SCALE_FACTOR 1.8 //2.0
        panNorm = (panNorm - 0.5)*SCALE_FACTOR + 0.5;
        
        //
        BL_FLOAT binNumF = panNorm*panoLine->GetSize();
        
        int binNum = bl_round(binNumF);
        
        BL_FLOAT magnVal = (l + r)*0.5;
        if ((binNum >= 0) && (binNum < panoLine->GetSize()))
            panoLine->Get()[binNum] += magnVal;
    }
    
#if 1 // Sharpness
    // Initial, leaks a lot
    int divisor = 12;
    divisor = (1.0 - mSharpness)*MIN_SMOOTH_DIVISOR + mSharpness*MAX_SMOOTH_DIVISOR;
    if (divisor <= 0)
        divisor = 1;
    if (divisor > panoLine->GetSize())
        divisor = panoLine->GetSize();
    
    // Smooth the Pano line
    //if (mSmoothWin.GetSize() == 0)
    if (mSmoothWin.GetSize() != panoLine->GetSize()/divisor)
    {
        Window::MakeHanning(panoLine->GetSize()/divisor, &mSmoothWin);
    }
    
    WDL_TypedBuf<BL_FLOAT> smoothLine;
    BLUtils::SmoothDataWin(&smoothLine, *panoLine, mSmoothWin);
#endif
    
    *panoLine = smoothLine;
}

#endif // IGRAPHICS_NANOVG
