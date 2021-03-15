//
//  ChromaFftObj.cpp
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
#include <BLUtilsFft.h>
#include <BLUtilsMath.h>

#include <SpectrogramDisplayScroll.h>

#include "ChromaFftObj.h"

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


ChromaFftObj::ChromaFftObj(int bufferSize, int oversampling, int freqRes,
                           BL_FLOAT sampleRate)
: ProcessObj(bufferSize)
{
    mSpectrogram = new BLSpectrogram4(sampleRate, bufferSize/4, -1);
    mSpectroDisplay = NULL;
    
    ProcessObj::Reset(bufferSize, oversampling, freqRes, sampleRate);
    
    mLineCount = 0;
    
    mATune = 440.0;
    mSharpness = 0.0;
    
#if USE_FREQ_OBJ
    mFreqObj = new FreqAdjustObj3(bufferSize, oversampling, 1, sampleRate);
#endif
}

ChromaFftObj::~ChromaFftObj()
{
    delete mSpectrogram;
    
#if USE_FREQ_OBJ
    delete mFreqObj;
#endif

}

void
ChromaFftObj::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                               const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer)
{
    BLUtils::TakeHalf(ioBuffer);
    
    WDL_TypedBuf<BL_FLOAT> magns;
    WDL_TypedBuf<BL_FLOAT> phases;
    BLUtilsComp::ComplexToMagnPhase(&magns, &phases, *ioBuffer);
    
    WDL_TypedBuf<BL_FLOAT> chromaLine;
    
#if !USE_FREQ_OBJ
    MagnsToCromaLine(magns, &chromaLine);
#endif

#if USE_FREQ_OBJ
    MagnsToCromaLine(magns, phases, &chromaLine);
#endif
    
    AddSpectrogramLine(chromaLine, phases);
    
    mLineCount++;
    
    BLUtils::ResizeFillZeros(ioBuffer, ioBuffer->GetSize()*2);
    BLUtilsFft::FillSecondFftHalf(ioBuffer);
}

void
ChromaFftObj::Reset(int bufferSize, int oversampling, int freqRes, BL_FLOAT sampleRate)
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
    
#if USE_FREQ_OBJ
    mFreqObj->Reset(mBufferSize, mOverlapping, 1, mSampleRate);
#endif
}

BLSpectrogram4 *
ChromaFftObj::GetSpectrogram()
{
    return mSpectrogram;
}

void
ChromaFftObj::SetSpectrogramDisplay(SpectrogramDisplayScroll *spectroDisplay)
{
    mSpectroDisplay = spectroDisplay;
}

void
ChromaFftObj::SetATune(BL_FLOAT aTune)
{
    mATune = aTune;
}

void
ChromaFftObj::SetSharpness(BL_FLOAT sharpness)
{
    mSharpness = sharpness;
    
    // Force re-creating the window
    mSmoothWin.Resize(0);
}

void
ChromaFftObj::AddSpectrogramLine(const WDL_TypedBuf<BL_FLOAT> &magns,
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
        mSpectroDisplay->AddSpectrogramLine(magns, phases);
    else
        mSpectrogram->AddLine(magns, phases);
#endif
}

void
ChromaFftObj::MagnsToCromaLine(const WDL_TypedBuf<BL_FLOAT> &magns,
                               WDL_TypedBuf<BL_FLOAT> *chromaLine)
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
    }
    
    // Smooth the chroma line
    if (mSmoothWin.GetSize() == 0)
    {
        int winSize = chromaLine->GetSize()/12;
        Window::MakeHanning(winSize, &mSmoothWin);
        //Window::MakeSquare(winSize, 1.0, &mSmoothWin); // TEST
    }
    
    WDL_TypedBuf<BL_FLOAT> smoothLine;
    BLUtils::SmoothDataWin(&smoothLine, *chromaLine, mSmoothWin);
    
    *chromaLine = smoothLine;
}

#if USE_FREQ_OBJ
void
ChromaFftObj::MagnsToCromaLine(const WDL_TypedBuf<BL_FLOAT> &magns,
                               const WDL_TypedBuf<BL_FLOAT> &phases,
                               WDL_TypedBuf<BL_FLOAT> *chromaLine)
{
    WDL_TypedBuf<BL_FLOAT> realFreqs;
    mFreqObj->ComputeRealFrequencies(phases, &realFreqs);
    
    BL_FLOAT c0Freq = ComputeC0Freq();
    
    BL_FLOAT toneMult = std::pow(2.0, 1.0/12.0);
    
    // Optim
    BL_FLOAT logToneMult = std::log(toneMult);
    BL_FLOAT logToneMultInv = 1.0/logToneMult;
    
    chromaLine->Resize(magns.GetSize());
    BLUtils::FillAllZero(chromaLine);
    
    // Do not take 0Hz!
    for (int i = 1; i < magns.GetSize(); i++)
    {
        BL_FLOAT magnVal = magns.Get()[i];
        
        BL_FLOAT freq = realFreqs.Get()[i];
        
        // See: https://pages.mtu.edu/~suits/NoteFreqCalcs.html
        BL_FLOAT fRatio = freq / c0Freq;
        BL_FLOAT tone = std::log(fRatio)*logToneMultInv;
        
        // Shift (strange ?)
        tone += 0.5;
        
        tone = fmod(tone, 12.0);
        
        BL_FLOAT toneNorm = tone/12.0;
        
        BL_FLOAT binNumF = toneNorm*chromaLine->GetSize();
        
        int binNum = bl_round(binNumF);
        
        if ((binNum >= 0) && (binNum < chromaLine->GetSize()))
            chromaLine->Get()[binNum] += magnVal;
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
    
    //BLDebug::DumpData("win.txt", mSmoothWin);
    
#if 1 // Smooth
    WDL_TypedBuf<BL_FLOAT> smoothLine;
    BLUtils::SmoothDataWin(&smoothLine, *chromaLine, mSmoothWin);
    *chromaLine = smoothLine;
#endif
}
#endif

BL_FLOAT
ChromaFftObj::ComputeC0Freq()
{
    BL_FLOAT AMinus1 = mATune/32.0;
    
    BL_FLOAT toneMult = std::pow(2.0, 1.0/12.0);
    
    BL_FLOAT ASharpMinus1 = AMinus1*toneMult;
    BL_FLOAT BMinus1 = ASharpMinus1*toneMult;
    
    BL_FLOAT C0 = BMinus1*toneMult;
    
    return C0;
}

#endif // IGRAPHICS_NANOVG
