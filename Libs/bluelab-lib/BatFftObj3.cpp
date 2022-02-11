//
//  BatFftObj3.cpp
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

#include <SourceLocalisationSystem2D.h>

#include "BatFftObj3.h"

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
#define SPECTRO_WIDTH 64 //128
#define SPECTRO_HEIGHT 64 //128

// Reverse coordinates ?
#define REVERSE_X 1
#define REVERSE_Y 1

#define USE_AMP_MAX 1

//
#define SOURCE_LOC_NUM_BANDS 65 //15 //65 //361 //15 //65 //361 //15 //361 //65 //15



BatFftObj3::BatFftObj3(int bufferSize, int oversampling, int freqRes,
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
    
    mSourceLocSystem =
            new SourceLocalisationSystem2D(bufferSize, sampleRate,
                                          SOURCE_LOC_NUM_BANDS);
}

BatFftObj3::~BatFftObj3()
{
    delete mSpectrogram;
    
    delete mSourceLocSystem;
}

void
BatFftObj3::ProcessInputFft(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > *ioFftSamples,
                            const vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > *scBuffer)
{
    if (ioFftSamples->size() != 2)
        return;
    
    if (scBuffer->empty())
        return;
    
    if (!mIsEnabled)
        return;
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> fftSamples[2];
    
    fftSamples[0] = *(*ioFftSamples)[0];
    fftSamples[1] = *(*ioFftSamples)[1];
    
    for (int i = 0; i < 2; i++)
    {
        if (i >= ioFftSamples->size())
            continue;
        
        BLUtils::TakeHalf(&fftSamples[i]);
    }
    
    // TODO: choose better signals
    WDL_TypedBuf<WDL_FFT_COMPLEX> fftSamplesY[2] = { fftSamples[0], (*scBuffer)[0] };
    BLUtils::TakeHalf(&fftSamples[1]);
    
    mSourceLocSystem->AddSamples(fftSamples, fftSamplesY);
    
    vector<WDL_TypedBuf<BL_FLOAT> > localization;
    mSourceLocSystem->GetLocalization(&localization);
    
    // Compute coords
    WDL_TypedBuf<BL_FLOAT> xCoords;
    ComputeCoords(localization.size(), &xCoords);
    
    WDL_TypedBuf<BL_FLOAT> yCoords;
    ComputeCoords(localization[0].GetSize(), &yCoords);
    
    WDL_TypedBuf<BL_FLOAT> amps;
    ComputeAmplitudes(localization, &amps);
    
    
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
    for (int j = 0; j < yCoords.GetSize(); j++)
    {
        for (int i = 0; i < xCoords.GetSize(); i++)
        {
            BL_FLOAT x = xCoords.Get()[i];
            BL_FLOAT y = yCoords.Get()[j];
            BL_FLOAT amp = amps.Get()[i + j*xCoords.GetSize()];
        
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
BatFftObj3::Reset(int bufferSize, int oversampling, int freqRes, BL_FLOAT sampleRate)
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
    
    mSourceLocSystem->Reset(bufferSize, sampleRate);
}

BLSpectrogram4 *
BatFftObj3::GetSpectrogram()
{
    return mSpectrogram;
}

void
BatFftObj3::SetSpectrogramDisplay(SpectrogramDisplay *spectroDisplay)
{
    mSpectroDisplay = spectroDisplay;
}

void
BatFftObj3::SetSharpness(BL_FLOAT sharpness)
{
    mSharpness = sharpness;
    
    // Force re-creating the window
    mSmoothWin.Resize(0);
}

void
BatFftObj3::SetTimeSmooth(BL_FLOAT smooth)
{
    mTimeSmooth = smooth;
}

void
BatFftObj3::SetEnabled(bool flag)
{
    mIsEnabled = flag;
}

void
BatFftObj3::SetFftSmooth(BL_FLOAT smooth)
{
    mFftSmooth = smooth;
}

void
BatFftObj3::ComputeCoords(long size, WDL_TypedBuf<BL_FLOAT> *coords)
{    
    coords->Resize(size);
   
    for (int i = 0; i < size; i++)
    {
        BL_FLOAT t = ((BL_FLOAT)i)/(size - 1);
        
        coords->Get()[i] = t;
    }
}

void
BatFftObj3::ComputeAmplitudes(const vector<WDL_TypedBuf<BL_FLOAT> > &localization,
                              WDL_TypedBuf<BL_FLOAT> *amps)
{
    for (int i = 0; i < localization.size(); i++)
    {
        const WDL_TypedBuf<BL_FLOAT> &loc = localization[i];
        
        amps->Add(loc.Get(), loc.GetSize());
    }
}

void
BatFftObj3::ApplySharpnessXY(vector<WDL_TypedBuf<BL_FLOAT> > *lines)
{
    ApplySharpness(lines);
    
    ReverseXY(lines);
    
    ApplySharpness(lines);
    
    ReverseXY(lines);
}

void
BatFftObj3::ReverseXY(vector<WDL_TypedBuf<BL_FLOAT> > *lines)
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
BatFftObj3::ApplySharpness(vector<WDL_TypedBuf<BL_FLOAT> > *lines)
{
    for (int i = 0; i < lines->size(); i++)
    {
        WDL_TypedBuf<BL_FLOAT> &line = (*lines)[i];
        
        ApplySharpness(&line);
    }
}

void
BatFftObj3::ApplySharpness(WDL_TypedBuf<BL_FLOAT> *line)
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
BatFftObj3::TimeSmooth(vector<WDL_TypedBuf<BL_FLOAT> > *lines)
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
