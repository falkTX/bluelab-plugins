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
//  BatFftObj2.cpp
//  BL-Pano
//
//  Created by Pan on 02/06/18.
//
//

#ifdef IGRAPHICS_NANOVG

#include <BLSpectrogram4.h>
#include <Window.h>
#include <BLUtils.h>

#include <SpectrogramDisplay.h>
#include <HistoMaskLine2.h>

//#include <SourcePos.h>

#include <SourceLocalisationSystem2.h>

#include "BatFftObj2.h"

// Implementation of "Localization of multiple sound sources with two microphones" (Chen Liu)
// See: https://www.semanticscholar.org/paper/Localization-of-multiple-sound-sources-with-two-Liu-Wheeler/a06d41123e81e066c42a8565f7dff1689c6aa82a
//
// Commercial argument: "binaural signal-processing scheme employed in biological systems for the localization of sources"
// Niko: in other words, mimic the functioning of animals
//

// Sharpness
#define MIN_SHARPNESS_SMOOTH_DIVISOR 12.0
#define MAX_SHARPNESS_SMOOTH_DIVISOR 48.0

// Resolution of the spectrogram
#define SPECTRO_WIDTH 128
#define SPECTRO_HEIGHT 128

// Reverse coordinates ?
#define REVERSE_X 1
#define REVERSE_Y 1

#define USE_AMP_MAX 1

//
#define SOURCE_LOC_NUM_BANDS 65 //361 //15 //65 //361 //15 //361 //65 //15

#define LIMIT_BINS 0
#define SOURCE_LOC_MAX_BIN 256

#define TEST_SC_SAMPLES 0

#define TEST_HEIGHT 0


BatFftObj2::BatFftObj2(int bufferSize, int oversampling, int freqRes,
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
    mTimeSmooth = 0.0;
    
    mFftSmooth = 0.0;
    
    mAddLineCount = 0;
    
    mIsEnabled = true;
    
#if !LIMIT_BINS
    mSourceLocSystem =
            new SourceLocalisationSystem2(bufferSize, sampleRate,
                                          SOURCE_LOC_NUM_BANDS);
#else
    mSourceLocSystem =
    new SourceLocalisationSystem2(SOURCE_LOC_MAX_BIN*2, sampleRate,
                                 SOURCE_LOC_NUM_BANDS);
#endif
}

BatFftObj2::~BatFftObj2()
{
    delete mSpectrogram;
    
    delete mSourceLocSystem;
}

void
BatFftObj2::ProcessInputFft(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > *ioFftSamples,
                            const vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > *scBuffer)
{
    if (ioFftSamples->size() != 2)
        return;
    
    if (scBuffer->empty())
        return;
    
    if (!mIsEnabled)
        return;
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> fftSamples[2];
    
#if !TEST_SC_SAMPLES
    fftSamples[0] = *(*ioFftSamples)[0];
    fftSamples[1] = *(*ioFftSamples)[1];
#else
    fftSamples[0] = (*scBuffer)[0];
    fftSamples[1] = (*scBuffer)[1];
#endif
    
#if TEST_HEIGHT
    fftSamples[0] = *(*ioFftSamples)[0];
    fftSamples[1] = (*scBuffer)[1];
#endif
    
    for (int i = 0; i < 2; i++)
    {
        if (i >= ioFftSamples->size())
            continue;
        
        BLUtils::TakeHalf(&fftSamples[i]);
        
#if LIMIT_BINS
        fftSamples[0].Resize(SOURCE_LOC_MAX_BIN);
        fftSamples[1].Resize(SOURCE_LOC_MAX_BIN);
#endif
    }
    
    mSourceLocSystem->AddSamples(fftSamples);
    
    WDL_TypedBuf<BL_FLOAT> localizationX;
    mSourceLocSystem->GetLocalization(&localizationX);
    
    // Compute coords
    WDL_TypedBuf<BL_FLOAT> xCoords;
    ComputeCoords(localizationX, &xCoords);
    
    WDL_TypedBuf<BL_FLOAT> amps;
    ComputeAmplitudes(localizationX, &amps);
    
    // Dummy Y
    WDL_TypedBuf<BL_FLOAT> yCoords;
    BLUtils::ResizeFillValue(&yCoords, xCoords.GetSize(), (BL_FLOAT)0.5);
    
    // Fill spectrogram
    //
    
    // Clear
    mSpectrogram->Reset(mSampleRate);
    
    // Init lines
    vector<WDL_TypedBuf<BL_FLOAT> > lines;
    WDL_TypedBuf<BL_FLOAT> zeros;
    BLUtils::ResizeFillZeros(&zeros, SPECTRO_HEIGHT);
    for (int i = 0; i < SPECTRO_WIDTH; i++)
        lines.push_back(zeros);
    
    // Fill the data
    for (int i = 0; i < xCoords.GetSize(); i++)
    {
        BL_FLOAT x = xCoords.Get()[i];
        BL_FLOAT y = yCoords.Get()[i];
        BL_FLOAT amp = amps.Get()[i];
        
#if REVERSE_X
        // Adjust
        x = 1.0 - x;
#endif

#if REVERSE_Y
        // Adjust
        y = 1.0 - y;
#endif

        int xi = x*(SPECTRO_WIDTH - 1);
        int yi = y*(SPECTRO_HEIGHT - 1);
        
#if !USE_AMP_MAX
        lines[xi].Get()[yi] += amp;
#else
        if (amp > lines[xi].Get()[yi])
            lines[xi].Get()[yi] = amp;
#endif
}
    
    // Smooth
    TimeSmooth(&lines);
    
    // Sharpness
    ApplySharpnessXY(&lines);
    
    // Smooth
    //TimeSmooth(&lines);
               
    // Fill spectrogram with lines
    for (int i = 0; i < SPECTRO_WIDTH; i++)
        mSpectrogram->AddLine(lines[i], lines[i]);
    
    mLineCount++;
}

void
BatFftObj2::Reset(int bufferSize, int oversampling, int freqRes, BL_FLOAT sampleRate)
{
    MultichannelProcess::Reset(bufferSize, oversampling, freqRes, sampleRate);
    
    mBufferSize = bufferSize;
    mOverlapping = oversampling;
    mFreqRes = freqRes;
    mSampleRate = sampleRate;
    
    mSpectrogram->Reset(mSampleRate, SPECTRO_WIDTH, SPECTRO_HEIGHT);
    
    mLineCount = 0;
    
    //mOverlapLines.clear();
    
    mAddLineCount = 0;
    
#if !LIMIT_BINS
    mSourceLocSystem->Reset(bufferSize, sampleRate);
#else
    mSourceLocSystem->Reset(SOURCE_LOC_MAX_BIN*2, sampleRate);
#endif
}

BLSpectrogram4 *
BatFftObj2::GetSpectrogram()
{
    return mSpectrogram;
}

void
BatFftObj2::SetSpectrogramDisplay(SpectrogramDisplay *spectroDisplay)
{
    mSpectroDisplay = spectroDisplay;
}

void
BatFftObj2::SetSharpness(BL_FLOAT sharpness)
{
    mSharpness = sharpness;
    
    // Force re-creating the window
    mSmoothWin.Resize(0);
}

void
BatFftObj2::SetTimeSmooth(BL_FLOAT smooth)
{
    mTimeSmooth = smooth;
}

void
BatFftObj2::SetEnabled(bool flag)
{
    mIsEnabled = flag;
}

void
BatFftObj2::SetFftSmooth(BL_FLOAT smooth)
{
    mFftSmooth = smooth;
}

void
BatFftObj2::ComputeCoords(const WDL_TypedBuf<BL_FLOAT> &localization,
                          WDL_TypedBuf<BL_FLOAT> *coords)
{    
    coords->Resize(localization.GetSize());
   
    for (int i = 0; i < localization.GetSize(); i++)
    {
        BL_FLOAT t = ((BL_FLOAT)i)/(localization.GetSize() - 1);
        
        coords->Get()[i] = t;
    }
}

void
BatFftObj2::ComputeAmplitudes(const WDL_TypedBuf<BL_FLOAT> &localization,
                              WDL_TypedBuf<BL_FLOAT> *amps)
{
    *amps = localization;
    
    // TODO: normalize if necessary
}

void
BatFftObj2::ApplySharpnessXY(vector<WDL_TypedBuf<BL_FLOAT> > *lines)
{
    ApplySharpness(lines);
    
    ReverseXY(lines);
    
    ApplySharpness(lines);
    
    ReverseXY(lines);
}

void
BatFftObj2::ReverseXY(vector<WDL_TypedBuf<BL_FLOAT> > *lines)
{
    vector<WDL_TypedBuf<BL_FLOAT> > newLines = *lines;
    
    for (int i = 0; i < lines->size(); i++)
    {
        WDL_TypedBuf<BL_FLOAT> &line = (*lines)[i];
        
        for (int j = 0; j < line.GetSize(); j++)
        {
            BL_FLOAT val = line.Get()[j];
            
            newLines[j].Get()[i] = val;
        }
    }
    
    *lines = newLines;
}

void
BatFftObj2::ApplySharpness(vector<WDL_TypedBuf<BL_FLOAT> > *lines)
{
    for (int i = 0; i < lines->size(); i++)
    {
        WDL_TypedBuf<BL_FLOAT> &line = (*lines)[i];
        
        ApplySharpness(&line);
    }
}

void
BatFftObj2::ApplySharpness(WDL_TypedBuf<BL_FLOAT> *line)
{
    // Initial, leaks a lot
    int divisor = 12;
    divisor = (1.0 - mSharpness)*MIN_SHARPNESS_SMOOTH_DIVISOR +
                mSharpness*MAX_SHARPNESS_SMOOTH_DIVISOR;
    if (divisor <= 0)
        divisor = 1;
    if (divisor > line->GetSize())
        divisor = line->GetSize();
    
    // Smooth the Pano line
    if (mSmoothWin.GetSize() == 0)
    {
        Window::MakeHanning(line->GetSize()/divisor, &mSmoothWin);
    }
    
    WDL_TypedBuf<BL_FLOAT> smoothLine;
    BLUtils::SmoothDataWin(&smoothLine, *line, mSmoothWin);
    
    *line = smoothLine;
}

void
BatFftObj2::TimeSmooth(vector<WDL_TypedBuf<BL_FLOAT> > *lines)
{
    if (!mPrevLines.empty())
    {
        for (int i = 0; i < lines->size(); i++)
        {
            const WDL_TypedBuf<BL_FLOAT> &line = (*lines)[i];
            const WDL_TypedBuf<BL_FLOAT> &prevLine = mPrevLines[i];
        
            for (int j = 0; j < line.GetSize(); j++)
            {
                BL_FLOAT val = line.Get()[j];
                BL_FLOAT prevVal = prevLine.Get()[j];
                
                val = mTimeSmooth*prevVal + (1.0 - mTimeSmooth)*val;
                
                line.Get()[j] = val;
            }
        }
    }
    
    mPrevLines = *lines;
}

#endif
