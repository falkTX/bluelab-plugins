//
//  BatFftObj.cpp
//  BL-Pano
//
//  Created by Pan on 02/06/18.
//
//

#ifdef IGRAPHICS_NANOVG

#include <BLSpectrogram4.h>
#include <Window.h>

#include <BLUtils.h>
#include <BLUtilsMath.h>
#include <BLUtilsComp.h>
#include <BLUtilsFft.h>

#include <SpectrogramDisplay.h>
#include <HistoMaskLine2.h>

#include <SourcePos.h>

#include "BatFftObj.h"

//
#define MIN_SMOOTH_DIVISOR 12.0
#define MAX_SMOOTH_DIVISOR 48.0

// When left and right magns are 0, the indices are pushed at the extreme left
#define FIX_EPS_MAGNS 1

#define SPECTRO_WIDTH 128 //64 //256 //128
#define SPECTRO_HEIGHT 128 //64 //256 //128

#define REVERSE_X 1
#define REVERSE_Y 1

#define COLORMAP_FREQUENCY 0 //1

#define USE_AMP_MAX 1

// BAD: nothing moves, everything in the center
#define USE_DB_MAGNS 0 //1


BatFftObj::BatFftObj(int bufferSize, int oversampling,
                     int freqRes, BL_FLOAT sampleRate)
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
}

BatFftObj::~BatFftObj()
{
    delete mSpectrogram;
}

void
BatFftObj::ProcessInputFft(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > *ioFftSamples,
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
    
    WDL_TypedBuf<BL_FLOAT> magns[2];
    WDL_TypedBuf<BL_FLOAT> phases[2];
    for (int i = 0; i < 2; i++)
    {
        BLUtils::TakeHalf(&fftSamples[i]);
        BLUtilsComp::ComplexToMagnPhase(&magns[i], &phases[i], fftSamples[i]);
        
#if USE_DB_MAGNS
        BLUtils::AmpToDBNorm(&magns[i], 1e-15, -200.0);
#endif
        
        if (mPrevMagns[i].GetSize() != magns[i].GetSize())
            mPrevMagns[i] = magns[i];
        BLUtils::Smooth(&magns[i], &mPrevMagns[i], mFftSmooth);
    }
  
    WDL_TypedBuf<WDL_FFT_COMPLEX> scFftSamples[2];
    scFftSamples[0] = (*scBuffer)[0];
    if (scBuffer->size() == 2)
        scFftSamples[1] = (*scBuffer)[1];
    
    WDL_TypedBuf<BL_FLOAT> scMagns[2];
    WDL_TypedBuf<BL_FLOAT> scPhases[2];
    for (int i = 0; i < 2; i++)
    {
        if (i >= scBuffer->size())
            continue;
        
        BLUtils::TakeHalf(&scFftSamples[i]);
        BLUtilsComp::ComplexToMagnPhase(&scMagns[i], &scPhases[i], scFftSamples[i]);
        
#if USE_DB_MAGNS
        BLUtils::AmpToDBNorm(&scMagns[i], 1e-15, -200.0);
#endif
      
        if (mPrevScMagns[i].GetSize() != scMagns[i].GetSize())
            mPrevScMagns[i] = scMagns[i];
        BLUtils::Smooth(&scMagns[i], &mPrevScMagns[i], mFftSmooth);
    }
    
    WDL_TypedBuf<BL_FLOAT> scMagns0 = scMagns[0];
    if (scBuffer->size() == 2)
    {
        BLUtils::StereoToMono(&scMagns0, scMagns[0], scMagns[1]);
    }
    
    //
    WDL_TypedBuf<BL_FLOAT> magns0;
    BLUtils::StereoToMono(&magns0, magns[0], magns[1]);
    
#if 1 //0 // Method 1
    // Compute coords
    WDL_TypedBuf<BL_FLOAT> xCoords;
    ComputeCoords1(magns, &xCoords);
    
    WDL_TypedBuf<BL_FLOAT> magnsY[2] = { magns0, scMagns0 };
    WDL_TypedBuf<BL_FLOAT> yCoords;
    ComputeCoords1(magnsY, &yCoords);
    
    WDL_TypedBuf<BL_FLOAT> amps;
    ComputeAmplitudes1(magns0, scMagns0, &amps);
#endif
    
#if 0 // Method2
    WDL_TypedBuf<BL_FLOAT> xCoords;
    ComputeCoords2(magns, &xCoords);
    
    WDL_TypedBuf<BL_FLOAT> magnsY[2] = { magns0, scMagns0 };
    WDL_TypedBuf<BL_FLOAT> yCoords;
    ComputeCoords2(magnsY, &yCoords);
    
    WDL_TypedBuf<BL_FLOAT> amps;
    ComputeAmplitudes2(magns0, scMagns0, &amps);
#endif

#if 0 //1 // Method3
    WDL_TypedBuf<BL_FLOAT> xCoords;
    ComputeCoords3(magns, phases, &xCoords);
    
    //BLDebug::DumpData("xcoords.txt", xCoords);
    
    WDL_TypedBuf<BL_FLOAT> magnsY[2] = { magns0, scMagns0 };
    WDL_TypedBuf<BL_FLOAT> phasesY[2] = { phases[0], scPhases[0] }; // Hack
    WDL_TypedBuf<BL_FLOAT> yCoords;
    ComputeCoords3(magnsY, phasesY, &yCoords);
    
    //BLDebug::DumpData("ycoords.txt", yCoords);
    
    WDL_TypedBuf<BL_FLOAT> amps;
    ComputeAmplitudes3(magns0, scMagns0, &amps);
#endif
    
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
        
#if !COLORMAP_FREQUENCY
        
#if !USE_AMP_MAX
        lines[xi].Get()[yi] += amp;
#else
        if (amp > lines[xi].Get()[yi])
            lines[xi].Get()[yi] = amp;
#endif
        
#else
        BL_FLOAT freq = ((BL_FLOAT)i)/SPECTRO_WIDTH;
        lines[xi].Get()[yi] += freq;
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
BatFftObj::Reset(int bufferSize, int oversampling, int freqRes, BL_FLOAT sampleRate)
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
}

BLSpectrogram4 *
BatFftObj::GetSpectrogram()
{
    return mSpectrogram;
}

void
BatFftObj::SetSpectrogramDisplay(SpectrogramDisplay *spectroDisplay)
{
    mSpectroDisplay = spectroDisplay;
}

void
BatFftObj::SetSharpness(BL_FLOAT sharpness)
{
    mSharpness = sharpness;
    
    // Force re-creating the window
    mSmoothWin.Resize(0);
}

void
BatFftObj::SetTimeSmooth(BL_FLOAT smooth)
{
    mTimeSmooth = smooth;
}

void
BatFftObj::SetEnabled(bool flag)
{
    mIsEnabled = flag;
}

void
BatFftObj::SetFftSmooth(BL_FLOAT smooth)
{
    mFftSmooth = smooth;
}

void
BatFftObj::ComputeCoords1(const WDL_TypedBuf<BL_FLOAT> magns[2],
                          WDL_TypedBuf<BL_FLOAT> *coords)
{    
    coords->Resize(magns[0].GetSize());
   
    for (int i = 0; i < magns[0].GetSize(); i++)
    {
        BL_FLOAT l = magns[0].Get()[i];
        BL_FLOAT r = magns[1].Get()[i];
        
        BL_FLOAT angle = std::atan2(r, l);
        //BL_FLOAT dist = std::sqrt(l*l + r*r);
        
        // Adjust
        angle = -angle;
        //angle -= M_PI/4.0;
        
        BL_FLOAT normAngle = (angle + M_PI/2.0)/M_PI;
        
        // Adjust
        normAngle *= 2.0;
        
        coords->Get()[i] = normAngle;
    }
}

void
BatFftObj::ComputeAmplitudes1(const WDL_TypedBuf<BL_FLOAT> &magns,
                              const WDL_TypedBuf<BL_FLOAT> &scMagns,
                              WDL_TypedBuf<BL_FLOAT> *amps)
{
    *amps = magns;
    BLUtils::AddValues(amps, scMagns);
    
    // TEST
    //BLUtils::FillAllValue(amps, 0.001);
}

void
BatFftObj::ComputeCoords2(const WDL_TypedBuf<BL_FLOAT> magns[2],
                          WDL_TypedBuf<BL_FLOAT> *coords)
{
    coords->Resize(1);
    
    BL_FLOAT sumMagns[2];
    sumMagns[0] = BLUtils::ComputeSum(magns[0]);
    sumMagns[1] = BLUtils::ComputeSum(magns[1]);
    
    //
    BL_FLOAT l = sumMagns[0];
    BL_FLOAT r = sumMagns[1];
        
    BL_FLOAT angle = std::atan2(r, l);
        
    // Adjust
    angle = -angle;
        
    BL_FLOAT normAngle = (angle + M_PI/2.0)/M_PI;
        
    // Adjust
    normAngle *= 2.0;
        
    coords->Get()[0] = normAngle;
}

void
BatFftObj::ComputeAmplitudes2(const WDL_TypedBuf<BL_FLOAT> &magns,
                              const WDL_TypedBuf<BL_FLOAT> &scMagns,
                              WDL_TypedBuf<BL_FLOAT> *amps)
{
    BL_FLOAT sumMagns = BLUtils::ComputeSum(magns);
    BL_FLOAT sumScMagns = BLUtils::ComputeSum(scMagns);
    
    BL_FLOAT sum = sumMagns + sumScMagns;
    
    amps->Resize(1);
    amps->Get()[0] = sum;
}

void
BatFftObj::ComputeCoords3(const WDL_TypedBuf<BL_FLOAT> magns[2],
                          const WDL_TypedBuf<BL_FLOAT> phases[2],
                          WDL_TypedBuf<BL_FLOAT> *coords)
{
    //
    WDL_TypedBuf<BL_FLOAT> freqs;
    BLUtilsFft::FftFreqs(&freqs, phases[0].GetSize(), mSampleRate);
    
    WDL_TypedBuf<BL_FLOAT> timeDelays;
    BLUtils::ComputeTimeDelays(&timeDelays, phases[0], phases[1], mSampleRate);
    
    WDL_TypedBuf<BL_FLOAT> sourceRs;
    WDL_TypedBuf<BL_FLOAT> sourceThetas;
    BLUtils::ResizeFillZeros(&sourceThetas, magns[0].GetSize());
    
    SourcePos::MagnPhasesToSourcePos(&sourceRs, &sourceThetas,
                                     magns[0],  magns[1],
                                     phases[0], phases[1],
                                     freqs,
                                     timeDelays);
    
    *coords = sourceThetas;
    
    for (int i = 0; i < coords->GetSize(); i++)
    {
        BL_FLOAT coord = coords->Get()[i];
        
        coord *= 1.0/M_PI;
        
        if (coord < 0.0)
            coord = 0.0;
        if (coord > 1.0)
            coord = 1.0;
        
        coords->Get()[i] = coord;
    }
}

void
BatFftObj::ComputeAmplitudes3(const WDL_TypedBuf<BL_FLOAT> &magns,
                              //const WDL_TypedBuf<BL_FLOAT> &phases,
                              const WDL_TypedBuf<BL_FLOAT> &scMagns,
                              //const WDL_TypedBuf<BL_FLOAT> &scPhases,
                              WDL_TypedBuf<BL_FLOAT> *amps)
{
    *amps = magns;
    BLUtils::AddValues(amps, scMagns);
    
    // TEST
    //BLUtils::FillAllValue(amps, 0.01);
}

void
BatFftObj::ApplySharpnessXY(vector<WDL_TypedBuf<BL_FLOAT> > *lines)
{
    ApplySharpness(lines);
    
    ReverseXY(lines);
    
    ApplySharpness(lines);
    
    ReverseXY(lines);
}

void
BatFftObj::ReverseXY(vector<WDL_TypedBuf<BL_FLOAT> > *lines)
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
BatFftObj::ApplySharpness(vector<WDL_TypedBuf<BL_FLOAT> > *lines)
{
    for (int i = 0; i < lines->size(); i++)
    {
        WDL_TypedBuf<BL_FLOAT> &line = (*lines)[i];
        
        ApplySharpness(&line);
    }
}

void
BatFftObj::ApplySharpness(WDL_TypedBuf<BL_FLOAT> *line)
{
    // Initial, leaks a lot
    int divisor = 12;
    divisor = (1.0 - mSharpness)*MIN_SMOOTH_DIVISOR + mSharpness*MAX_SMOOTH_DIVISOR;
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
BatFftObj::TimeSmooth(vector<WDL_TypedBuf<BL_FLOAT> > *lines)
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
