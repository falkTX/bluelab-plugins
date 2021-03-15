//
//  ChromaFftObj2.cpp
//  BL-Chroma
//
//  Created by Pan on 02/06/18.
//
//

#ifdef IGRAPHICS_NANOVG

#include <BLSpectrogram4.h>
#include <Window.h>

#include <BLUtils.h>
#include <BLUtilsComp.h>
#include <BLUtilsMath.h>

#include <SpectrogramDisplayScroll3.h>
#include <HistoMaskLine2.h>

#include "ChromaFftObj2.h"

#if USE_FREQ_OBJ
#include <FreqAdjustObj3.h>
#endif

// 6 seems a bit limit for performances
// test: in debug, set shaprness to 0
// => this slows down and crackles
//#define MIN_SMOOTH_DIVISOR 6.0

#define MIN_SMOOTH_DIVISOR 12.0
#define MAX_SMOOTH_DIVISOR 48.0

#define USE_AVG_LINES 0 //1

// Take the 4 last lines at the maximum
// (otherwise it is too blurry)
#define MAX_LINES 2 //4

// No need to process output data, BL-Chroma doesn't modify the sound!
#define OPTIM_NO_RESYNTH 1

ChromaFftObj2::ChromaFftObj2(int bufferSize, int oversampling, int freqRes,
                             BL_FLOAT sampleRate)
: ProcessObj(bufferSize)
{
    mSpectrogram = new BLSpectrogram4(sampleRate, bufferSize/4, -1);
    mSpectroDisplay = NULL;
    
    ProcessObj::Reset(bufferSize, oversampling, freqRes, sampleRate);
    
    mLineCount = 0;
    
    mATune = 440.0;
    mSharpness = 0.0;
    
    mSpeedMod = 1;
    
#if USE_FREQ_OBJ
    mFreqObj = new FreqAdjustObj3(bufferSize, oversampling, 1, sampleRate);
#endif

    ResetQueue();
}

ChromaFftObj2::~ChromaFftObj2()
{
    delete mSpectrogram;
    
#if USE_FREQ_OBJ
    delete mFreqObj;
#endif
}

void
ChromaFftObj2::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer0,
                                const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer)
{
    WDL_TypedBuf<WDL_FFT_COMPLEX> &ioBuffer = mTmpBuf0;
    BLUtils::TakeHalf(*ioBuffer0, &ioBuffer);
    
    WDL_TypedBuf<BL_FLOAT> &magns = mTmpBuf1;
    WDL_TypedBuf<BL_FLOAT> &phases = mTmpBuf2;
    BLUtilsComp::ComplexToMagnPhase(&magns, &phases, ioBuffer);

    WDL_TypedBuf<BL_FLOAT> &chromaLine = mTmpBuf3;
    
#if USE_FREQ_OBJ
    // Must update the freq obj at each step
    // (otherwise the detection will be very bad if mSpeedMod != 1)
    WDL_TypedBuf<BL_FLOAT> &realFreqs = mTmpBuf4;
    mFreqObj->ComputeRealFrequencies(phases, &realFreqs);
#endif
    
    if (mLineCount % mSpeedMod == 0)
    {
#if !USE_FREQ_OBJ
      MagnsToChromaLine(magns, &chromaLine);
#else
      MagnsToChromaLineFreqs(magns, realFreqs, &chromaLine);
#endif
      
      AddSpectrogramLine(chromaLine, phases);
    }   

    mLineCount++;

#if !OPTIM_NO_RESYNTH
    //BLUtils::ResizeFillZeros(ioBuffer, ioBuffer->GetSize()*2);
    BLUtils::FillSecondFftHalf(ioBuffer, ioBuffer0);
#endif
}

void
ChromaFftObj2::Reset(int bufferSize, int oversampling,
                     int freqRes, BL_FLOAT sampleRate)
{
    ProcessObj::Reset(bufferSize, oversampling, freqRes, sampleRate);
    
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

    //
    ResetQueue();
    
#if USE_FREQ_OBJ
    mFreqObj->Reset(mBufferSize, mOverlapping, 1, mSampleRate);
#endif
}

BLSpectrogram4 *
ChromaFftObj2::GetSpectrogram()
{
    return mSpectrogram;
}

void
ChromaFftObj2::SetSpectrogramDisplay(SpectrogramDisplayScroll3 *spectroDisplay)
{
    mSpectroDisplay = spectroDisplay;
}

void
ChromaFftObj2::SetATune(BL_FLOAT aTune)
{
    mATune = aTune;
}

void
ChromaFftObj2::SetSharpness(BL_FLOAT sharpness)
{
    mSharpness = sharpness;
    
    // Force re-creating the window
    mSmoothWin.Resize(0);
}

void
ChromaFftObj2::MagnsToChromaLine(const WDL_TypedBuf<BL_FLOAT> &magns,
                                 const WDL_TypedBuf<BL_FLOAT> &phases,
                                 WDL_TypedBuf<BL_FLOAT> *chromaLine,
                                 HistoMaskLine2 *maskLine)
{
#if USE_FREQ_OBJ
  // Must update the freq obj at each step
  // (otherwise the detection will be very bad if mSpeedMod != 1)
  WDL_TypedBuf<BL_FLOAT> &realFreqs = mTmpBuf7;
  mFreqObj->ComputeRealFrequencies(phases, &realFreqs);
#endif
 
    // DEBUG
#if 1 //!USE_FREQ_OBJ
  MagnsToChromaLine(magns, chromaLine, maskLine);
#else
  MagnsToChromaLineFreqs(magns, realFreqs, chromaLine, maskLine);
#endif
}

void
ChromaFftObj2::SetSpeedMod(int speedMod)
{
    mSpeedMod = speedMod;
}

void
ChromaFftObj2::AddSpectrogramLine(const WDL_TypedBuf<BL_FLOAT> &magns,
                                  const WDL_TypedBuf<BL_FLOAT> &phases)
{
#if USE_AVG_LINES
    
#define USE_SIMPLE_AVG 1

    // Take the 4 last lines at the maximum
    // (otherwise it is too blurry)
    //#define MAX_LINES 2 //4
    
    int maxLines = mOverlapping;
    if (maxLines > MAX_LINES)
        maxLines = MAX_LINES;
        
    // Keep track of previous lines
    // For correctly display, with overlapping
    mOverlapLines.push_pop(magns);
    
    // Simply make the average of the previous lines
    WDL_TypedBuf<BL_FLOAT> &line = mTmpBuf8;
    BLUtils::ResizeFillZeros(&line, magns.GetSize());
    
    for (int i = 0; i < mOverlapLines.size(); i++)
    {
        WDL_TypedBuf<BL_FLOAT> &currentLine = mTmpBuf9;
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
        mSpectroDisplay->AddSpectrogramLine(magns, phases);
    else
        mSpectrogram->AddLine(magns, phases);
#endif
}

void
ChromaFftObj2::MagnsToChromaLine(const WDL_TypedBuf<BL_FLOAT> &magns,
                                 WDL_TypedBuf<BL_FLOAT> *chromaLine,
                                 HistoMaskLine2 *maskLine)
{
// Corresponding to A 440
//#define C0_TONE 16.35160
    
    BL_FLOAT c0Freq = ComputeC0Freq();
    
    BL_FLOAT toneMult = std::pow(2.0, 1.0/12.0);
    
    chromaLine->Resize(magns.GetSize());
    BLUtils::FillAllZero(chromaLine);
    
    BL_FLOAT hzPerBin = mSampleRate/mBufferSize;
    
    // Do not take 0Hz!
    for (int i = 1; i < magns.GetSize(); i++)
    {
        BL_FLOAT magnVal = magns.Get()[i];
        
        BL_FLOAT freq = i*hzPerBin;
        
        // See: https://pages.mtu.edu/~suits/NoteFreqCalcs.html
        BL_FLOAT fRatio = freq / c0Freq;
        BL_FLOAT tone = std::log(fRatio)/std::log(toneMult);
        
        // Shift by one (strange...)
        tone += 1.0;
        
        // Adjust to the axis labels
        tone -= 1.0/12.0;
        
        tone = fmod(tone, 12.0);
        
        BL_FLOAT toneNorm = tone/12.0;
        
        BL_FLOAT binNumF = toneNorm*chromaLine->GetSize();
        
        int binNum = bl_round(binNumF);
        
        if ((binNum >= 0) && (binNum < chromaLine->GetSize()))
            chromaLine->Get()[binNum] += magnVal;

        if (maskLine != NULL)
        {
            if ((binNum >= 0) && (binNum < chromaLine->GetSize()))
                maskLine->AddValue(binNum, i);
        }
    }
    
#if 0 // ORIG: before SpectroExpe
    // Smooth the chroma line
    if (mSmoothWin.GetSize() == 0)
    {
        int winSize = chromaLine->GetSize()/12;
        Window::MakeHanning(winSize, &mSmoothWin);
    }
#endif
    
#if 1 // New: manage sharpness
    // Initial, leaks a lot
    int divisor = 12;
    divisor = (1.0 - mSharpness)*MIN_SMOOTH_DIVISOR + mSharpness*MAX_SMOOTH_DIVISOR;
    if (divisor <= 0)
        divisor = 1;
    if (divisor > chromaLine->GetSize())
        divisor = chromaLine->GetSize();
    
    // Smooth the chroma line
    if (mSmoothWin.GetSize() == 0)
    {
        int windowSize = chromaLine->GetSize()/divisor;
        Window::MakeHanning(windowSize, &mSmoothWin);
    }
#endif
    
    WDL_TypedBuf<BL_FLOAT> &smoothLine = mTmpBuf5;
    BLUtils::SmoothDataWin(&smoothLine, *chromaLine, mSmoothWin);
    
    *chromaLine = smoothLine;
}

#if USE_FREQ_OBJ
void
ChromaFftObj2::MagnsToChromaLineFreqs(const WDL_TypedBuf<BL_FLOAT> &magns,
                                      const WDL_TypedBuf<BL_FLOAT> &realFreqs,
                                      WDL_TypedBuf<BL_FLOAT> *chromaLine,
                                      HistoMaskLine2 *maskLine)
{
    BL_FLOAT c0Freq = ComputeC0Freq();
    
    BL_FLOAT toneMult = std::pow(2.0, 1.0/12.0);
    
    // Optim
    BL_FLOAT logToneMult = std::log(toneMult);
    BL_FLOAT logToneMultInv = 1.0/logToneMult;
    
    chromaLine->Resize(magns.GetSize());
    BLUtils::FillAllZero(chromaLine);

    BL_FLOAT c0FreqInv = 1.0/c0Freq;
    BL_FLOAT inv12 = 1.0/12.0;
    
    int magnsSize = magns.GetSize();
    BL_FLOAT *magnsData = magns.Get();
    BL_FLOAT *realFreqsData = realFreqs.Get();

    int chromaLineSize = chromaLine->GetSize();
    
    // Do not take 0Hz!
    //for (int i = 1; i < magns.GetSize(); i++)
    for (int i = 1; i < magnsSize; i++)
    {
        //BL_FLOAT magnVal = magns.Get()[i];
        //BL_FLOAT freq = realFreqs.Get()[i];
        BL_FLOAT magnVal = magnsData[i];
        BL_FLOAT freq = realFreqsData[i];
        
        // See: https://pages.mtu.edu/~suits/NoteFreqCalcs.html
        //BL_FLOAT fRatio = freq / c0Freq;
        BL_FLOAT fRatio = freq*c0FreqInv;
        
        BL_FLOAT tone = std::log(fRatio)*logToneMultInv;
        
        // Shift (strange ?)
        tone += 0.5;
        
        tone = fmod(tone, 12.0);
        
        //BL_FLOAT toneNorm = tone/12.0;
        BL_FLOAT toneNorm = tone*inv12;
        
        //BL_FLOAT binNumF = toneNorm*chromaLine->GetSize();
        BL_FLOAT binNumF = toneNorm*chromaLineSize;
        
        int binNum = bl_round(binNumF);
        
        //if ((binNum >= 0) && (binNum < chromaLine->GetSize()))
        if ((binNum >= 0) && (binNum < chromaLineSize))
            chromaLine->Get()[binNum] += magnVal;

        if (maskLine != NULL)
        {
            //if ((binNum >= 0) && (binNum < chromaLine->GetSize()))
            if ((binNum >= 0) && (binNum < chromaLineSize))
                maskLine->AddValue(binNum, i);
        }
    }
    
    // Initial, leaks a lot
    int divisor = 12;
    divisor = (1.0 - mSharpness)*MIN_SMOOTH_DIVISOR + mSharpness*MAX_SMOOTH_DIVISOR;
    if (divisor <= 0)
        divisor = 1;
    if (divisor > chromaLine->GetSize())
        divisor = chromaLine->GetSize();
    
    // Smooth the chroma line
    if (mSmoothWin.GetSize() == 0)
    {
        int windowSize = chromaLine->GetSize()/divisor;
        Window::MakeHanning(windowSize, &mSmoothWin);
    }
    
    // Smooth
    WDL_TypedBuf<BL_FLOAT> &smoothLine = mTmpBuf6;
    BLUtils::SmoothDataWin(&smoothLine, *chromaLine, mSmoothWin);
    *chromaLine = smoothLine;
}
#endif

BL_FLOAT
ChromaFftObj2::ComputeC0Freq()
{
    BL_FLOAT AMinus1 = mATune/32.0;
    
    BL_FLOAT toneMult = std::pow(2.0, 1.0/12.0);
    
    BL_FLOAT ASharpMinus1 = AMinus1*toneMult;
    BL_FLOAT BMinus1 = ASharpMinus1*toneMult;
    
    BL_FLOAT C0 = BMinus1*toneMult;
    
    return C0;
}

void
ChromaFftObj2::ResetQueue()
{
    // Resize
    int maxLines = mOverlapping;
    if (maxLines > MAX_LINES)
        maxLines = MAX_LINES;
    mOverlapLines.resize(maxLines);

    // Set zero value
    WDL_TypedBuf<BL_FLOAT> &zeroLine = mTmpBuf10;
    zeroLine.Resize(mBufferSize/2);
    BLUtils::FillAllZero(&zeroLine);
    mOverlapLines.clear(zeroLine);
}

#endif // IGRAPHICS_NANOVG
