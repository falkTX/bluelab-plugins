//
//  DUETFftObj2.cpp
//  BL-Pano
//
//  Created by Pan on 02/06/18.
//
//

#include <BLSpectrogram3.h>

#include <BLUtils.h>

// #bl-iplug2
//#include "nanovg.h"

#include <GraphControl11.h>
#include <SpectrogramDisplay.h>
#include <ImageDisplay.h>

#include <DUETSeparator2.h>

#include "DUETFftObj2.h"

// Resolution of the spectrogram
#define SPECTRO_WIDTH 128 //512
#define SPECTRO_HEIGHT 128 //512

// Paper: 35x50
#define HISTO_SIZE 64 //32 //64 //128 //32 //8

// Increase the default brightness
#define DEBUG_INCREASE_BRIGHTNESS 1


DUETFftObj2::DUETFftObj2(GraphControl11 *graph,
                         int bufferSize, int oversampling, int freqRes,
                         BL_FLOAT sampleRate)
: MultichannelProcess()
{
    mGraph = graph;
    
    mSpectrogram = new BLSpectrogram3(bufferSize/4, -1);
    mSpectroDisplay = NULL;
    
    mImageDisplay = NULL;
    
    MultichannelProcess::Reset(bufferSize, oversampling, freqRes, sampleRate);
    
    //
    mBufferSize = bufferSize;
    mOverlapping = oversampling;
    mFreqRes = freqRes;
    mSampleRate = sampleRate;
    
    //
    mSeparator = new DUETSeparator2(HISTO_SIZE, mSampleRate);
    
    mUseSoftMasksComp = false;
    
    mMustReprocess = false;
    
    mUsePhaseAliasingCorrection = false;
}

DUETFftObj2::~DUETFftObj2()
{
    delete mSpectrogram;
    
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
    BLUtils::ComplexToMagnPhase(&magns[0], &phases[0], ioBuffers[0]);
    BLUtils::ComplexToMagnPhase(&magns[1], &phases[1], ioBuffers[1]);
    
    //
    // Must set magns anyway
    mSeparator->SetInputData(magns, phases);
    
    if (mUseSoftMasksComp)
        mSeparator->SetInputDataComp(ioBuffers);
    
    Process();
    
    if (!mUseSoftMasksComp)
    {
        mSeparator->GetOutputData(magns, phases);
        
        BLUtils::MagnPhaseToComplex(&ioBuffers[0], magns[0], phases[0]);
        BLUtils::MagnPhaseToComplex(&ioBuffers[1], magns[1], phases[1]);
    }
    else
        mSeparator->GetOutputDataComp(ioBuffers);
        
    //
    BLUtils::ResizeFillZeros(&ioBuffers[0], ioBuffers[0].GetSize()*2);
    BLUtils::ResizeFillZeros(&ioBuffers[1], ioBuffers[1].GetSize()*2);
    
    BLUtils::FillSecondFftHalf(&ioBuffers[0]);
    BLUtils::FillSecondFftHalf(&ioBuffers[1]);
    
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
        BLUtils::MultValues(&mPACOversampledFft[i],  0.5); // Hack
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
    
    mSpectrogram->Reset(SPECTRO_WIDTH, SPECTRO_HEIGHT);
    
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

BLSpectrogram3 *
DUETFftObj2::GetSpectrogram()
{
    return mSpectrogram;
}

void
DUETFftObj2::SetSpectrogramDisplay(SpectrogramDisplay *spectroDisplay)
{
    mSpectroDisplay = spectroDisplay;
}

void
DUETFftObj2::SetThresholdFloor(BL_FLOAT threshold)
{
    mSeparator->SetThresholdFloor(threshold);
    
    mMustReprocess = true;
    
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
}

void
DUETFftObj2::SetThresholdPeaks(BL_FLOAT threshold)
{
    mSeparator->SetThresholdPeaks(threshold);
    
    mMustReprocess = true;
    
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
}

void
DUETFftObj2::SetThresholdPeaksWidth(BL_FLOAT threshold)
{
    mSeparator->SetThresholdPeaksWidth(threshold);
    
    mMustReprocess = true;
    
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
}

void
DUETFftObj2::SetDisplayThresholded(bool flag)
{
    mSeparator->SetDisplayThresholded(flag);
    
    mMustReprocess = true;
    
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
}

void
DUETFftObj2::SetDisplayMaxima(bool flag)
{
    mSeparator->SetDisplayMaxima(flag);
    
    mMustReprocess = true;
    
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
}

void
DUETFftObj2::SetDisplayMasks(bool flag)
{
    mSeparator->SetDisplayMasks(flag);
    
    mMustReprocess = true;
    
    //mGraph->SetDirty(true);
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
    
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
}

void
DUETFftObj2::SetUseGradientMasks(bool flag)
{
    mSeparator->SetUseGradientMasks(flag);

    mMustReprocess = true;
    
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
}

void
DUETFftObj2::SetThresholdAll(bool flag)
{
    mSeparator->SetThresholdAll(flag);
    
    mMustReprocess = true;
    
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
}

void
DUETFftObj2::SetImageDisplay(ImageDisplay *imageDisplay)
{
    mImageDisplay = imageDisplay;
}

void
DUETFftObj2::SetPickingActive(bool flag)
{
    mSeparator->SetPickingActive(flag);
    
    mMustReprocess = true;
    
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
}

void
DUETFftObj2::SetPickPosition(BL_FLOAT x, BL_FLOAT y)
{
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
    
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
}

void
DUETFftObj2::SetInvertPickSelection(bool flag)
{
    mSeparator->SetInvertPickSelection(flag);
    
    mMustReprocess = true;
    
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
}

void
DUETFftObj2::SetHistogramSize(int histoSize)
{
    mSeparator->SetHistogramSize(histoSize);
    
    mMustReprocess = true;
    
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
}

void
DUETFftObj2::SetAlphaZoom(BL_FLOAT zoom)
{
    mSeparator->SetAlphaZoom(zoom);
    
    mMustReprocess = true;
    
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
}

void
DUETFftObj2::SetDeltaZoom(BL_FLOAT zoom)
{
    mSeparator->SetDeltaZoom(zoom);
    
    mMustReprocess = true;
    
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
}

void
DUETFftObj2::SetUsePhaseAliasingCorrection(bool flag)
{
    mUsePhaseAliasingCorrection = flag;
    mSeparator->SetUsePhaseAliasingCorrection(flag);
    
    mMustReprocess = true;
    
    //mGraph->SetDirty(true);
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
    
    int width = mSeparator->GetHistogramWidth();
    int height = mSeparator->GetHistogramHeight();
    mImageDisplay->SetImage(width, height, histogram);
    
    mMustReprocess = false;
}
