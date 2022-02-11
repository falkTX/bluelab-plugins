//
//  GhostViewerFftObj.cpp
//  BL-GhostViewer
//
//  Created by Pan on 02/06/18.
//
//

#include <BLSpectrogram3.h>
#include <BLUtils.h>

// #bl-iplug2
//#include "nanovg.h"

#include <SpectrogramDisplay.h>
#include <SpectrogramDisplayScroll.h>

#include "GhostViewerFftObj.h"

#define USE_AVG_LINES 0 //1

GhostViewerFftObj::GhostViewerFftObj(int bufferSize, int oversampling, int freqRes,
                                     BL_FLOAT sampleRate)
: ProcessObj(bufferSize)
{
    mSpectrogram = new BLSpectrogram3(bufferSize/4, -1);
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
GhostViewerFftObj::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                                    const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer)
{
    BLUtils::TakeHalf(ioBuffer);
    
    WDL_TypedBuf<BL_FLOAT> magns;
    WDL_TypedBuf<BL_FLOAT> phases;
    BLUtils::ComplexToMagnPhase(&magns, &phases, *ioBuffer);
    
    if (mLineCount % mSpeedMod == 0)
        AddSpectrogramLine(magns, phases);
    
    mLineCount++;
    
    BLUtils::ResizeFillZeros(ioBuffer, ioBuffer->GetSize()*2);
    BLUtils::FillSecondFftHalf(ioBuffer);
}

void
GhostViewerFftObj::Reset(int bufferSize, int oversampling, int freqRes, BL_FLOAT sampleRate)
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
    
    mSpectrogram->Reset(mBufferSize/4, numCols);
    
    mLineCount = 0;
    
    mOverlapLines.clear();
}

BLSpectrogram3 *
GhostViewerFftObj::GetSpectrogram()
{
    return mSpectrogram;
}

#if USE_SPECTRO_SCROLL
void
GhostViewerFftObj::SetSpectrogramDisplay(SpectrogramDisplayScroll *spectroDisplay)
{
    mSpectroDisplay = spectroDisplay;
}
#else
void
GhostViewerFftObj::SetSpectrogramDisplay(SpectrogramDisplay *spectroDisplay)
{
    mSpectroDisplay = spectroDisplay;
}

#endif

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
