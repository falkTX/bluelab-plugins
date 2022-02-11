//
//  PolarViz.cpp
//  BL-PitchShift
//
//  Created by Pan on 22/04/18.
//
//

#include "FftProcessObj14.h"
#include "Utils.h"
#include "PolarViz.h"

#define EPS_DB 1e-15
#define MIN_DB -120.0

// To display only on the upper half circle
#define USE_ABS_Y 1

void
PolarViz::PolarToCartesian(const WDL_TypedBuf<double> &magns,
                           const WDL_TypedBuf<double> &phaseDiffs,
                           WDL_TypedBuf<double> *xValues,
                           WDL_TypedBuf<double> *yValues)
{
    // Rotate the phases, so we will have the 0 at max Y
    WDL_TypedBuf<double> phasesRotate = phaseDiffs;
    Utils::AddValues(&phasesRotate, M_PI/2.0);
    
    WDL_TypedBuf<double> amplitudes;
 
    ///
    
    //amplitudes = magns;
    Utils::AmpToDBNorm(&amplitudes, magns, EPS_DB, MIN_DB);
    
    ///
    Utils::PhasesPolarToCartesian(phasesRotate, &amplitudes, xValues, yValues);
    
#if USE_ABS_Y
    Utils::ComputeAbs(yValues);
#endif
}

void
PolarViz::PolarSamplesToCartesian(const WDL_TypedBuf<double> &magns,
                                  const WDL_TypedBuf<double> &phaseDiffs,
                                  const WDL_TypedBuf<double> &phases,
                                  WDL_TypedBuf<double> *xValues,
                                  WDL_TypedBuf<double> *yValues)
{
    // Rotate the phases, so we will have the 0 at max Y
    WDL_TypedBuf<double> phasesRotate = phaseDiffs;
    Utils::AddValues(&phasesRotate, M_PI/2.0);
    
    WDL_TypedBuf<double> amplitudes;
    
    
    ///
    WDL_TypedBuf<double> samples;
    
    // TODO: optimize that !
    
    // Get the samples
    WDL_TypedBuf<WDL_FFT_COMPLEX> complex;
    Utils::MagnPhaseToComplex(&complex, magns, phases);
    complex.Resize(complex.GetSize()*2);
    Utils::FillSecondFftHalf(&complex);
    FftProcessObj14::FftToSamples(complex, &samples);
    
    // TODO: optimize this !
    WDL_TypedBuf<double> fullPhases;
    WDL_TypedBuf<double> dummyMagns;
    Utils::ComplexToMagnPhase(&dummyMagns, &fullPhases, complex);
    
    WDL_TypedBuf<int> samplesIds;
    Utils::FftIdsToSamplesIds(fullPhases, &samplesIds);
    Utils::Permute(&samples, samplesIds, false); // TODO check direction
    
    Utils::TakeHalf(&samples);
    
    WDL_TypedBuf<double> resSamples = samples;
    Utils::AmpToDBNorm(&resSamples, samples, EPS_DB, MIN_DB); // TODO; check this
    
    amplitudes = resSamples;

    //
    
    Utils::PhasesPolarToCartesian(phasesRotate, &amplitudes, xValues, yValues);
    
#if USE_ABS_Y
    Utils::ComputeAbs(yValues);
#endif
}
