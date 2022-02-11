//
//  SpectrogramFftObj.cpp
//  BL-Ghost
//
//  Created by Pan on 02/06/18.
//
//

#include <BLSpectrogram3.h>
#include <Utils.h>

#include "SpectrogramFftObj.h"

#define USE_AVG_LINES 0 //1

SpectrogramFftObj::SpectrogramFftObj(int bufferSize, int oversampling, int freqRes,
                                     double sampleRate)
: ProcessObj(bufferSize)
{
    mSpectrogram = new BLSpectrogram3(bufferSize/4, -1);

    ProcessObj::Reset(bufferSize, oversampling, freqRes, sampleRate);
    
    mLineCount = 0;
    
    mAdaptiveSpeed = false;
    mAddStep = ComputeAddStep();
    
    mMode = VIEW;
}

SpectrogramFftObj::~SpectrogramFftObj()
{
    delete mSpectrogram;
}

void
SpectrogramFftObj::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                                    const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer)
{
    if (mAdaptiveSpeed)
    {
        if (mMode == VIEW)
        {
            if (mLineCount % mAddStep != 0)
            {
                mLineCount++;
                
                return;
            }
        }
    }
    
    Utils::TakeHalf(ioBuffer);
    
    WDL_TypedBuf<double> magns;
    WDL_TypedBuf<double> phases;
    Utils::ComplexToMagnPhase(&magns, &phases, *ioBuffer);
    
    AddSpectrogramLine(magns, phases);
    
    mLineCount++;
    
    Utils::ResizeFillZeros(ioBuffer, ioBuffer->GetSize()*2);
    Utils::FillSecondFftHalf(ioBuffer);
}

void
SpectrogramFftObj::Reset(int bufferSize, int oversampling, int freqRes, double sampleRate)
{
    ProcessObj::Reset(bufferSize, oversampling, freqRes, sampleRate);
    
    mLineCount = 0;
    
    mOverlapLines.clear();
    
    mAddStep = ComputeAddStep();
}

BLSpectrogram3 *
SpectrogramFftObj::GetSpectrogram()
{
    return mSpectrogram;
}

void
SpectrogramFftObj::SetFullData(const vector<WDL_TypedBuf<double> > &magns,
                               const vector<WDL_TypedBuf<double> > &phases)
{
    mOverlapLines.clear();
    
    mSpectrogram->Reset();
    
    for (int i = 0; i < magns.size(); i++)
    {
        const WDL_TypedBuf<double> &magns0 = magns[i];
        const WDL_TypedBuf<double> &phases0 = phases[i];
        
        AddSpectrogramLine(magns0, phases0);
    }
}

void
SpectrogramFftObj::SetMode(enum Mode mode)
{
    mMode = mode;
    
    // Re-init
    if (mode == ACQUIRE)
    {
        mSpectrogram->Reset(mBufferSize/4, -1);
        
        ProcessObj::Reset(mBufferSize, mOverlapping, mFreqRes, mSampleRate);
    }
    else if (mode == VIEW)
    {
        int numCols = mBufferSize/8;
        
        mSpectrogram->Reset(mBufferSize/4, numCols);
        
        ProcessObj::Reset(mBufferSize, mOverlapping, mFreqRes, mSampleRate);
    }
    
    // For EDIT and RENDER, to not reset the spectrogram !
    
    //ProcessObj::Reset(mOverlapping, mFreqRes, mSampleRate);
    
    mLineCount = 0;
}

#if (ADAPTIVE_SPEED_FEATURE || ADAPTIVE_SPEED_FEATURE2)
void
SpectrogramFftObj::SetAdaptiveSpeed(bool flag)
{
    mAdaptiveSpeed = flag;
    
    mAddStep = ComputeAddStep();
}
#endif

void
SpectrogramFftObj::AddSpectrogramLine(const WDL_TypedBuf<double> &magns,
                                      const WDL_TypedBuf<double> &phases)
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
    WDL_TypedBuf<double> line;
    Utils::ResizeFillZeros(&line, magns.GetSize());
    
    for (int i = 0; i < mOverlapLines.size(); i++)
    {
        WDL_TypedBuf<double> currentLine = mOverlapLines[i];
        
#if !USE_SIMPLE_AVG
        // Multiply by a coeff to smooth
        double coeff = (i + 1.0)/mOverlapLines.size();
        
        // Adjust, for MAX_LINES == 4
        coeff /= 2.5;
        
        Utils::MultValues(&currentLine, coeff);
#endif
        
        Utils::AddValues(&line, currentLine);
    }
    
#if USE_SIMPLE_AVG
    if (mOverlapping > 0)
        // Just in case
        Utils::MultValues(&line, 1.0/maxLines);
#endif
    
    mSpectrogram->AddLine(line, phases);

#else // Simple add
    mSpectrogram->AddLine(magns, phases);
#endif
}

#if ADAPTIVE_SPEED_FEATURE
int
SpectrogramFftObj::GetSpectroNumCols()
{
    int numCols = mBufferSize/8;

    if (mAdaptiveSpeed)
    {
        double srCoeff = mSampleRate/44100.0;
        srCoeff = round(srCoeff);
        numCols *= srCoeff;
    }
    
    return numCols;
}
#endif

#if ADAPTIVE_SPEED_FEATURE2
int
SpectrogramFftObj::ComputeAddStep()
{
    int addStep = 1;
    
    double srCoeff = mSampleRate/44100.0;
    srCoeff = round(srCoeff);
    addStep *= srCoeff;
    
    return addStep;
}
#endif
