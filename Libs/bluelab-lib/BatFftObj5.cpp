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
//  BatFftObj5.cpp
//  BL-Pano
//
//  Created by Pan on 02/06/18.
//
//

#ifdef IGRAPHICS_NANOVG

#include <Window.h>
#include <BLUtils.h>

#include <BLSpectrogram4.h>
#include <BLImage.h>
#include <SpectrogramDisplay2.h>
#include <ImageDisplay2.h>

#include <HistoMaskLine2.h>

#include <SourceLocalisationSystem3.h>

#include "BatFftObj5.h"

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
#define SPECTRO_WIDTH 128 //512 //128
#define SPECTRO_HEIGHT 128 //512 //128

// Reverse coordinates ?
#define REVERSE_X 1
#define REVERSE_Y 0 //1

#define USE_AMP_MAX 1

// OPTIM: cans set num bands to 31 => optim x4 !
#define SOURCE_LOC_NUM_BANDS 65 //127 //65 //31  //361

// Only horizontal line ?
#define DEBUG_2D 0 //1 //0 //1

#define METHOD_2D 0 //1
#define METHOD_LEFT_RIGHT 0
#define METHOD_CROSS 1


BatFftObj5::BatFftObj5(int bufferSize, int oversampling, int freqRes,
                       BL_FLOAT sampleRate)
: MultichannelProcess()
{
    mSpectrogram = new BLSpectrogram4(sampleRate, bufferSize/4, -1);
    mSpectroDisplay = NULL;
    
    mImage = new BLImage(1, 1);
    mImageDisplay = NULL;
    
    MultichannelProcess::Reset(bufferSize, oversampling, freqRes, sampleRate);
    
    //
    mBufferSize = bufferSize;
    mOverlapping = oversampling;
    mFreqRes = freqRes;
    mSampleRate = sampleRate;
    
    //
    mLineCount = 0;
    
    mSharpness = 0.0;
    
    mTimeSmoothData = 0.0;
    
    mAddLineCount = 0;
    
    mIsEnabled = true;
    
#if METHOD_2D
    for (int i = 0; i < 2; i++)
    {
        mSourceLocSystems[i] =
            new SourceLocalisationSystem3(SourceLocalisationSystem3::PHASE_DIFF,
                                          bufferSize, sampleRate,
                                          SOURCE_LOC_NUM_BANDS);
    }
#endif
    
#if METHOD_LEFT_RIGHT
    for (int i = 0; i < 2; i++)
    {
        mSourceLocSystems[i] =
        new SourceLocalisationSystem3(SourceLocalisationSystem3::AMP_DIFF,
                                      bufferSize, sampleRate,
                                      SOURCE_LOC_NUM_BANDS);
    }
#endif
    
#if METHOD_CROSS
    mSourceLocSystems[0] =
            new SourceLocalisationSystem3(SourceLocalisationSystem3::PHASE_DIFF,
                                          bufferSize, sampleRate,
                                          SOURCE_LOC_NUM_BANDS);
    
    mSourceLocSystems[1] =
        new SourceLocalisationSystem3(SourceLocalisationSystem3::AMP_DIFF,
                                      bufferSize, sampleRate,
                                      SOURCE_LOC_NUM_BANDS);
#endif
}

BatFftObj5::~BatFftObj5()
{
    delete mSpectrogram;
    
    for (int i = 0; i < 2; i++)
    {
        delete mSourceLocSystems[i];
    }
    
    delete mImage;
}

void
BatFftObj5::ProcessInputFft(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > *ioFftSamples,
                            const vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > *scBuffer)
{
#if METHOD_2D
    Process2D(ioFftSamples, scBuffer);
#endif
    
#if METHOD_LEFT_RIGHT
    ProcessLeftRight(ioFftSamples, scBuffer);
#endif
    
#if METHOD_CROSS
    ProcessCross(ioFftSamples, scBuffer);
#endif
}

void
BatFftObj5::Reset(int bufferSize, int oversampling, int freqRes, BL_FLOAT sampleRate)
{
    MultichannelProcess::Reset(bufferSize, oversampling, freqRes, sampleRate);
    
    mBufferSize = bufferSize;
    mOverlapping = oversampling;
    mFreqRes = freqRes;
    mSampleRate = sampleRate;
    
    mSpectrogram->Reset(mSampleRate, SPECTRO_WIDTH, SPECTRO_HEIGHT);
    if (mSpectroDisplay != NULL)
        mSpectroDisplay->SetSpectrogram(mSpectrogram);
    
    mLineCount = 0;
    
    mAddLineCount = 0;
    
    for (int i = 0; i < 2; i++)
    {
        mSourceLocSystems[i]->Reset(bufferSize, sampleRate);
    }
}

BLSpectrogram4 *
BatFftObj5::GetSpectrogram()
{
    return mSpectrogram;
}

BLImage *
BatFftObj5::GetImage()
{
    return mImage;
}

void
BatFftObj5::SetSharpness(BL_FLOAT sharpness)
{
    mSharpness = sharpness;
    
    // Force re-creating the window
    mSmoothWin.Resize(0);
}

void
BatFftObj5::SetTimeSmoothData(BL_FLOAT smooth)
{
    mTimeSmoothData = smooth;
}

void
BatFftObj5::SetTimeSmooth(BL_FLOAT smooth)
{
    for (int i = 0; i < 2; i++)
        mSourceLocSystems[i]->SetTimeSmooth(smooth);
}

void
BatFftObj5::SetEnabled(bool flag)
{
    mIsEnabled = flag;
}

void
BatFftObj5::SetSpectrogramDisplay(SpectrogramDisplay2 *spectroDisplay)
{
    mSpectroDisplay = spectroDisplay;
}

void
BatFftObj5::SetImageDisplay(ImageDisplay2 *imageDisplay)
{
    mImageDisplay = imageDisplay;
}

void
BatFftObj5::SetIntermicCoeff(BL_FLOAT coeff)
{
    for (int i = 0; i < 2; i++)
    {
        mSourceLocSystems[i]->SetIntermicDistanceCoeff(coeff);
    }
}

void
BatFftObj5::ComputeCoords(const WDL_TypedBuf<BL_FLOAT> &localization,
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
BatFftObj5::ComputeCoords(int numBands, WDL_TypedBuf<BL_FLOAT> *coords)
{
    coords->Resize(numBands);
    
    for (int i = 0; i < numBands; i++)
    {
        BL_FLOAT t = ((BL_FLOAT)i)/(numBands - 1);
        
        coords->Get()[i] = t;
    }
}

void
BatFftObj5::ComputeCoords2D(int width, int height,
                            WDL_TypedBuf<BL_FLOAT> *coordsX,
                            WDL_TypedBuf<BL_FLOAT> *coordsY)
{
    coordsX->Resize(width*height);
    coordsY->Resize(width*height);
    
    for (int j = 0; j < height; j++)
    {
        BL_FLOAT ty = ((BL_FLOAT)j)/(height - 1);
        
        for (int i = 0; i < width; i++)
        {
            BL_FLOAT tx = ((BL_FLOAT)i)/(width - 1);
        
            coordsX->Get()[i + j*height] = tx;
            coordsY->Get()[i + j*height] = ty;
        }
    }
}

void
BatFftObj5::ComputeAmplitudes(const WDL_TypedBuf<BL_FLOAT> &localization,
                              WDL_TypedBuf<BL_FLOAT> *amps)
{
    *amps = localization;
    
    // TODO: normalize if necessary
}

void
BatFftObj5::ApplySharpnessXY(vector<WDL_TypedBuf<BL_FLOAT> > *lines)
{
    ApplySharpness(lines);
    
    ReverseXY(lines);
    
    ApplySharpness(lines);
    
    ReverseXY(lines);
}

void
BatFftObj5::ReverseXY(vector<WDL_TypedBuf<BL_FLOAT> > *lines)
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
BatFftObj5::ApplySharpness(vector<WDL_TypedBuf<BL_FLOAT> > *lines)
{
    for (int i = 0; i < lines->size(); i++)
    {
        WDL_TypedBuf<BL_FLOAT> &line = (*lines)[i];
        
        ApplySharpness(&line);
    }
}

void
BatFftObj5::ApplySharpness(WDL_TypedBuf<BL_FLOAT> *line)
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
BatFftObj5::TimeSmoothData(vector<WDL_TypedBuf<BL_FLOAT> > *lines)
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
                
                val = mTimeSmoothData*prevVal + (1.0 - mTimeSmoothData)*val;
                
                line.Get()[j] = val;
            }
        }
    }
    
    mPrevLines = *lines;
}

void
BatFftObj5::GetLocalization(int index, const WDL_TypedBuf<WDL_FFT_COMPLEX> &samples0,
                            const WDL_TypedBuf<WDL_FFT_COMPLEX> &samples1,
                            WDL_TypedBuf<BL_FLOAT> *localization)
{
    WDL_TypedBuf<WDL_FFT_COMPLEX> fftSamples[2];
    fftSamples[0] = samples0;
    fftSamples[1] = samples1;
    for (int i = 0; i < 2; i++)
    {
        BLUtils::TakeHalf(&fftSamples[i]);
    }
    
    mSourceLocSystems[index]->AddSamples(fftSamples);
    
    mSourceLocSystems[index]->GetLocalization(localization);
}

void
BatFftObj5::ComputeLocalization2D(const WDL_TypedBuf<BL_FLOAT> &localizationX,
                                  const WDL_TypedBuf<BL_FLOAT> &localizationY,
                                  WDL_TypedBuf<BL_FLOAT> *localization2D)
{
//#define LOC2D_FACTOR 0.5
#define LOC2D_FACTOR 0.1
    
    int width = localizationX.GetSize();
    int height = localizationY.GetSize();
    
    localization2D->Resize(width*height);
    
    for (int j = 0; j < height; j++)
    {
        BL_FLOAT valY = localizationY.Get()[j];
        
        for (int i = 0; i < width; i++)
        {
            BL_FLOAT valX = localizationX.Get()[i];
            
            //BL_FLOAT res = valX;
            BL_FLOAT res = valX*valY;
            
            res *= LOC2D_FACTOR;
            
            localization2D->Get()[i + j*width] = res;
        }
    }
}

void
BatFftObj5::Process2D(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > *ioFftSamples,
                      const vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > *scBuffer)
{
    if (ioFftSamples->size() != 2)
        return;
    
    if (scBuffer->empty())
        return;
    
    if (!mIsEnabled)
        return;
    
    int numBands = mSourceLocSystems[0]->GetNumBands();
    
    //2D
    //
    
    // X
    WDL_TypedBuf<BL_FLOAT> localizationX;
    GetLocalization(0, *(*ioFftSamples)[0], *(*ioFftSamples)[1], &localizationX);
    
    // Compute coords
    WDL_TypedBuf<BL_FLOAT> xCoords;
    ComputeCoords(localizationX, &xCoords);
    
    WDL_TypedBuf<BL_FLOAT> ampsX;
    ComputeAmplitudes(localizationX, &ampsX);
    
    // Debug
    int width;
    int height;
    WDL_TypedBuf<BL_FLOAT> data;
    mSourceLocSystems[0/*1*//*0*/]->DBG_GetCoincidence(&width, &height, &data);
    mImage->SetData(width, height, data);
    if (mImageDisplay != NULL)
        mImageDisplay->SetImage(mImage);
    
    // Y
    
    // Compute coords
    WDL_TypedBuf<BL_FLOAT> yCoords;
    ComputeCoords(numBands, &yCoords);
    
#if !DEBUG_2D
    WDL_TypedBuf<BL_FLOAT> localizationY;
    GetLocalization(1, (*scBuffer)[0], (*scBuffer)[1], &localizationY);
    
    WDL_TypedBuf<BL_FLOAT> ampsY;
    ComputeAmplitudes(localizationY, &ampsY);
#endif
    
    // Multiply amplitudes / lines
    //
    
    // Init lines
    vector<WDL_TypedBuf<BL_FLOAT> > lines;
    WDL_TypedBuf<BL_FLOAT> zeros;
    BLUtils::ResizeFillZeros(&zeros, SPECTRO_HEIGHT);
    for (int i = 0; i < SPECTRO_WIDTH; i++)
        lines.push_back(zeros);
    
    // Fill the data
    for (int j = 0; j < yCoords.GetSize(); j++)
    {
        for (int i = 0; i < xCoords.GetSize(); i++)
        {
            BL_FLOAT x = xCoords.Get()[i];
            BL_FLOAT y = yCoords.Get()[j];
            
            BL_FLOAT ampX = ampsX.Get()[i];
            
#if !DEBUG_2D
            // Normal code
            BL_FLOAT ampY = ampsY.Get()[j];
            BL_FLOAT amp = ampX*ampY;
            
            //BL_FLOAT amp = ampX + ampY;
#else
            // Horizontal line
            BL_FLOAT amp = ampX;
            y = 0.5;
#endif
            
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
    }
    
    // Fill spectrogram
    //
    
    // Clear
    mSpectrogram->Reset(mSampleRate);
    if (mSpectroDisplay != NULL)
        mSpectroDisplay->SetSpectrogram(mSpectrogram);
    
    // Smooth
    TimeSmoothData(&lines);
    
    // Sharpness
    ApplySharpnessXY(&lines);
    
    // Smooth
    //TimeSmooth(&lines);
    
    // Fill spectrogram with lines
    for (int i = 0; i < SPECTRO_WIDTH; i++)
        mSpectrogram->AddLine(lines[i], lines[i]);
    
    if (mSpectroDisplay != NULL)
        mSpectroDisplay->SetSpectrogram(mSpectrogram);
    
    mLineCount++;
}

void
BatFftObj5::ProcessLeftRight(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > *ioFftSamples,
                             const vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > *scBuffer)
{
    if (ioFftSamples->size() != 2)
        return;
    
    if (scBuffer->empty())
        return;
    
    if (!mIsEnabled)
        return;
    
    int numBands = mSourceLocSystems[0]->GetNumBands();
    
    //
    WDL_TypedBuf<WDL_FFT_COMPLEX> left = (*scBuffer)[0];
    BLUtils::AddValues(&left, (*scBuffer)[1]);
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> right = *(*ioFftSamples)[0];
    BLUtils::AddValues(&right, *(*ioFftSamples)[1]);
    
    // Output
    *(*ioFftSamples)[0] = left;
    *(*ioFftSamples)[1] = right;
    
    //2D
    //
    
    // X
    WDL_TypedBuf<BL_FLOAT> localizationX;
    GetLocalization(0, left, right, &localizationX);
    
    // Compute coords
    WDL_TypedBuf<BL_FLOAT> xCoords;
    ComputeCoords(localizationX, &xCoords);
    
    WDL_TypedBuf<BL_FLOAT> ampsX;
    ComputeAmplitudes(localizationX, &ampsX);
    
    // Debug
    int width;
    int height;
    WDL_TypedBuf<BL_FLOAT> data;
    mSourceLocSystems[0/*1*//*0*/]->DBG_GetCoincidence(&width, &height, &data);
    mImage->SetData(width, height, data);
    if (mImageDisplay != NULL)
        mImageDisplay->SetImage(mImage);
    
    // Y
    
    // Compute coords
    WDL_TypedBuf<BL_FLOAT> yCoords;
    ComputeCoords(numBands, &yCoords);
    
    // Multiply amplitudes / lines
    //
    
    // Init lines
    vector<WDL_TypedBuf<BL_FLOAT> > lines;
    WDL_TypedBuf<BL_FLOAT> zeros;
    BLUtils::ResizeFillZeros(&zeros, SPECTRO_HEIGHT);
    for (int i = 0; i < SPECTRO_WIDTH; i++)
        lines.push_back(zeros);
    
    // Fill the data
    for (int j = 0; j < yCoords.GetSize(); j++)
    {
        for (int i = 0; i < xCoords.GetSize(); i++)
        {
            BL_FLOAT x = xCoords.Get()[i];
            BL_FLOAT y = yCoords.Get()[j];
            
            BL_FLOAT ampX = ampsX.Get()[i];
            
// DEBUG_2D
            // Horizontal line
            BL_FLOAT amp = ampX;
            y = 0.5;
            
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
    }
    
    // Fill spectrogram
    //
    
    // Clear
    mSpectrogram->Reset(mSampleRate);
    if (mSpectroDisplay != NULL)
        mSpectroDisplay->SetSpectrogram(mSpectrogram);
    
    // Smooth
    TimeSmoothData(&lines);
    
    // Sharpness
    ApplySharpnessXY(&lines);
    
    // Smooth
    //TimeSmooth(&lines);
    
    // Fill spectrogram with lines
    for (int i = 0; i < SPECTRO_WIDTH; i++)
        mSpectrogram->AddLine(lines[i], lines[i]);
    if (mSpectroDisplay != NULL)
        mSpectroDisplay->SetSpectrogram(mSpectrogram);
    
    mLineCount++;
}

void
BatFftObj5::ProcessCross(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > *ioFftSamples,
                         const vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > *scBuffer)
{
    if (ioFftSamples->size() != 2)
        return;
    
    if (scBuffer->empty())
        return;
    
    if (!mIsEnabled)
        return;
    
    int numBands = mSourceLocSystems[0]->GetNumBands();
    
    // Left/Right
    WDL_TypedBuf<WDL_FFT_COMPLEX> left = (*scBuffer)[0];
    BLUtils::AddValues(&left, (*scBuffer)[1]);
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> right = *(*ioFftSamples)[0];
    BLUtils::AddValues(&right, *(*ioFftSamples)[1]);
    
    // Up/Down
    WDL_TypedBuf<WDL_FFT_COMPLEX> up = (*scBuffer)[0];
    BLUtils::AddValues(&up, *(*ioFftSamples)[0]);
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> down = (*scBuffer)[1];
    BLUtils::AddValues(&down, *(*ioFftSamples)[1]);
    
    // Output
    *(*ioFftSamples)[0] = left;
    *(*ioFftSamples)[1] = right;
    
    //2D
    //
    
    // X
    WDL_TypedBuf<BL_FLOAT> localizationX;
    GetLocalization(0, left, right, &localizationX);
    
    // Compute coords
    WDL_TypedBuf<BL_FLOAT> xCoords;
    ComputeCoords(localizationX, &xCoords);
    
    WDL_TypedBuf<BL_FLOAT> ampsX;
    ComputeAmplitudes(localizationX, &ampsX);
    
    // Debug
    int width;
    int height;
    WDL_TypedBuf<BL_FLOAT> data;
    mSourceLocSystems[0/*1*//*0*/]->DBG_GetCoincidence(&width, &height, &data);
    mImage->SetData(width, height, data);
    if (mImageDisplay != NULL)
        mImageDisplay->SetImage(mImage);
    
    // Y
    // X
    WDL_TypedBuf<BL_FLOAT> localizationY;
    GetLocalization(1, up, down, &localizationY);
    
    // Compute coords
    WDL_TypedBuf<BL_FLOAT> yCoords;
    ComputeCoords(numBands, &yCoords);
    
    WDL_TypedBuf<BL_FLOAT> ampsY;
    ComputeAmplitudes(localizationY, &ampsY);
    
    // Multiply amplitudes / lines
    //
    
    // Init lines
    vector<WDL_TypedBuf<BL_FLOAT> > lines;
    WDL_TypedBuf<BL_FLOAT> zeros;
    BLUtils::ResizeFillZeros(&zeros, SPECTRO_HEIGHT);
    for (int i = 0; i < SPECTRO_WIDTH; i++)
        lines.push_back(zeros);
    
    // Fill the data
    for (int j = 0; j < yCoords.GetSize(); j++)
    {
        for (int i = 0; i < xCoords.GetSize(); i++)
        {
            BL_FLOAT x = xCoords.Get()[i];
            BL_FLOAT y = yCoords.Get()[j];
            
            BL_FLOAT ampX = ampsX.Get()[i];
            BL_FLOAT ampY = ampsY.Get()[j];
            
            //
            BL_FLOAT amp = ampX*ampY;
            
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
    }
    
    // Fill spectrogram
    //
    
    // Clear
    mSpectrogram->Reset(mSampleRate);
    if (mSpectroDisplay != NULL)
        mSpectroDisplay->SetSpectrogram(mSpectrogram);
    
    // Smooth
    TimeSmoothData(&lines);
    
    // Sharpness
    ApplySharpnessXY(&lines);
    
    // Smooth
    //TimeSmooth(&lines);
    
    // Fill spectrogram with lines
    for (int i = 0; i < SPECTRO_WIDTH; i++)
        mSpectrogram->AddLine(lines[i], lines[i]);
    
    if (mSpectroDisplay != NULL)
        mSpectroDisplay->SetSpectrogram(mSpectrogram);
    
    mLineCount++;
}

#endif // IGRAPHICS_NANOVG

