//
//  DenoiserPostTransientObj.cpp
//  BL-Denoiser
//
//  Created by applematuer on 6/25/20.
//
//

#include <TransientShaperFftObj3.h>
#include <BLUtils.h>

#include "DenoiserPostTransientObj.h"


DenoiserPostTransientObj::DenoiserPostTransientObj(const vector<ProcessObj *> &processObjs,
                                                   int numChannels, int numScInputs,
                                                   int bufferSize, int overlapping, int freqRes,
                                                   BL_FLOAT sampleRate,
                                                   BL_FLOAT freqAmpRatio, BL_FLOAT transBoostFactor)
: PostTransientFftObj3(processObjs, numChannels, numScInputs,
                       bufferSize, overlapping, freqRes,
                       sampleRate, numChannels/2,
                       freqAmpRatio, transBoostFactor) {}

DenoiserPostTransientObj::~DenoiserPostTransientObj() {}

void
DenoiserPostTransientObj::ProcessAllFftSteps()
{
    if (mChannels.size() != 6)
        return;
    
    // First: compute signal objects
    // Then compute noise objects
    // Finally, set the denoised signal to the transient objects
    // (so we apply compute the transientness from the denoised signal
    // to have a better sound)
    //
    // NOTE: transient objects use VARIABLE_HANNING at the origin,
    // here, we use constant Hanning instead (because the denoised
    // signal has constant hanning
    
    // Process the signal channels
    ProcessFftStepChannel(0);
    ProcessFftStepChannel(1);
    
    // Process the noise channels
    ProcessFftStepChannel(2);
    ProcessFftStepChannel(3);
    
    // VERY GOOD !
    // Take the buffers from the signal channels and set them to the transient channels
    // So we process the transientness on the denoised signal
    
    // Signal
    WDL_TypedBuf<WDL_FFT_COMPLEX> *fft0 = GetChannelFft(0);
    WDL_TypedBuf<WDL_FFT_COMPLEX> *fft1 = GetChannelFft(1);
    
    // Transient
    WDL_TypedBuf<WDL_FFT_COMPLEX> *fft4 = GetChannelFft(4);
    WDL_TypedBuf<WDL_FFT_COMPLEX> *fft5 = GetChannelFft(5);
    
    // Assign
    *fft4 = *fft0;
    *fft5 = *fft1;
    
    // Process the transient channels
    ProcessFftStepChannel(4);
    ProcessFftStepChannel(5);
}

void
DenoiserPostTransientObj::ResultSamplesWinReady()
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
        int transIdx = i % 2;
        TransientShaperFftObj3 *transObj = GetTransObj(transIdx);
        if (transObj == NULL)
            continue;
        
        WDL_TypedBuf<BL_FLOAT> transientness;
        transObj->GetCurrentTransientness(&transientness);
        
        // Transient boost
        BLUtils::MultValues(&transientness, mTransientBoostFactor);
        
        //BLUtils::ClipMax(&transientness, 1.0);
        
        // Make a test on the size, because with new latency management,
        // the first buffer may not have been processes
        // Then transientness will be empty at the beginning
        if (transientness.GetSize() == buffer->GetSize())
            transObj->ApplyTransientness(buffer, transientness);
    }
}
