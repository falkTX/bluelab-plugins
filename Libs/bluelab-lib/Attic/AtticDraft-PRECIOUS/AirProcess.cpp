//
//  AirProcess.cpp
//  BL-Air
//
//  Created by Pan on 20/04/18.
//
//

#include <Utils.h>
#include <Debug.h>
#include <DebugGraph.h>

#include <PartialTracker.h>

#include "AirProcess.h"

// Scales (x and y)
#define FREQ_MEL_SCALE 0
#define FREQ_LOG_SCALE 1

#define MAGNS_TO_DB    1 //0
#define EPS_DB 1e-15
// With -60, avoid taking background noise
// With -80, takes more partials (but some noise)
#define MIN_DB -60.0 //-80.0

#define MEL_SCALE_COEFF 8.0 //4.0
#define LOG_SCALE_COEFF 0.02 //0.01


DEFAULT_SHARPNESS_EXTRACT_NOISE 0.000025 //0.00012 //0.00001 //0.000025

AirProcess::AirProcess(int bufferSize,
                       double overlapping, double oversampling,
                       double sampleRate)
: ProcessObj(bufferSize)
{
    mBufferSize = bufferSize;
    mOverlapping = overlapping;
    mOversampling = oversampling;
    
    mSampleRate = sampleRate;
    
    mPartialTracker = new PartialTracker(bufferSize, sampleRate, overlapping);
    
#if SAS_VIEWER_PROCESS_PROFILE
    BlaTimer::Reset(&mTimer0, &mTimerCount0);
#endif
}

AirProcess::~AirProcess()
{
    delete mPartialTracker;
}

void
AirProcess::Reset()
{
    Reset(mOverlapping, mOversampling, mSampleRate);
}

void
AirProcess::Reset(int overlapping, int oversampling,
                  double sampleRate)
{
    mOverlapping = overlapping;
    mOversampling = oversampling;
    
    mSampleRate = sampleRate;
    
    mValues.Resize(0);
}

void
AirProcess::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                             const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer)

{
    WDL_TypedBuf<WDL_FFT_COMPLEX> fftSamples = *ioBuffer;
    
    // Take half of the complexes
    Utils::TakeHalf(&fftSamples);
    
    WDL_TypedBuf<double> magns;
    WDL_TypedBuf<double> phases;
    Utils::ComplexToMagnPhase(&magns, &phases, fftSamples);
        
    DetectPartials(magns, phases);
    
    if (mPartialTracker != NULL)
    {
        vector<PartialTracker::Partial> partials;
        mPartialTracker->GetPartials(&partials);
        
        // Avoid sending garbage partials to the SASFrame
        // (would slow down a lot when many garbage partial
        // are used to compute the SASFrame)
        PartialTracker::RemoveRealDeadPartials(&partials); // Useless
        
#if 1 // Normal behavior
      // Noise envelope
        mPartialTracker->GetNoiseEnvelope(&magns);
#endif
        
#if 0 // Test
        mPartialTracker->GetHarmonicEnvelope(&magns);
#endif
    }
    
    // For noise envelope
    Utils::MagnPhaseToComplex(ioBuffer, magns, phases);
    ioBuffer->Resize(ioBuffer->GetSize()*2);
    Utils::FillSecondFftHalf(ioBuffer);
}

void
AirProcess::SetThreshold(double threshold)
{
    mPartialTracker->SetThreshold(threshold);
}

void
AirProcess::SetSharpnessExtractNoise(double threshold)
{
    TODO
    
    mPartialTracker->SetSharpnessExtractNoise(threshold);
}

double
AirProcess::AmpToDBNorm(double val)
{
    double result = 0.0;
    
#if MAGNS_TO_DB
    result = Utils::AmpToDBNorm(val, EPS_DB, MIN_DB);
#endif
    
    return result;
}

double
AirProcess::DBToAmpNorm(double val)
{
    double result = 0.0;
    
#if MAGNS_TO_DB
    result = Utils::DBToAmpNorm(val, EPS_DB, MIN_DB);
#endif
    
    return result;
}

void
AirProcess::AmpsToDBNorm(WDL_TypedBuf<double> *amps)
{
    for (int i = 0; i < amps->GetSize(); i++)
    {
        double amp = amps->Get()[i];
        amp = AmpToDBNorm(amp);
        
        amps->Get()[i] = amp;
    }
}

void
AirProcess::ScaleFreqs(WDL_TypedBuf<double> *values)
{
    WDL_TypedBuf<double> valuesRescale = *values;
    
    double hzPerBin = mSampleRate/mBufferSize;
    
#if FREQ_MEL_SCALE
    // Convert to Mel
    
    // Artificially modify the coeff, to increase the spread on the
    hzPerBin *= MEL_SCALE_COEFF;
    
    Utils::FreqsToMelNorm(&valuesRescale, *values, hzPerBin);
#endif
    
#if FREQ_LOG_SCALE
    // Convert to log
    
    // Artificially modify the coeff, to increase the spread on the
    hzPerBin *= LOG_SCALE_COEFF;
    
    Utils::FreqsToLogNorm(&valuesRescale, *values, hzPerBin);
#endif
    
    *values = valuesRescale;
}

int
AirProcess::ScaleFreq(int idx)
{
    double hzPerBin = mSampleRate/mBufferSize;
    
#if FREQ_MEL_SCALE
    // Convert to Mel
    
    // Artificially modify the coeff, to increase the spread on the
    hzPerBin *= MEL_SCALE_COEFF;
    
    int result = Utils::FreqIdToMelNormId(idx, hzPerBin, mBufferSize);
#endif
    
#if FREQ_LOG_SCALE
    // Convert to log
    
    // Artificially modify the coeff, to increase the spread on the
    hzPerBin *= LOG_SCALE_COEFF;
    
    int result = Utils::FreqIdToLogNormId(idx, hzPerBin, mBufferSize);
#endif
    
    return result;
}

void
AirProcess::AmpsToDb(WDL_TypedBuf<double> *magns)
{
#if MAGNS_TO_DB
    WDL_TypedBuf<double> magnsDB;
    Utils::AmpToDBNorm(&magnsDB, *magns, EPS_DB, MIN_DB);
    
    *magns = magnsDB;
#endif
}

void
AirProcess::DetectPartials(const WDL_TypedBuf<double> &magns,
                                 const WDL_TypedBuf<double> &phases)
{
#if AIR_PROCESS_PROFILE
    BlaTimer::Start(&mTimer0);
#endif

    mPartialTracker->SetData(magns, phases);
    mPartialTracker->DetectPartials();
    
    mPartialTracker->ExtractNoiseEnvelope();
    
    //mPartialTracker->FilterPartials();
    
#if AIR_PROCESS_PROFILE
    BlaTimer::StopAndDump(&mTimer0, &mTimerCount0, "profile.txt", "detect: %ld"); //
#endif
}
