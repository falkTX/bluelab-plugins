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
//  InfrasonicViewerFftObj2.cpp
//  BL-GhostViewer
//
//  Created by Pan on 02/06/18.
//
//

#ifdef IGRAPHICS_NANOVG

#include <BLSpectrogram4.h>
#include <SpectrogramDisplay.h>
#include <SpectrogramDisplayScroll4.h>

#include <BLUtils.h>
#include <BLUtilsComp.h>
#include <BLUtilsFft.h>

#include "InfrasonicViewerFftObj2.h"


#define USE_AVG_LINES 0 //1

// Spectrogram accuracy is far better accurate without decimation
// => Set to 0 !
//
// Decimate the frequencies ?
#define DECIMATE_FREQUENCIES 0


InfrasonicViewerFftObj2::InfrasonicViewerFftObj2(int bufferSize,
                                               int oversampling,
                                               int freqRes,
                                               BL_FLOAT sampleRate)
: ProcessObj(bufferSize)
{
    mMaxFreq = 100.0;
    
    int lastBin = ComputeLastBin(mMaxFreq);
    
#if DECIMATE_FREQUENCIES
    mSpectrogram = new BLSpectrogram4(sampleRate, lastBin/4, -1);
#else
    mSpectrogram = new BLSpectrogram4(sampleRate, lastBin, -1);
#endif
    
    mSpectroDisplay = NULL;
    
    ProcessObj::Reset(bufferSize, oversampling, freqRes, sampleRate);
    
    mLineCount = 0;
    
    mTimeWindowSec = 5.0;
}

InfrasonicViewerFftObj2::~InfrasonicViewerFftObj2()
{
    delete mSpectrogram;
}

void
InfrasonicViewerFftObj2::
ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer0,
                 const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer)
{    
    //BLUtils::TakeHalf(ioBuffer);
    WDL_TypedBuf<WDL_FFT_COMPLEX> &ioBuffer = mTmpBuf0;
    BLUtils::TakeHalf(*ioBuffer0, &ioBuffer);

#if 0 // ORIGIN
    WDL_TypedBuf<BL_FLOAT> &magns0 = mTmpBuf1;
    WDL_TypedBuf<BL_FLOAT> &phases0 = mTmpBuf2;
    BLUtilsComp::ComplexToMagnPhase(&magns0, &phases0, ioBuffer);

    WDL_TypedBuf<BL_FLOAT> &magns = mTmpBuf5;
    WDL_TypedBuf<BL_FLOAT> &phases = mTmpBuf6;
    
    SelectSubSonic(magns0, phases0, &magns, &phases);
#endif

#if 1 // OPTIM
      // Reduce the number of data before computing magns and phases
    WDL_TypedBuf<WDL_FFT_COMPLEX> &ioBuffer1 = mTmpBuf7;
    SelectSubSonic(ioBuffer, &ioBuffer1);

    WDL_TypedBuf<BL_FLOAT> &magns = mTmpBuf8;
    WDL_TypedBuf<BL_FLOAT> &phases = mTmpBuf9;
    BLUtilsComp::ComplexToMagnPhase(&magns, &phases, ioBuffer1);
#endif
    
    AddSpectrogramLine(magns, phases);
    
    mLineCount++;

#if 0 // No need, we don't modify the sound
    //BLUtils::ResizeFillZeros(ioBuffer, ioBuffer->GetSize()*2);
    BLUtils::SetBuf(ioBuffer0, ioBuffer);
    BLUtilsFft::FillSecondFftHalf(ioBuffer0);
#endif
}

#if 0 // ORIGIN
void
InfrasonicViewerFftObj2::Reset(int bufferSize, int oversampling,
                               int freqRes, BL_FLOAT sampleRate)
{
    ProcessObj::Reset(bufferSize, oversampling, freqRes, sampleRate);
    
    int lastBin = ComputeLastBin(mMaxFreq);
    
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
    
#if DECIMATE_FREQUENCIES // ORIG
    // Values will be decimated later when added to the spectrogram
    mSpectrogram->Reset(mSampleRate, lastBin/4, numCols);
#else // More smooth but less accurate
    mSpectrogram->Reset(mSampleRate, lastBin, numCols);
#endif
    
    // NOTE not tested, but should be activated,
    // otherwise the SpectrogramDisplayScroll would be misconfigured
    // (for the moment, makes jitter InfrssonicViewer)
#if 0
    // Update the spectrogram in the scroll obj
    if (mSpectroDisplay != NULL)
        mSpectroDisplay->SetSpectrogram(mSpectrogram, 0.0, 0.0, 1.0, 1.0);
#endif
    
    mLineCount = 0;
    
    mOverlapLines.clear();
}
#endif

#if 1 // NEW
void
InfrasonicViewerFftObj2::Reset(int bufferSize, int oversampling,
                               int freqRes, BL_FLOAT sampleRate,
                               BL_FLOAT timeWindowSec)
{
    mTimeWindowSec = timeWindowSec;
    
    ProcessObj::Reset(bufferSize, oversampling, freqRes, sampleRate);
    
    int lastBin = ComputeLastBin(mMaxFreq);
    
    BL_FLOAT numSamples = timeWindowSec*sampleRate;
    int numCols = ceil(numSamples/(((BL_FLOAT)bufferSize)/oversampling));

#if DECIMATE_FREQUENCIES // ORIG
    // Values will be decimated later when added to the spectrogram
    mSpectrogram->Reset(mSampleRate, lastBin/4, numCols);
#else // More smooth but less accurate
    mSpectrogram->Reset(mSampleRate, lastBin, numCols);
#endif
    
#if 1 // Shoud be activated !
      // Update the spectrogram in the scroll obj
    if (mSpectroDisplay != NULL)
        mSpectroDisplay->SetSpectrogram(mSpectrogram, 0.0, 0.0, 1.0, 1.0);
#endif
    
    mLineCount = 0;
    
    mOverlapLines.clear();
}
#endif

BLSpectrogram4 *
InfrasonicViewerFftObj2::GetSpectrogram()
{
    return mSpectrogram;
}

#if USE_SPECTRO_SCROLL
void
InfrasonicViewerFftObj2::
SetSpectrogramDisplay(SpectrogramDisplayScroll4 *spectroDisplay)
{
    mSpectroDisplay = spectroDisplay;
}
#else
void
InfrasonicViewerFftObj2::SetSpectrogramDisplay(SpectrogramDisplay *spectroDisplay)
{
    mSpectroDisplay = spectroDisplay;
}

#endif

BL_FLOAT
InfrasonicViewerFftObj2::GetMaxFreq()
{
    int lastBin = ComputeLastBin(mMaxFreq);
    
    BL_FLOAT hzPerBin = ((BL_FLOAT)mSampleRate)/mBufferSize;
    
    BL_FLOAT maxFreq = lastBin*hzPerBin;
    
    return maxFreq;
}

void
InfrasonicViewerFftObj2::SetMaxFreq(BL_FLOAT freq)
{
    mMaxFreq = freq;
    
    Reset(mBufferSize, mOverlapping, mFreqRes, mSampleRate, mTimeWindowSec);
}

void
InfrasonicViewerFftObj2::SetTimeWindowSec(BL_FLOAT timeWindowSec)
{
    mTimeWindowSec = timeWindowSec;
    
    Reset(mBufferSize, mOverlapping, mFreqRes, mSampleRate, mTimeWindowSec);
}

void
InfrasonicViewerFftObj2::AddSpectrogramLine(const WDL_TypedBuf<BL_FLOAT> &magns,
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
    WDL_TypedBuf<BL_FLOAT> &line = mTmpBuf3;
    BLUtils::ResizeFillZeros(&line, magns.GetSize());
    
    for (int i = 0; i < mOverlapLines.size(); i++)
    {
        WDL_TypedBuf<BL_FLOAT> &currentLine = mTmpBuf4;
        currentLine = mOverlapLines[i];
        
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
InfrasonicViewerFftObj2::SelectSubSonic(const WDL_TypedBuf<BL_FLOAT> &inMagns,
                                        const WDL_TypedBuf<BL_FLOAT> &inPhases,
                                        WDL_TypedBuf<BL_FLOAT> *outMagns,
                                        WDL_TypedBuf<BL_FLOAT> *outPhases)
{
    // 100 Hz max (??)
    int lastBin = ComputeLastBin(mMaxFreq);
    
    outMagns->Resize(lastBin);
    outPhases->Resize(lastBin);

    BLUtils::SetBuf(outMagns, inMagns);
    BLUtils::SetBuf(outPhases, inPhases);
}

void
InfrasonicViewerFftObj2::SelectSubSonic(const WDL_TypedBuf<WDL_FFT_COMPLEX> &inData,
                                        WDL_TypedBuf<WDL_FFT_COMPLEX> *outData)
{
    // 100 Hz max (??)
    int lastBin = ComputeLastBin(mMaxFreq);
    
    outData->Resize(lastBin);

    BLUtils::SetBuf(outData, inData);
}

int
InfrasonicViewerFftObj2::ComputeLastBin(BL_FLOAT freq)
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
