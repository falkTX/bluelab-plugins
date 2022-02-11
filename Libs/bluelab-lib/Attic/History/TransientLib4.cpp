//
//  TransientLib.cpp
//  Transient
//
//  Created by Apple m'a Tuer on 21/05/17.
//
//

#include "IPlug_include_in_plug_hdr.h"

#include "../../WDL/fft.h"

//#include "../../WDL/IPlug/Containers.h"

#include <CMA2Smoother.h>
#include <BLUtils.h>
#include <CMA2Smoother.h>
#include <Window.h>

#include "TransientLib4.h"

#define DEBUG_GRAPH 0
#if DEBUG_GRAPH
#include <DebugGraph.h>
#endif

// NOTE: This first method works well, even if there something a bit unlogical
void
TransientLib4::DetectTransients(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioFftBuf,
                                BL_FLOAT threshold,
                                BL_FLOAT mix,
                                WDL_TypedBuf<BL_FLOAT> *transientIntensityMagns)
{
#define LOG_MAGN_COEFF0 10000.0
    
// ln(10000) = 9 !
#define THRESHOLD_COEFF0 9.0
    
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
        BL_FLOAT weight = std::log(1.0 + magn*LOG_MAGN_COEFF0);
        if (weight > threshold*THRESHOLD_COEFF0)
        {
            transientIntensityMagns->Get()[(int)transPos] += weight;
        }
        
        // Test if the current magn has been used to participate to a transient
        // and classify it
        //
        // NOTE: this is not logical, this leads only to threholding the fft
        // depending on the intensity
        // ... but it seems to work
        if (weight > threshold*THRESHOLD_COEFF0)
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
#define CORRECTION_COEFF0 2.0
        newMagn *= CORRECTION_COEFF0;
        
        BLUtils::MagnPhaseToComplex(&comp, newMagn, phase);
        
        ioFftBuf->Get()[i] = comp;
    }
}

// This method works well, even if there something a bit unlogical
void
TransientLib4::DetectTransientsSmooth(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioFftBuf,
                                      BL_FLOAT threshold,
                                      BL_FLOAT mix,
                                      BL_FLOAT precision,
                                      WDL_TypedBuf<BL_FLOAT> *outTransients,
                                      WDL_TypedBuf<BL_FLOAT> *outSmoothedTransients)
{
#define ZERO_POS_SMOOTH 0.1
#define THRS_EPS_SMOOTH 1e-5
    
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
    
    NormalizeCurve(&smoothedTransients);
    
    BLUtils::AddValues(&smoothedTransients, (BL_FLOAT)ZERO_POS_SMOOTH);
    
    //
    // Get the contribution of each source bin depending on the curve
    // (by reversing the process)
    //
    WDL_TypedBuf<BL_FLOAT> newMagns = magns;
    for (int i = 0; i < bufSize; i++)
    {
        BL_FLOAT smoothTrans = smoothedTransients.Get()[i];
        vector<int> &trToMagn = transToMagn[i];
        
        if (smoothTrans > ZERO_POS_SMOOTH + THRS_EPS_SMOOTH)
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
#define COEFF_SMOOTH 2.0
    BLUtils::MultValues(&newMagns, (BL_FLOAT)COEFF_SMOOTH);
    
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
    
#define GRAPH_MIN_SMOOTH 0.0 
#define GRAPH_WEIGHT_MAX_SMOOTH -TRANSIENT_INF_LOG
    
    DebugGraph::SetCurveValues(weightsKeep, 0,
                               GRAPH_MIN_SMOOTH, GRAPH_WEIGHT_MAX_SMOOTH,
                               1.0, 0, 255, 0);
    
    DebugGraph::SetCurveValues(weightsThrow, 1,
                               GRAPH_MIN_SMOOTH, GRAPH_WEIGHT_MAX_SMOOTH,
                               1.0, 255, 0, 255);
    
    DebugGraph::SetCurveSingleValue(-TRANSIENT_INF_LOG + threshold, 2,
                                    GRAPH_MIN_SMOOTH, GRAPH_WEIGHT_MAX_SMOOTH,
                                    2.0,
                                    150, 150, 255,
                                    true, 0.2);
#endif
}

void
TransientLib4::DetectTransientsSmooth2(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioFftBuf,
                                       BL_FLOAT threshold,
                                       BL_FLOAT mix,
                                       BL_FLOAT precision,
                                       WDL_TypedBuf<BL_FLOAT> *outTransients,
                                       WDL_TypedBuf<BL_FLOAT> *outSmoothedTransients)
{    
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
    
#if DEBUG_GRAPH // Not working anymore (must be scaled...)
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
        
        // Amplifies less very small (and numerous) values
        BL_FLOAT weight = std::log(1.0 + magn);
        
        transients.Get()[(int)transPos] += weight;
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
    
    //
    // Scale adjustment
    //
    
    // First, multpiply by a big constant coeff
#define CURVE_COEFF_SMOOTH2 500.0
    BLUtils::MultValues(&smoothedTransients, (BL_FLOAT)CURVE_COEFF_SMOOTH2);
    
    // Compute a scale, depending on the precision
    // Because when precision is high, some values are not very smoothed,
    // so they are big
#define PRECISION0_COEFF_SMOOTH2 1.0
#define PRECISION1_COEFF_SMOOTH2 0.2
    BL_FLOAT scaleCoeff = (1.0 - precision)*PRECISION0_COEFF_SMOOTH2 +
    precision*PRECISION1_COEFF_SMOOTH2;
    
    BLUtils::MultValues(&smoothedTransients, scaleCoeff);
    
    //
    // Get the contribution of each source bin depending on the curve
    // (by reversing the process)
    //
    WDL_TypedBuf<BL_FLOAT> newMagns = magns;
    for (int i = 0; i < bufSize; i++)
    {
        //BL_FLOAT smoothTrans = transients.Get()[i]; //smoothedTransients.Get()[i];
        BL_FLOAT smoothTrans = smoothedTransients.Get()[i];
        vector<int> &trToMagn = transToMagn[i];
        
      // Temporal smooth with previous values
      // Avoids "silence holes" when curves moves very quickly
      // (and then is thresholdedd in a very not-continuous way)
      //
      // Works great !
      // (transient amplification and attenuation seems better)
#define SMOOTH_COEFF_SMOOTH2 0.8
        
        if ((outSmoothedTransients != NULL) &&
            (outSmoothedTransients->GetSize() == smoothedTransients.GetSize()))
            // We have previous values
        {
            BL_FLOAT prevSmoothTrans = outSmoothedTransients->Get()[i];
            smoothTrans = SMOOTH_COEFF_SMOOTH2*prevSmoothTrans +
            (1.0 - SMOOTH_COEFF_SMOOTH2)*smoothTrans;
            smoothedTransients.Get()[i] = smoothTrans;
        }
        
#if 0   // Test, to try to avoid "silence holes"
        // (not convincing)
        BL_FLOAT factor = GetThresholdedFactor(smoothTrans, threshold, mix);
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
#define COEFF_SMOOTH2 2.0
    BLUtils::MultValues(&newMagns, (BL_FLOAT)COEFF_SMOOTH2);
    
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
    {
        *outTransients = transients;
        
        BLUtils::MultValues(outTransients,
                            (BL_FLOAT)(CURVE_COEFF_SMOOTH2*scaleCoeff));
    }
    
    if (outSmoothedTransients != NULL)
        *outSmoothedTransients = smoothedTransients;
    
    //
    // Graph display
    //
#if DEBUG_GRAPH
    
#define GRAPH_MIN_SMOOTH2 0.0
#define GRAPH_WEIGHT_MAX_SMOOTH2 -TRANSIENT_INF_LOG
    
    DebugGraph::SetCurveValues(weightsKeep, 0,
                               GRAPH_MIN_SMOOTH2, GRAPH_WEIGHT_MAX_SMOOTH2,
                               1.0, 0, 255, 0);
    
    DebugGraph::SetCurveValues(weightsThrow, 1,
                               GRAPH_MIN_SMOOTH2, GRAPH_WEIGHT_MAX_SMOOTH2,
                               1.0, 255, 0, 255);
    
    DebugGraph::SetCurveSingleValue(-TRANSIENT_INF_LOG + threshold, 2,
                                    GRAPH_MIN_SMOOTH2, GRAPH_WEIGHT_MAX_SMOOTH2,
                                    2.0,
                                    150, 150, 255,
                                    true, 0.2);
#endif
}

//
// See (again): http://werner.yellowcouch.org/Papers/transients12/index.html
//
void
TransientLib4::ComputeTransientness(const WDL_TypedBuf<BL_FLOAT> &magns,
                                    const WDL_TypedBuf<BL_FLOAT> &phases,
                                    const WDL_TypedBuf<BL_FLOAT> *prevPhases,
                                    bool freqsToTrans,
                                    bool ampsToTrans,
                                    BL_FLOAT smoothFactor,
                                    WDL_TypedBuf<BL_FLOAT> *transientness)
{
#define DB_THRESHOLD_TR -64.0
#define DB_EPS_TR 1e-15
    
#define TRANS_COEFF_FREQ_TR 1.0
#define TRANS_COEFF_AMP_TR 0.25
    
    transientness->Resize(phases.GetSize());
    BLUtils::FillAllZero(transientness);
    
    WDL_TypedBuf<int> sampleIds;
    BLUtils::FftIdsToSamplesIds(phases, &sampleIds);
    
    for (int i = 0; i < sampleIds.GetSize(); i++)
    {
        int sampleId = sampleIds.Get()[i];
        
        //Do as Werner Van Belle
        BL_FLOAT magn = magns.Get()[i];
        
        // Ignore small magns
        BL_FLOAT magnDB = BLUtils::AmpToDB(magn,
                                           (BL_FLOAT)DB_EPS_TR,
                                           (BL_FLOAT)DB_THRESHOLD_TR);
        if (magnDB <= DB_THRESHOLD_TR)
            continue;
        
        BL_FLOAT freqWeight = 0.0;
        if (freqsToTrans)
        {
            //Do as Werner Van Belle
            BL_FLOAT w = -(magnDB - DB_THRESHOLD_TR)/DB_THRESHOLD_TR;
                    
            w *= TRANS_COEFF_FREQ_TR;
            
            freqWeight = w;
        }
        
        BL_FLOAT ampWeight = 0.0;
        if (ampsToTrans &&
            (prevPhases != NULL) && (prevPhases->GetSize() == sampleIds.GetSize()))
        {
            // Use additional method: compute derivative of phase over time
            // This is a very good indicator of transientness !
            
            BL_FLOAT phase0 = prevPhases->Get()[i];
            BL_FLOAT phase1 = phases.Get()[i];
            
            // Ensure that phase1 is greater than phase0
            while(phase1 < phase0)
                phase1 += 2.0*M_PI;
            
            BL_FLOAT delta = phase1 - phase0;
            
            delta = fmod(delta, 2.0*M_PI);
            
            if (delta > M_PI)
                delta = 2.0*M_PI - delta;
            
            BL_FLOAT w = delta/M_PI;
            
            w *= TRANS_COEFF_AMP_TR;
            
            ampWeight = w;
        }
        
        BL_FLOAT sumWeights = freqWeight + ampWeight;
        
        // Avoid increasing the transientness when we choose both
        if (freqsToTrans && ampsToTrans)
            sumWeights /= 2.0;
        
        transientness->Get()[sampleId] += sumWeights;
    }
    
    // At the end, the transientness is reversed
    // So reverse back...
    BLUtils::Reverse(transientness);
    
    // Smooth transients if necessary
    SmoothTransients(transientness, smoothFactor);
}

//
// See (again): http://werner.yellowcouch.org/Papers/transients12/index.html
//
void
TransientLib4::ComputeTransientness2(const WDL_TypedBuf<BL_FLOAT> &magns,
                                     const WDL_TypedBuf<BL_FLOAT> &phases,
                                     const WDL_TypedBuf<BL_FLOAT> *prevPhases,
                                     BL_FLOAT freqAmpRatio,
                                     BL_FLOAT smoothFactor,
                                     WDL_TypedBuf<BL_FLOAT> *transientness)
{
#define DB_THRESHOLD_TR2 -64.0
#define DB_EPS_TR2 1e-15
      
#define TRANS_COEFF_GLOBAL_TR2 0.5
#define TRANS_COEFF_FREQ_TR2 3.0
#define TRANS_COEFF_AMP_TR2 1.0
    
    transientness->Resize(phases.GetSize());
    BLUtils::FillAllZero(transientness);
    
    WDL_TypedBuf<int> sampleIds;
    BLUtils::FftIdsToSamplesIds(phases, &sampleIds);
    
    for (int i = 0; i < sampleIds.GetSize(); i++)
    {
        int sampleId = sampleIds.Get()[i];
        
        //Do as Werner Van Belle
        BL_FLOAT magn = magns.Get()[i];
        
        // Ignore small magns
        BL_FLOAT magnDB = BLUtils::AmpToDB(magn,
                                           (BL_FLOAT)DB_EPS_TR2,
                                           (BL_FLOAT)DB_THRESHOLD_TR2);
        if (magnDB <= DB_THRESHOLD_TR2)
            continue;
        
        BL_FLOAT freqWeight = 0.0;

        //Do as Werner Van Belle
        BL_FLOAT wf = -(magnDB - DB_THRESHOLD_TR2)/DB_THRESHOLD_TR2;
            
        wf *= TRANS_COEFF_FREQ_TR2*TRANS_COEFF_GLOBAL_TR2;
            
        freqWeight = wf*(1.0 - freqAmpRatio);
        
        BL_FLOAT ampWeight = 0.0;
        if ((prevPhases != NULL) && (prevPhases->GetSize() == sampleIds.GetSize()))
        {
            // Use additional method: compute derivative of phase over time
            // This is a very good indicator of transientness !
            
            BL_FLOAT phase0 = prevPhases->Get()[i];
            BL_FLOAT phase1 = phases.Get()[i];
            
            // Ensure that phase1 is greater than phase0
            while(phase1 < phase0)
                phase1 += 2.0*M_PI;
            
            BL_FLOAT delta = phase1 - phase0;
            
            delta = fmod(delta, 2.0*M_PI);
            
            if (delta > M_PI)
                delta = 2.0*M_PI - delta;
            
            BL_FLOAT w = delta/M_PI;
            
            w *= TRANS_COEFF_AMP_TR2*TRANS_COEFF_GLOBAL_TR2;
            
            ampWeight = freqAmpRatio*w;
        }
        
        BL_FLOAT sumWeights = freqWeight + ampWeight;
        
        transientness->Get()[sampleId] += sumWeights;
    }
    
    // At the end, the transientness is reversed
    // So reverse back...
    BLUtils::Reverse(transientness);
    
    // Smooth transients if necessary
    SmoothTransients(transientness, smoothFactor);
}

//
// See (again): http://werner.yellowcouch.org/Papers/transients12/index.html
//
// Fixed version, for extracting "s" only 
void
TransientLib4::ComputeTransientness3(const WDL_TypedBuf<BL_FLOAT> &magns,
                                     const WDL_TypedBuf<BL_FLOAT> &phases,
                                     const WDL_TypedBuf<BL_FLOAT> *prevPhases,
                                     BL_FLOAT freqAmpRatio,
                                     BL_FLOAT smoothFactor,
                                     WDL_TypedBuf<BL_FLOAT> *transientness)
{
#define DB_THRESHOLD_TR3 -64.0
#define DB_EPS_TR3 1e-15
    
#define TRANS_COEFF_GLOBAL_TR3 0.5
#define TRANS_COEFF_FREQ_TR3 3.0
#define TRANS_COEFF_AMP_TR3 1.0
    
    transientness->Resize(phases.GetSize());
    BLUtils::FillAllZero(transientness);
    
    WDL_TypedBuf<BL_FLOAT> transientnessS;
    transientnessS.Resize(phases.GetSize());
    BLUtils::FillAllZero(&transientnessS);
    
    WDL_TypedBuf<BL_FLOAT> transientnessP;
    transientnessP.Resize(phases.GetSize());
    BLUtils::FillAllZero(&transientnessP);
    
    WDL_TypedBuf<int> sampleIds;
    BLUtils::FftIdsToSamplesIds(phases, &sampleIds);
    
    for (int i = 0; i < sampleIds.GetSize(); i++)
    {
        int sampleId = sampleIds.Get()[i];
        
        //Do as Werner Van Belle
        BL_FLOAT magn = magns.Get()[i];
        
        // Ignore small magns
        BL_FLOAT magnDB = BLUtils::AmpToDB(magn,
                                           (BL_FLOAT)DB_EPS_TR3,
                                           (BL_FLOAT)DB_THRESHOLD_TR3);
        if (magnDB <= DB_THRESHOLD_TR3)
            continue;
        
        BL_FLOAT freqWeight = 0.0;
        
        //Do as Werner Van Belle
        BL_FLOAT wf = -(magnDB - DB_THRESHOLD_TR3)/DB_THRESHOLD_TR3;
        
        wf *= TRANS_COEFF_FREQ_TR3*TRANS_COEFF_GLOBAL_TR3;
        
        freqWeight = wf;
        
        BL_FLOAT ampWeight = 0.0;
        if ((prevPhases != NULL) && (prevPhases->GetSize() == sampleIds.GetSize()))
        {
            // Use additional method: compute derivative of phase over time
            // This is a very good indicator of transientness !
            
            BL_FLOAT phase0 = prevPhases->Get()[i];
            BL_FLOAT phase1 = phases.Get()[i];
            
            // Ensure that phase1 is greater than phase0
            while(phase1 < phase0)
                phase1 += 2.0*M_PI;
            
            BL_FLOAT delta = phase1 - phase0;
            
            delta = fmod(delta, 2.0*M_PI);
            
            if (delta > M_PI)
                delta = 2.0*M_PI - delta;
            
            BL_FLOAT w = delta/M_PI;
            
            w *= TRANS_COEFF_AMP_TR3*TRANS_COEFF_GLOBAL_TR3;
            
            ampWeight = w;
        }
        
        transientnessS.Get()[sampleId] += freqWeight;
        transientnessP.Get()[sampleId] += ampWeight;
    }
    
    // At the end, the transientness is reversed
    // So reverse back...
    BLUtils::Reverse(&transientnessS);
    BLUtils::Reverse(&transientnessP);
    
    // Smooth transients if necessary
    SmoothTransients(&transientnessS, smoothFactor);
    SmoothTransients(&transientnessP, smoothFactor);
    
    for (int i = 0; i < transientness->GetSize(); i++)
    {
        BL_FLOAT ts = transientnessS.Get()[i];
        BL_FLOAT tp = transientnessP.Get()[i];
        
        BL_FLOAT a = tp - ts;
        if (a < 0.0)
            a = 0.0;
        
        BL_FLOAT b = ts;
        
        // HACK
        // With that, the global volume does not increase, compared to bypass
        b *= 0.5;
        
        transientness->Get()[i] = freqAmpRatio*a + (1.0 - freqAmpRatio)*b;
    }
    
    BLUtils::ClipMin(transientness, (BL_FLOAT)0.0);
}

//
// See (again): http://werner.yellowcouch.org/Papers/transients12/index.html
//
// Fixed version, for extracting "s" only e.g
// Fixed version: does not shift the transient peaks when smoothing
// (which made bad separation with sample rate 88200Hz)
void
TransientLib4::ComputeTransientness4(const WDL_TypedBuf<BL_FLOAT> &magns,
                                     const WDL_TypedBuf<BL_FLOAT> &phases,
                                     const WDL_TypedBuf<BL_FLOAT> *prevPhases,
                                     BL_FLOAT freqAmpRatio,
                                     BL_FLOAT smoothFactor,
                                     BL_FLOAT sampleRate,
                                     WDL_TypedBuf<BL_FLOAT> *smoothWin,
                                     WDL_TypedBuf<BL_FLOAT> *transientness)
{
#define DB_THRESHOLD_TR4 -64.0
    
#define DB_EPS_TR4 1e-15
    
#define TRANS_COEFF_GLOBAL_TR4 0.5
#define TRANS_COEFF_FREQ_TR4 3.0
#define TRANS_COEFF_AMP_TR4 1.0
    
    transientness->Resize(phases.GetSize());
    BLUtils::FillAllZero(transientness);
    
    WDL_TypedBuf<BL_FLOAT> transientnessS;
    transientnessS.Resize(phases.GetSize());
    BLUtils::FillAllZero(&transientnessS);
    
    WDL_TypedBuf<BL_FLOAT> transientnessP;
    transientnessP.Resize(phases.GetSize());
    BLUtils::FillAllZero(&transientnessP);
    
    WDL_TypedBuf<int> sampleIds;
    BLUtils::FftIdsToSamplesIds(phases, &sampleIds);
        
    for (int i = 0; i < sampleIds.GetSize(); i++)
    {
        int sampleId = sampleIds.Get()[i];
        
        //Do as Werner Van Belle
        BL_FLOAT magn = magns.Get()[i];
        
        // Ignore small magns
        BL_FLOAT magnDB = BLUtils::AmpToDB(magn,
                                           (BL_FLOAT)DB_EPS_TR4,
                                           (BL_FLOAT)DB_THRESHOLD_TR4);
        if (magnDB <= DB_THRESHOLD_TR4)
            continue;
        
        BL_FLOAT freqWeight = 0.0;
        
        //Do as Werner Van Belle
        BL_FLOAT wf = -(magnDB - DB_THRESHOLD_TR4)/DB_THRESHOLD_TR4;
        
        wf *= TRANS_COEFF_FREQ_TR4*TRANS_COEFF_GLOBAL_TR4;
        
        freqWeight = wf;
        
        BL_FLOAT ampWeight = 0.0;
        if ((prevPhases != NULL) && (prevPhases->GetSize() == sampleIds.GetSize()))
        {
            // Use additional method: compute derivative of phase over time
            // This is a very good indicator of transientness !
            
            BL_FLOAT phase0 = prevPhases->Get()[i];
            BL_FLOAT phase1 = phases.Get()[i];
            
            // Ensure that phase1 is greater than phase0
            while(phase1 < phase0)
                phase1 += 2.0*M_PI;
            
            BL_FLOAT delta = phase1 - phase0;
                        
            delta = fmod(delta, 2.0*M_PI);
            
            if (delta > M_PI)
                delta = 2.0*M_PI - delta;
            
            BL_FLOAT w = delta/M_PI;
            
            w *= TRANS_COEFF_AMP_TR4*TRANS_COEFF_GLOBAL_TR4;
            
            ampWeight = w;
        }
        
        transientnessS.Get()[sampleId] += freqWeight;
        transientnessP.Get()[sampleId] += ampWeight;
    }
    
    // At the end, the transientness is reversed
    // So reverse back...
    BLUtils::Reverse(&transientnessS);
    BLUtils::Reverse(&transientnessP);
    
    // Smooth transients if necessary
#if 0 // ORIGIN
    // CMA
    // Works well with 44100
        
    SmoothTransients(&transientnessS, smoothFactor);
    SmoothTransients(&transientnessP, smoothFactor);
#endif
    
#if 1 // NEW
    // Smooth with pyramid
    // (smoothes less than CMA)
    SmoothTransients3(&transientnessS, smoothFactor);
    SmoothTransients3(&transientnessP, smoothFactor);
#endif
    
    for (int i = 0; i < transientness->GetSize(); i++)
    {
        BL_FLOAT ts = transientnessS.Get()[i];
        BL_FLOAT tp = transientnessP.Get()[i];
        
        BL_FLOAT a = tp - ts;
        if (a < 0.0)
            a = 0.0;
        
        BL_FLOAT b = ts;
        
        // HACK
        // With that, the global volume does not increase, compared to bypass
        b *= 0.5;
        
        transientness->Get()[i] = freqAmpRatio*a + (1.0 - freqAmpRatio)*b;
    }
    
    BLUtils::ClipMin(transientness, (BL_FLOAT)0.0);
}

//
// See (again): http://werner.yellowcouch.org/Papers/transients12/index.html
//
// Fixed version, for extracting "s" only e.g
// Fixed version: does not shift the transient peaks when smoothing
// (which made bad separation with sample rate 88200Hz)
void
TransientLib4::ComputeTransientness5(const WDL_TypedBuf<BL_FLOAT> &magns,
                                     const WDL_TypedBuf<BL_FLOAT> &phases,
                                     const WDL_TypedBuf<BL_FLOAT> *prevPhases,
                                     BL_FLOAT freqAmpRatio,
                                     BL_FLOAT smoothFactor,
                                     BL_FLOAT sampleRate,
                                     WDL_TypedBuf<BL_FLOAT> *smoothWin,
                                     WDL_TypedBuf<BL_FLOAT> *transientness)
{
#define DB_THRESHOLD_TR5 -64.0
    
#define DB_EPS_TR5 1e-15
    
#define TRANS_COEFF_GLOBAL_TR5 0.5
#define TRANS_COEFF_FREQ_TR5 3.0
#define TRANS_COEFF_AMP_TR5 1.0
    
#define USE_SAMPLE_RATE_COEFF_TR5 0 //1
    
    transientness->Resize(phases.GetSize());
    BLUtils::FillAllZero(transientness);
    
    WDL_TypedBuf<BL_FLOAT> transientnessS;
    transientnessS.Resize(phases.GetSize());
    BLUtils::FillAllZero(&transientnessS);
    
    WDL_TypedBuf<BL_FLOAT> transientnessP;
    transientnessP.Resize(phases.GetSize());
    BLUtils::FillAllZero(&transientnessP);
    
    WDL_TypedBuf<int> sampleIds;
    BLUtils::FftIdsToSamplesIds(phases, &sampleIds);
    
#if USE_SAMPLE_RATE_COEFF
    // With 88200Hz by default, transient data looks wrong
    BL_FLOAT srCoeff = sampleRate/44100.0;
#endif
    
    for (int i = 0; i < sampleIds.GetSize(); i++)
    {
        int sampleId = sampleIds.Get()[i];
        
        //Do as Werner Van Belle
        BL_FLOAT magn = magns.Get()[i];
        
        // Ignore small magns
        BL_FLOAT magnDB = BLUtils::AmpToDB(magn,
                                           (BL_FLOAT)DB_EPS_TR5,
                                           (BL_FLOAT)DB_THRESHOLD_TR5);
        if (magnDB <= DB_THRESHOLD_TR5)
            continue;
        
        BL_FLOAT freqWeight = 0.0;
        
        //Do as Werner Van Belle
        BL_FLOAT wf = -(magnDB - DB_THRESHOLD_TR5)/DB_THRESHOLD_TR5;
        
        freqWeight = wf * TRANS_COEFF_FREQ_TR5*TRANS_COEFF_GLOBAL_TR5;
        
        BL_FLOAT ampWeight = 0.0;
        if ((prevPhases != NULL) && (prevPhases->GetSize() == sampleIds.GetSize()))
        {
            // Use additional method: compute derivative of phase over time
            // This is a very good indicator of transientness !
            
            BL_FLOAT phase0 = prevPhases->Get()[i];
            BL_FLOAT phase1 = phases.Get()[i];
            
            // Ensure that phase1 is greater than phase0
            while(phase1 < phase0)
                phase1 += 2.0*M_PI;
            
            BL_FLOAT delta = phase1 - phase0;
            
#if USE_SAMPLE_RATE_COEFF
            delta *= srCoeff;
#endif
            
            delta = fmod(delta, 2.0*M_PI);
            
            if (delta > M_PI)
                delta = 2.0*M_PI - delta;
            
            BL_FLOAT w = delta/M_PI;
            
            w *= TRANS_COEFF_AMP_TR5*TRANS_COEFF_GLOBAL_TR5;
            
#if 1
            // NEW: makes more coherent result for Loris voice
#define WP_COEFF_TR5 100.0
            
            // Multiply before with coeff, to avoid numerical limits
            w *= WP_COEFF_TR5;
            w *= magn; //
            //w *= wf;
#endif
            
            ampWeight = w;
        }
        
        transientnessS.Get()[sampleId] += freqWeight;
        transientnessP.Get()[sampleId] += ampWeight;
    }
    
    // At the end, the transientness is reversed
    // So reverse back...
    BLUtils::Reverse(&transientnessS);
    BLUtils::Reverse(&transientnessP);
    
#if 1 // ORIGIN smooth
    // CMA
    // Works well with 44100
    SmoothTransients(&transientnessS, smoothFactor);
    SmoothTransients(&transientnessP, smoothFactor);
#endif
    
#if 0 // Too CPU heavy
    // Window
    SmoothTransients2(&transientnessS, smoothFactor, smoothWin);
    SmoothTransients2(&transientnessP, smoothFactor, smoothWin);
#endif
    
#if 0 // Amplifies too much
    // Adjust the level depending on the sample rate
    BL_FLOAT ampCoeff = sampleRate/44100.0;
    BLUtils::MultValues(&transientnessS, ampCoeff);
    BLUtils::MultValues(&transientnessP, ampCoeff);
#endif
    
#if 0 // NEW
    // Smooth with pyramid
    // (smoothes less than CMA)
    SmoothTransients3(&transientnessS, smoothFactor);
    SmoothTransients3(&transientnessP, smoothFactor);
#endif
    
    // ORIG
#if 0 // NEW 2
    // Simply compute avg
    // (Smooth more than CMA)
    
    // Smooth too much for "p"
    SmoothTransients4(&transientnessS);
    SmoothTransients4(&transientnessP);
#endif
    
#if 0 // ORIGIN
    for (int i = 0; i < transientness->GetSize(); i++)
    {
        BL_FLOAT ts = transientnessS.Get()[i];
        BL_FLOAT tp = transientnessP.Get()[i];
        
        BL_FLOAT a = tp - ts;
        if (a < 0.0)
            a = 0.0;
        
#if 1 // ORIG
        BL_FLOAT b = ts;
        
        // HACK
        // With that, the global volume does not increase, compared to bypass
        b *= 0.5;
#endif
        
#if 0 // NEW
        BL_FLOAT b = ts - tp;
        if (b < 0.0)
            b = 0.0;
        
        // HACK
        // With that, the global volume does not increase, compared to bypass
        b *= 0.5;
#endif
        
        transientness->Get()[i] = freqAmpRatio*a + (1.0 - freqAmpRatio)*b;
    }
#endif
    
    
#if 1 // TEST 2
    for (int i = 0; i < transientness->GetSize(); i++)
    {
        BL_FLOAT ts = transientnessS.Get()[i];
        BL_FLOAT tp = transientnessP.Get()[i];
        
        BL_FLOAT a = tp - ts;
        if (a < 0.0)
            a = 0.0;
        
        BL_FLOAT b = ts - tp;
        if (b < 0.0)
            b = 0.0;
        
        // HACK
        // With that, the global volume does not increase, compared to bypass
        //b *= 0.25;
        b *= 0.125;
        
        transientness->Get()[i] = freqAmpRatio*a + (1.0 - freqAmpRatio)*b;
    }
    
    // ORIGIN
    //SmoothTransients4(transientness);
    SmoothTransients(transientness, smoothFactor);
    
#define GLOBAL_COEFF_TR5 8.0
    BLUtils::MultValues(transientness, (BL_FLOAT)GLOBAL_COEFF_TR5);
#endif
    
#if 0 // TEST
    for (int i = 0; i < transientness->GetSize(); i++)
    {
        BL_FLOAT ts = transientnessS.Get()[i];
        BL_FLOAT tp = transientnessP.Get()[i];
        
        transientness->Get()[i] = freqAmpRatio*tp + (1.0 - freqAmpRatio)*ts;
    }
#endif
        
    BLUtils::ClipMin(transientness, (BL_FLOAT)0.0);
}

void
TransientLib4::ComputeTransientness6(const WDL_TypedBuf<BL_FLOAT> &magns,
                                     const WDL_TypedBuf<BL_FLOAT> &phases,
                                     const WDL_TypedBuf<BL_FLOAT> *prevPhases,
                                     BL_FLOAT freqAmpRatio,
                                     BL_FLOAT smoothFactor,
                                     BL_FLOAT sampleRate,
                                     WDL_TypedBuf<BL_FLOAT> *transientness)
{
#define DB_THRESHOLD_TR6 -64.0
    
#define DB_EPS_TR6 1e-15
    
#define TRANS_COEFF_GLOBAL_TR6 0.5
#define TRANS_COEFF_FREQ_TR6 3.0
#define TRANS_COEFF_AMP_TR6 1.0
    
    transientness->Resize(phases.GetSize());
    BLUtils::FillAllZero(transientness);
    
    WDL_TypedBuf<BL_FLOAT> transientnessS;
    transientnessS.Resize(phases.GetSize());
    BLUtils::FillAllZero(&transientnessS);
    
    WDL_TypedBuf<BL_FLOAT> transientnessP;
    transientnessP.Resize(phases.GetSize());
    BLUtils::FillAllZero(&transientnessP);
    
    WDL_TypedBuf<int> sampleIds;
    BLUtils::FftIdsToSamplesIds(phases, &sampleIds);
    
    for (int i = 0; i < sampleIds.GetSize(); i++)
    {
        int sampleId = sampleIds.Get()[i];
        
        // Do as Werner Van Belle
        BL_FLOAT magn = magns.Get()[i];
        
        // Ignore small magns
        BL_FLOAT magnDB = BLUtils::AmpToDB(magn,
                                           (BL_FLOAT)DB_EPS_TR6,
                                           (BL_FLOAT)DB_THRESHOLD_TR6);
        if (magnDB <= DB_THRESHOLD_TR6)
            continue;
        
        BL_FLOAT freqWeight = 0.0;
        
        // Do as Werner Van Belle
        BL_FLOAT wf = -(magnDB - DB_THRESHOLD_TR6)/DB_THRESHOLD_TR6;
        
        wf *= TRANS_COEFF_FREQ_TR6*TRANS_COEFF_GLOBAL_TR6;
        
        freqWeight = wf;
        
        BL_FLOAT ampWeight = 0.0;
        if ((prevPhases != NULL) && (prevPhases->GetSize() == sampleIds.GetSize()))
        {
            // Use additional method: compute derivative of phase over time
            // This is a very good indicator of transientness !
            
            BL_FLOAT phase0 = prevPhases->Get()[i];
            BL_FLOAT phase1 = phases.Get()[i];
            
            // Ensure that phase1 is greater than phase0
            while(phase1 < phase0)
                phase1 += 2.0*M_PI;
            
            BL_FLOAT delta = phase1 - phase0;
            
            delta = fmod(delta, 2.0*M_PI);
            
            if (delta > M_PI)
                delta = 2.0*M_PI - delta;
            
            BL_FLOAT w = delta/M_PI;
            
            w *= TRANS_COEFF_AMP_TR6*TRANS_COEFF_GLOBAL_TR6;
            
            ampWeight = w;
        }
        
        transientnessS.Get()[sampleId] += freqWeight;
        transientnessP.Get()[sampleId] += ampWeight;
    }
    
    // At the end, the transientness is reversed
    // So reverse back...
    BLUtils::Reverse(&transientnessS);
    BLUtils::Reverse(&transientnessP);
    
#if 1 // ORIGIN
    // CMA
    // Works well
    
#define NATIVE_BUFFER_SIZE_TR6 2048
    BL_FLOAT bufCoeff = ((BL_FLOAT)phases.GetSize())/NATIVE_BUFFER_SIZE_TR6;
    
    SmoothTransients(&transientnessS, smoothFactor);
    SmoothTransients(&transientnessP, smoothFactor);
    
    BLUtils::MultValues(&transientnessS, bufCoeff);
    BLUtils::MultValues(&transientnessP, bufCoeff);
#endif
    
#if 0 //0 // NEW
    // Smooth with pyramid
    // (smooth less than CMA)
    int level = 9;
    SmoothTransients3(&transientnessS, level);
    SmoothTransients3(&transientnessP, level);
#endif

#if 0 // TEST
    // Smooth with simple avg
    SmoothTransients4(&transientnessS);
    SmoothTransients4(&transientnessP);
#endif

    
    for (int i = 0; i < transientness->GetSize(); i++)
    {
        BL_FLOAT ts = transientnessS.Get()[i];
        BL_FLOAT tp = transientnessP.Get()[i];
        
        BL_FLOAT a = tp - ts;
        if (a < 0.0)
            a = 0.0;
        
#if 1 // ORIGIN
        BL_FLOAT b = ts;
        
        // HACK
        // With that, the global volume does not increase, compared to bypass
        b *= 0.5;
#endif
        
#if 0 // TEST
        BL_FLOAT b = ts - tp;
        if (b < 0.0)
            b = 0.0;
#endif
        
        transientness->Get()[i] = freqAmpRatio*a + (1.0 - freqAmpRatio)*b;
    }
    
    BLUtils::ClipMin(transientness, (BL_FLOAT)0.0);
}

void
TransientLib4::ComputeTransientnessMix(WDL_TypedBuf<BL_FLOAT> *magns,
                                       const WDL_TypedBuf<BL_FLOAT> &phases,
                                       const WDL_TypedBuf<BL_FLOAT> *prevPhases,
                                       //BL_FLOAT threshold,
                                       const WDL_TypedBuf<BL_FLOAT> &thresholds,
                                       BL_FLOAT mix,
                                       bool freqsToTrans,
                                       bool ampsToTrans,
                                       BL_FLOAT smoothFactor,
                                       WDL_TypedBuf<BL_FLOAT> *transientness,
                                       const WDL_TypedBuf<BL_FLOAT> *window)
{
    //
    // Main coefficient for scale !
    //
#define TRANS_COEFF_GLOBAL_MIX 5.0
    //
    //
    
#define TRANS_COEFF_FREQ_MIX 0.2
#define TRANS_COEFF_AMP_MIX 0.1
    
#define DB_MIN_MIX -120.0
#define DB_EPS_MIX 1e-15
    
    // GOOD: keep a good y scale whatever the precision
    // Drawback: maybe less accurate since we don't use an exact version of
    // the smooth window
    //
#define USE_ADVANCED_SMOOTHING_MIX 1
    
    // BAD ! (in the curent version)
    //
    // Can only be used if transientness is in the frequency domain !
    //
    // Temporal smoothing
#define USE_TEMPORAL_SMOOTHING_MIX 0
#define TEMPORAL_SMOOTH_COEFF_MIX 0.8
    
    
    //
    // Threshold small db (as original)
    // TODO: don't forget to check "keep track of non-transient in all cases"
#define USE_THRESHOLD_DB_MIX 1
#if USE_THRESHOLD_DB_MIX
    
    // -50.0: not good
    // -60.0: maybe good...
#define THRESHOLD_DB_MIX -64.0
    
    // GOOD !
    // Without that, there is a remaining background noise
    // and the mix at 50% is not like bypass
#define KEEP_TRACK_OF_THRESHOLDED_MIX 1
    
#endif
    
    // Init
    WDL_TypedBuf<BL_FLOAT> prevTransientness;
    if (transientness != NULL)
    {
        prevTransientness = *transientness;
    }
    
    //
    // FirstPhase phase: compute the contribution of each bin to the transoentness
    // and compute the transientness
    //
    
    transientness->Resize(phases.GetSize());
    BLUtils::FillAllZero(transientness);
    
    WDL_TypedBuf<int> sampleIds;
    BLUtils::FftIdsToSamplesIds(phases, &sampleIds);
    
    // Map, to find the magn positions who have participated to
    // a given transient value
    vector< vector< int > > transToMagn;
    transToMagn.resize(magns->GetSize());
    
    for (int i = 0; i < sampleIds.GetSize(); i++)
    {
        int sampleId = sampleIds.Get()[i];
        
        //Do as Werner Van Belle
        BL_FLOAT magn = magns->Get()[i];
        
        BL_FLOAT magnDB = BLUtils::AmpToDB(magn,
                                           (BL_FLOAT)DB_EPS_MIX,
                                           (BL_FLOAT)DB_MIN_MIX);
        
        // Threshold, to avoid negative weights values later
        if (magnDB < DB_MIN_MIX)
            magnDB = DB_MIN_MIX;
        
        // Works great for extracting whisper ?
#if USE_THRESHOLD_DB_MIX
        // Ignore small magns
        if (magnDB <= THRESHOLD_DB_MIX)
        {
#if KEEP_TRACK_OF_THRESHOLDED_MIX
            // In all cases, keep track of the position
            // (it will be used to discard non-transient bins)
            transToMagn[(int)sampleId].push_back(i);
#endif
            continue;
        }
#endif
        // Frequency weight
        BL_FLOAT freqWeight = 0.0;
        if (freqsToTrans)
        {
            BL_FLOAT w = -(magnDB - DB_MIN_MIX)/DB_MIN_MIX;
            
            w *= TRANS_COEFF_FREQ_MIX*TRANS_COEFF_GLOBAL_MIX;
                
            freqWeight = w;
        }
        
        // Amplitude weight
        BL_FLOAT ampWeight = 0.0;
        if (ampsToTrans &&
            (prevPhases != NULL) && (prevPhases->GetSize() == sampleIds.GetSize()))
        {
            // Use additional method: compute derivative of phase over time
            // This is a very good indicator of transientness !
                
            BL_FLOAT phase0 = prevPhases->Get()[i];
            BL_FLOAT phase1 = phases.Get()[i];
                
            // Ensure that phase1 is greater than phase0
            while(phase1 < phase0)
                phase1 += 2.0*M_PI;
                
            BL_FLOAT delta = phase1 - phase0;
                
            delta = fmod(delta, 2.0*M_PI);
                
            if (delta > M_PI)
                delta = 2.0*M_PI - delta;
                
            BL_FLOAT w = delta/M_PI;
            
            w *= TRANS_COEFF_AMP_MIX*TRANS_COEFF_GLOBAL_MIX;
                
            ampWeight = w;
        }
        
        // Sum the weights
        BL_FLOAT sumWeights = freqWeight + ampWeight;
        
        // Avoid increasing the transientness when we choose both
        if (freqsToTrans && ampsToTrans)
            sumWeights /= 2.0;
        
        transientness->Get()[sampleId]  += sumWeights;
        
        // In all cases, keep track of the position
        // (it will be used to discard non-transient bins)
        transToMagn[(int)sampleId].push_back(i);
    }
    
    // At the end, the transientness is reversed
    // So reverse back...
    BLUtils::Reverse(transientness);
    
    // NOTE: take care of half !
    BLUtils::ApplyWindowRescale(transientness, *window);
    
    
    //
    // Do smoothing
    //
    
#if !USE_ADVANCED_SMOOTHING_MIX
    // Original method
    SmoothTransients(transientness, smoothFactor);
#else
    // Nw method: keep constant scale whatever the CMA smoothing window size
    SmoothTransientsAdvanced(transientness, smoothFactor);
#endif
    
    BLUtils::ClipMin(transientness, (BL_FLOAT)0.0);
    
#if USE_TEMPORAL_SMOOTHING_MIX
    //
    // Additional pass: temporal smoothing
    //
    // Temporal smooth with previous values
    // Avoids "silence holes" when curves moves very quickly
    // (and then is thresholdedd in a very not-continuous way)
    // Worked great (before)
    if (prevTransientness.GetSize() == transientness->GetSize())
        // We have previous values
    {
        for (int i = 0; i < transientness->GetSize(); i++)
        {
            BL_FLOAT trans = transientness->Get()[i];
            BL_FLOAT prevTrans = prevTransientness.Get()[i];
            
            trans = TEMPORAL_SMOOTH_COEF_MIX*prevTrans +
            (1.0 - TEMPORAL_SMOOTH_COEFF_MIX)*trans;
            transientness->Get()[i] = trans;
        }
    }
#endif
    
    //
    // Second phase: reverse the process and mix
    //
    // NOTE: well checked: this method separates well the two parts
    //
    WDL_TypedBuf<BL_FLOAT> newMagns;
    ProcessMix(&newMagns, *magns, *transientness, transToMagn, thresholds, mix);
    
    *magns = newMagns;
}

void
TransientLib4::ComputeTransientnessMix2(WDL_TypedBuf<BL_FLOAT> *magns,
                                        const WDL_TypedBuf<BL_FLOAT> &phases,
                                        const WDL_TypedBuf<BL_FLOAT> *prevPhases,
                                        //BL_FLOAT threshold,
                                        const WDL_TypedBuf<BL_FLOAT> &thresholds,
                                        BL_FLOAT mix,
                                        BL_FLOAT freqAmpRatio,
                                        BL_FLOAT smoothFactor,
                                        WDL_TypedBuf<BL_FLOAT> *transientness,
                                        const WDL_TypedBuf<BL_FLOAT> *window)
{
    //
    // Main coefficient for scale !
    //
#define TRANS_COEFF_GLOBAL_MIX2 5.0
    //
    //
    
#define TRANS_COEFF_FREQ_MIX2 0.1
#define TRANS_COEFF_AMP_MIX2 0.1
    
#define DB_MIN_MIX2 -120.0
#define DB_EPS_MIX2 1e-15
    
    // GOOD: keep a good y scale whatever the precision
    // Drawback: maybe less accurate since we don't use an exact version of
    // the smooth window
    //
#define USE_ADVANCED_SMOOTHING_MIX2 1
    
    // BAD ! (in the curent version)
    //
    // Can only be used if transientness is in the frequency domain !
    //
    // Temporal smoothing
#define USE_TEMPORAL_SMOOTHING_MIX2 0
#define TEMPORAL_SMOOTH_COEFF_MIX2 0.8
    
    
    //
    // Threshold small db (as original)
    // TODO: don't forget to check "keep track of non-transient in all cases"
#define USE_THRESHOLD_DB_MIX2 1
#if USE_THRESHOLD_DB_MIX2
    
    // -50.0: not good
    // -60.0: maybe good...
#define THRESHOLD_DB_MIX2 -64.0
    
    // GOOD !
    // Without that, there is a remaining background noise
    // and the mix at 50% is not like bypass
#define KEEP_TRACK_OF_THRESHOLDED_MIX2 1
    
#endif
    
    // Init
    WDL_TypedBuf<BL_FLOAT> prevTransientness;
    if (transientness != NULL)
    {
        prevTransientness = *transientness;
    }
    
    //
    // FirstPhase phase: compute the contribution of each bin to the transoentness
    // and compute the transientness
    //
    
    transientness->Resize(phases.GetSize());
    BLUtils::FillAllZero(transientness);
    
    WDL_TypedBuf<int> sampleIds;
    BLUtils::FftIdsToSamplesIds(phases, &sampleIds);
    
    // Map, to find the magn positions who have participated to
    // a given transient value
    vector< vector< int > > transToMagn;
    transToMagn.resize(magns->GetSize());
    
    for (int i = 0; i < sampleIds.GetSize(); i++)
    {
        int sampleId = sampleIds.Get()[i];
        
        //Do as Werner Van Belle
        BL_FLOAT magn = magns->Get()[i];
        
        BL_FLOAT magnDB = BLUtils::AmpToDB(magn,
                                           (BL_FLOAT)DB_EPS_MIX2,
                                           (BL_FLOAT)DB_MIN_MIX2);
        
        // Threshold, to avoid negative weights values later
        if (magnDB < DB_MIN_MIX2)
            magnDB = DB_MIN_MIX2;
        
        // Works great for extracting whisper ?
#if USE_THRESHOLD_DB_MIX2
        // Ignore small magns
        if (magnDB <= THRESHOLD_DB_MIX2)
        {
#if KEEP_TRACK_OF_THRESHOLDED_MIX2
            // In all cases, keep track of the position
            // (it will be used to discard non-transient bins)
            transToMagn[(int)sampleId].push_back(i);
#endif
            continue;
        }
#endif
        // Frequency weight
        BL_FLOAT freqWeight = 0.0;
        BL_FLOAT fw = -(magnDB - DB_MIN_MIX2)/DB_MIN_MIX2;
            
        fw *= TRANS_COEFF_FREQ_MIX2*TRANS_COEFF_GLOBAL_MIX2;
            
        freqWeight = fw*(1.0 - freqAmpRatio);
        
        // Amplitude weight
        BL_FLOAT ampWeight = 0.0;
        if ((prevPhases != NULL) && (prevPhases->GetSize() == sampleIds.GetSize()))
        {
            // Use additional method: compute derivative of phase over time
            // This is a very good indicator of transientness !
            
            BL_FLOAT phase0 = prevPhases->Get()[i];
            BL_FLOAT phase1 = phases.Get()[i];
            
            // Ensure that phase1 is greater than phase0
            while(phase1 < phase0)
                phase1 += 2.0*M_PI;
            
            BL_FLOAT delta = phase1 - phase0;
            
            delta = fmod(delta, 2.0*M_PI);
            
            if (delta > M_PI)
                delta = 2.0*M_PI - delta;
            
            BL_FLOAT w = delta/M_PI;
            
            w *= TRANS_COEFF_AMP_MIX2*TRANS_COEFF_GLOBAL_MIX2;
            
            ampWeight = w*freqAmpRatio;
        }
        
        // Sum the weights
        BL_FLOAT sumWeights = freqWeight + ampWeight;
        
        transientness->Get()[sampleId] += sumWeights;
        
        // In all cases, keep track of the position
        // (it will be used to discard non-transient bins)
        transToMagn[(int)sampleId].push_back(i);
    }
    
    // At the end, the transientness is reversed
    // So reverse back...
    BLUtils::Reverse(transientness);
    
    // NOTE: take care of half !
    BLUtils::ApplyWindowRescale(transientness, *window);
    
    //
    // Do smoothing
    //
    
#if !USE_ADVANCED_SMOOTHING_MIX2
    // Original method
    SmoothTransients(transientness, smoothFactor);
#else
    // Nw method: keep constant scale whatever the CMA smoothing window size
    SmoothTransientsAdvanced(transientness, smoothFactor);
#endif
    
    BLUtils::ClipMin(transientness, (BL_FLOAT)0.0);
    
#if USE_TEMPORAL_SMOOTHING_MIX2
    //
    // Additional pass: temporal smoothing
    //
    // Temporal smooth with previous values
    // Avoids "silence holes" when curves moves very quickly
    // (and then is thresholdedd in a very not-continuous way)
    // Worked great (before)
    if (prevTransientness.GetSize() == transientness->GetSize())
        // We have previous values
    {
        for (int i = 0; i < transientness->GetSize(); i++)
        {
            BL_FLOAT trans = transientness->Get()[i];
            BL_FLOAT prevTrans = prevTransientness.Get()[i];
            
            trans = TEMPORAL_SMOOTH_COEFF_MIX2*prevTrans +
            (1.0 - TEMPORAL_SMOOTH_COEFF_MIX2)*trans;
            transientness->Get()[i] = trans;
        }
    }
#endif
    
    //
    // Second phase: reverse the process and mix
    //
    // NOTE: well checked: this method separates well the two parts
    //
    WDL_TypedBuf<BL_FLOAT> newMagns;
    ProcessMix(&newMagns, *magns, *transientness, transToMagn, thresholds, mix);
    
    *magns = newMagns;
}

void
TransientLib4::ProcessMix(WDL_TypedBuf<BL_FLOAT> *newMagns,
                          const WDL_TypedBuf<BL_FLOAT> &magns,
                          const WDL_TypedBuf<BL_FLOAT> &transientness,
                          const vector< vector< int > > &transToMagn,
                          const WDL_TypedBuf<BL_FLOAT> &thresholds,
                          BL_FLOAT mix)
{
    // Get the contribution of each source bin depending on the curve
    *newMagns = magns;
    for (int i = 0; i < magns.GetSize(); i++)
    {
        BL_FLOAT trans = transientness.Get()[i];
        const vector<int> &trToMagn = transToMagn[i];
        
        BL_FLOAT threshold = thresholds.Get()[i];
        
        // Ignore the zero !
        // Otherwise, when thrs = 0 and mix = 100%,
        // we would still have some sound !
        
        if (trans > threshold)
            // This is transient !
        {
            for (int j = 0; j < trToMagn.size(); j++)
            {
                int index = trToMagn[j];
                newMagns->Get()[index] *= mix;
            }
        }
        else if (trans < threshold)
            // This is not transient
        {
            for (int j = 0; j < trToMagn.size(); j++)
            {
                int index = trToMagn[j];
                
                newMagns->Get()[index] *= (1.0 - mix);
            }
        } else if ((trans == 0.0) && (threshold == 0.0))
        {
            for (int j = 0; j < trToMagn.size(); j++)
            {
                int index = trToMagn[j];
                
                newMagns->Get()[index] *= 0.0;
            }
        }
    }
}

void
TransientLib4::TransientnessToFreqDomain(WDL_TypedBuf<BL_FLOAT> *transientness,
                                         vector< vector< int > > *transToMagn,
                                         const WDL_TypedBuf<int> &sampleIds)
{
    BLUtils::Permute(transientness, sampleIds, false);
    
    BLUtils::Permute(transToMagn, sampleIds, false);
}

void
TransientLib4::GetSmoothedTransients(const WDL_TypedBuf<BL_FLOAT> &transients,
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
TransientLib4::GetSmoothedTransientsInterp(const WDL_TypedBuf<BL_FLOAT> &transients,
                                           WDL_TypedBuf<BL_FLOAT> *smoothedTransients,
                                           BL_FLOAT precision)
{
#define RAW_SMOOTH_EXP_INT  2
//#define FINE_SMOOTH_EXP 6
  
    // Increase the max precision...
#define FINE_SMOOTH_EXP_INT 8
    // ...but decrease the progression of the precision
    // (so the visual progression of the precision will look constant)
  precision = std::pow(precision, 4.0);
    
    int numSteps = FINE_SMOOTH_EXP_INT - RAW_SMOOTH_EXP_INT;
    
    BL_FLOAT stepNum = precision*numSteps;
    int stepNumI = (int)stepNum;
    
    BL_FLOAT t = stepNum - stepNumI;
    
    int exps[2];
    exps[0] = RAW_SMOOTH_EXP_INT + stepNumI;
    exps[1] = RAW_SMOOTH_EXP_INT + stepNumI + 1;
    if (exps[1] > FINE_SMOOTH_EXP_INT)
        exps[1] = FINE_SMOOTH_EXP_INT;
    
    int rawSmoothDiv = (int)std::pow(2.0, exps[0]);
    int fineSmoothDiv = (int)std::pow(2.0, exps[1]);
    
    WDL_TypedBuf<BL_FLOAT> smoothedTransientsRaw;
    WDL_TypedBuf<BL_FLOAT> smoothedTransientsFine;
    GetSmoothedTransients(transients,
                          &smoothedTransientsRaw, &smoothedTransientsFine,
                          rawSmoothDiv, fineSmoothDiv);
    
    BLUtils::Interp(smoothedTransients, &smoothedTransientsRaw,
                  &smoothedTransientsFine, t);
}

void
TransientLib4::NormalizeVolume(const WDL_TypedBuf<BL_FLOAT> &origin,
                               WDL_TypedBuf<BL_FLOAT> *result)
{
    //#define EPS 1e-15
    
    // Security
#define NORM_MAX_COEFF_NV 4.0
    
    BL_FLOAT avgOrigin = BLUtils::ComputeAvgSquare(origin);
    BL_FLOAT avgResult = BLUtils::ComputeAvgSquare(*result);
    
    if (avgOrigin > BL_EPS)
    {
        BL_FLOAT coeff = avgOrigin / avgResult;
        
        if (coeff > NORM_MAX_COEFF_NV)
            coeff = NORM_MAX_COEFF_NV;
        
        BLUtils::MultValues(result, coeff);
    }
}

void
TransientLib4::NormalizeCurve(WDL_TypedBuf<BL_FLOAT> *ioCurve)
{
#define NORM_COEFF_NC 0.5
    
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
        res *= NORM_COEFF_NC;
        
        ioCurve->Get()[i] = res;
    }
}

// Not used
BL_FLOAT
TransientLib4::GetThresholdedFactor(BL_FLOAT smoothTrans, BL_FLOAT threshold, BL_FLOAT mix)
{
    BL_FLOAT factor = 1.0;
    
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

void
TransientLib4::SmoothTransients(WDL_TypedBuf<BL_FLOAT> *transients,
                                BL_FLOAT smoothFactor)
{
    // Smooth if necessary
    if (smoothFactor > 0.0)
    {
#define SMOOTH_FACTOR_TR 4.0
        
        WDL_TypedBuf<BL_FLOAT> smoothTransients;
        smoothTransients.Resize(transients->GetSize());
        
        BL_FLOAT cmaCoeff = smoothFactor*transients->GetSize()/SMOOTH_FACTOR_TR;
        
        int cmaWindowSize = (int)cmaCoeff;
        
        CMA2Smoother::ProcessOne(transients->Get(),
                                 smoothTransients.Get(),
                                 transients->GetSize(),
                                 cmaWindowSize);
        
        *transients = smoothTransients;
    }
    
    // After smoothing, we can have negative values
    BLUtils::ClipMin(transients, (BL_FLOAT)0.0);
}

void
TransientLib4::SmoothTransients2(WDL_TypedBuf<BL_FLOAT> *transients,
                                 BL_FLOAT smoothFactor,
                                 WDL_TypedBuf<BL_FLOAT> *smoothWin)
{
#define MIN_WIN_SIZE_TR2 0
#define MAX_WIN_SIZE_TR2 512
    
    // Smooth if necessary
    if (smoothFactor > 0.0)
    {
        int maxWinSize = (MAX_WIN_SIZE_TR2 < transients->GetSize()) ?
                            MAX_WIN_SIZE_TR2 : transients->GetSize();
    
        int winSize = smoothFactor*maxWinSize;
    
        if (smoothWin->GetSize() != winSize)
            // Make window
        {
            Window::MakeHanning(winSize, smoothWin);
        }
    
        WDL_TypedBuf<BL_FLOAT> result;
        BLUtils::SmoothDataWin(&result, *transients, *smoothWin);
        
        *transients = result;
    }
    
    // After smoothing, we can have negative values
    BLUtils::ClipMin(transients, (BL_FLOAT)0.0);
}

#if 0
void
TransientLib4::SmoothTransients3(WDL_TypedBuf<BL_FLOAT> *transients,
                                 BL_FLOAT smoothFactor)
{
#define MIN_LEVEL_TR3 1
//#define MAX_LEVEL 11
#define MAX_LEVEL_TR3 8 //9 //8
    
    // Smooth if necessary
    if (smoothFactor > 0.0)
    {
        int level = MIN_LEVEL_TR3 + smoothFactor*(MAX_LEVEL_TR3 - MIN_LEVEL_TR3);
        
        WDL_TypedBuf<BL_FLOAT> result;
        BLUtils::SmoothDataPyramid(&result, *transients, level);
        
        *transients = result;
    }
    
    // After smoothing, we can have negative values
    BLUtils::ClipMin(transients, 0.0);
}
#endif

void
TransientLib4::SmoothTransients3(WDL_TypedBuf<BL_FLOAT> *transients, int level)
{
    // Smooth if necessary
    if (level > 0)
    {
        WDL_TypedBuf<BL_FLOAT> result;
        BLUtils::SmoothDataPyramid(&result, *transients, level);
        
        *transients = result;
    }
    
    // After smoothing, we can have negative values
    BLUtils::ClipMin(transients, (BL_FLOAT)0.0);
}

void
TransientLib4::SmoothTransients4(WDL_TypedBuf<BL_FLOAT> *transients)
{
    BL_FLOAT avg = BLUtils::ComputeAvg(*transients);
    BLUtils::FillAllValue(transients, avg);
    
    // After smoothing, we can have negative values
    BLUtils::ClipMin(transients, (BL_FLOAT)0.0);
}

void
TransientLib4::SmoothTransientsAdvanced(WDL_TypedBuf<BL_FLOAT> *transients,
                                        BL_FLOAT smoothFactor)
{
    // Smoother curve, for keeping non-transient parts
    WDL_TypedBuf<BL_FLOAT> smoothedTransientness;
    GetSmoothedTransientsInterp(*transients, &smoothedTransientness, 1.0 - smoothFactor);
    *transients = smoothedTransientness;

    // After smoothing, we can have negative values
    BLUtils::ClipMin(transients, (BL_FLOAT)0.0);

    // Scale adjustment

    // Compute a scale, depending on the precision
    // Because when precision is high, some values are not very smoothed,
    // so they are big
//#define PRECISION0_COEFF 1.0
//#define PRECISION1_COEFF 0.2

#define PRECISION0_COEFF_ADV 1.2
#define PRECISION1_COEFF_ADV 2.4
    
    BL_FLOAT scaleCoeff = (1.0 - smoothFactor)*PRECISION0_COEFF_ADV +
                        smoothFactor*PRECISION1_COEFF_ADV;

    BLUtils::MultValues(transients, scaleCoeff);
}
