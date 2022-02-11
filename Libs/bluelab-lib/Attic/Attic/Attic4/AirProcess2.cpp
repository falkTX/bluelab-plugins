//
//  AirProcess2.cpp
//  BL-Air
//
//  Created by Pan on 20/04/18.
//
//

#include <BLUtils.h>
#include <DebugGraph.h>

#include <PartialTracker3.h>

#include "AirProcess2.h"

#define EPS 1e-15

#define FIX_RESET 1


AirProcess2::AirProcess2(int bufferSize,
                        BL_FLOAT overlapping, BL_FLOAT oversampling,
                        BL_FLOAT sampleRate)
: ProcessObj(bufferSize)
{
    mBufferSize = bufferSize;
    mOverlapping = overlapping;
    mOversampling = oversampling;
    
    mSampleRate = sampleRate;
    
    mPartialTracker = new PartialTracker3(bufferSize, sampleRate, overlapping);
    
    mMix = 0.5;
    mTransientSP = 0.5;
    
    mDebugFreeze = false;
    
#if AIR_PROCESS_PROFILE
    BlaTimer::Reset(&mTimer, &mCount);
#endif
}

AirProcess2::~AirProcess2()
{
    delete mPartialTracker;
}

void
AirProcess2::Reset()
{
    Reset(mBufferSize, mOverlapping, mOversampling, mSampleRate);
}

void
AirProcess2::Reset(int bufferSize, int overlapping, int oversampling,
                   BL_FLOAT sampleRate)
{
    mBufferSize = bufferSize;
    
    mOverlapping = overlapping;
    mOversampling = oversampling;
    
    mSampleRate = sampleRate;
    
#if FIX_RESET
    mPartialTracker->Reset();
#endif
}

void
AirProcess2::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                              const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer)

{
#if AIR_PROCESS_PROFILE
    BlaTimer::Start(&mTimer);
#endif
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> fftSamples = *ioBuffer;
    
    // Take half of the complexes
    BLUtils::TakeHalf(&fftSamples);
    
    WDL_TypedBuf<BL_FLOAT> magns;
    WDL_TypedBuf<BL_FLOAT> phases;
    BLUtils::ComplexToMagnPhase(&magns, &phases, fftSamples);
    
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
        
#if 0   // TEST: try to have exactly the same signal when mix is at 0
        // => dos not give exactly the same signal with mix at 0
        // => maxes musical noise when mix is at -100.0
        harmo = magns;
        BLUtils::SubstractValues(&harmo, noise);
        BLUtils::ClipMin(&harmo, 0.0);
#endif
                
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
    BLUtils::MagnPhaseToComplex(ioBuffer, magns, phases);
    ioBuffer->Resize(ioBuffer->GetSize()*2);
    BLUtils::FillSecondFftHalf(ioBuffer);
    
#if AIR_PROCESS_PROFILE
    BlaTimer::StopAndDump(&mTimer, &mCount, "AirProcess-profile.txt", "%ld");
#endif
}

void
AirProcess2::SetThreshold(BL_FLOAT threshold)
{
    mPartialTracker->SetThreshold(threshold);
}

void
AirProcess2::SetMix(BL_FLOAT mix)
{
    mMix = mix;
}

void
AirProcess2::DetectPartials(const WDL_TypedBuf<BL_FLOAT> &magns,
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
