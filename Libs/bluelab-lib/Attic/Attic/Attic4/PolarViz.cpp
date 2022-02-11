//
//  PolarViz.cpp
//  BL-PitchShift
//
//  Created by Pan on 22/04/18.
//
//

#include "FftProcessObj16.h"
#include "BLUtils.h"
#include "PolarViz.h"

#define EPS_DB 1e-15
#define MIN_DB -120.0

// To display only on the upper half circle
#define USE_ABS_Y 1

// BAD
// Makes a black hole circle in the graph
#define USE_THREHOLD_DB 0

void
PolarViz::PolarToCartesian(const WDL_TypedBuf<BL_FLOAT> &magns,
                           const WDL_TypedBuf<BL_FLOAT> &phaseDiffs,
                           WDL_TypedBuf<BL_FLOAT> *xValues,
                           WDL_TypedBuf<BL_FLOAT> *yValues,
                           BL_FLOAT thresholdDB)
{
    // Rotate the phases, so we will have the 0 at max Y
    WDL_TypedBuf<BL_FLOAT> phasesRotate = phaseDiffs;
    BLUtils::AddValues(&phasesRotate, (BL_FLOAT)(M_PI/2.0));
    
#if USE_THREHOLD_DB
    // Threshold tiny values
    // This removes many points in output !
    WDL_TypedBuf<BL_FLOAT> magnsThrs;
    WDL_TypedBuf<BL_FLOAT> phasesRotateThrs;
    for (int i = 0; i < magns.GetSize(); i++)
    {
        BL_FLOAT magn = magns.Get()[i];
        BL_FLOAT magnDB = AmpToDB(magn);
        
        BL_FLOAT phase = phasesRotate.Get()[i];
        
        if (magnDB >= thresholdDB)
        {
            magnsThrs.Add(magn);
            phasesRotateThrs.Add(phase);
        }
    }
    
    WDL_TypedBuf<BL_FLOAT> amplitudes;
    BLUtils::AmpToDBNorm(&amplitudes, magnsThrs, EPS_DB, MIN_DB);
    
    BLUtils::PhasesPolarToCartesian(phasesRotateThrs, &amplitudes, xValues, yValues);
#else
    
    WDL_TypedBuf<BL_FLOAT> amplitudes;
    
    //BLDebug::DumpData("dist0.txt", magns);
    
    // ORIGIN
    BLUtils::AmpToDBNorm(&amplitudes, magns, (BL_FLOAT)EPS_DB, (BL_FLOAT)MIN_DB);
    
    //BLDebug::DumpData("dist1.txt", amplitudes);
    
    // TEST
    //amplitudes = magns;
    //BLUtils::MultValues(&amplitudes, 100.0);
    //BLUtils::ApplyPow(&amplitudes, 0.5);
    //BLUtils::MultValues(&amplitudes, 4.0);
    
    BLUtils::PhasesPolarToCartesian(phasesRotate, &amplitudes, xValues, yValues);
#endif
    
#if USE_ABS_Y
    BLUtils::ComputeAbs(yValues);
#endif
}

void
PolarViz::PolarToCartesian2(const WDL_TypedBuf<BL_FLOAT> &Rs,
                            const WDL_TypedBuf<BL_FLOAT> &thetas,
                            WDL_TypedBuf<BL_FLOAT> *xValues,
                            WDL_TypedBuf<BL_FLOAT> *yValues)
{
    //BLUtils::PhasesPolarToCartesian(Rs, &thetas, xValues, yValues);
    BLUtils::PolarToCartesian(Rs, thetas, xValues, yValues);
    
    // Change, to have origin at (0, 1)
    WDL_TypedBuf<BL_FLOAT> tmp = *xValues;
    *xValues = *yValues;
    *yValues = tmp;
    
#if USE_ABS_Y
    BLUtils::ComputeAbs(yValues);
#endif
}

void
PolarViz::PolarSamplesToCartesian(const WDL_TypedBuf<BL_FLOAT> &magns,
                                  const WDL_TypedBuf<BL_FLOAT> &phaseDiffs,
                                  const WDL_TypedBuf<BL_FLOAT> &phases,
                                  WDL_TypedBuf<BL_FLOAT> *xValues,
                                  WDL_TypedBuf<BL_FLOAT> *yValues)
{
    // Rotate the phases, so we will have the 0 at max Y
    WDL_TypedBuf<BL_FLOAT> phasesRotate = phaseDiffs;
    BLUtils::AddValues(&phasesRotate, (BL_FLOAT)(M_PI/2.0));
    
    WDL_TypedBuf<BL_FLOAT> amplitudes;
    
    
    ///
    WDL_TypedBuf<BL_FLOAT> samples;
    
    // TODO: optimize that !
    
    // Get the samples
    WDL_TypedBuf<WDL_FFT_COMPLEX> complex;
    BLUtils::MagnPhaseToComplex(&complex, magns, phases);
    complex.Resize(complex.GetSize()*2);
    BLUtils::FillSecondFftHalf(&complex);
    FftProcessObj16::FftToSamples(complex, &samples);
    
    // TODO: optimize this !
    WDL_TypedBuf<BL_FLOAT> fullPhases;
    WDL_TypedBuf<BL_FLOAT> dummyMagns;
    BLUtils::ComplexToMagnPhase(&dummyMagns, &fullPhases, complex);
    
    WDL_TypedBuf<int> samplesIds;
    //BLUtils::FftIdsToSamplesIds(fullPhases, &samplesIds);
    BLUtils::SamplesIdsToFftIds(fullPhases, &samplesIds);
    BLUtils::Permute(&samples, samplesIds, false); // TODO check direction
    
    BLUtils::TakeHalf(&samples);
    //BLUtils::ScaleNearest(&samples, 0.5);
    
    WDL_TypedBuf<BL_FLOAT> resSamples = samples;
    BLUtils::AmpToDBNorm(&resSamples, samples, (BL_FLOAT)EPS_DB, (BL_FLOAT)MIN_DB); // TODO; check this
    amplitudes = resSamples;

    //
    
    BLUtils::PhasesPolarToCartesian(phasesRotate, &amplitudes, xValues, yValues);
    
#if USE_ABS_Y
    BLUtils::ComputeAbs(yValues);
#endif
}


void
PolarViz::PolarSamplesToCartesian2(const WDL_TypedBuf<BL_FLOAT> &samples,
                                   const WDL_TypedBuf<BL_FLOAT> &phaseDiffs,
                                   const WDL_TypedBuf<BL_FLOAT> &phases,
                                   WDL_TypedBuf<BL_FLOAT> *xValues,
                                   WDL_TypedBuf<BL_FLOAT> *yValues)
{
    // Rotate the phases, so we will have the 0 at max Y
    WDL_TypedBuf<BL_FLOAT> phasesRotate = phaseDiffs;
    BLUtils::AddValues(&phasesRotate, (BL_FLOAT)(M_PI/2.0));
    
    WDL_TypedBuf<BL_FLOAT> amplitudes;
    
    WDL_TypedBuf<int> samplesIds;
    //BLUtils::FftIdsToSamplesIds(fullPhases, &samplesIds);
    BLUtils::SamplesIdsToFftIds(phases, &samplesIds);
    
    WDL_TypedBuf<BL_FLOAT> permSamples = samples;
    BLUtils::Permute(&permSamples, samplesIds, true); // TODO check direction
    
    //BLUtils::TakeHalf(&samples);
    BLUtils::ScaleNearest(&permSamples, 0.5);
    
    BLUtils::AmpToDBNorm(&permSamples, samples, (BL_FLOAT)EPS_DB,(BL_FLOAT)MIN_DB); // TODO; check this
    
    BLUtils::PhasesPolarToCartesian(phasesRotate, &permSamples, xValues, yValues);
    
#if USE_ABS_Y
    BLUtils::ComputeAbs(yValues);
#endif
}
