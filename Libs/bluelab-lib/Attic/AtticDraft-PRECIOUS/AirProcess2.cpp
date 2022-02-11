//
//  AirProcess2.cpp
//  BL-Air
//
//  Created by Pan on 20/04/18.
//
//

#include <Utils.h>
#include <Debug.h>
#include <DebugGraph.h>

#include <PartialTracker3.h>

#include "AirProcess2.h"

#define EPS 1e-15


AirProcess2::AirProcess2(int bufferSize,
                        double overlapping, double oversampling,
                        double sampleRate)
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
    Reset(mOverlapping, mOversampling, mSampleRate);
}

void
AirProcess2::Reset(int overlapping, int oversampling,
                   double sampleRate)
{
    mOverlapping = overlapping;
    mOversampling = oversampling;
    
    mSampleRate = sampleRate;
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
    Utils::TakeHalf(&fftSamples);
    
    WDL_TypedBuf<double> magns;
    WDL_TypedBuf<double> phases;
    Utils::ComplexToMagnPhase(&magns, &phases, fftSamples);
    
    Debug::DumpData("magns0.txt", magns);// TEST
    
    DetectPartials(magns, phases);
        
    if (mPartialTracker != NULL)
    {
        // "Envelopes"
        
#if 1 // ORIGIN
        // Noise "envelope"
        WDL_TypedBuf<double> noise;
        mPartialTracker->GetNoiseEnvelope(&noise);
#endif
        
#if 0 // TEST: smooth noise, but remove transients
        Utils::GenNoise(&phases);
        Utils::MultValues(&phases, 2.0*M_PI);
#endif
        
#if 0 // Same result, with 6 ou 8 dB less
        // Noise "envelope"
        WDL_TypedBuf<double> noiseEnv;
        mPartialTracker->GetNoiseEnvelope(&noiseEnv);
        
        // Gen noise, and mult by envelope
        WDL_TypedBuf<double> noise;
        noise.Resize(noiseEnv.GetSize());
        Utils::GenNoise(&noise);
        
        Utils::MultValues(&noise, noiseEnv);
#endif
        
        // Harmonic "envelope"
        WDL_TypedBuf<double> harmo;
        mPartialTracker->GetHarmonicEnvelope(&harmo);
        
#if 0   // TEST: try to have exactly the same signal when mix is at 0
        // => dos not give exactly the same signal with mix at 0
        // => maxes musical noise when mix is at -100.0
        harmo = magns;
        Utils::SubstractValues(&harmo, noise);
        Utils::ClipMin(&harmo, 0.0);
#endif
                
        double noiseCoeff;
        double harmoCoeff;
        Utils::MixParamToCoeffs(mMix, &noiseCoeff, &harmoCoeff);
        
        Debug::DumpData("harmo.txt", harmo);// TEST
        Debug::DumpData("noise.txt", noise);// TEST
        
        // Result
        WDL_TypedBuf<double> newMagns;
        newMagns.Resize(magns.GetSize());
        for (int i = 0; i < newMagns.GetSize(); i++)
        {
            double n = noise.Get()[i];
            double h = harmo.Get()[i];
            
            double val = n*noiseCoeff + h*harmoCoeff;
            newMagns.Get()[i] = val;
        }
        magns = newMagns;
    }
    
    Debug::DumpData("magns1.txt", magns);// TEST
    
    // For noise envelope
    Utils::MagnPhaseToComplex(ioBuffer, magns, phases);
    ioBuffer->Resize(ioBuffer->GetSize()*2);
    Utils::FillSecondFftHalf(ioBuffer);
    
#if AIR_PROCESS_PROFILE
    BlaTimer::StopAndDump(&mTimer, &mCount, "AirProcess-profile.txt", "%ld");
#endif
}

void
AirProcess2::SetThreshold(double threshold)
{
    mPartialTracker->SetThreshold(threshold);
}

void
AirProcess2::SetMix(double mix)
{
    mMix = mix;
}

void
AirProcess2::DetectPartials(const WDL_TypedBuf<double> &magns,
                           const WDL_TypedBuf<double> &phases)
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
