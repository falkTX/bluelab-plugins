//
//  AirProcess.cpp
//  BL-Air
//
//  Created by Pan on 20/04/18.
//
//

#include <BLUtils.h>
#include <BLUtilsComp.h>
#include <BLUtilsFft.h>

#include <BLDebug.h>
#include <DebugGraph.h>

#include <PartialTracker3.h>
//#include <TransientShaperFftObj3.h>
//#include <TransientLib4.h>
#include <TransientLib5.h>

#include "AirProcess.h"

#define EPS 1e-15


AirProcess::AirProcess(int bufferSize,
                       BL_FLOAT overlapping, BL_FLOAT oversampling,
                       BL_FLOAT sampleRate)
: ProcessObj(bufferSize)
{
    //mBufferSize = bufferSize;
    mOverlapping = overlapping;
    //mOversampling = oversampling;
    mFreqRes = oversampling;
    mSampleRate = sampleRate;
    
    mPartialTracker = new PartialTracker3(bufferSize, sampleRate, overlapping);
    
    mMix = 0.5;
    mTransientSP = 0.5;

    mTransLib = new TransientLib5();
    //
    //int freqRes = 1;
    //mSTransientObj = new TransientShaperFftObj3(bufferSize, overlapping, freqRes,
    //                                            sampleRate,
    //                                            0.0, 1.0,
    //                                            false);
    //mSTransientObj->SetFreqAmpRatio(0.0);
    
    //
    //mPTransientObj = new TransientShaperFftObj3(bufferSize, overlapping, freqRes,
    //                                           sampleRate,
    //                                           0.0, 1.0,
    //                                           false);
    //mPTransientObj->SetFreqAmpRatio(1.0);
    
    mDebugFreeze = false;
}

AirProcess::~AirProcess()
{
    delete mPartialTracker;

    //delete mSTransientObj;
    //delete mPTransientObj;

    delete mTransLib;
}

void
AirProcess::Reset()
{
    Reset(mOverlapping, mFreqRes/*mOversampling*/, mSampleRate);
    
    // Transient
    mSPRatio.Resize(0);
    mPrevPhases.Resize(0);
}

void
AirProcess::Reset(int overlapping, int oversampling, BL_FLOAT sampleRate)
{
    mOverlapping = overlapping;
    //mOversampling = oversampling;
    mFreqRes = oversampling;
    
    mSampleRate = sampleRate;
}

void
AirProcess::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                             const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer)

{
    WDL_TypedBuf<WDL_FFT_COMPLEX> fftSamples = *ioBuffer;
    
    // Transientness
    //ComputeTransientness(ioBuffer);
    
    // Take half of the complexes
    BLUtils::TakeHalf(&fftSamples);
    
    WDL_TypedBuf<BL_FLOAT> magns;
    WDL_TypedBuf<BL_FLOAT> phases;
    BLUtilsComp::ComplexToMagnPhase(&magns, &phases, fftSamples);
                    
    DetectPartials(magns, phases);
    
    if (mPartialTracker != NULL)
    {
        // "Envelopes"
        
#if 1 // ORIGIN
        // Noise "envelope"
        WDL_TypedBuf<BL_FLOAT> noise;
        mPartialTracker->GetNoiseEnvelope(&noise);
#endif
        
#if 0 // TEST: smooth noise, but remove transients
        BLUtils::GenNoise(&phases);
        BLUtils::MultValues(&phases, 2.0*M_PI);
#endif
        
#if 0 // Same result, with 6 ou 8 dB less
        // Noise "envelope"
        WDL_TypedBuf<BL_FLOAT> noiseEnv;
        mPartialTracker->GetNoiseEnvelope(&noiseEnv);
        
        // Gen noise, and mult by envelope
        WDL_TypedBuf<BL_FLOAT> noise;
        noise.Resize(noiseEnv.GetSize());
        BLUtils::GenNoise(&noise);
        
        BLUtils::MultValues(&noise, noiseEnv);
#endif
        
        // Harmonic "envelope"
        WDL_TypedBuf<BL_FLOAT> harmo;
        mPartialTracker->GetHarmonicEnvelope(&harmo);

        BL_FLOAT noiseCoeff;
        BL_FLOAT harmoCoeff;
        BLUtils::MixParamToCoeffs(mMix, &noiseCoeff, &harmoCoeff);
        
        // Result
        WDL_TypedBuf<BL_FLOAT> newMagns;
        newMagns.Resize(magns.GetSize());
        for (int i = 0; i < newMagns.GetSize(); i++)
        {
            BL_FLOAT n = noise.Get()[i];
            BL_FLOAT h = harmo.Get()[i];
            
            BL_FLOAT val = n*noiseCoeff + h*harmoCoeff;
            newMagns.Get()[i] = val;
        }
        magns = newMagns;
    }
    
    // For noise envelope
    BLUtilsComp::MagnPhaseToComplex(ioBuffer, magns, phases);
    ioBuffer->Resize(ioBuffer->GetSize()*2);
    BLUtilsFft::FillSecondFftHalf(ioBuffer);
}

void
AirProcess::SetThreshold(BL_FLOAT threshold)
{
    mPartialTracker->SetThreshold(threshold);
}

void
AirProcess::SetMix(BL_FLOAT mix)
{
    mMix = mix;
}

void
AirProcess::SetTransientSP(BL_FLOAT transientSP)
{
    mTransientSP = transientSP;
}

void
AirProcess::DetectPartials(const WDL_TypedBuf<BL_FLOAT> &magns,
                           const WDL_TypedBuf<BL_FLOAT> &phases)
{    
    if (!mDebugFreeze)
        mPartialTracker->SetData(magns, phases);
    
    mPartialTracker->DetectPartials();
    
    // Filter or not ?
    //
    // If we filter and we get zombie partials
    // this would make musical noise
    mPartialTracker->FilterPartials();
    
    mPartialTracker->ExtractNoiseEnvelope();
}

void
AirProcess::ComputeTransientness(const WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer)
{
    WDL_TypedBuf<BL_FLOAT> magns;
    WDL_TypedBuf<BL_FLOAT> phases;
    BLUtilsComp::ComplexToMagnPhase(&magns, &phases, *ioBuffer);
    
    WDL_TypedBuf<BL_FLOAT> transS;
    BL_FLOAT freqAmpRatioS = 0.0;
    mTransLib->ComputeTransientness2(magns, phases,
                                     &mPrevPhases,
                                     freqAmpRatioS,
                                     1.0, &transS);
    
    WDL_TypedBuf<BL_FLOAT> transP;
    BL_FLOAT freqAmpRatioP = 1.0;
    mTransLib->ComputeTransientness2(magns, phases,
                                     &mPrevPhases,
                                     freqAmpRatioP,
                                     1.0, &transP);
    
    mSPRatio.Resize(magns.GetSize());
    for (int i = 0; i < mSPRatio.GetSize(); i++)
    {
        BL_FLOAT s = transS.Get()[i];
        BL_FLOAT p = transP.Get()[i];
        
        BL_FLOAT ratio = 1.0;
        if (s + p > EPS)
        {
            ratio = s/(s + p);
        }
        
        mSPRatio.Get()[i] = ratio;
    }
    
    mPrevPhases = phases;
}
