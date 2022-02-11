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
#include <Utils.h>
#include <Debug.h>
#include <CMA2Smoother.h>

#include "TransientLib4.h"

#define DEBUG_GRAPH 0
#if DEBUG_GRAPH
#include <DebugGraph.h>
#endif

// NOTE: This first method works well, even if there something a bit unlogical
void
TransientLib4::DetectTransients(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioFftBuf,
                                double threshold,
                                double mix,
                                WDL_TypedBuf<double> *transientIntensityMagns)
{
#define LOG_MAGN_COEFF 10000.0
    
// ln(10000) = 9 !
#define THRESHOLD_COEFF 9.0
    
    // Compute transients probability (and other things...)
    int bufSize = ioFftBuf->GetSize();
    
    WDL_TypedBuf<double> strippedMagns;
    strippedMagns.Resize(bufSize);
    
    WDL_TypedBuf<double> transientMagns;
    transientMagns.Resize(bufSize);
    
    transientIntensityMagns->Resize(bufSize);
    
    // Set to zero
    for (int i = 0; i < bufSize; i++)
        strippedMagns.Get()[i] = 0.0;
    for (int i = 0; i < bufSize; i++)
        transientMagns.Get()[i] = 0.0;
    
    for (int i = 0; i < bufSize; i++)
        transientIntensityMagns->Get()[i] = 0.0;
    
    double prev = 0.0;
    for (int i = 0; i < bufSize; i++)
    {
        WDL_FFT_COMPLEX c = ioFftBuf->Get()[i];
        double re = c.re;
        double im = c.im;
        
        double magn = sqrt(re*re + im*im);
        
        double phase = 0.0;
        if (fabs(re) > 0.0)
            phase = atan2(im, re);
        
        double phaseDiff = phase - prev;
        prev = phase;
        
        // Niko: Avoid having a big phase diff due to prev == 0
        if (i == 0)
            continue;
        
        // Niko: Works well with a dirac !
        phaseDiff = phaseDiff - M_PI/2.0;
        
        while(phaseDiff < 0.0)
            phaseDiff += 2.0*M_PI;
        
        double transPos = ((double)bufSize)*phaseDiff/(2.0*M_PI);
        
        // Must use a coeff, since magn is < 1.0
        double weight = log(1.0 + magn*LOG_MAGN_COEFF);
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
        double magn;
        double phase;
        WDL_FFT_COMPLEX comp = ioFftBuf->Get()[i];
        Utils::ComplexToMagnPhase(comp, &magn, &phase);
    
        // NOTE: maybe mistake in the direction of the mix parameter
        double newMagn = mix*strippedMagns.Get()[i] + (1.0 - mix)*transientMagns.Get()[i];
        
        // With that, the result output is at the same scale as the input
#define CORRECTION_COEFF 2.0
        newMagn *= CORRECTION_COEFF;
        
        Utils::MagnPhaseToComplex(&comp, newMagn, phase);
        
        ioFftBuf->Get()[i] = comp;
    }
}

// This method works well, even if there something a bit unlogical
void
TransientLib4::DetectTransientsSmooth(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioFftBuf,
                                      double threshold,
                                      double mix,
                                      double precision,
                                      WDL_TypedBuf<double> *outTransients,
                                      WDL_TypedBuf<double> *outSmoothedTransients)
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
    
    WDL_TypedBuf<double> transients;
    transients.Resize(bufSize);
    for (int i = 0; i < bufSize; i++)
        transients.Get()[i] = 0.0;
    
    // Map, to find the magn positions who have participated to
    // a given transient value
    vector< vector< int > > transToMagn;
    transToMagn.resize(bufSize);
    
    WDL_TypedBuf<double> magns;
    magns.Resize(bufSize);
    
#if DEBUG_GRAPH
    WDL_TypedBuf<double> weightsKeep;
    WDL_TypedBuf<double> weightsThrow;
#endif
    
    //
    // Compute the transients weights
    // and detect the transients
    //
    double prev = 0.0;
    for (int i = 0; i < bufSize; i++)
    {
        WDL_FFT_COMPLEX c = ioFftBuf->Get()[i];
        
        double magn;
        double phase;
        Utils::ComplexToMagnPhase(c, &magn, &phase);
        
        magns.Get()[i] = magn;
        
        double phaseDiff = phase - prev;
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
        
        double transPos = phaseDiff*((double)bufSize)/(2.0*M_PI);
        
        double weight = log(magn*magn);
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
    WDL_TypedBuf<double> smoothedTransients;
    GetSmoothedTransientsInterp(transients, &smoothedTransients, precision);
    
    // Set the transient curve to log scale
    // with a zoom factor
    //NormalizeCurveLog(&smoothedTransients, zoom);
    NormalizeCurve(&smoothedTransients);
    
    Utils::AddValues(&smoothedTransients, ZERO_POS);
    
    //
    // Get the contribution of each source bin depending on the curve
    // (by reversing the process)
    //
    WDL_TypedBuf<double> newMagns = magns;
    for (int i = 0; i < bufSize; i++)
    {
        double smoothTrans = smoothedTransients.Get()[i];
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
    Utils::MultValues(&newMagns, COEFF);
    
    //
    // Convert back to complex
    //
    for (int i = 0; i < bufSize; i++)
    {
        double magn;
        double phase;
        WDL_FFT_COMPLEX comp = ioFftBuf->Get()[i];
        Utils::ComplexToMagnPhase(comp, &magn, &phase);
        
        double newMagn = newMagns.Get()[i];
        
        Utils::MagnPhaseToComplex(&comp, newMagn, phase);
        
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
TransientLib4::DetectTransientsSmooth2(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioFftBuf,
                                       double threshold,
                                       double mix,
                                       double precision,
                                       WDL_TypedBuf<double> *outTransients,
                                       WDL_TypedBuf<double> *outSmoothedTransients)
{    
    //
    // Setup
    //
    
    // Compute transients probability (and other things...)
    int bufSize = ioFftBuf->GetSize();
    
    WDL_TypedBuf<double> transients;
    transients.Resize(bufSize);
    for (int i = 0; i < bufSize; i++)
        transients.Get()[i] = 0.0;
    
    // Map, to find the magn positions who have participated to
    // a given transient value
    vector< vector< int > > transToMagn;
    transToMagn.resize(bufSize);
    
    WDL_TypedBuf<double> magns;
    magns.Resize(bufSize);
    
#if DEBUG_GRAPH // Not working anymore (must be scaled...)
    WDL_TypedBuf<double> weightsKeep;
    WDL_TypedBuf<double> weightsThrow;
#endif
    
    //
    // Compute the transients weights
    // and detect the transients
    //
    double prev = 0.0;
    for (int i = 0; i < bufSize; i++)
    {
        WDL_FFT_COMPLEX c = ioFftBuf->Get()[i];
        
        double magn;
        double phase;
        Utils::ComplexToMagnPhase(c, &magn, &phase);
        
        magns.Get()[i] = magn;
        
        double phaseDiff = phase - prev;
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
        
        double transPos = phaseDiff*((double)bufSize)/(2.0*M_PI);
        
        // Amplifies less very small (and numerous) values
        double weight = log(1.0 + magn);
        
        transients.Get()[(int)transPos] += weight;
        // In all cases, keep track of the position
        // (it will be used to discard non-transient bins)
        transToMagn[(int)transPos].push_back(i);
    }
    
    //
    // Smooth annd other processings on the curve
    //
    
    // Smoother curve, for keeping non-transient parts
    WDL_TypedBuf<double> smoothedTransients;
    
    GetSmoothedTransientsInterp(transients, &smoothedTransients, precision);
    
    //
    // Scale adjustment
    //
    
    // First, multpiply by a big constant coeff
#define CURVE_COEFF 500.0
    Utils::MultValues(&smoothedTransients, CURVE_COEFF);
    
    // Compute a scale, depending on the precision
    // Because when precision is high, some values are not very smoothed,
    // so they are big
#define PRECISION0_COEFF 1.0
#define PRECISION1_COEFF 0.2
    double scaleCoeff = (1.0 - precision)*PRECISION0_COEFF + precision*PRECISION1_COEFF;
    
    Utils::MultValues(&smoothedTransients, scaleCoeff);
    
    //
    // Get the contribution of each source bin depending on the curve
    // (by reversing the process)
    //
    WDL_TypedBuf<double> newMagns = magns;
    for (int i = 0; i < bufSize; i++)
    {
        //double smoothTrans = transients.Get()[i]; //smoothedTransients.Get()[i];
        double smoothTrans = smoothedTransients.Get()[i];
        vector<int> &trToMagn = transToMagn[i];
        
      // Temporal smooth with previous values
      // Avoids "silence holes" when curves moves very quickly
      // (and then is thresholdedd in a very not-continuous way)
      //
      // Works great !
      // (transient amplification and attenuation seems better)
#define SMOOTH_COEFF 0.8
        
        if ((outSmoothedTransients != NULL) &&
            (outSmoothedTransients->GetSize() == smoothedTransients.GetSize()))
            // We have previous values
        {
            double prevSmoothTrans = outSmoothedTransients->Get()[i];
            smoothTrans = SMOOTH_COEFF*prevSmoothTrans + (1.0 - SMOOTH_COEFF)*smoothTrans;
            smoothedTransients.Get()[i] = smoothTrans;
        }
        
#if 0   // Test, to try to avoid "silence holes"
        // (not convincing)
        double factor = GetThresholdedFactor(smoothTrans, threshold, mix);
        for (int j = 0; j < trToMagn.size(); j++)
        {
            int index = trToMagn[j];
            newMagns.Get()[index] *= factor;
        }
#endif
        
        if (smoothTrans > threshold)
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
    Utils::MultValues(&newMagns, COEFF);
    
    //
    // Convert back to complex
    //
    for (int i = 0; i < bufSize; i++)
    {
        double magn;
        double phase;
        WDL_FFT_COMPLEX comp = ioFftBuf->Get()[i];
        Utils::ComplexToMagnPhase(comp, &magn, &phase);
        
        double newMagn = newMagns.Get()[i];
        
        Utils::MagnPhaseToComplex(&comp, newMagn, phase);
        
        ioFftBuf->Get()[i] = comp;
    }
    
    // Set the result
    if (outTransients != NULL)
    {
        *outTransients = transients;
        
        Utils::MultValues(outTransients, CURVE_COEFF*scaleCoeff);
    }
    
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

//
// See (again): http://werner.yellowcouch.org/Papers/transients12/index.html
//
void
TransientLib4::ComputeTransientness(const WDL_TypedBuf<double> &magns,
                                    const WDL_TypedBuf<double> &phases,
                                    const WDL_TypedBuf<double> *prevPhases,
                                    bool freqsToTrans,
                                    bool ampsToTrans,
                                    double smoothFactor,
                                    WDL_TypedBuf<double> *transientness)
{
#define THRESHOLD -64.0
#define DB_EPS 1e-15
    
//#define TRANS_COEFF_FREQ 0.5
#define TRANS_COEFF_FREQ 1.0
#define TRANS_COEFF_AMP 0.25
    
    transientness->Resize(phases.GetSize());
    Utils::FillAllZero(transientness);
    
    WDL_TypedBuf<int> sampleIds;
    Utils::FftIdsToSamplesIds(phases, &sampleIds);
    
    for (int i = 0; i < sampleIds.GetSize(); i++)
    {
        int sampleId = sampleIds.Get()[i];
        
        double freqWeight = 0.0;
        if (freqsToTrans)
        {
            //Do as Werner Van Belle
            double magn = magns.Get()[i];
    
            double magnDB = Utils::AmpToDB(magn, DB_EPS, THRESHOLD);
            if (magnDB > THRESHOLD)
            {
                double w = -(magnDB - THRESHOLD)/THRESHOLD;
                    
                w *= TRANS_COEFF_FREQ;
                    
                freqWeight = w;
            }
        }
        
        double ampWeight = 0.0;
        if (ampsToTrans &&
            (prevPhases != NULL) && (prevPhases->GetSize() == sampleIds.GetSize()))
        {
            double magn = magns.Get()[i];
            double magnDB = Utils::AmpToDB(magn, DB_EPS, THRESHOLD);
            if (magnDB > THRESHOLD)
            {
                // Use additional method: compute derivative of phase over time
                // This is a very good indicator of transientness !
            
                double phase0 = prevPhases->Get()[i];
                double phase1 = phases.Get()[i];
            
                // Ensure that phase1 is greater than phase0
                while(phase1 < phase0)
                    phase1 += 2.0*M_PI;
            
                double delta = phase1 - phase0;
            
                delta = fmod(delta, 2.0*M_PI);
            
                if (delta > M_PI)
                    delta = 2.0*M_PI - delta;
            
                double w = delta/M_PI;
            
#if 0
                // TODO: check this
            
                // Threshold, to avoid having a continuous value of transientness
                w -= 0.5;
                if (w < 0.0)
                    w = 0.0;
#endif
                w *= TRANS_COEFF_AMP;
            
                ampWeight = w;
            }
        }
        
        double sumWeights = freqWeight + ampWeight;
        
        // Avoid increasing the transientness when we choose both
        if (freqsToTrans && ampsToTrans)
            sumWeights /= 2.0;
        
        transientness->Get()[sampleId] += sumWeights;
    }
    
    // At the end, the transientness is reversed
    // So reverse back...
    Utils::Reverse(transientness);
    
    // Smooth if necessary
    if (smoothFactor > 0.0)
    {
#define SMOOTH_FACTOR 4.0
        
        WDL_TypedBuf<double> smoothTransientness;
        smoothTransientness.Resize(transientness->GetSize());
        
        double cmaCoeff = smoothFactor*transientness->GetSize()/SMOOTH_FACTOR;
        
        int cmaWindowSize = (int)cmaCoeff;
        
        CMA2Smoother::ProcessOne(transientness->Get(),
                                 smoothTransientness.Get(),
                                 transientness->GetSize(),
                                 cmaWindowSize);
        
        *transientness = smoothTransientness;
    }
    
    // After smoothing, we can have negative values
    Utils::ClipMin(transientness, 0.0);
}

void
TransientLib4::ComputeTransientnessMix(WDL_TypedBuf<double> *magns,
                                       const WDL_TypedBuf<double> &phases,
                                       const WDL_TypedBuf<double> *prevPhases,
                                       //double threshold,
                                       const WDL_TypedBuf<double> &thresholds,
                                       double mix,
                                       bool freqsToTrans,
                                       bool ampsToTrans,
                                       double smoothFactor,
                                       WDL_TypedBuf<double> *transientness)
{
    //
    // FirstPhase phase: compute the contribution of each bin to the transoentness
    // and compute the transientness
    //
    
    // TODO
#define THRESHOLD -64.0
#define DB_EPS 1e-15
    
//#define TRANS_COEFF_FREQ 0.5
#define TRANS_COEFF_FREQ 1.0
#define TRANS_COEFF_AMP 0.25
    
#define TRANS_SCALE 8.0
    
    transientness->Resize(phases.GetSize());
    Utils::FillAllZero(transientness);
    
    WDL_TypedBuf<int> sampleIds;
    Utils::FftIdsToSamplesIds(phases, &sampleIds);
    
    // Map, to find the magn positions who have participated to
    // a given transient value
    vector< vector< int > > transToMagn;
    transToMagn.resize(magns->GetSize());
    
    for (int i = 0; i < sampleIds.GetSize(); i++)
    {
        int sampleId = sampleIds.Get()[i];
        
        double freqWeight = 0.0;
        if (freqsToTrans)
        {
            //Do as Werner Van Belle
            double magn = magns->Get()[i];
            
            double magnDB = Utils::AmpToDB(magn, DB_EPS, THRESHOLD);
            if (magnDB > THRESHOLD)
            {
                double w = -(magnDB - THRESHOLD)/THRESHOLD;
                
                w *= TRANS_COEFF_FREQ;
                
                freqWeight = w;
            }
        }
        
        double ampWeight = 0.0;
        if (ampsToTrans &&
            (prevPhases != NULL) && (prevPhases->GetSize() == sampleIds.GetSize()))
        {
            double magn = magns->Get()[i];
            double magnDB = Utils::AmpToDB(magn, DB_EPS, THRESHOLD);
            if (magnDB > THRESHOLD)
            {
                // Use additional method: compute derivative of phase over time
                // This is a very good indicator of transientness !
                
                double phase0 = prevPhases->Get()[i];
                double phase1 = phases.Get()[i];
                
                // Ensure that phase1 is greater than phase0
                while(phase1 < phase0)
                    phase1 += 2.0*M_PI;
                
                double delta = phase1 - phase0;
                
                delta = fmod(delta, 2.0*M_PI);
                
                if (delta > M_PI)
                    delta = 2.0*M_PI - delta;
                
                double w = delta/M_PI;
                
#if 0
                // TODO: check this
                
                // Threshold, to avoid having a continuous value of transientness
                w -= 0.5;
                if (w < 0.0)
                    w = 0.0;
#endif
                w *= TRANS_COEFF_AMP;
                
                ampWeight = w;
            }
        }
        
        double sumWeights = freqWeight + ampWeight;
        
        // Avoid increasing the transientness when we choose both
        if (freqsToTrans && ampsToTrans)
            sumWeights /= 2.0;
        
        transientness->Get()[sampleId] += sumWeights;
        
        // In all cases, keep track of the position
        // (it will be used to discard non-transient bins)
        transToMagn[(int)sampleId].push_back(i);
    }
    
    // At the end, the transientness is reversed
    // So reverse back...
    Utils::Reverse(transientness);
    
    // Smooth if necessary
    if (smoothFactor > 0.0)
    {
#define SMOOTH_FACTOR 4.0
        
        WDL_TypedBuf<double> smoothTransientness;
        smoothTransientness.Resize(transientness->GetSize());
        
        double cmaCoeff = smoothFactor*transientness->GetSize()/SMOOTH_FACTOR;
        
        int cmaWindowSize = (int)cmaCoeff;
        
        CMA2Smoother::ProcessOne(transientness->Get(),
                                 smoothTransientness.Get(),
                                 transientness->GetSize(),
                                 cmaWindowSize);
        
        *transientness = smoothTransientness;
    }
    
    // After smoothing, we can have negative values
    Utils::ClipMin(transientness, 0.0);
    
    Utils::MultValues(transientness, TRANS_SCALE);
    
    //
    // Second phase: reverse the process and mix
    //
    
    // Get the contribution of each source bin depending on the curve
    WDL_TypedBuf<double> newMagns = *magns;
    for (int i = 0; i < magns->GetSize(); i++)
    {
        //double smoothTrans = transients.Get()[i]; //smoothedTransients.Get()[i];
        double trans = transientness->Get()[i];
        vector<int> &trToMagn = transToMagn[i];
        
        //
        // NOTE: the temporal smooting has been removed !!!
        //
        
#if 0
        int sampId = sampleIds.Get()[i];
        double threshold = thresholds.Get()[sampId];
#endif
        double threshold = thresholds.Get()[i];
        
        // Ignore the zero !
        // Otherwise, when thrs = 0 and mix = 100%,
        // we would still have some sound !
        
        if (trans > threshold)
            // This is transient !
        {
            for (int j = 0; j < trToMagn.size(); j++)
            {
                int index = trToMagn[j];
                newMagns.Get()[index] *= mix;
            }
        }
        else if (trans < threshold)
            // This is not transient
        {
            for (int j = 0; j < trToMagn.size(); j++)
            {
                int index = trToMagn[j];
                
                newMagns.Get()[index] *= (1.0 - mix);
            }
        } else if ((trans == 0.0) && (threshold == 0.0))
        {
            for (int j = 0; j < trToMagn.size(); j++)
            {
                int index = trToMagn[j];
                
                newMagns.Get()[index] *= 0.0;
            }
        }
    }
    
#if 0
    Debug::DumpData("magns.txt", *magns);
    Debug::DumpData("new-magns.txt", newMagns);
    Debug::DumpData("trans.txt", *transientness);
    Debug::DumpData("thresholds.txt", thresholds);
    
    static WDL_TypedBuf<double> maxTrans = *transientness;
    Utils::ComputeMax(&maxTrans, *transientness);
    Debug::DumpData("max-trans.txt", maxTrans);
#endif
    
    // TODO: maybe apply a coeff...
    
    *magns = newMagns;
    
    // TODO: maybe output the raw transients...
}

void
TransientLib4::GetSmoothedTransients(const WDL_TypedBuf<double> &transients,
                                     WDL_TypedBuf<double> *smoothedTransientsRaw,
                                     WDL_TypedBuf<double> *smoothedTransientsFine,
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
TransientLib4::GetSmoothedTransientsInterp(const WDL_TypedBuf<double> &transients,
                                           WDL_TypedBuf<double> *smoothedTransients,
                                           double precision)
{
#define RAW_SMOOTH_EXP  2
#define FINE_SMOOTH_EXP 6
    
    int numSteps = FINE_SMOOTH_EXP - RAW_SMOOTH_EXP;
    
    double stepNum = precision*numSteps;
    int stepNumI = (int)stepNum;
    
    double t = stepNum - stepNumI;
    
    int exps[2];
    exps[0] = RAW_SMOOTH_EXP + stepNumI;
    exps[1] = RAW_SMOOTH_EXP + stepNumI + 1;
    if (exps[1] > FINE_SMOOTH_EXP)
        exps[1] = FINE_SMOOTH_EXP;
    
    int rawSmoothDiv = (int)pow(2.0, exps[0]);
    int fineSmoothDiv = (int)pow(2.0, exps[1]);
    
    WDL_TypedBuf<double> smoothedTransientsRaw;
    WDL_TypedBuf<double> smoothedTransientsFine;
    GetSmoothedTransients(transients,
                          &smoothedTransientsRaw, &smoothedTransientsFine,
                          rawSmoothDiv, fineSmoothDiv);
    
    Utils::Interp(smoothedTransients, &smoothedTransientsRaw,
                  &smoothedTransientsFine, t);
}

void
TransientLib4::NormalizeVolume(const WDL_TypedBuf<double> &origin,
                               WDL_TypedBuf<double> *result)
{
#define EPS 1e-15
    
    // Security
#define NORM_MAX_COEFF 4.0
    
    double avgOrigin = Utils::ComputeAvgSquare(origin);
    double avgResult = Utils::ComputeAvgSquare(*result);
    
    if (avgOrigin > EPS)
    {
        double coeff = avgOrigin / avgResult;
        
        if (coeff > NORM_MAX_COEFF)
            coeff = NORM_MAX_COEFF;
        
        Utils::MultValues(result, coeff);
    }
}

void
TransientLib4::NormalizeCurve(WDL_TypedBuf<double> *ioCurve)
{
#define NORM_COEFF 0.5
    
    for (int i = 0; i < ioCurve->GetSize(); i++)
    {
        double val = ioCurve->Get()[i];
        
        // Smoothed curve can go a bit under zero
        // So we need a security for sqrt
        if (val < 0.0)
            val = 0.0;
        
        // sqrt is good for flattening a bit to high curves,
        // and increasing too low curves
        double res = sqrt(val);
        
        // Tried two times sqrt: this is not good, this makes lobes
        
        // We can then multiply by a coefficient, because
        // the curve never goes more than about 50% (75% at the maximum)
        res *= NORM_COEFF;
        
        ioCurve->Get()[i] = res;
    }
}

// Not used
double
TransientLib4::GetThresholdedFactor(double smoothTrans, double threshold, double mix)
{
    double factor = 1.0;
    
    if (smoothTrans > threshold)
        // This is transient !
    {
        factor = (1.0 - (smoothTrans - threshold))*mix;
    }
    else
    {
        factor = ((1.0 - (threshold - smoothTrans)))*(1.0 - mix);
    }
    
    return factor;
}
