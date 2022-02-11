//
//  InfrasonicViewerFftObj2.cpp
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
#include <SpectrogramDisplayScroll2.h>

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
    mSpectrogram = new BLSpectrogram3(lastBin/4, -1);
#else
    mSpectrogram = new BLSpectrogram3(lastBin, -1);
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
InfrasonicViewerFftObj2::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                                    const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer)
{
    BLUtils::TakeHalf(ioBuffer);
    
    WDL_TypedBuf<BL_FLOAT> magns;
    WDL_TypedBuf<BL_FLOAT> phases;
    BLUtils::ComplexToMagnPhase(&magns, &phases, *ioBuffer);
    
    SelectSubSonic(&magns, &phases);
    
    AddSpectrogramLine(magns, phases);
    
    mLineCount++;
    
    BLUtils::ResizeFillZeros(ioBuffer, ioBuffer->GetSize()*2);
    BLUtils::FillSecondFftHalf(ioBuffer);
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
    mSpectrogram->Reset(lastBin/4, numCols);
#else // More smooth but less accurate
    mSpectrogram->Reset(lastBin, numCols);
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
    ProcessObj::Reset(bufferSize, oversampling, freqRes, sampleRate);
    
    int lastBin = ComputeLastBin(mMaxFreq);
    
    BL_FLOAT numSamples = timeWindowSec*sampleRate;
    int numCols = ceil(numSamples/(((BL_FLOAT)bufferSize)/oversampling));

#if DECIMATE_FREQUENCIES // ORIG
    // Values will be decimated later when added to the spectrogram
    mSpectrogram->Reset(lastBin/4, numCols);
#else // More smooth but less accurate
    mSpectrogram->Reset(lastBin, numCols);
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

BLSpectrogram3 *
InfrasonicViewerFftObj2::GetSpectrogram()
{
    return mSpectrogram;
}

#if USE_SPECTRO_SCROLL
void
InfrasonicViewerFftObj2::SetSpectrogramDisplay(SpectrogramDisplayScroll2 *spectroDisplay)
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
InfrasonicViewerFftObj2::SelectSubSonic(WDL_TypedBuf<BL_FLOAT> *magns,
                                          WDL_TypedBuf<BL_FLOAT> *phases)
{
    // 100 Hz max
    int lastBin = ComputeLastBin(mMaxFreq);
    
    //fprintf(stderr, "last bin: %d\n", lastBin);
    
    magns->Resize(lastBin);
    phases->Resize(lastBin);
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
