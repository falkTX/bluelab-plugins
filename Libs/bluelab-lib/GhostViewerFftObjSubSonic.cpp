//
//  GhostViewerFftObjSubSonic.cpp
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

#include <SpectrogramDisplay.h>
#include <SpectrogramDisplayScroll.h>

#include "GhostViewerFftObjSubSonic.h"


#define MAX_FREQ 100.0

#define USE_AVG_LINES 0 //1


GhostViewerFftObjSubSonic::GhostViewerFftObjSubSonic(int bufferSize,
                                                     int oversampling,
                                                     int freqRes,
                                                     BL_FLOAT sampleRate)
: ProcessObj(bufferSize)
{
    int lastBin = ComputeLastBin(MAX_FREQ);

    //mSpectrogram = new BLSpectrogram4(sampleRate, bufferSize/4, -1);
    //mSpectrogram = new BLSpectrogram4(sampleRate, lastBin/(2*oversampling)/*4*/, -1);
    mSpectrogram = new BLSpectrogram4(sampleRate, lastBin/4, -1);
    //mSpectrogram = new BLSpectrogram4(sampleRate, lastBin, -1);
    
    mSpectroDisplay = NULL;
    
    ProcessObj::Reset(bufferSize, oversampling, freqRes, sampleRate);
    
    mLineCount = 0;
    
    mSpeed = 1.0;
}

GhostViewerFftObjSubSonic::~GhostViewerFftObjSubSonic()
{
    delete mSpectrogram;
}

void
GhostViewerFftObjSubSonic::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
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

void
GhostViewerFftObjSubSonic::Reset(int bufferSize, int oversampling,
                                 int freqRes, BL_FLOAT sampleRate)
{
    ProcessObj::Reset(bufferSize, oversampling, freqRes, sampleRate);
    
    int lastBin = ComputeLastBin(MAX_FREQ);
    
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
    
    // Adjust to the sample rate to avoid scrolling
    // 2 times faster when we go from 44100 to 88200
    //
    BL_FLOAT srCoeff = sampleRate/44100.0;
    srCoeff = bl_round(srCoeff);
    numCols *= srCoeff;
    
    //mSpectrogram->Reset(mSampleRate, mBufferSize/4, numCols);
    //mSpectrogram->Reset(mSampleRate, lastBin/(2*mOverlapping)/*4*/, numCols);
    mSpectrogram->Reset(mSampleRate, lastBin/4, numCols);
    //mSpectrogram->Reset(mSampleRate, lastBin, numCols);
    
    mLineCount = 0;
    
    mOverlapLines.clear();
}

BLSpectrogram4 *
GhostViewerFftObjSubSonic::GetSpectrogram()
{
    return mSpectrogram;
}

#if USE_SPECTRO_SCROLL
void
GhostViewerFftObjSubSonic::SetSpectrogramDisplay(SpectrogramDisplayScroll *spectroDisplay)
{
    mSpectroDisplay = spectroDisplay;
}
#else
void
GhostViewerFftObjSubSonic::SetSpectrogramDisplay(SpectrogramDisplay *spectroDisplay)
{
    mSpectroDisplay = spectroDisplay;
}

#endif

BL_FLOAT
GhostViewerFftObjSubSonic::GetMaxFreq()
{
    int lastBin = ComputeLastBin(MAX_FREQ);
    
    BL_FLOAT hzPerBin = ((BL_FLOAT)mSampleRate)/mBufferSize;
    
    BL_FLOAT maxFreq = lastBin*hzPerBin;
    
    return maxFreq;
}

void
GhostViewerFftObjSubSonic::SetSpeed(BL_FLOAT speed)
{
    mSpeed = speed;
    
    Reset(mBufferSize, mOverlapping, mFreqRes, mSampleRate);
}

void
GhostViewerFftObjSubSonic::AddSpectrogramLine(const WDL_TypedBuf<BL_FLOAT> &magns,
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
GhostViewerFftObjSubSonic::SelectSubSonic(WDL_TypedBuf<BL_FLOAT> *magns,
                                          WDL_TypedBuf<BL_FLOAT> *phases)
{
    // 100 Hz max
    int lastBin = ComputeLastBin(MAX_FREQ);
    
    magns->Resize(lastBin);
    phases->Resize(lastBin);
}

int
GhostViewerFftObjSubSonic::ComputeLastBin(BL_FLOAT freq)
{
    BL_FLOAT hzPerBin = ((BL_FLOAT)mSampleRate)/mBufferSize;
    int binNum = ceil(freq*hzPerBin);
    
    int binNumPow2 = BLUtilsMath::NextPowerOfTwo(binNum);
    if (binNumPow2/2 == binNum)
        binNumPow2 /= 2;
    
    return binNumPow2;
}

#endif // IGRAPHICS_NANOVG
