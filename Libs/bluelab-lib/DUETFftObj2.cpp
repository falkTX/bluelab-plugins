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
//  DUETFftObj2.cpp
//  BL-Pano
//
//  Created by Pan on 02/06/18.
//
//

#ifdef IGRAPHICS_NANOVG

#include <BLSpectrogram4.h>
#include <SpectrogramDisplay2.h>

#include <BLImage.h>
#include <ImageDisplay2.h>

#include <BLUtils.h>
#include <BLUtilsComp.h>
#include <BLUtilsFft.h>

#include <GraphControl12.h>

#include <DUETSeparator2.h>

#include "DUETFftObj2.h"

// Resolution of the spectrogram
#define SPECTRO_WIDTH 128
#define SPECTRO_HEIGHT 128

// Paper: 35x50
#define HISTO_SIZE 64

// Increase the default brightness
#define DEBUG_INCREASE_BRIGHTNESS 1


DUETFftObj2::DUETFftObj2(GraphControl12 *graph,
                         int bufferSize, int oversampling, int freqRes,
                         BL_FLOAT sampleRate)
: MultichannelProcess()
{
    mSeparator = new DUETSeparator2(HISTO_SIZE, mSampleRate);
    
    mGraph = graph;
    
    mSpectrogram = new BLSpectrogram4(sampleRate, bufferSize/4, -1);
    mSpectroDisplay = NULL;
    
    int width = mSeparator->GetHistogramWidth();
    int height = mSeparator->GetHistogramHeight();

    mImage = new BLImage(width, height);
    mImageDisplay = NULL;
    
    MultichannelProcess::Reset(bufferSize, oversampling, freqRes, sampleRate);
    
    //
    mBufferSize = bufferSize;
    mOverlapping = oversampling;
    mFreqRes = freqRes;
    mSampleRate = sampleRate;
    
    mUseSoftMasksComp = false;
    
    mMustReprocess = false;
    
    mUsePhaseAliasingCorrection = false;
}

DUETFftObj2::~DUETFftObj2()
{
    delete mSpectrogram;
    delete mImage;
    
    delete mSeparator;
}

void
DUETFftObj2::ProcessInputFft(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > *ioFftSamples,
                             const vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > *scBuffer)
{
    if (ioFftSamples->size() != 2)
        return;
    
    //
    WDL_TypedBuf<WDL_FFT_COMPLEX> ioBuffers[2];
    ioBuffers[0] = *(*ioFftSamples)[0];
    ioBuffers[1] = *(*ioFftSamples)[1];
    
    BLUtils::TakeHalf(&ioBuffers[0]);
    BLUtils::TakeHalf(&ioBuffers[1]);
    
    WDL_TypedBuf<BL_FLOAT> magns[2];
    WDL_TypedBuf<BL_FLOAT> phases[2];
    BLUtilsComp::ComplexToMagnPhase(&magns[0], &phases[0], ioBuffers[0]);
    BLUtilsComp::ComplexToMagnPhase(&magns[1], &phases[1], ioBuffers[1]);
    
    //
    // Must set magns anyway
    mSeparator->SetInputData(magns, phases);
    
    if (mUseSoftMasksComp)
        mSeparator->SetInputDataComp(ioBuffers);
    
    Process();
    
    if (!mUseSoftMasksComp)
    {
        mSeparator->GetOutputData(magns, phases);
        
        BLUtilsComp::MagnPhaseToComplex(&ioBuffers[0], magns[0], phases[0]);
        BLUtilsComp::MagnPhaseToComplex(&ioBuffers[1], magns[1], phases[1]);
    }
    else
        mSeparator->GetOutputDataComp(ioBuffers);
        
    //
    BLUtils::ResizeFillZeros(&ioBuffers[0], ioBuffers[0].GetSize()*2);
    BLUtils::ResizeFillZeros(&ioBuffers[1], ioBuffers[1].GetSize()*2);
    
    BLUtilsFft::FillSecondFftHalf(&ioBuffers[0]);
    BLUtilsFft::FillSecondFftHalf(&ioBuffers[1]);
    
    *(*ioFftSamples)[0] = ioBuffers[0];
    *(*ioFftSamples)[1] = ioBuffers[1];
}

void
DUETFftObj2::ProcessInputSamplesWin(vector<WDL_TypedBuf<BL_FLOAT> * > *ioSamples,
                                    const vector<WDL_TypedBuf<BL_FLOAT> > *scBuffer)
{
    if (!mUsePhaseAliasingCorrection)
        return;
    
    if (ioSamples->size() != 2)
        return;
    
    WDL_TypedBuf<BL_FLOAT> samples[2] = { *(*ioSamples)[0], *(*ioSamples)[1] };
    
    int oversampling = mSeparator->GetPACOversampling();
    for (int i = 0; i < 2; i++)
    {
        BLUtils::ResizeFillZeros(&samples[i], samples[i].GetSize()*oversampling);
        FftProcessObj16::ComputeFft(samples[i], &mPACOversampledFft[i], oversampling);
        
        BLUtils::TakeHalf(&mPACOversampledFft[i]);
        BLUtils::MultValues(&mPACOversampledFft[i], 0.5); // Hack
    }
    
    mSeparator->SetPACOversamplesFft(mPACOversampledFft);
}

void
DUETFftObj2::Reset(int bufferSize, int oversampling, int freqRes, BL_FLOAT sampleRate)
{
    MultichannelProcess::Reset(bufferSize, oversampling, freqRes, sampleRate);
    
    mBufferSize = bufferSize;
    mOverlapping = oversampling;
    mFreqRes = freqRes;
    mSampleRate = sampleRate;
    
    mSpectrogram->Reset(mSampleRate, SPECTRO_WIDTH, SPECTRO_HEIGHT);
    
    mSeparator->Reset(sampleRate);
    
    for (int i = 0; i < 2; i++)
        mPACOversampledFft[i].Resize(0);
}

void
DUETFftObj2::Update()
{
    if (mMustReprocess)
        Process();
}

void
DUETFftObj2::SetGraph(GraphControl12 *graph)
{
    mGraph = graph;
}

BLSpectrogram4 *
DUETFftObj2::GetSpectrogram()
{
    return mSpectrogram;
}

void
DUETFftObj2::SetSpectrogramDisplay(SpectrogramDisplay2 *spectroDisplay)
{
    mSpectroDisplay = spectroDisplay;
}

BLImage *
DUETFftObj2::GetImage()
{
    return mImage;
}

void
DUETFftObj2::SetImageDisplay(ImageDisplay2 *imageDisplay)
{
    mImageDisplay = imageDisplay;
}

void
DUETFftObj2::SetThresholdFloor(BL_FLOAT threshold)
{
    mSeparator->SetThresholdFloor(threshold);
    
    mMustReprocess = true;
    
    if (mGraph != NULL)
        mGraph->SetDataChanged();
}

void
DUETFftObj2::SetThresholdPeaks(BL_FLOAT threshold)
{
    mSeparator->SetThresholdPeaks(threshold);
    
    mMustReprocess = true;
    
    if (mGraph != NULL)
        mGraph->SetDataChanged();
}

void
DUETFftObj2::SetThresholdPeaksWidth(BL_FLOAT threshold)
{
    mSeparator->SetThresholdPeaksWidth(threshold);
    
    mMustReprocess = true;
    
    if (mGraph != NULL)
        mGraph->SetDataChanged();
}

void
DUETFftObj2::SetDisplayThresholded(bool flag)
{
    mSeparator->SetDisplayThresholded(flag);
    
    mMustReprocess = true;
    
    if (mGraph != NULL)
        mGraph->SetDataChanged();
}

void
DUETFftObj2::SetDisplayMaxima(bool flag)
{
    mSeparator->SetDisplayMaxima(flag);
    
    mMustReprocess = true;
    
    if (mGraph != NULL)
        mGraph->SetDataChanged();
}

void
DUETFftObj2::SetDisplayMasks(bool flag)
{
    mSeparator->SetDisplayMasks(flag);
    
    mMustReprocess = true;
    
    if (mGraph != NULL)
        mGraph->SetDataChanged();
}

void
DUETFftObj2::SetUseSoftMasks(bool flag)
{
    mSeparator->SetUseSoftMasks(flag);
}

void
DUETFftObj2::SetUseSoftMasksComp(bool flag)
{
    mUseSoftMasksComp = flag;
    
    mSeparator->SetUseSoftMasksComp(flag);
}

void
DUETFftObj2::SetSoftMaskSize(int size)
{
    mSeparator->SetSoftMaskSize(size);
}

void
DUETFftObj2::SetTimeSmooth(BL_FLOAT smoothFactor)
{
    mSeparator->SetTimeSmooth(smoothFactor);
}

void
DUETFftObj2::SetUseKernelSmooth(bool kernelSmoothFlag)
{
    mSeparator->SetUseKernelSmooth(kernelSmoothFlag);
    
    mMustReprocess = true;
    
    if (mGraph != NULL)
        mGraph->SetDataChanged();
}

void
DUETFftObj2::SetUseGradientMasks(bool flag)
{
    mSeparator->SetUseGradientMasks(flag);

    mMustReprocess = true;
    
    if (mGraph != NULL)
        mGraph->SetDataChanged();
}

void
DUETFftObj2::SetThresholdAll(bool flag)
{
    mSeparator->SetThresholdAll(flag);
    
    mMustReprocess = true;
    
    if (mGraph != NULL)
        mGraph->SetDataChanged();
}

void
DUETFftObj2::SetPickingActive(bool flag)
{
    mSeparator->SetPickingActive(flag);
    
    mMustReprocess = true;
    
    if (mGraph != NULL)
        mGraph->SetDataChanged();
}

void
DUETFftObj2::SetPickPosition(BL_FLOAT x, BL_FLOAT y)
{
    if (mImageDisplay == NULL)
        return;
    
    BL_FLOAT left;
    BL_FLOAT top;
    BL_FLOAT right;
    BL_FLOAT bottom;
    mImageDisplay->GetBounds(&left, &top, &right, &bottom);
    BL_FLOAT widthNorm = right - left;
    
    x /= widthNorm;
    
    y = 1.0 - y;
    
    mSeparator->SetPickPosition(x, y);
    
    mMustReprocess = true;
    
    if (mGraph != NULL)
        mGraph->SetDataChanged();
}

void
DUETFftObj2::SetInvertPickSelection(bool flag)
{
    mSeparator->SetInvertPickSelection(flag);
    
    mMustReprocess = true;
    
    if (mGraph != NULL)
        mGraph->SetDataChanged();
}

void
DUETFftObj2::SetHistogramSize(int histoSize)
{
    mSeparator->SetHistogramSize(histoSize);
    
    mMustReprocess = true;
    
    if (mGraph != NULL)
        mGraph->SetDataChanged();
}

void
DUETFftObj2::SetAlphaZoom(BL_FLOAT zoom)
{
    mSeparator->SetAlphaZoom(zoom);
    
    mMustReprocess = true;
    
    if (mGraph != NULL)
        mGraph->SetDataChanged();
}

void
DUETFftObj2::SetDeltaZoom(BL_FLOAT zoom)
{
    mSeparator->SetDeltaZoom(zoom);
    
    mMustReprocess = true;
    
    if (mGraph != NULL)
        mGraph->SetDataChanged();
}

void
DUETFftObj2::SetUsePhaseAliasingCorrection(bool flag)
{
    mUsePhaseAliasingCorrection = flag;
    mSeparator->SetUsePhaseAliasingCorrection(flag);
    
    mMustReprocess = true;
    
    if (mGraph != NULL)
        mGraph->SetDataChanged();
}

void
DUETFftObj2::Process()
{
    mSeparator->Process();
    
    WDL_TypedBuf<BL_FLOAT> histogram;
    mSeparator->GetHistogramData(&histogram);
    
#if DEBUG_INCREASE_BRIGHTNESS
    BLUtils::MultValues(&histogram, (BL_FLOAT)16.0);
#endif
    
    if (mImageDisplay != NULL)
    {
        mImage->SetData(histogram);
        mImageDisplay->SetImage(mImage);
    }
    
    mMustReprocess = false;
}

#endif // IGRAPHICS_NANOVG
