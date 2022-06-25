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
//  SpectroExpeFftObj.cpp
//  BL-SpectroExpe
//
//  Created by Pan on 02/06/18.
//
//

#ifdef IGRAPHICS_NANOVG

#include <BLSpectrogram4.h>

#include <BLUtils.h>
#include <BLUtilsComp.h>
#include <BLUtilsMath.h>

#include <BLDebug.h>

#include <SpectrogramDisplay.h>
#include <SpectrogramDisplayScroll3.h>

#include <PanogramFftObj.h>
#include <HistoMaskLine2.h>
#include <ChromaFftObj2.h>

#include <StereoWidenProcess.h>

#include "SpectroExpeFftObj.h"

#define USE_AVG_LINES 0 //1

#define PANOGRAM_FREQ_FACTOR 0.8
// 100 is good for spectro value dB scale
//#define STEREO_WIDTH_FACTOR 100.0 //1000.0
// 100 is good for spectro value linear scale
#define STEREO_WIDTH_FACTOR 1000.0

#define DUET_MAGNS_FACTOR 0.8
#define DUET_PHASES_FACTOR 5000.0 //1000.0

SpectroExpeFftObj::SpectroExpeFftObj(int bufferSize, int oversampling,
                                     int freqRes, BL_FLOAT sampleRate)
: MultichannelProcess()
{
    mBufferSize = bufferSize;
    mSampleRate = sampleRate;
    
    mSpectrogram = new BLSpectrogram4(sampleRate, bufferSize/4, -1);
    mSpectroDisplay = NULL;
    
    MultichannelProcess::Reset(bufferSize, oversampling, freqRes, sampleRate);
    
    mLineCount = 0;
    
    mSpeedMod = 1;
    
    mMode = SPECTROGRAM;

    mPanogramObj = new PanogramFftObj(bufferSize, oversampling, freqRes, sampleRate);
    mChromaObj = new ChromaFftObj2(bufferSize, oversampling, freqRes, sampleRate);
    mChromaObj->SetSharpness(1.0); // Better than 0.0
}

SpectroExpeFftObj::~SpectroExpeFftObj()
{
    delete mSpectrogram;
    delete mPanogramObj;
    delete mChromaObj;
}

void
SpectroExpeFftObj::
ProcessInputFft(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > *ioFftSamples,
                const vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > *scBuffer)
{
    WDL_TypedBuf<WDL_FFT_COMPLEX> fftSamples[2];
    fftSamples[0] = *(*ioFftSamples)[0];
    fftSamples[1] = *(*ioFftSamples)[1];
    
    WDL_TypedBuf<BL_FLOAT> magns[2];
    WDL_TypedBuf<BL_FLOAT> phases[2];
    
    for (int i = 0; i < 2; i++)
    {
        BLUtils::TakeHalf(&fftSamples[i]);
        BLUtilsComp::ComplexToMagnPhase(&magns[i], &phases[i], fftSamples[i]);
    }

    if (mLineCount % mSpeedMod == 0)
    {
      if (mMode == SPECTROGRAM)
          AddSpectrogramLine(magns[0], phases[0]);
      else if (mMode == PANOGRAM)
      {
          WDL_TypedBuf<BL_FLOAT> panoLine;
          mPanogramObj->MagnsToPanoLine(magns, &panoLine);

          AddSpectrogramLine(panoLine, phases[0]);
      }
      else if (mMode == PANOGRAM_FREQ)
      {
          WDL_TypedBuf<BL_FLOAT> panoFreqLine;
          ComputePanoFreqLine(magns, &panoFreqLine);
          
          BLUtils::MultValues(&panoFreqLine, (BL_FLOAT)PANOGRAM_FREQ_FACTOR);
          
          AddSpectrogramLine(panoFreqLine, phases[0]);
      }
      else if (mMode == CHROMAGRAM)
      {
          WDL_TypedBuf<BL_FLOAT> chromaLine;
          mChromaObj->MagnsToChromaLine(magns[0], phases[0], &chromaLine);

          AddSpectrogramLine(chromaLine, phases[0]);
      }
      else if (mMode == CHROMAGRAM_FREQ)
      {
          WDL_TypedBuf<BL_FLOAT> chromaFreqLine;
          ComputeChromaFreqLine(magns, phases, &chromaFreqLine);
          AddSpectrogramLine(chromaFreqLine, phases[0]);
      }
      else if (mMode == STEREO_WIDTH)
      {
          WDL_TypedBuf<BL_FLOAT> width;
          StereoWidenProcess::ComputeStereoWidth(magns, phases, &width);
          
          BLUtils::MultValues(&width, (BL_FLOAT)STEREO_WIDTH_FACTOR);

          AddSpectrogramLine(width, phases[0]);
      }
      else if (mMode == DUET_MAGNS)
      {
          WDL_TypedBuf<BL_FLOAT> duetMagnsLine;
          ComputeDuetMagns(magns, &duetMagnsLine);
          
          BLUtils::MultValues(&duetMagnsLine, (BL_FLOAT)DUET_MAGNS_FACTOR);
          
          AddSpectrogramLine(duetMagnsLine, phases[0]);
      }
      else if (mMode == DUET_PHASES)
      {
          WDL_TypedBuf<BL_FLOAT> duetPhasesLine;
          ComputeDuetPhases(magns, &duetPhasesLine);
          
          //BLUtils::MultValues(&duetPhasesLine, (BL_FLOAT)DUET_PHASES_FACTOR);
          
          AddSpectrogramLine(duetPhasesLine, phases[0]);
      }
    }
    
    mLineCount++;
}

void
SpectroExpeFftObj::Reset(int bufferSize, int oversampling,
                         int freqRes, BL_FLOAT sampleRate)
{
    MultichannelProcess::Reset(bufferSize, oversampling,
                               freqRes, sampleRate);
    
    mBufferSize = bufferSize;
    mSampleRate = sampleRate;
    
    // ORIGIN
    //int numCols = mBufferSize/8;
    
    // TODO: modify this in SpectroExpe too
    // NEW
    // Prefer this, so the scroll speed won't be modified when
    // the overlapping changes
    int numCols = bufferSize/(32/oversampling);
    
    // Adjust to the sample rate to avoid scrolling
    // 2 times faster when we go from 44100 to 88200
    //
    BL_FLOAT srCoeff = sampleRate/44100.0;
    srCoeff = bl_round(srCoeff);
    numCols *= srCoeff;
    
    mSpectrogram->Reset(sampleRate, bufferSize/4, numCols);
    
    mLineCount = 0;
    
    mOverlapLines.clear();

    mPanogramObj->Reset(bufferSize, oversampling, freqRes, sampleRate);
    mChromaObj->Reset(bufferSize, oversampling, freqRes, sampleRate);;
}

BLSpectrogram4 *
SpectroExpeFftObj::GetSpectrogram()
{
    return mSpectrogram;
}

void
SpectroExpeFftObj::SetSpectrogramDisplay(SpectrogramDisplayScroll3 *spectroDisplay)
{
    mSpectroDisplay = spectroDisplay;
}

void
SpectroExpeFftObj::SetSpeedMod(int speedMod)
{
    mSpeedMod = speedMod;
}

void
SpectroExpeFftObj::SetMode(Mode mode)
{
    mMode = mode;
}

void
SpectroExpeFftObj::AddSpectrogramLine(const WDL_TypedBuf<BL_FLOAT> &magns,
                                      const WDL_TypedBuf<BL_FLOAT> &phases)
{
#if USE_AVG_LINES
    
#define USE_SIMPLE_AVG 1

    // Take the 4 last lines at the maximum
    // (otherwise it is too blurry)
#define MAX_LINES 2 //4
    
    int maxLines = mOverlapping;
    if (maxLines > MAX_LINES)
        maxLines = MAX_LINES;
        
    // Keep track of previous lines
    // For correctly display, with overlapping
    mOverlapLines.push_back(magns);
    if (mOverlapLines.size() > maxLines)
        mOverlapLines.pop_front();
    
    // Simply make the average of the previous lines
    WDL_TypedBuf<BL_FLOAT> line;
    BLUtils::ResizeFillZeros(&line, magns.GetSize());
    
    for (int i = 0; i < mOverlapLines.size(); i++)
    {
        WDL_TypedBuf<BL_FLOAT> currentLine = mOverlapLines[i];
        
#if !USE_SIMPLE_AVG
        // Multiply by a coeff to smooth
        BL_FLOAT coeff = (i + 1.0)/mOverlapLines.size();
        
        // Adjust, for MAX_LINES == 4
        coeff /= 2.5;
        
        BLUtils::MultValues(&currentLine, coeff);
#endif
        
        BLUtils::AddValues(&line, currentLine);
    }
    
#if USE_SIMPLE_AVG
    if (mOverlapping > 0)
        // Just in case
        BLUtils::MultValues(&line, 1.0/maxLines);
#endif
    
    mSpectrogram->AddLine(line, phases);

#else // Simple add
    if (mSpectroDisplay != NULL)
    {
        mSpectroDisplay->AddSpectrogramLine(magns, phases);
    }
    else
        mSpectrogram->AddLine(magns, phases);
#endif
}

void
SpectroExpeFftObj::ComputePanoFreqLine(const WDL_TypedBuf<BL_FLOAT> magns[2],
                                       WDL_TypedBuf<BL_FLOAT> *panoFreqLine)
{
    WDL_TypedBuf<BL_FLOAT> panoLine;
    HistoMaskLine2 maskLine(magns[0].GetSize()*2);
    mPanogramObj->MagnsToPanoLine(magns, &panoLine, &maskLine);
    
    panoFreqLine->Resize(panoLine.GetSize());
    BLUtils::FillAllZero(panoFreqLine);
    
    vector<int> maskValues;
    maskLine.GetValues(&maskValues);
    
    for (int i = 0; i < maskValues.size(); i++)
    {
        int idx = maskValues[i];
        if (idx == -1)
            continue;
        
        BL_FLOAT t = 0.0;
        if (maskValues.size() > 1)
            t = ((BL_FLOAT)idx)/(maskValues.size() - 1);
        
        panoFreqLine->Get()[i] = t;
    }
}

void
SpectroExpeFftObj::ComputeChromaFreqLine(const WDL_TypedBuf<BL_FLOAT> magns[2],
                                         const WDL_TypedBuf<BL_FLOAT> phases[2],
                                         WDL_TypedBuf<BL_FLOAT> *chromaFreqLine)
{
#if 0 // TEST: align the tune to the max amp frequency
    // Find the max freq
    int maxAmpIdx = BLUtils::FindMaxIndex(magns[0]);
    BL_FLOAT hzPerBin = mSampleRate/mBufferSize;
    BL_FLOAT maxFreq = maxAmpIdx*hzPerBin;
    while(maxFreq > 50.0)
        maxFreq *= 0.5;
    mChromaObj->SetATune(maxFreq);
#endif
    
    WDL_TypedBuf<BL_FLOAT> chromaLine;
    HistoMaskLine2 maskLine(magns[0].GetSize()*2);
    mChromaObj->MagnsToChromaLine(magns[0], phases[0], &chromaLine, &maskLine);
    
    chromaFreqLine->Resize(chromaLine.GetSize());
    //BLUtils::FillAllZero(chromaFreqLine);
    BLUtils::FillAllValue(chromaFreqLine, (BL_FLOAT)-1.0);
    
    vector<int> maskValues;
    maskLine.GetValues(&maskValues);
    
    for (int i = 0; i < maskValues.size(); i++)
    {
        int idx = maskValues[i];
        if (idx == -1)
            continue;
        
#if 0 // Just this => makes a chroma constant pattern
        BL_FLOAT t = 0.0;
        if (maskValues.size() > 1)
            t = ((BL_FLOAT)idx)/(maskValues.size() - 1);
        
        chromaFreqLine->Get()[i] = t;
#endif
        
#if 1 // Use chroma intensities, projected back
        if (chromaFreqLine->Get()[idx] < 0.0)
            chromaFreqLine->Get()[idx] = 0.0;
        chromaFreqLine->Get()[idx] += chromaLine.Get()[i];
#endif
    }
    
    // Fill holes
    BLUtils::FillMissingValues2(chromaFreqLine, true, (BL_FLOAT)-1.0);
}

void
SpectroExpeFftObj::ComputeDuetMagns(WDL_TypedBuf<BL_FLOAT> magns[2],
				    WDL_TypedBuf<BL_FLOAT> *duetMagnsLine)
{
    duetMagnsLine->Resize(magns[0].GetSize());
    
  for (int i = 0; i < magns[0].GetSize(); i++)
  {
    // Alpha
    BL_FLOAT magn0 = magns[0].Get()[i];
    BL_FLOAT magn1 = magns[1].Get()[i];
        
    //
    BL_FLOAT alpha = 0.0;
    if ((magn0 > BL_EPS) && (magn1 > BL_EPS))
    {
      if (magn0 >= magn1)
      {
          alpha = magn1/magn0;
          
          alpha = ((1.0 - alpha) + 1.0)*0.5;
      }
            
      if (magn1 > magn0)
      {
          alpha = magn0/magn1;
          
          alpha = alpha*0.5;
      }
    }
    
    duetMagnsLine->Get()[i] = alpha;
  }
}

void
SpectroExpeFftObj::ComputeDuetPhases(WDL_TypedBuf<BL_FLOAT> phases[2],
				     WDL_TypedBuf<BL_FLOAT> *duetPhasesLine)
{
    duetPhasesLine->Resize(phases[0].GetSize());
    
  for (int i = 0; i < phases[0].GetSize(); i++)
  {
    // Delta
    BL_FLOAT phase0 = phases[0].Get()[i];
    BL_FLOAT phase1 = phases[1].Get()[i];
    
    BL_FLOAT delta = (phase0 - phase1)*(1.0/TWO_PI);
      
      delta = (delta*DUET_PHASES_FACTOR + 1.0)*0.5;
      
    duetPhasesLine->Get()[i] = delta;
  }       
}

#endif // IGRAPHICS_NANOVG
