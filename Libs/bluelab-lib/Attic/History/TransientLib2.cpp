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

#include "TransientLib2.h"

#define DEBUG_GRAPH 1
#if DEBUG_GRAPH
#include <DebugGraph.h>
#endif

// TODO:
// "factorize" code !
// put the graph stuff at the end
// normalize the threshold related things
// Curve fill up or down side of the threshold, depending on the mix param

void
TransientLib2::DetectTransients(const WDL_TypedBuf<WDL_FFT_COMPLEX> *fftBuf,
                                BL_FLOAT threshold,
                                WDL_TypedBuf<BL_FLOAT> *strippedMagns,
                                WDL_TypedBuf<BL_FLOAT> *transientMagns,
                                WDL_TypedBuf<BL_FLOAT> *transientIntensityMagns)
{
#define SIGNAL_COEFF 1.0/50
    
    // Compute transients probability (and other things...)
    int bufSize = fftBuf->GetSize();
    
    // Set to zero
    for (int i = 0; i < bufSize; i++)
        strippedMagns->Get()[i] = 0.0;
    for (int i = 0; i < bufSize; i++)
        transientMagns->Get()[i] = 0.0;
    
    for (int i = 0; i < bufSize; i++)
        transientIntensityMagns->Get()[i] = 0.0;
    
    BL_FLOAT prev = 0.0;
    for (int i = 0; i < bufSize; i++)
    {
        WDL_FFT_COMPLEX c = fftBuf->Get()[i];
        BL_FLOAT re = c.re;
        BL_FLOAT im = c.im;
        
        BL_FLOAT magn = std::sqrt(re*re + im*im);
        
        BL_FLOAT phase = 0.0;
        if (std::fabs(re) > 0.0)
	  phase = std::atan2(im, re);
        
        BL_FLOAT phaseDiff = phase - prev;
        prev = phase;
        
        // TEST NIKO
        
        // Avoid having a big phase diff due to prev == 0
        if (i == 0)
            continue;
        
        // TEST NIKO: Works well with a dirac !
        phaseDiff = phaseDiff - M_PI/2.0;
        
        while(phaseDiff < 0.0)
            phaseDiff += 2.0*M_PI;
        
        BL_FLOAT transPos = ((BL_FLOAT)bufSize)*phaseDiff/(2.0*M_PI);
        
        //BL_FLOAT weight = std::log(1.0 + j*j + r*r);
        
        BL_FLOAT weight = std::log(1.0 + magn);
        
        if (weight > threshold)
            transientIntensityMagns->Get()[(int)transPos] += weight;
        
        if (weight*SIGNAL_COEFF > threshold)
            transientMagns->Get()[i] = magn;
        else
            strippedMagns->Get()[i] = magn;
    }
}

// This method works well, even if there something a bit unlogical
void
TransientLib2::DetectTransients2(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioFftBuf,
                                 BL_FLOAT threshold,
                                 BL_FLOAT mix,
                                 WDL_TypedBuf<BL_FLOAT> *transientIntensityMagns)
{
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
        
        BL_FLOAT weight = std::log(1.0 + magn);
        
        if (weight > threshold)
            transientIntensityMagns->Get()[(int)transPos] += weight;
        
        // The following is a bit not logical
        // Either, we should have waited to finish the loop,
        // then read the summed transientIntensityMagns
        //
        // DetectTransients3() tries to do that
        if (weight*SIGNAL_COEFF > threshold)
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
        
        BL_FLOAT newMagn = (1.0 - mix)*transientMagns.Get()[i] + mix*strippedMagns.Get()[i];
        
        BLUtils::MagnPhaseToComplex(&comp, newMagn, phase);
        
        ioFftBuf->Get()[i] = comp;
    }
}

// This method works well, even if there something a bit unlogical
// NOTE: original method
// Coefficients checked and improved
void
TransientLib2::DetectTransients4(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioFftBuf,
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
    
#if DEBUG_GRAPH
    WDL_TypedBuf<BL_FLOAT> magns;
    magns.Resize(bufSize);
    
    WDL_TypedBuf<BL_FLOAT> weights;
#endif
    
    BL_FLOAT prev = 0.0;
    for (int i = 0; i < bufSize; i++)
    {
        WDL_FFT_COMPLEX c = ioFftBuf->Get()[i];
        BL_FLOAT re = c.re;
        BL_FLOAT im = c.im;
        
        BL_FLOAT magn = std::sqrt(re*re + im*im);
        
#if DEBUG_GRAPH
        magns.Get()[i] = magn;
#endif
        
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
        
#if DEBUG_GRAPH
        weights.Add(weight);
#endif
        
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
    
#if DEBUG_GRAPH
    WDL_TypedBuf<BL_FLOAT> smoothedTransientIntensityMagns;
    smoothedTransientIntensityMagns.Resize(transientIntensityMagns->GetSize());
    
    CMA2Smoother::ProcessOne(transientIntensityMagns->Get(),
                             smoothedTransientIntensityMagns.Get(),
                             transientIntensityMagns->GetSize(),
                             transientIntensityMagns->GetSize()/32);
    
#define GRAPH_MIN 0.0    
#define GRAPH_MAGN_MAX 0.1
#define GRAPH_WEIGHT_MAX 10.0
    
    DebugGraph::SetCurveValues(magns, 0,
                               GRAPH_MIN, GRAPH_MAGN_MAX,
                               1.0, 255, 0, 0);
    
    DebugGraph::SetCurveValues(*transientIntensityMagns, 1,
                               GRAPH_MIN, GRAPH_WEIGHT_MAX,
                               1.0, 0, 0, 255);
    
    DebugGraph::SetCurveValues(smoothedTransientIntensityMagns, 2,
                               GRAPH_MIN, GRAPH_WEIGHT_MAX,
                               2.0, 128, 128, 255);
    
    DebugGraph::SetCurveValues(weights, 3,
                               GRAPH_MIN, GRAPH_WEIGHT_MAX,
                               1.0, 200, 200, 200);

    DebugGraph::SetCurveSingleValue(threshold*THRESHOLD_COEFF, 4,
                                    GRAPH_MIN, GRAPH_WEIGHT_MAX,
                                    2.0, 200, 200, 255);

    DebugGraph::SetCurveValues(transientMagns, 5,
                               GRAPH_MIN, GRAPH_MAGN_MAX,
                               1.0, 0, 128, 0);
    
    DebugGraph::SetCurveValues(strippedMagns, 6,
                               GRAPH_MIN, GRAPH_MAGN_MAX,
                               1.0, 255, 0, 255);
    
#endif
    
    for (int i = 0; i < bufSize; i++)
    {
        BL_FLOAT magn;
        BL_FLOAT phase;
        WDL_FFT_COMPLEX comp = ioFftBuf->Get()[i];
        BLUtils::ComplexToMagnPhase(comp, &magn, &phase);
    
        // NOTE: maybe mistake in the direction of the mix parameter
        BL_FLOAT newMagn = mix*strippedMagns.Get()[i] + (1.0 - mix)*transientMagns.Get()[i];
        
        // TODO: check better the volume difference when bypass
        
        // With that, the result output is at the same scale as the input
#define CORRECTION_COEFF 2.0
        newMagn *= CORRECTION_COEFF;
        
        BLUtils::MagnPhaseToComplex(&comp, newMagn, phase);
        
        ioFftBuf->Get()[i] = comp;
    }
}

// This method works well, even if there something a bit unlogical
void
TransientLib2::DetectTransientsSmooth4(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioFftBuf,
                                       BL_FLOAT threshold,
                                       BL_FLOAT mix,
                                       BL_FLOAT detectThreshold,
                                       WDL_TypedBuf<BL_FLOAT> *transientIntensityMagns)
{
#define LOG_MAGN_COEFF 10000.0
    
    // ln(10000) = 9.xxx !
#define THRESHOLD_COEFF 1.0
#define THRESHOLD_OFFSET 0.5
    
#define TRANSIENT_CURVE_COEFF 1.0/4.0
    
    // Compute transients probability (and other things...)
    int bufSize = ioFftBuf->GetSize();
    
    transientIntensityMagns->Resize(bufSize);
    for (int i = 0; i < bufSize; i++)
        transientIntensityMagns->Get()[i] = 0.0;
    
    // Map, to find the magn positions who have participated to
    // a given transient value
    vector< vector< int > > transToMagn;
    transToMagn.resize(bufSize);
    
    WDL_TypedBuf<BL_FLOAT> magns;
    magns.Resize(bufSize);
    
#if DEBUG_GRAPH
    WDL_TypedBuf<BL_FLOAT> weights;
    
    WDL_TypedBuf<BL_FLOAT> strippedMagns;
    strippedMagns.Resize(bufSize);
    
    WDL_TypedBuf<BL_FLOAT> transientMagns;
    transientMagns.Resize(bufSize);
#endif
    
    BL_FLOAT prev = 0.0;
    for (int i = 0; i < bufSize; i++)
    {
        WDL_FFT_COMPLEX c = ioFftBuf->Get()[i];
        BL_FLOAT re = c.re;
        BL_FLOAT im = c.im;
        
        BL_FLOAT magn = std::sqrt(re*re + im*im);
        
        magns.Get()[i] = magn;
        
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
        
        BL_FLOAT transPos = phaseDiff*((BL_FLOAT)bufSize)/(2.0*M_PI);
        
        BL_FLOAT weight = std::log(1.0 + magn*LOG_MAGN_COEFF);
        
#if DEBUG_GRAPH
        weights.Add(weight);
#endif
        
        // TEST threshold
        
        if (magn > 0.0)
        {
	  BL_FLOAT weight2 = std::log(magn*magn);
            if (weight2 > detectThreshold)
            {
                // Don't threshold !!
                transientIntensityMagns->Get()[(int)transPos] += weight;
                
                // TODO: put the following out of the if !
                transToMagn[(int)transPos].push_back(i);
            }
        }
    }
    
    BL_FLOAT magnsAvg0 = BLUtils::ComputeAvgSquare(magns);
    
    // Smoother curve, for keeping non-transient parts
    WDL_TypedBuf<BL_FLOAT> smoothedTransientIntensityMagns4;
    smoothedTransientIntensityMagns4.Resize(transientIntensityMagns->GetSize());
    CMA2Smoother::ProcessOne(transientIntensityMagns->Get(),
                             smoothedTransientIntensityMagns4.Get(),
                             transientIntensityMagns->GetSize(),
                             transientIntensityMagns->GetSize()/4);
    
    // Sharper curve, to extract only transients accurately
    WDL_TypedBuf<BL_FLOAT> smoothedTransientIntensityMagns32;
    smoothedTransientIntensityMagns32.Resize(transientIntensityMagns->GetSize());
    CMA2Smoother::ProcessOne(transientIntensityMagns->Get(),
                             smoothedTransientIntensityMagns32.Get(),
                             transientIntensityMagns->GetSize(),
                             transientIntensityMagns->GetSize()/16);
    
#if 0
    // Make a linear interpolation of the two curves,
    // depending on the mix parameter
    WDL_TypedBuf<BL_FLOAT> smoothedTransientIntensityMagns;
    smoothedTransientIntensityMagns.Resize(bufSize);
    
    for (int i = 0; i < smoothedTransientIntensityMagns.GetSize(); i++)
    {
        BL_FLOAT v4 = smoothedTransientIntensityMagns4.Get()[i];
        BL_FLOAT v32 = smoothedTransientIntensityMagns32.Get()[i];
        
        BL_FLOAT v = (1.0 - mix)*v4 + mix*v32;
        smoothedTransientIntensityMagns.Get()[i] = v;
    }
#endif
    
#if 1
    WDL_TypedBuf<BL_FLOAT> smoothedTransientIntensityMagns = smoothedTransientIntensityMagns32;
#endif
    
#define AVERAGE_TRANS_CONTRIB 1
#if AVERAGE_TRANS_CONTRIB
    // Divide the smoothed magns by the maximum of transients for the maximum bin
    // So we will get a sort of average
    // And the maximum of the curve will be 1
    int maxNumTransients = 0;
    for (int i = 0; i < bufSize; i++)
    {
        vector<int> &trToMagn = transToMagn[i];
        
        int numTransients = trToMagn.size();
        if (numTransients > maxNumTransients)
            maxNumTransients = numTransients;
    }
    
    if (maxNumTransients > 0)
        BLUtils::MultValues(&smoothedTransientIntensityMagns, 1.0/maxNumTransients);
#endif
    
    // We must reduce the trandient curve amplitude, in order to be sure that
    // we could threshold it fully
    // For example with sin + white noise, the maximum goes high
#if 0
    BLUtils::MultValues(&smoothedTransientIntensityMagns, TRANSIENT_CURVE_COEFF);
#endif
    
#if 1 // TEST
    // Substract the average of the curve to itself
    // So it will be more "constant"
    BL_FLOAT avg = BLUtils::ComputeAvg(smoothedTransientIntensityMagns);
    BL_FLOAT offset = - avg;
    
#if !AVERAGE_TRANS_CONTRIB
    offset += THRESHOLD_COEFF/4
#else
    offset += THRESHOLD_OFFSET;
#endif
    
    BLUtils::AddValues(&smoothedTransientIntensityMagns, offset);
#endif
    
#if DEBUG_GRAPH
    // Set to all values
    strippedMagns = magns;
    transientMagns = magns;
#endif
    
    WDL_TypedBuf<BL_FLOAT> newMagns = magns;
    
    // Then set to 0 if not participated
    for (int i = 0; i < bufSize; i++)
    {
        BL_FLOAT smoothTrans = smoothedTransientIntensityMagns.Get()[i];
        vector<int> &trToMagn = transToMagn[i];
        
        if (smoothTrans > threshold*THRESHOLD_COEFF + THRESHOLD_OFFSET)
            // Transient
        {
            // Zero the stripped bins
            for (int j = 0; j < trToMagn.size(); j++)
            {
                int index = trToMagn[j];
                newMagns.Get()[index] *= mix;
                
#if DEBUG_GRAPH
                strippedMagns.Get()[index] = 0.0;
#endif
            }
        }
        else
        {
            // Zero the transient bins
            for (int j = 0; j < trToMagn.size(); j++)
            {
                int index = trToMagn[j];
               
                newMagns.Get()[index] *= (1.0 - mix);
                
#if DEBUG_GRAPH
                transientMagns.Get()[index] = 0.0;
#endif
            }
        }
    }
    
#if DEBUG_GRAPH
    
#define GRAPH_MIN 0.0
    
#define GRAPH_MAGN_MAX 0.1
#define GRAPH_WEIGHT_MAX 10.0
#define GRAPH_TRANS_MAX 20.0
#define GRAPH_TRANS_CURVE_MAX 4.0
#define GRAPH_THRESHOLD_MAX 1.0
    
    DebugGraph::SetCurveValues(magns, 0,
                               GRAPH_MIN, GRAPH_MAGN_MAX,
                               1.0, 255, 0, 0);
    
    DebugGraph::SetCurveValues(*transientIntensityMagns, 1,
                               GRAPH_MIN, GRAPH_TRANS_MAX,
                               1.0, 0, 0, 128);
    
    DebugGraph::SetCurveValues(smoothedTransientIntensityMagns4, 2,
                               GRAPH_MIN, GRAPH_WEIGHT_MAX,
                               2.0, 128, 128, 255);
    
    DebugGraph::SetCurveValues(smoothedTransientIntensityMagns32, 3,
                               GRAPH_MIN, GRAPH_WEIGHT_MAX,
                               2.0, 128, 128, 255);
    
    DebugGraph::SetCurveValues(smoothedTransientIntensityMagns, 4,
                               GRAPH_MIN, GRAPH_TRANS_CURVE_MAX,
                               3.0, 240, 240, 255);
    
    DebugGraph::SetCurveValues(weights, 5,
                               GRAPH_MIN, GRAPH_WEIGHT_MAX,
                               1.0, 150, 150, 150);
    
    // TODO: add the threshold offset !
    DebugGraph::SetCurveSingleValue(threshold*THRESHOLD_COEFF, 6,
                                    GRAPH_MIN, GRAPH_THRESHOLD_MAX,
                                    2.0, 200, 200, 255);
    
#if 0
    DebugGraph::SetCurveValues(transientMagns, 7,
                               GRAPH_MIN, GRAPH_MAGN_MAX,
                               1.0, 0, 255, 0);
    
    DebugGraph::SetCurveValues(strippedMagns, 8,
                               GRAPH_MIN, GRAPH_MAGN_MAX,
                               1.0, 255, 0, 255);    
#endif
    
#endif
    
#if 1
    // Normalize, to keep the same volume
    
    // Security
#define NORM_MAX_COEFF 4.0
    
    BL_FLOAT magnsAvg1 = BLUtils::ComputeAvgSquare(newMagns);
    
    if (magnsAvg0 > 0.0)
    {
        BL_FLOAT coeff = magnsAvg0 / magnsAvg1;
        
        if (coeff > NORM_MAX_COEFF)
            coeff = NORM_MAX_COEFF;
        
        BLUtils::MultValues(&newMagns, coeff);
    }
#endif
    
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
}
