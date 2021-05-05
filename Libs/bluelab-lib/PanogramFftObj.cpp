//
//  PanogramFftObj.cpp
//  BL-Pano
//
//  Created by Pan on 02/06/18.
//
//

#ifdef IGRAPHICS_NANOVG

#include <BLSpectrogram4.h>
#include <Window.h>
#include <SpectrogramDisplayScroll4.h>
#include <PanogramPlayFftObj.h>

#include <BLUtils.h>
#include <BLUtilsComp.h>
#include <BLUtilsMath.h>

#include "PanogramFftObj.h"

//
#define MIN_SMOOTH_DIVISOR 12.0
#define MAX_SMOOTH_DIVISOR 48.0

// FIX: fixed bad display with high sample rates
// (and this, without resampling signal (USE_RESAMPLER) )
// USE_RESAMPLER was bad for Panogram, because in Panogram, we need to play the sound
#define FIX_BAD_DISPLAY_HIGH_SAMPLERATES 1

// When left and right magns are 0, the indices are pushed at the extreme left
#define FIX_EPS_MAGNS 1


PanogramFftObj::PanogramFftObj(int bufferSize, int oversampling, int freqRes,
                               BL_FLOAT sampleRate)
: MultichannelProcess(),
  mMaskLine(bufferSize)
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
    
    mPlayFftObjs[0] = NULL;
    mPlayFftObjs[1] = NULL;
    
    mAddLineCount = 0;
    
    mIsEnabled = true;
}

PanogramFftObj::~PanogramFftObj()
{
    delete mSpectrogram;
}

void
PanogramFftObj::
ProcessInputFft(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > *ioFftSamples,
                const vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > *scBuffer)
{
    if (ioFftSamples->size() != 2)
        return;
    
    if (!mIsEnabled)
        return;
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> *fftSamples = mTmpBuf0;
    fftSamples[0] = *(*ioFftSamples)[0];
    fftSamples[1] = *(*ioFftSamples)[1];
    
    WDL_TypedBuf<BL_FLOAT> *magns = mTmpBuf1;
    WDL_TypedBuf<BL_FLOAT> *phases = mTmpBuf2;
    
    for (int i = 0; i < 2; i++)
    {
        //BLUtils::TakeHalf(&fftSamples[i]);
        
        WDL_TypedBuf<WDL_FFT_COMPLEX> &half = mTmpBuf3;
        BLUtils::TakeHalf(fftSamples[i], &half);

        ////BLUtils::ComplexToMagnPhase(&magns[i], &phases[i], *(*ioFftSamples)[i]);
        //BLUtils::ComplexToMagnPhase(&magns[i], &phases[i], fftSamples[i]);
        BLUtilsComp::ComplexToMagnPhase(&magns[i], &phases[i], half);
    }
    
    //HistoMaskLine2 maskLine(mBufferSize);
    mMaskLine.Reset(mBufferSize);

    WDL_TypedBuf<BL_FLOAT> &panoLine = mTmpBuf4;
    MagnsToPanoLine(magns, &panoLine, &mMaskLine);
    
    for (int i = 0; i < 2; i++)
    {
        if (mPlayFftObjs[i] != NULL)
            mPlayFftObjs[i]->AddMaskLine(mMaskLine);
    }
  
    mLineCount++;
    
#if FIX_BAD_DISPLAY_HIGH_SAMPLERATES
    mAddLineCount++;
    
    BL_FLOAT srCoeff = mSampleRate/44100.0;
    srCoeff = bl_round(srCoeff);
    
    if (mAddLineCount % (int)srCoeff != 0)
        // Do not add line
        // This allows to keep constant speed at high sample rates
        return;
#endif
    
    // Take left phases... (we don't need them)
    WDL_TypedBuf<BL_FLOAT> &phases0 = phases[0];
    AddSpectrogramLine(panoLine, phases0);
}

void
PanogramFftObj::Reset(int bufferSize, int oversampling,
                      int freqRes, BL_FLOAT sampleRate)
{
    MultichannelProcess::Reset(bufferSize, oversampling, freqRes, sampleRate);
    
    mBufferSize = bufferSize;
    mOverlapping = oversampling;
    mFreqRes = freqRes;
    mSampleRate = sampleRate;
    
    int numCols = GetNumCols();
    
    mSpectrogram->Reset(mSampleRate, mBufferSize/4, numCols);
    
    mLineCount = 0;
    
    //mOverlapLines.clear();
    
    mAddLineCount = 0;
}

BLSpectrogram4 *
PanogramFftObj::GetSpectrogram()
{
    return mSpectrogram;
}

void
PanogramFftObj::SetSpectrogramDisplay(SpectrogramDisplayScroll4 *spectroDisplay)
{
    mSpectroDisplay = spectroDisplay;
}

void
PanogramFftObj::SetSharpness(BL_FLOAT sharpness)
{
    mSharpness = sharpness;
    
    // Force re-creating the window
    mSmoothWin.Resize(0);
}

int
PanogramFftObj::GetNumCols()
{
    // NEW
    // Prefer this, so the scroll speed won't be modified when
    // the overlapping changes
    int numCols = mBufferSize/(32/mOverlapping);
    
#if !FIX_BAD_DISPLAY_HIGH_SAMPLERATES
    // Adjust to the sample rate to avoid scrolling
    // 2 times faster when we go from 44100 to 88200
    //
    BL_FLOAT srCoeff = mSampleRate/44100.0;
    srCoeff = bl_round(srCoeff);
    numCols *= srCoeff;
#endif
    
    return numCols;
}

int
PanogramFftObj::GetNumColsAdd()
{
#if FIX_BAD_DISPLAY_HIGH_SAMPLERATES
    // NEW
    // Prefer this, so the scroll speed won't be modified when
    // the overlapping changes
    int numCols = mBufferSize/(32/mOverlapping);
    
    // Adjust to the sample rate to avoid scrolling
    // 2 times faster when we go from 44100 to 88200
    //
    BL_FLOAT srCoeff = mSampleRate/44100.0;
    srCoeff = bl_round(srCoeff);
    numCols *= srCoeff;
    
    return numCols;
#endif
}

void
PanogramFftObj::SetPlayFftObjs(PanogramPlayFftObj *objs[2])
{
    mPlayFftObjs[0] = objs[0];
    mPlayFftObjs[1] = objs[1];
}

void
PanogramFftObj::SetEnabled(bool flag)
{
    mIsEnabled = flag;
}

void
PanogramFftObj::AddSpectrogramLine(const WDL_TypedBuf<BL_FLOAT> &magns,
                                   const WDL_TypedBuf<BL_FLOAT> &phases)
{
    // Simple add
    if (mSpectroDisplay != NULL)
        mSpectroDisplay->AddSpectrogramLine(magns, phases);
    else
        mSpectrogram->AddLine(magns, phases);
}

void
PanogramFftObj::MagnsToPanoLine(const WDL_TypedBuf<BL_FLOAT> magns[2],
                                WDL_TypedBuf<BL_FLOAT> *panoLine,
                                HistoMaskLine2 *maskLine)
{
    panoLine->Resize(magns[0].GetSize());
    BLUtils::FillAllZero(panoLine);
    
#if 0
    // Manage 0Hz
    if (maskLine != NULL)
        maskLine->AddValue(0, 0);
#endif
    
    // Do not take 0Hz!
    for (int i = 1; i < magns[0].GetSize(); i++)
    {
        // Compute pan
        BL_FLOAT l = magns[0].Get()[i];
        BL_FLOAT r = magns[1].Get()[i];
        
#if FIX_EPS_MAGNS
#define EPS 1e-10
        
        if ((std::fabs(l) < EPS) && (std::fabs(r) < EPS))
            continue;
#endif
        
        BL_FLOAT angle = std::atan2(r, l);
        
        BL_FLOAT panNorm = angle/M_PI;
        panNorm = (panNorm + 0.25);
        panNorm = 1.0 - panNorm;
        
        // With 2, display goes outside of view with extreme pans
#define SCALE_FACTOR 1.8 //2.0
        panNorm = (panNorm - 0.5)*SCALE_FACTOR + 0.5;
        
        //
        BL_FLOAT binNumF = panNorm*panoLine->GetSize();
        
        int binNum = bl_round(binNumF);
        
        BL_FLOAT magnVal = (l + r)*0.5;
        if ((binNum >= 0) && (binNum < panoLine->GetSize()))
            panoLine->Get()[binNum] += magnVal;
        
        if (maskLine != NULL)
            maskLine->AddValue(binNum, i);
    }
    
#if 1 // Sharpness
    // Initial, leaks a lot
    int divisor = 12;
    divisor = (1.0 - mSharpness)*MIN_SMOOTH_DIVISOR + mSharpness*MAX_SMOOTH_DIVISOR;
    if (divisor <= 0)
        divisor = 1;
    if (divisor > panoLine->GetSize())
        divisor = panoLine->GetSize();
    
    // Smooth the Pano line
    if (mSmoothWin.GetSize() == 0)
    {
        Window::MakeHanning(panoLine->GetSize()/divisor, &mSmoothWin);
    }
    
    WDL_TypedBuf<BL_FLOAT> &smoothLine = mTmpBuf5;
    BLUtils::SmoothDataWin(&smoothLine, *panoLine, mSmoothWin);
#endif
    
    *panoLine = smoothLine;
}

#endif // IGRAPHICS_NANOVG
