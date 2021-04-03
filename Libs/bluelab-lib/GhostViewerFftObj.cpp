//
//  GhostViewerFftObj.cpp
//  BL-GhostViewer
//
//  Created by Pan on 02/06/18.
//
//

#ifdef IGRAPHICS_NANOVG

#include <BLSpectrogram4.h>

#include <BLUtils.h>
#include <BLUtilsComp.h>
#include <BLUtilsFft.h>
#include <BLUtilsMath.h>

#include <BLDebug.h>

#include <SpectrogramDisplay.h>
//#include <SpectrogramDisplayScroll3.h>
#include <SpectrogramDisplayScroll4.h>

#include "GhostViewerFftObj.h"

#define USE_AVG_LINES 0 //1

GhostViewerFftObj::GhostViewerFftObj(int bufferSize, int oversampling, int freqRes,
                                     BL_FLOAT sampleRate)
: ProcessObj(bufferSize)
{
    mSpectrogram = new BLSpectrogram4(sampleRate, bufferSize/4, -1);
    mSpectroDisplay = NULL;
    
    ProcessObj::Reset(bufferSize, oversampling, freqRes, sampleRate);
    
    mLineCount = 0;
    
    mSpeedMod = 1;
}

GhostViewerFftObj::~GhostViewerFftObj()
{
    delete mSpectrogram;
}

void
GhostViewerFftObj::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer0,
                                    const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer)
{
    WDL_TypedBuf<WDL_FFT_COMPLEX> &ioBuffer = mTmpBuf0;
    BLUtils::TakeHalf(*ioBuffer0, &ioBuffer);
    
    WDL_TypedBuf<BL_FLOAT> &magns = mTmpBuf1;
    WDL_TypedBuf<BL_FLOAT> &phases = mTmpBuf2;
    BLUtilsComp::ComplexToMagnPhase(&magns, &phases, ioBuffer);
    
    if (mLineCount % mSpeedMod == 0)
        AddSpectrogramLine(magns, phases);
    
    mLineCount++;
    
    //BLUtils::ResizeFillZeros(ioBuffer, ioBuffer->GetSize()*2);
    BLUtils::SetBuf(ioBuffer0, ioBuffer);
    BLUtilsFft::FillSecondFftHalf(ioBuffer0);
}

void
GhostViewerFftObj::Reset(int bufferSize, int oversampling,
                         int freqRes, BL_FLOAT sampleRate)
{
    ProcessObj::Reset(bufferSize, oversampling, freqRes, sampleRate);
    
    // ORIGIN
    //int numCols = mBufferSize/8;
    
    // TODO: modify this in GhostViewer too
    // NEW
    // Prefer this, so the scroll speed won't be modified when
    // the overlapping changes
    int numCols = mBufferSize/(32/mOverlapping);
    
    // Adjust to the sample rate to avoid scrolling
    // 2 times faster when we go from 44100 to 88200
    //
    BL_FLOAT srCoeff = sampleRate/44100.0;
    srCoeff = bl_round(srCoeff);
    numCols *= srCoeff;
    
    mSpectrogram->Reset(mSampleRate, mBufferSize/4, numCols);
    
    mLineCount = 0;
    
    mOverlapLines.clear();
}

BLSpectrogram4 *
GhostViewerFftObj::GetSpectrogram()
{
    return mSpectrogram;
}

void
GhostViewerFftObj::SetSpectrogramDisplay(SpectrogramDisplayScroll4 *spectroDisplay)
{
    mSpectroDisplay = spectroDisplay;
}

void
GhostViewerFftObj::SetSpeedMod(int speedMod)
{
    mSpeedMod = speedMod;
}

void
GhostViewerFftObj::AddSpectrogramLine(const WDL_TypedBuf<BL_FLOAT> &magns,
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
    /*mOverlapLines.push_back(magns);
    if (mOverlapLines.size() > maxLines)
    mOverlapLines.pop_front();*/
    if (mOverlapLines.size() < maxLines)
        mOverlapLines.push_back(magns);
    else
    {
        mOverlapLines.freeze();
        mOverlapLines.push_pop(magns);
    }
    
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
        mSpectroDisplay->AddSpectrogramLine(magns, phases);
    }
    else
        mSpectrogram->AddLine(magns, phases);
#endif
}

#endif // IGRAPHICS_NANOVG
