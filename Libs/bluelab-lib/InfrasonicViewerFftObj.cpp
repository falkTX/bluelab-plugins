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
//  InfrasonicViewerFftObj.cpp
//  BL-GhostViewer
//
//  Created by Pan on 02/06/18.
//
//

#ifdef IGRAPHICS_NANOVG

#include <BLSpectrogram4.h>
#include <SpectrogramDisplay.h>
#include <SpectrogramDisplayScroll.h>

#include <BLUtils.h>
#include <BLUtilsComp.h>
#include <BLUtilsFft.h>
#include <BLUtilsMath.h>

#include "InfrasonicViewerFftObj.h"


#define USE_AVG_LINES 0 //1

// Decimate the frequencies ?
#define DECIMATE_FREQUENCIES 1

InfrasonicViewerFftObj::InfrasonicViewerFftObj(int bufferSize,
                                               int oversampling,
                                               int freqRes,
                                               BL_FLOAT sampleRate)
: ProcessObj(bufferSize)
{
    mMaxFreq = 100.0;
    
    int lastBin = ComputeLastBin(mMaxFreq);

    //mSpectrogram = new BLSpectrogram4(sampleRate, bufferSize/4, -1);
    //mSpectrogram = new BLSpectrogram4(sampleRate, lastBin/(2*oversampling)/*4*/, -1);
    mSpectrogram = new BLSpectrogram4(sampleRate, lastBin/4, -1);
    //mSpectrogram = new BLSpectrogram4(sampleRate, lastBin, -1);
    
    mSpectroDisplay = NULL;
    
    ProcessObj::Reset(bufferSize, oversampling, freqRes, sampleRate);
    
    mLineCount = 0;
    
    mSpeed = 1.0;
}

InfrasonicViewerFftObj::~InfrasonicViewerFftObj()
{
    delete mSpectrogram;
}

void
InfrasonicViewerFftObj::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                                    const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer)
{
    BLUtils::TakeHalf(ioBuffer);
    
    WDL_TypedBuf<BL_FLOAT> magns;
    WDL_TypedBuf<BL_FLOAT> phases;
    BLUtilsComp::ComplexToMagnPhase(&magns, &phases, *ioBuffer);
    
    SelectSubSonic(&magns, &phases);
    
    AddSpectrogramLine(magns, phases);
    
    mLineCount++;
    
    BLUtils::ResizeFillZeros(ioBuffer, ioBuffer->GetSize()*2);
    BLUtilsFft::FillSecondFftHalf(ioBuffer);
}

#if 1 //0 // ORIGIN
void
InfrasonicViewerFftObj::Reset(int bufferSize, int oversampling,
                              int freqRes, BL_FLOAT sampleRate)
{
    ProcessObj::Reset(bufferSize, oversampling, freqRes, sampleRate);
    
    int lastBin = ComputeLastBin(mMaxFreq);
    
    // ORIGIN
    //int numCols = mBufferSize/8;
    
    // TODO: modify this in GhostViewer too
    // NEW
    // Prefer this, so the scroll speed won't be modified when
    // the overlapping changes
    //int numCols = mBufferSize/(32/mOverlapping);
    int numCols = lastBin/(32.0/mOverlapping);
    
    // TEST SubSonic
    numCols *= 4/mSpeed;
    
#if 1 // HACK to avoid speed increase when choosing lower max freqs
    BL_FLOAT coeff = 100.0/mMaxFreq; // 100 is max freq in the plugin
    numCols *= coeff;
#endif
    
    // Adjust to the sample rate to avoid scrolling
    // 2 times faster when we go from 44100 to 88200
    //
    BL_FLOAT srCoeff = sampleRate/44100.0;
    srCoeff = bl_round(srCoeff);
    numCols *= srCoeff;
    
    //mSpectrogram->Reset(mSampleRate, mBufferSize/4, numCols);
    //mSpectrogram->Reset(mSampleRate, lastBin/(2*mOverlapping)/*4*/, numCols);
   
    // TEST
    //fprintf(stderr, "maxFreq: %g numCols: %d\n", mMaxFreq, numCols);
    //numCols = 32; // TEST
    
    //fprintf(stderr, "lastBin: %d numCols: %d\n", lastBin, numCols);
    
#if DECIMATE_FREQUENCIES // ORIG
    // Values will be decimated later when added to the spectrogram
    mSpectrogram->Reset(mSampleRate, lastBin/4, numCols);
#else // More smooth but less accurate
    mSpectrogram->Reset(mSampleRate, lastBin, numCols);
#endif
    
    mLineCount = 0;
    
    mOverlapLines.clear();
}
#endif

#if 0 //1 // NEW
void
InfrasonicViewerFftObj::Reset(int bufferSize, int oversampling,
                              int freqRes, BL_FLOAT sampleRate)
{
    // TEST
#define TIME_WINDOW 10.0 //11.8886167800454 //10.0 //11.85 //10.0 // 10 seconds
    
    ProcessObj::Reset(bufferSize, oversampling, freqRes, sampleRate);
    
    int lastBin = ComputeLastBin(mMaxFreq);
    
    BL_FLOAT numSamples = TIME_WINDOW*sampleRate;
    int numCols = ceil(numSamples/(((BL_FLOAT)bufferSize)/oversampling));
    
    // TEST
    //numCols = 32;
    
    // ORIGIN
    //int numCols = mBufferSize/8;
    
    // TODO: modify this in GhostViewer too
    // NEW
    // Prefer this, so the scroll speed won't be modified when
    // the overlapping changes
    //int numCols = mBufferSize/(32/mOverlapping);
    
    //int numCols = lastBin/(32.0/mOverlapping);
    
    // TEST SubSonic
    //numCols *= 4/mSpeed;
    
#if 1 // HACK to avoid speed increase when choosing lower max freqs
    //BL_FLOAT coeff = 100.0/mMaxFreq; // 100 is max freq in the plugin
    //numCols *= coeff;
#endif
    
    // Adjust to the sample rate to avoid scrolling
    // 2 times faster when we go from 44100 to 88200
    //
    //BL_FLOAT srCoeff = sampleRate/44100.0;
    //srCoeff = bl_round(srCoeff);
    //numCols *= srCoeff;
    
    //mSpectrogram->Reset(mSampleRate, mBufferSize/4, numCols);
    //mSpectrogram->Reset(mSampleRate, lastBin/(2*mOverlapping)/*4*/, numCols);
    
    // TEST
    //numCols = 32;
    
    // TEST
    //numCols = BLUtils::NextPowerOfTwo(numCols);
    
    // TEST
    //fprintf(stderr, "maxFreq: %g numCols: %d\n", mMaxFreq, numCols);
    //fprintf(stderr, "lastBin: %d numCols: %d\n", lastBin, numCols);

#if DECIMATE_FREQUENCIES // ORIG
    // Values will be decimated later when added to the spectrogram
    mSpectrogram->Reset(mSampleRate, lastBin/4, numCols);
#else // More smooth but less accurate
    mSpectrogram->Reset(mSampleRate, lastBin, numCols);
#endif
    
    mLineCount = 0;
    
    mOverlapLines.clear();
}
#endif

BLSpectrogram4 *
InfrasonicViewerFftObj::GetSpectrogram()
{
    return mSpectrogram;
}

#if USE_SPECTRO_SCROLL
void
InfrasonicViewerFftObj::SetSpectrogramDisplay(SpectrogramDisplayScroll *spectroDisplay)
{
    mSpectroDisplay = spectroDisplay;
}
#else
void
InfrasonicViewerFftObj::SetSpectrogramDisplay(SpectrogramDisplay *spectroDisplay)
{
    mSpectroDisplay = spectroDisplay;
}

#endif

BL_FLOAT
InfrasonicViewerFftObj::GetMaxFreq()
{
    int lastBin = ComputeLastBin(mMaxFreq);
    
    BL_FLOAT hzPerBin = ((BL_FLOAT)mSampleRate)/mBufferSize;
    
    BL_FLOAT maxFreq = lastBin*hzPerBin;
    
    return maxFreq;
}

void
InfrasonicViewerFftObj::SetMaxFreq(BL_FLOAT freq)
{
    mMaxFreq = freq;
    
    Reset(mBufferSize, mOverlapping, mFreqRes, mSampleRate);
}

void
InfrasonicViewerFftObj::SetSpeed(BL_FLOAT speed)
{
    mSpeed = speed;
    
    Reset(mBufferSize, mOverlapping, mFreqRes, mSampleRate);
}

void
InfrasonicViewerFftObj::AddSpectrogramLine(const WDL_TypedBuf<BL_FLOAT> &magns,
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
#if USE_SPECTRO_SCROLL
        mSpectroDisplay->AddSpectrogramLine(magns, phases);
#endif
    }
    else
        mSpectrogram->AddLine(magns, phases);
#endif
}

void
InfrasonicViewerFftObj::SelectSubSonic(WDL_TypedBuf<BL_FLOAT> *magns,
                                          WDL_TypedBuf<BL_FLOAT> *phases)
{
    // 100 Hz max
    int lastBin = ComputeLastBin(mMaxFreq);
    
    //fprintf(stderr, "last bin: %d\n", lastBin);
    
    magns->Resize(lastBin);
    phases->Resize(lastBin);
}

int
InfrasonicViewerFftObj::ComputeLastBin(BL_FLOAT freq)
{
    BL_FLOAT hzPerBin = ((BL_FLOAT)mSampleRate)/mBufferSize;

    //int binNum = ceil(freq*hzPerBin);
    int binNum = ceil(freq/hzPerBin);
    
    int binNumPow2 = binNum;
    
    // BAD
    // Not necessary, and makes inaccuracy in axis
#if 0
    int binNumPow2 = BLUtils::NextPowerOfTwo(binNum);
    if (binNumPow2/2 == binNum)
        binNumPow2 /= 2;
#endif
    
    return binNumPow2;
}

#endif // IGRAPHICS_NANOVG
