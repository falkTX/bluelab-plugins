//
//  DUETFftObj.cpp
//  BL-Pano
//
//  Created by Pan on 02/06/18.
//
//

#ifdef IGRAPHICS_NANOVG

#include <BLSpectrogram4.h>

#include <BLUtils.h>
#include <BLUtilsComp.h>
#include <BLUtilsFft.h>

#include <SpectrogramDisplay.h>
//#include <HistoMaskLine2.h>

#include <SourceLocalisationSystem3.h>

#include <ImageDisplay.h>

#include <DUETSeparator.h>

#include <GraphControl11.h>

#include "DUETFftObj.h"

// Resolution of the spectrogram
#define SPECTRO_WIDTH 128 //512
#define SPECTRO_HEIGHT 128 //512

// Increase the default brightness
#define DEBUG_INCREASE_BRIGHTNESS 1


DUETFftObj::DUETFftObj(GraphControl11 *graph,
                       int bufferSize, int oversampling, int freqRes,
                       BL_FLOAT sampleRate)
: MultichannelProcess()
{
    mGraph = graph;
    
    mSpectrogram = new BLSpectrogram4(sampleRate, bufferSize/4, -1);
    mSpectroDisplay = NULL;
    
    mImageDisplay = NULL;
    
    MultichannelProcess::Reset(bufferSize, oversampling, freqRes, sampleRate);
    
    //
    mBufferSize = bufferSize;
    mOverlapping = oversampling;
    mFreqRes = freqRes;
    mSampleRate = sampleRate;
    
    //
    mLineCount = 0;
    
    //mThreshold = 0.0;
    
    //
    mSeparator = new DUETSeparator(mSampleRate);
    
    mUseSoftMasksComp = false;
}

DUETFftObj::~DUETFftObj()
{
    delete mSpectrogram;
    
    delete mSeparator;
}

void
DUETFftObj::ProcessInputFft(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > *ioFftSamples,
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
        mSeparator->SetInputData(ioBuffers);
    
    Process();
    
    if (!mUseSoftMasksComp)
    {
        mSeparator->GetOutputData(magns, phases);
        
        BLUtilsComp::MagnPhaseToComplex(&ioBuffers[0], magns[0], phases[0]);
        BLUtilsComp::MagnPhaseToComplex(&ioBuffers[1], magns[1], phases[1]);
    }
    else
        mSeparator->GetOutputData(ioBuffers);
        
    //
    BLUtils::ResizeFillZeros(&ioBuffers[0], ioBuffers[0].GetSize()*2);
    BLUtils::ResizeFillZeros(&ioBuffers[1], ioBuffers[1].GetSize()*2);
    
    BLUtilsFft::FillSecondFftHalf(&ioBuffers[0]);
    BLUtilsFft::FillSecondFftHalf(&ioBuffers[1]);
    
    *(*ioFftSamples)[0] = ioBuffers[0];
    *(*ioFftSamples)[1] = ioBuffers[1];
}

void
DUETFftObj::Reset(int bufferSize, int oversampling, int freqRes, BL_FLOAT sampleRate)
{
    MultichannelProcess::Reset(bufferSize, oversampling, freqRes, sampleRate);
    
    mBufferSize = bufferSize;
    mOverlapping = oversampling;
    mFreqRes = freqRes;
    mSampleRate = sampleRate;
    
    mSpectrogram->Reset(mSampleRate, SPECTRO_WIDTH, SPECTRO_HEIGHT);
    
    mLineCount = 0;
    
    mSeparator->Reset(sampleRate);
}

BLSpectrogram4 *
DUETFftObj::GetSpectrogram()
{
    return mSpectrogram;
}

void
DUETFftObj::SetSpectrogramDisplay(SpectrogramDisplay *spectroDisplay)
{
    mSpectroDisplay = spectroDisplay;
}

void
DUETFftObj::SetThresholdFloor(BL_FLOAT threshold)
{
    mSeparator->SetThresholdFloor(threshold);
    
    Process();
    
    mGraph->SetDirty(true);
}

void
DUETFftObj::SetThresholdPeaks(BL_FLOAT threshold)
{
    mSeparator->SetThresholdPeaks(threshold);
    
    Process();
    
    mGraph->SetDirty(true);
}

void
DUETFftObj::SetThresholdPeaksWidth(BL_FLOAT threshold)
{
    mSeparator->SetThresholdPeaksWidth(threshold);
    
    Process();
    
    mGraph->SetDirty(true);
}

void
DUETFftObj::SetDisplayThresholded(bool flag)
{
    mSeparator->SetDisplayThresholded(flag);
    
    Process();
    
    mGraph->SetDirty(true);
}

void
DUETFftObj::SetDisplayMaxima(bool flag)
{
    mSeparator->SetDisplayMaxima(flag);
    
    Process();
    
    mGraph->SetDirty(true);
}

void
DUETFftObj::SetDisplayMasks(bool flag)
{
    mSeparator->SetDisplayMasks(flag);
    
    Process();
    
    mGraph->SetDirty(true);
}

void
DUETFftObj::SetUseSoftMasks(bool flag)
{
    mSeparator->SetUseSoftMasks(flag);
}

void
DUETFftObj::SetUseSoftMasksComp(bool flag)
{
    mUseSoftMasksComp = flag;
    
    mSeparator->SetUseSoftMasksComp(flag);
}

void
DUETFftObj::SetSoftMaskSize(int size)
{
    mSeparator->SetSoftMaskSize(size);
}

void
DUETFftObj::SetTimeSmooth(BL_FLOAT smoothFactor)
{
    mSeparator->SetTimeSmooth(smoothFactor);
}

void
DUETFftObj::SetKernelSmooth(bool kernelSmoothFlag)
{
    mSeparator->SetKernelSmooth(kernelSmoothFlag);
    
    Process();
    
    mGraph->SetDirty(true);
}

void
DUETFftObj::SetUseDuetSoftMasks(bool flag)
{
    mSeparator->SetUseDuetSoftMasks(flag);
    
#if 0 // Bad Thread
    Process();
    
    mGraph->SetDirty(true);
#endif
}

void
DUETFftObj::SetThresholdAll(bool flag)
{
    mSeparator->SetThresholdAll(flag);
}

void
DUETFftObj::SetImageDisplay(ImageDisplay *imageDisplay)
{
    mImageDisplay = imageDisplay;
}

void
DUETFftObj::SetPickingActive(bool flag)
{
    mSeparator->SetPickingActive(flag);
    
    // Must not process here, it is not the right thread
    //Process();
    
    mGraph->SetDirty(true);
}

void
DUETFftObj::SetPickPosition(BL_FLOAT x, BL_FLOAT y)
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
    
    Process();
    
    mGraph->SetDirty(true);
}

void
DUETFftObj::SetPickModeBg(bool flag)
{
    mSeparator->SetPickModeBg(flag);
}

void
DUETFftObj::Process()
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
}

#endif // IGRAPHICS_NANOVG
