/* Copyright (C) 2022 Nicolas Dittlo <deadlab.plugins@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this software; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
 
//
//  PostTransientFftObj3.cpp
//  BL-PitchShift
//
//  Created by Pan on 18/04/18.
//
//

#include <TransientShaperFftObj3.h>
#include <BLUtils.h>

#include "PostTransientFftObj3.h"


// Keep this comment for reminding
//
// KEEP_SYNTHESIS_ENERGY and VARIABLE_HANNING are tightliy bounded !
//
// KEEP_SYNTHESIS_ENERGY=1 + VARIABLE_HANNING=1
// Not so bad, but we loose frequencies
//
// KEEP_SYNTHESIS_ENERGY=0 + VARIABLE_HANNING=0: GOOD

// When set to 0, we will keep transient boost !
#define KEEP_SYNTHESIS_ENERGY 0

// When set to 0, we are accurate for frequencies
#define VARIABLE_HANNING 1


// Smoothing: we set to 0 to have a fine loking bell curve
// In the current implementation, if we increase the precision,
// we will have blurry frequencies (there is no threshold !),
// and the result transient precision won't be increased !
#define TRANSIENT_PRECISION 0.0

// OLD: before fix good separation "s"/"p" for Shaper
//#define FREQ_AMP_RATIO 0.5

// NEW: after shaper fix
// 0.0 for maximum "p"
//#define FREQ_AMP_RATIO 0.0

// ... Should be better to also boost "s"
#define FREQ_AMP_RATIO 0.5

// Validated!
#define DENOISER_OPTIM7 1

// Validated!
#define DENOISER_OPTIM10 1

#define FIX_CRASH_PITCH_SHIFT 1

PostTransientFftObj3::PostTransientFftObj3(const vector<ProcessObj *> &processObjs,
                                           int numChannels, int numScInputs,
                                           int bufferSize, int overlapping, int freqRes,
                                           BL_FLOAT sampleRate, int numTransObjs,
                                           BL_FLOAT freqAmpRatio, BL_FLOAT transientBoostFator)
// Add two additional channels, for transient detection (windowing may be different)
: FftProcessObj16(processObjs,
                  (numTransObjs == -1) ? numChannels*2 : numChannels + numTransObjs,
                  numScInputs,
                  bufferSize, overlapping, freqRes,
                  sampleRate)
{
    mTransBoost = 0.0;
    
    mNumChannelsTransient = numChannels;
    
    // DENOISER_OPTIM11
    // Default
    mTransientBoostFactor = TRANSIENT_BOOST_FACTOR;
    // Override
    if (transientBoostFator > 0.0)
        mTransientBoostFactor = transientBoostFator;
    
    
#if DENOISER_OPTIM10
    
#if !FIX_CRASH_PITCH_SHIFT
    // This code seems incorrect
    mNumChannelsTransient = (numTransObjs == -1) ? numChannels*2 : numTransObjs;
#else
    mNumChannelsTransient = (numTransObjs == -1) ? numChannels : numTransObjs;
#endif
    
#endif
    
    for (int i = 0; i < mNumChannelsTransient; i++)
    {
        // Only detect transients, do not apply
        TransientShaperFftObj3 *transObj =
                new TransientShaperFftObj3(bufferSize, overlapping, freqRes,
                                           sampleRate,
                                           0.0, 1.0,
                                           false);
        
        if (freqAmpRatio < 0.0)
            freqAmpRatio = FREQ_AMP_RATIO;
        
        // This configuration seems good (only amp)
        transObj->SetFreqAmpRatio(freqAmpRatio);
        
        transObj->SetPrecision(TRANSIENT_PRECISION);
        
        SetChannelProcessObject(numChannels + i, transObj);
        
        // Extra channels for transient boost
        if (VARIABLE_HANNING)
            SetAnalysisWindow(numChannels + i, WindowVariableHanning);
        else
            SetAnalysisWindow(numChannels + i, WindowVariableHanning);
        
        FftProcessObj16::SetKeepSynthesisEnergy(numChannels + i, KEEP_SYNTHESIS_ENERGY);
        
#if DENOISER_OPTIM7
        // We don't need to compute the ifft,
        // we only just need to compute the transientness from the input signal
        SetSkipIFft(numChannels + i, true);
#endif
    }
}

PostTransientFftObj3::~PostTransientFftObj3()
{
#if !DENOISER_OPTIM10
    int numChannels = GetNumChannels();
#else
    int numChannels = mNumChannelsTransient;
#endif
    
    for (int i = 0; i < numChannels; i++)
    {
        TransientShaperFftObj3 *transObj = GetTransObj(i);
        
        if (transObj != NULL)
            delete transObj;
    }
}

void
PostTransientFftObj3::Reset()
{
    FftProcessObj16::Reset();
    
#if !DENOISER_OPTIM10
    int numChannels = GetNumChannels();
#else
    int numChannels = mNumChannelsTransient;
#endif

    for (int i = 0; i < numChannels; i++)
    {
        ProcessObj *transObj = GetTransObj(i);
        
        if (transObj != NULL)
            transObj->Reset(mBufferSize, mOverlapping, mFreqRes, mSampleRate);
    }
}

void
PostTransientFftObj3::Reset(int bufferSize, int overlapping, int freqRes, BL_FLOAT sampleRate)
{
    FftProcessObj16::Reset(bufferSize, overlapping, freqRes, sampleRate);
    
#if !DENOISER_OPTIM10
    int numChannels = GetNumChannels();
#else
    int numChannels = mNumChannelsTransient;
#endif

    for (int i = 0; i < numChannels; i++)
    {
        ProcessObj *transObj = GetTransObj(i);
        
        if (transObj != NULL)
            transObj->Reset(bufferSize, overlapping, freqRes, sampleRate);
    }
}

void
PostTransientFftObj3::SetKeepSynthesisEnergy(int channelNum, bool flag)
{
    FftProcessObj16::SetKeepSynthesisEnergy(channelNum, flag);
    
    // Reset the transient boost extra channels to the correct value
#if !DENOISER_OPTIM10
    int numChannels = GetNumChannels();
    int numChannelsTransient = numChannels;
#else
    int numChannels = mNumChannelsTransient;
    int numChannelsTransient = mNumChannelsTransient;
#endif

    for (int i = 0; i < numChannelsTransient; i++)
    {
        FftProcessObj16::SetKeepSynthesisEnergy(numChannels + i, KEEP_SYNTHESIS_ENERGY);
    }
}

void
PostTransientFftObj3::SetTransBoost(BL_FLOAT factor)
{
    mTransBoost = factor;
    
#if !DENOISER_OPTIM10
    int numChannels = GetNumChannels();
#else
    int numChannels = mNumChannelsTransient;
#endif

    for (int i = 0; i < numChannels; i++)
    {
        TransientShaperFftObj3 *transObj = GetTransObj(i);
        
        if (transObj != NULL)
            transObj->SetSoftHard(factor);
    }
}

void
PostTransientFftObj3::ResultSamplesWinReady()
{
    // Do not apply transientness if not necessary
    if (mTransBoost <= TRANS_BOOST_EPS)
        return;
    
#if !DENOISER_OPTIM10
    int numChannels = GetNumChannels();
#else
    int numChannels = mNumChannelsTransient;
#endif

    for (int i = 0; i < numChannels; i++)
    {
        // Result buffer
        WDL_TypedBuf<BL_FLOAT> *buffer = GetChannelResultSamples(i);
        
        // Result transientness
        TransientShaperFftObj3 *transObj = GetTransObj(i);
        if (transObj == NULL)
            continue;
        
        WDL_TypedBuf<BL_FLOAT> &transientness = mTmpBuf0;;
        transObj->GetCurrentTransientness(&transientness);
    
        // Transient boost
        BLUtils::MultValues(&transientness, mTransientBoostFactor); //TRANSIENT_BOOST_FACTOR);
        
        //BLUtils::ClipMax(&transientness, 1.0);
        
        // Make a test on the size, because with new latency management,
        // the first buffer may not have been processes
        // Then transientness will be empty at the beginning
        if (transientness.GetSize() == buffer->GetSize())
            transObj->ApplyTransientness(buffer, transientness);
    }
}

#if !DENOISER_OPTIM10
int
PostTransientFftObj3::GetNumChannels()
{
    int numChannels = (mChannels.size() - mNumScInputs)/2;
    
    return numChannels;
}
#else
int
PostTransientFftObj3::GetNumChannels()
{
    int numChannels = (mChannels.size() - mNumScInputs) - mNumChannelsTransient;
    
    return numChannels;
}
#endif

TransientShaperFftObj3 *
PostTransientFftObj3::GetTransObj(int channelNum)
{
    int numChannels = GetNumChannels();
    
    ProcessObj *obj = GetChannelProcessObject(numChannels + channelNum);
    
    // Dynamic cast ?
    TransientShaperFftObj3 *transObj = (TransientShaperFftObj3 *)obj;
    
    return transObj;
}

void
PostTransientFftObj3::AddSamples(const vector<WDL_TypedBuf<BL_FLOAT> > &inputs,
                                 const vector<WDL_TypedBuf<BL_FLOAT> > &scInputs)
{
    FftProcessObj16::AddSamples(inputs, scInputs);
    
    int numChannels = GetNumChannels();
    
    // Add again the samples
    // for the two transient processing channels
    FftProcessObj16::AddSamples(numChannels, inputs);
}
