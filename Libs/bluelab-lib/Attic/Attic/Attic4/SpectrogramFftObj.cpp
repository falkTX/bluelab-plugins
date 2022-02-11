//
//  SpectrogramFftObj.cpp
//  BL-Ghost
//
//  Created by Pan on 02/06/18.
//
//

#include <BLSpectrogram3.h>
#include <BLUtils.h>

#include "SpectrogramFftObj.h"

#define USE_AVG_LINES 0 //1

SpectrogramFftObj::SpectrogramFftObj(int bufferSize, int oversampling, int freqRes,
                                     BL_FLOAT sampleRate)
: ProcessObj(bufferSize)
{
    mSpectrogram = new BLSpectrogram3(bufferSize/4, -1);

    ProcessObj::Reset(bufferSize, oversampling, freqRes, sampleRate);
    
    mLineCount = 0;
    
    // For constant speed
    mConstantSpeed = false;
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
#if CONSTANT_SPEED_FEATURE
    if (mConstantSpeed)
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
#endif
    
    BLUtils::TakeHalf(ioBuffer);
    
    WDL_TypedBuf<BL_FLOAT> magns;
    WDL_TypedBuf<BL_FLOAT> phases;
    BLUtils::ComplexToMagnPhase(&magns, &phases, *ioBuffer);
    
    AddSpectrogramLine(magns, phases);
    
    mLineCount++;
    
    BLUtils::ResizeFillZeros(ioBuffer, ioBuffer->GetSize()*2);
    BLUtils::FillSecondFftHalf(ioBuffer);
}

void
SpectrogramFftObj::Reset(int bufferSize, int oversampling, int freqRes, BL_FLOAT sampleRate)
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
SpectrogramFftObj::SetFullData(const vector<WDL_TypedBuf<BL_FLOAT> > &magns,
                               const vector<WDL_TypedBuf<BL_FLOAT> > &phases)
{
    mOverlapLines.clear();
    
    mSpectrogram->Reset();
    
    for (int i = 0; i < magns.size(); i++)
    {
        const WDL_TypedBuf<BL_FLOAT> &magns0 = magns[i];
        const WDL_TypedBuf<BL_FLOAT> &phases0 = phases[i];
        
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

#if CONSTANT_SPEED_FEATURE
void
SpectrogramFftObj::SetConstantSpeed(bool flag)
{
    mConstantSpeed = flag;
    
    mAddStep = ComputeAddStep();
}
#endif

void
SpectrogramFftObj::AddSpectrogramLine(const WDL_TypedBuf<BL_FLOAT> &magns,
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
    mSpectrogram->AddLine(magns, phases);
#endif
}

int
SpectrogramFftObj::ComputeAddStep()
{
    int addStep = 1;
    
    BL_FLOAT srCoeff = mSampleRate/44100.0;
    srCoeff = bl_round(srCoeff);
    addStep *= srCoeff;
    
    return addStep;
}
