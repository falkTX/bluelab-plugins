//
//  BatFftObj4.cpp
//  BL-Pano
//
//  Created by Pan on 02/06/18.
//
//

#include <BLSpectrogram3.h>
#include <Window.h>
#include <BLUtils.h>

// #bl-iplug2
//#include "nanovg.h"

#include <SpectrogramDisplay.h>
#include <HistoMaskLine2.h>

//#include <SourcePos.h>

#include <SourceLocalisationSystem2.h>

#include "BatFftObj4.h"

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

// Set one of these modes
#define MODE_1D  1
#define MODE_2D  0
#define MODE_2D2 0


BatFftObj4::BatFftObj4(int bufferSize, int oversampling, int freqRes,
                       BL_FLOAT sampleRate)
: MultichannelProcess()
{
    mSpectrogram = new BLSpectrogram3(bufferSize/4, -1);
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
            new SourceLocalisationSystem2(bufferSize, sampleRate,
                                          SOURCE_LOC_NUM_BANDS);
}

BatFftObj4::~BatFftObj4()
{
    delete mSpectrogram;
    
    delete mSourceLocSystem;
}

void
BatFftObj4::ProcessInputFft(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > *ioFftSamples,
                            const vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > *scBuffer)
{
    if (ioFftSamples->size() != 2)
        return;
    
    if (scBuffer->empty())
        return;
    
    if (!mIsEnabled)
        return;
    
#if MODE_1D
    WDL_TypedBuf<BL_FLOAT> localizationX;
    GetLocalization(*(*ioFftSamples)[0], *(*ioFftSamples)[1], &localizationX);
    
    // Compute coords
    WDL_TypedBuf<BL_FLOAT> xCoords;
    ComputeCoords(localizationX, &xCoords);
    
    WDL_TypedBuf<BL_FLOAT> amps;
    ComputeAmplitudes(localizationX, &amps);
    
    // Dummy Y
    WDL_TypedBuf<BL_FLOAT> yCoords;
    BLUtils::ResizeFillValue(&yCoords, xCoords.GetSize(), (BL_FLOAT)0.5);
#endif

#if MODE_2D
    // For 2D
    WDL_TypedBuf<BL_FLOAT> localizationY;
    GetLocalization(*(*ioFftSamples)[0], (*scBuffer)[0], &localizationY);
    
    WDL_TypedBuf<BL_FLOAT> localization2D;
    ComputeLocalization2D(localizationX, localizationY, &localization2D);
    
    WDL_TypedBuf<BL_FLOAT> xCoords;
    WDL_TypedBuf<BL_FLOAT> yCoords;
    ComputeCoords2D(localizationX.GetSize(),
                    localizationY.GetSize(),
                    &xCoords, &yCoords);
    
    WDL_TypedBuf<BL_FLOAT> amps = localization2D;
#endif
    
#if MODE_2D2
    if (scBuffer->size() != 2)
        return;
    
    WDL_TypedBuf<BL_FLOAT> amps;
    for (int j = 0; j < SOURCE_LOC_NUM_BANDS; j++)
    {
        BL_FLOAT t = ((BL_FLOAT)j)/(SOURCE_LOC_NUM_BANDS - 1);
        
        WDL_TypedBuf<WDL_FFT_COMPLEX> samples[2];
        BLUtils::Interp(&samples[0], (*ioFftSamples)[0], &(*scBuffer)[0], t);
        BLUtils::Interp(&samples[1], (*ioFftSamples)[1], &(*scBuffer)[1], t);
        
        WDL_TypedBuf<BL_FLOAT> localization;
        GetLocalization(samples[0], samples[1], &localization);
        
        amps.Add(localization.Get(), localization.GetSize());
    }
    
    // Hack
    BLUtils::MultValues(&amps, 0.1);
    
    WDL_TypedBuf<BL_FLOAT> xCoords;
    WDL_TypedBuf<BL_FLOAT> yCoords;
    ComputeCoords2D(SOURCE_LOC_NUM_BANDS,
                    SOURCE_LOC_NUM_BANDS,
                    &xCoords, &yCoords);
#endif
    
    // Fill spectrogram
    //
    
    // Clear
    mSpectrogram->Reset();
    
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
BatFftObj4::Reset(int bufferSize, int oversampling, int freqRes, BL_FLOAT sampleRate)
{
    MultichannelProcess::Reset(bufferSize, oversampling, freqRes, sampleRate);
    
    mBufferSize = bufferSize;
    mOverlapping = oversampling;
    mFreqRes = freqRes;
    mSampleRate = sampleRate;
    
    mSpectrogram->Reset(SPECTRO_WIDTH, SPECTRO_HEIGHT);
    
    mLineCount = 0;
    
    //mOverlapLines.clear();
    
    mAddLineCount = 0;
    
    mSourceLocSystem->Reset(bufferSize, sampleRate);
}

BLSpectrogram3 *
BatFftObj4::GetSpectrogram()
{
    return mSpectrogram;
}

void
BatFftObj4::SetSpectrogramDisplay(SpectrogramDisplay *spectroDisplay)
{
    mSpectroDisplay = spectroDisplay;
}

void
BatFftObj4::SetSharpness(BL_FLOAT sharpness)
{
    mSharpness = sharpness;
    
    // Force re-creating the window
    mSmoothWin.Resize(0);
}

void
BatFftObj4::SetTimeSmooth(BL_FLOAT smooth)
{
    mTimeSmooth = smooth;
}

void
BatFftObj4::SetEnabled(bool flag)
{
    mIsEnabled = flag;
}

void
BatFftObj4::SetFftSmooth(BL_FLOAT smooth)
{
    mFftSmooth = smooth;
}

void
BatFftObj4::ComputeCoords(const WDL_TypedBuf<BL_FLOAT> &localization,
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
BatFftObj4::ComputeCoords2D(int width, int height,
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
BatFftObj4::ComputeAmplitudes(const WDL_TypedBuf<BL_FLOAT> &localization,
                              WDL_TypedBuf<BL_FLOAT> *amps)
{
    *amps = localization;
    
    // TODO: normalize if necessary
}

void
BatFftObj4::ApplySharpnessXY(vector<WDL_TypedBuf<BL_FLOAT> > *lines)
{
    ApplySharpness(lines);
    
    ReverseXY(lines);
    
    ApplySharpness(lines);
    
    ReverseXY(lines);
}

void
BatFftObj4::ReverseXY(vector<WDL_TypedBuf<BL_FLOAT> > *lines)
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
BatFftObj4::ApplySharpness(vector<WDL_TypedBuf<BL_FLOAT> > *lines)
{
    for (int i = 0; i < lines->size(); i++)
    {
        WDL_TypedBuf<BL_FLOAT> &line = (*lines)[i];
        
        ApplySharpness(&line);
    }
}

void
BatFftObj4::ApplySharpness(WDL_TypedBuf<BL_FLOAT> *line)
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
BatFftObj4::TimeSmooth(vector<WDL_TypedBuf<BL_FLOAT> > *lines)
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

void
BatFftObj4::GetLocalization(const WDL_TypedBuf<WDL_FFT_COMPLEX> &samples0,
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
    
    mSourceLocSystem->AddSamples(fftSamples);
    
    mSourceLocSystem->GetLocalization(localization);
}

void
BatFftObj4::ComputeLocalization2D(const WDL_TypedBuf<BL_FLOAT> &localizationX,
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
