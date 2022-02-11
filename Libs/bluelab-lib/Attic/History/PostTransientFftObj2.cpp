//
//  PostTransientFftObj2.cpp
//  BL-PitchShift
//
//  Created by Pan on 18/04/18.
//
//

#include <TransientShaperFftObj3.h>
#include <BLUtils.h>

#include "PostTransientFftObj2.h"


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

// NOTE: can saturate if greater than 1.0

// OLD: before fix good separation "s"/"p" for Shaper
//#define TRANSIENT_BOOST_FACTOR 1.0

// NEW: after shaper fix
#define TRANSIENT_BOOST_FACTOR 2.0

// Above this threshold, we don't compute transients
// (performances gain)
#define TRANS_BOOST_EPS 1e-15

// OLD: before fix good separation "s"/"p" for Shaper
//#define FREQ_AMP_RATIO 0.5

// NEW: after shaper fix
// 0.0 for maximum "p"
//#define FREQ_AMP_RATIO 0.0

// ... Should be better to also boost "s"
#define FREQ_AMP_RATIO 0.5


PostTransientFftObj2::PostTransientFftObj2(const vector<ProcessObj *> &processObjs,
                                           int numChannels, int numScInputs,
                                           int bufferSize, int overlapping, int freqRes,
                                           BL_FLOAT sampleRate)
// Add two additional channels, for transient detection (windowing may be different)
: FftProcessObj15(processObjs,
                  numChannels*2, numScInputs,
                  bufferSize, overlapping, freqRes,
                  sampleRate)
{
    mTransBoost = 0.0;
    
    for (int i = 0; i < numChannels; i++)
    {
        // Only detect transients, do not apply
        TransientShaperFftObj3 *transObj =
                new TransientShaperFftObj3(bufferSize, overlapping, freqRes,
                                           sampleRate,
                                           0.0, 1.0,
                                           false);
        
        // This configuration seems good (only amp)
        transObj->SetFreqAmpRatio(FREQ_AMP_RATIO);
        
        transObj->SetPrecision(TRANSIENT_PRECISION);
        
        SetChannelProcessObject(numChannels + i, transObj);
        
        // Extra channels for transient boost
        if (VARIABLE_HANNING)
            SetAnalysisWindow(numChannels + i, WindowVariableHanning);
        else
            SetAnalysisWindow(numChannels + i, WindowVariableHanning);
        
        FftProcessObj15::SetKeepSynthesisEnergy(numChannels + i, KEEP_SYNTHESIS_ENERGY);
    }
}

PostTransientFftObj2::~PostTransientFftObj2()
{
    int numChannels = GetNumChannels();
    for (int i = 0; i < numChannels; i++)
    {
        TransientShaperFftObj3 *transObj = GetTransObj(i);
        
        delete transObj;
    }
}

void
PostTransientFftObj2::Reset()
{
    FftProcessObj15::Reset();
    
    int numChannels = GetNumChannels();
    for (int i = 0; i < numChannels; i++)
    {
        ProcessObj *transObj = GetTransObj(i);
            
        transObj->Reset();
    }
}

void
PostTransientFftObj2::SetKeepSynthesisEnergy(int channelNum, bool flag)
{
    FftProcessObj15::SetKeepSynthesisEnergy(channelNum, flag);
    
    // Reset the transient boost extra channels to the correct value
    int numChannels = GetNumChannels();
    for (int i = 0; i < numChannels; i++)
    {
        FftProcessObj15::SetKeepSynthesisEnergy(numChannels + i, KEEP_SYNTHESIS_ENERGY);
    }
}

void
PostTransientFftObj2::SetTransBoost(BL_FLOAT factor)
{
    mTransBoost = factor;
    
    int numChannels = GetNumChannels();
    for (int i = 0; i < numChannels; i++)
    {
        TransientShaperFftObj3 *transObj = GetTransObj(i);
        
        transObj->SetSoftHard(factor);
    }
}

void
PostTransientFftObj2::ResultSamplesWinReady()
{
    // Do not apply transientness if not necessary
    if (mTransBoost <= TRANS_BOOST_EPS)
        return;
    
    int numChannels = GetNumChannels();
    for (int i = 0; i < numChannels; i++)
    {
        // Result buffer
        WDL_TypedBuf<BL_FLOAT> *buffer = GetChannelResultSamples(i);
        
        // Result transientness
        TransientShaperFftObj3 *transObj = GetTransObj(i);
        
        WDL_TypedBuf<BL_FLOAT> transientness;
        transObj->GetCurrentTransientness(&transientness);
    
        // Transient boost
        BLUtils::MultValues(&transientness, TRANSIENT_BOOST_FACTOR);
        
        //BLUtils::ClipMax(&transientness, 1.0);
        
        // Make a test on the size, because with new latency management,
        // the first buffer may not have been processes
        // Then transientness will be empty at the beginning
        if (transientness.GetSize() == buffer->GetSize())
            transObj->ApplyTransientness(buffer, transientness);
    }
}

int
PostTransientFftObj2::GetNumChannels()
{
    int numChannels = (mChannels.size() - mNumScInputs)/2;
    
    return numChannels;
}
    
TransientShaperFftObj3 *
PostTransientFftObj2::GetTransObj(int channelNum)
{
    int numChannels = GetNumChannels();
    
    ProcessObj *obj = GetChannelProcessObject(numChannels + channelNum);
    
    // Dynamic cast ?
    TransientShaperFftObj3 *transObj = (TransientShaperFftObj3 *)obj;
    
    return transObj;
}

void
PostTransientFftObj2::AddSamples(const vector<WDL_TypedBuf<BL_FLOAT> > &inputs,
                                 const vector<WDL_TypedBuf<BL_FLOAT> > &scInputs)
{
    FftProcessObj15::AddSamples(inputs, scInputs);
    
    int numChannels = GetNumChannels();
    
    // Add again the samples
    // for the two transient processing channels
    FftProcessObj15::AddSamples(numChannels, inputs);
}
