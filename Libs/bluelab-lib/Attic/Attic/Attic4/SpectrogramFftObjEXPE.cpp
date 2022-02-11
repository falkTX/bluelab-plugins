//
//  SpectrogramFftObjEXPE.cpp
//  BL-Ghost
//
//  Created by Pan on 02/06/18.
//
//

#include <BLSpectrogram3.h>
#include <BLUtils.h>

#include "SpectrogramFftObjEXPE.h"

// EXPE
#include "TransientLib4.h"

#define USE_AVG_LINES 0 //1

SpectrogramFftObjEXPE::SpectrogramFftObjEXPE(int bufferSize, int oversampling, int freqRes,
                                     BL_FLOAT sampleRate)
: ProcessObj(bufferSize)
{
    mSpectrogram = new BLSpectrogram3(bufferSize/4, -1);

    ProcessObj::Reset(bufferSize, oversampling, freqRes, sampleRate);
    
    mLineCount = 0;
    
    mSmoothFactor = 0.5;
    mFreqAmpRatio = 0.5;
    mTransThreshold = 0.0;
}

SpectrogramFftObjEXPE::~SpectrogramFftObjEXPE()
{
    delete mSpectrogram;
}

void
SpectrogramFftObjEXPE::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                                    const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer)
{
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
SpectrogramFftObjEXPE::Reset(int oversampling, int freqRes, BL_FLOAT sampleRate)
{
    ProcessObj::Reset(mBufferSize, oversampling, freqRes, sampleRate);
    
    mLineCount = 0;
    
    mOverlapLines.clear();
}

BLSpectrogram3 *
SpectrogramFftObjEXPE::GetSpectrogram()
{
    return mSpectrogram;
}

void
SpectrogramFftObjEXPE::SetFullData(const vector<WDL_TypedBuf<BL_FLOAT> > &magns,
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
SpectrogramFftObjEXPE::SetMode(enum Mode mode)
{
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

void
SpectrogramFftObjEXPE::SetSmoothFactor(BL_FLOAT factor)
{
    mSmoothFactor = factor;
}

void
SpectrogramFftObjEXPE::SetFreqAmpRatio(BL_FLOAT ratio)
{
    mFreqAmpRatio = ratio;
}

void
SpectrogramFftObjEXPE::SetTransThreshold(BL_FLOAT thrs)
{
    mTransThreshold = thrs;
}

void
SpectrogramFftObjEXPE::AddSpectrogramLine(const WDL_TypedBuf<BL_FLOAT> &magns,
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
    
    // EXPE
    WDL_TypedBuf<BL_FLOAT> *prevPhases = NULL;
    if (mPrevPhases.GetSize() > 0)
        prevPhases = &mPrevPhases;
    
    // TEST
    //WDL_TypedBuf<BL_FLOAT> magnsNorm = magns;
    //BL_FLOAT avg = BLUtils::ComputeAvg(magns);
    //if (avg > 0.0)
    //    BLUtils::MultValues(&magnsNorm, 1.0/avg);
    
    WDL_TypedBuf<BL_FLOAT> transientness;
    TransientLib4::ComputeTransientness2(magns,
                                         phases,
                                         prevPhases,
                                         mFreqAmpRatio,
                                         mSmoothFactor,
                                         &transientness);
    
    mPrevPhases = phases;
    
    //
    WDL_TypedBuf<int> samplesIds;
    BLUtils::FftIdsToSamplesIds(phases, &samplesIds);
    
    WDL_TypedBuf<BL_FLOAT> sampleTrans = transientness;
    
    BLUtils::Permute(&sampleTrans, samplesIds, false/*true*/);
    //
    
   //BLUtils::MultValues(&magns, sampleTrans); // TEST (seems to work better, not sure)
    
    // WARNING: the magns are displayed, not the transients
    for (int i = 0; i < magns.GetSize(); i++)
    {
        BL_FLOAT magn = magns.Get()[i];
        BL_FLOAT trans = sampleTrans.Get()[i];
        
        if (trans < mTransThreshold)
        {
            magn = 0.0;
            trans = 0.0;
        }
        
        magns.Get()[i] = magn;
        sampleTrans.Get()[i] = trans;
    }
    
    mSpectrogram->AddLine(magns/*sampleTrans*//*transientness*/, phases);
    
    //
    //mSpectrogram->AddLine(magns, phases);
#endif
}
