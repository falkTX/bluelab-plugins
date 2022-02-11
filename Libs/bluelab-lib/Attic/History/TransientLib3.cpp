//
//  TransientLib.cpp
//  Transient
//
//  Created by Apple m'a Tuer on 21/05/17.
//
//

#include "../../WDL/fft.h"
#include "../../WDL/IPlug/Containers.h"

#include <CMA2Smoother.h>
#include <BLUtils.h>

#include "TransientLib3.h"

#define DEBUG_GRAPH 0
#if DEBUG_GRAPH
#include <DebugGraph.h>
#endif

// NOTE: This first method works well, even if there something a bit unlogical
void
TransientLib3::DetectTransients(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioFftBuf,
                                BL_FLOAT threshold,
                                BL_FLOAT mix,
                                WDL_TypedBuf<BL_FLOAT> *transientIntensityMagns)
{
#define LOG_MAGN_COEFF 10000.0
    
// ln(10000) = 9 !
#define THRESHOLD_COEFF 9.0
    
    // Compute transients probability (and other things...)
    int bufSize = ioFftBuf->GetSize();
    
    WDL_TypedBuf<BL_FLOAT> strippedMagns;
    strippedMagns.Resize(bufSize);
    
    WDL_TypedBuf<BL_FLOAT> transientMagns;
    transientMagns.Resize(bufSize);
    
    transientIntensityMagns->Resize(bufSize);
    
    // Set to zero
    for (int i = 0; i < bufSize; i++)
        strippedMagns.Get()[i] = 0.0;
    for (int i = 0; i < bufSize; i++)
        transientMagns.Get()[i] = 0.0;
    
    for (int i = 0; i < bufSize; i++)
        transientIntensityMagns->Get()[i] = 0.0;
    
    BL_FLOAT prev = 0.0;
    for (int i = 0; i < bufSize; i++)
    {
        WDL_FFT_COMPLEX c = ioFftBuf->Get()[i];
        BL_FLOAT re = c.re;
        BL_FLOAT im = c.im;
        
        BL_FLOAT magn = std::sqrt(re*re + im*im);
        
        BL_FLOAT phase = 0.0;
        if (std::fabs(re) > 0.0)
	  phase = std::atan2(im, re);
        
        BL_FLOAT phaseDiff = phase - prev;
        prev = phase;
        
        // Niko: Avoid having a big phase diff due to prev == 0
        if (i == 0)
            continue;
        
        // Niko: Works well with a dirac !
        phaseDiff = phaseDiff - M_PI/2.0;
        
        while(phaseDiff < 0.0)
            phaseDiff += 2.0*M_PI;
        
        BL_FLOAT transPos = ((BL_FLOAT)bufSize)*phaseDiff/(2.0*M_PI);
        
        // Must use a coeff, since magn is < 1.0
        BL_FLOAT weight = std::log(1.0 + magn*LOG_MAGN_COEFF);
        if (weight > threshold*THRESHOLD_COEFF)
        {
            transientIntensityMagns->Get()[(int)transPos] += weight;
        }
        
        // Test if the current magn has been used to participate to a transient
        // and classify it
        //
        // NOTE: this is not logical, this leads only to threholding the fft
        // depending on the intensity
        // ... but it seems to work
        if (weight > threshold*THRESHOLD_COEFF)
            transientMagns.Get()[i] = magn;
        else
            strippedMagns.Get()[i] = magn;
    }
    
    for (int i = 0; i < bufSize; i++)
    {
        BL_FLOAT magn;
        BL_FLOAT phase;
        WDL_FFT_COMPLEX comp = ioFftBuf->Get()[i];
        BLUtils::ComplexToMagnPhase(comp, &magn, &phase);
    
        // NOTE: maybe mistake in the direction of the mix parameter
        BL_FLOAT newMagn = mix*strippedMagns.Get()[i] + (1.0 - mix)*transientMagns.Get()[i];
        
        // With that, the result output is at the same scale as the input
#define CORRECTION_COEFF 2.0
        newMagn *= CORRECTION_COEFF;
        
        BLUtils::MagnPhaseToComplex(&comp, newMagn, phase);
        
        ioFftBuf->Get()[i] = comp;
    }
}

// This method works well, even if there something a bit unlogical
void
TransientLib3::DetectTransientsSmooth(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioFftBuf,
                                      BL_FLOAT threshold,
                                      BL_FLOAT mix,
                                      BL_FLOAT precision,
                                      WDL_TypedBuf<BL_FLOAT> *outTransients,
                                      WDL_TypedBuf<BL_FLOAT> *outSmoothedTransients)
{
#define ZERO_POS 0.1
#define THRS_EPS 1e-5
    
    // Set threshold from normalized to dB
    threshold = (1.0 - threshold)*TRANSIENT_INF_LOG;
    
    //
    // Setup
    //
    
    // Compute transients probability (and other things...)
    int bufSize = ioFftBuf->GetSize();
    
    WDL_TypedBuf<BL_FLOAT> transients;
    transients.Resize(bufSize);
    for (int i = 0; i < bufSize; i++)
        transients.Get()[i] = 0.0;
    
    // Map, to find the magn positions who have participated to
    // a given transient value
    vector< vector< int > > transToMagn;
    transToMagn.resize(bufSize);
    
    WDL_TypedBuf<BL_FLOAT> magns;
    magns.Resize(bufSize);
    
#if DEBUG_GRAPH
    WDL_TypedBuf<BL_FLOAT> weightsKeep;
    WDL_TypedBuf<BL_FLOAT> weightsThrow;
#endif
    
    //
    // Compute the transients weights
    // and detect the transients
    //
    BL_FLOAT prev = 0.0;
    for (int i = 0; i < bufSize; i++)
    {
        WDL_FFT_COMPLEX c = ioFftBuf->Get()[i];
        
        BL_FLOAT magn;
        BL_FLOAT phase;
        BLUtils::ComplexToMagnPhase(c, &magn, &phase);
        
        magns.Get()[i] = magn;
        
        BL_FLOAT phaseDiff = phase - prev;
        prev = phase;
        
        // Niko: Avoid having a big phase diff due to prev == 0
        if (i == 0)
        {
#if DEBUG_GRAPH
            weightsThrow.Add(0.0);
            weightsKeep.Add(0.0);
#endif
            continue;
        }
        
        // Niko: Works well with a dirac !
        phaseDiff = phaseDiff - M_PI/2.0;
        
        while(phaseDiff < 0.0)
            phaseDiff += 2.0*M_PI;
        
        BL_FLOAT transPos = phaseDiff*((BL_FLOAT)bufSize)/(2.0*M_PI);
        
        BL_FLOAT weight = std::log(magn*magn);
        if (weight < TRANSIENT_INF_LOG)
            weight = TRANSIENT_INF_LOG;
        weight += -TRANSIENT_INF_LOG;
        
        // Threshold for the tiny magns
        if (weight + TRANSIENT_INF_LOG > threshold)
        {
            // Don't threshold !!
            transients.Get()[(int)transPos] += weight/-TRANSIENT_INF_LOG;
            
#if DEBUG_GRAPH
            weightsKeep.Add(weight);
            weightsThrow.Add(0.0);
#endif
        }
        else
        {
#if DEBUG_GRAPH
            weightsThrow.Add(weight);
            weightsKeep.Add(0.0);
#endif
        }
        
        // In all cases, keep track of the position
        // (it will be used to discard non-transient bins)
        transToMagn[(int)transPos].push_back(i);
    }
    
    //
    // Smooth annd other processings on the curve
    //
    
    // Smoother curve, for keeping non-transient parts
    WDL_TypedBuf<BL_FLOAT> smoothedTransients;
    GetSmoothedTransientsInterp(transients, &smoothedTransients, precision);
    
    // Set the transient curve to log scale
    // with a zoom factor
    //NormalizeCurveLog(&smoothedTransients, zoom);
    NormalizeCurve(&smoothedTransients);
    
    BLUtils::AddValues(&smoothedTransients, ZERO_POS);
    
    //
    // Get the contribution of each source bin depending on the curve
    // (by reversing the process)
    //
    WDL_TypedBuf<BL_FLOAT> newMagns = magns;
    for (int i = 0; i < bufSize; i++)
    {
        BL_FLOAT smoothTrans = smoothedTransients.Get()[i];
        vector<int> &trToMagn = transToMagn[i];
        
        if (smoothTrans > ZERO_POS + THRS_EPS)
        // This is transient !
        {
            for (int j = 0; j < trToMagn.size(); j++)
            {
                int index = trToMagn[j];
                newMagns.Get()[index] *= mix;
            }
        }
        else
        // This is not transient
        {
            for (int j = 0; j < trToMagn.size(); j++)
            {
                int index = trToMagn[j];
               
                newMagns.Get()[index] *= (1.0 - mix);
            }
        }
    }
    
    // TODO: retest without that
#if 0 // Deactivated to try to prevent a crash
    // Normalize, to keep the same volume
    NormalizeVolume(magns, &newMagns);
#endif
    
    // Apply a coefficient
    // So at mix == 0.5, we will keep the same volume
    // Otherwise, we will loose half of the volume
#define COEFF 2.0
    BLUtils::MultValues(&newMagns, COEFF);
    
    //
    // Convert back to complex
    //
    for (int i = 0; i < bufSize; i++)
    {
        BL_FLOAT magn;
        BL_FLOAT phase;
        WDL_FFT_COMPLEX comp = ioFftBuf->Get()[i];
        BLUtils::ComplexToMagnPhase(comp, &magn, &phase);
        
        BL_FLOAT newMagn = newMagns.Get()[i];
        
        BLUtils::MagnPhaseToComplex(&comp, newMagn, phase);
        
        ioFftBuf->Get()[i] = comp;
    }
    
    // Set the result
    if (outTransients != NULL)
        *outTransients = transients;
    
    if (outSmoothedTransients != NULL)
        *outSmoothedTransients = smoothedTransients;
    
    //
    // Graph display
    //
#if DEBUG_GRAPH
    
#define GRAPH_MIN 0.0 
#define GRAPH_WEIGHT_MAX -TRANSIENT_INF_LOG
    
    DebugGraph::SetCurveValues(weightsKeep, 0,
                               GRAPH_MIN, GRAPH_WEIGHT_MAX,
                               1.0, 0, 255, 0);
    
    DebugGraph::SetCurveValues(weightsThrow, 1,
                               GRAPH_MIN, GRAPH_WEIGHT_MAX,
                               1.0, 255, 0, 255);
    
    DebugGraph::SetCurveSingleValue(-TRANSIENT_INF_LOG + threshold, 2,
                                    GRAPH_MIN, GRAPH_WEIGHT_MAX,
                                    2.0,
                                    150, 150, 255,
                                    true, 0.2);
#endif
}

void
TransientLib3::GetSmoothedTransients(const WDL_TypedBuf<BL_FLOAT> &transients,
                                     WDL_TypedBuf<BL_FLOAT> *smoothedTransientsRaw,
                                     WDL_TypedBuf<BL_FLOAT> *smoothedTransientsFine,
                                     int rawSmooth, int fineSmooth)
{
    // Smoother curve, to extract the non-transient part easily
    smoothedTransientsRaw->Resize(transients.GetSize());
    CMA2Smoother::ProcessOne(transients.Get(),
                             smoothedTransientsRaw->Get(),
                             transients.GetSize(),
                             transients.GetSize()/rawSmooth);
    
    // Sharper curve, to extract only transients accurately
    smoothedTransientsFine->Resize(transients.GetSize());
    
    CMA2Smoother::ProcessOne(transients.Get(),
                             smoothedTransientsFine->Get(),
                             transients.GetSize(),
                             transients.GetSize()/fineSmooth);
}

// Compute the nearest two curves, then interpolate between the two curves
// (this is better than computing the two extrema then interpolating)
void
TransientLib3::GetSmoothedTransientsInterp(const WDL_TypedBuf<BL_FLOAT> &transients,
                                           WDL_TypedBuf<BL_FLOAT> *smoothedTransients,
                                           BL_FLOAT precision)
{
#define RAW_SMOOTH_EXP  2
#define FINE_SMOOTH_EXP 6
    
    int numSteps = FINE_SMOOTH_EXP - RAW_SMOOTH_EXP;
    
    BL_FLOAT stepNum = precision*numSteps;
    int stepNumI = (int)stepNum;
    
    BL_FLOAT t = stepNum - stepNumI;
    
    int exps[2];
    exps[0] = RAW_SMOOTH_EXP + stepNumI;
    exps[1] = RAW_SMOOTH_EXP + stepNumI + 1;
    if (exps[1] > FINE_SMOOTH_EXP)
        exps[1] = FINE_SMOOTH_EXP;
    
    int rawSmoothDiv = (int)pow(2.0, exps[0]);
    int fineSmoothDiv = (int)pow(2.0, exps[1]);
    
    WDL_TypedBuf<BL_FLOAT> smoothedTransientsRaw;
    WDL_TypedBuf<BL_FLOAT> smoothedTransientsFine;
    GetSmoothedTransients(transients,
                          &smoothedTransientsRaw, &smoothedTransientsFine,
                          rawSmoothDiv, fineSmoothDiv);
    
    BLUtils::Interp(smoothedTransients, &smoothedTransientsRaw,
                  &smoothedTransientsFine, t);
}

void
TransientLib3::NormalizeVolume(const WDL_TypedBuf<BL_FLOAT> &origin,
                               WDL_TypedBuf<BL_FLOAT> *result)
{
#define EPS 1e-15
    
    // Security
#define NORM_MAX_COEFF 4.0
    
    BL_FLOAT avgOrigin = BLUtils::ComputeAvgSquare(origin);
    BL_FLOAT avgResult = BLUtils::ComputeAvgSquare(*result);
    
    if (avgOrigin > EPS)
    {
        BL_FLOAT coeff = avgOrigin / avgResult;
        
        if (coeff > NORM_MAX_COEFF)
            coeff = NORM_MAX_COEFF;
        
        BLUtils::MultValues(result, coeff);
    }
}

void
TransientLib3::NormalizeCurve(WDL_TypedBuf<BL_FLOAT> *ioCurve)
{
#define NORM_COEFF 0.5
    
    for (int i = 0; i < ioCurve->GetSize(); i++)
    {
        BL_FLOAT val = ioCurve->Get()[i];
        
        // Smoothed curve can go a bit under zero
        // So we need a security for sqrt
        if (val < 0.0)
            val = 0.0;
        
        // sqrt is good for flattening a bit to high curves,
        // and increasing too low curves
        BL_FLOAT res = std::sqrt(val);
        
        // Tried two times sqrt: this is not good, this makes lobes
        
        // We can then multiply by a coefficient, because
        // the curve never goes more than about 50% (75% at the maximum)
        res *= NORM_COEFF;
        
        ioCurve->Get()[i] = res;
    }
}
