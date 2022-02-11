//
//  PitchShiftTransientFftObj.cpp
//  BL-PitchShift
//
//  Created by Pan on 18/04/18.
//
//

#include <PitchShiftFftObj2.h>
#include <TransientShaperFftObj2.h>
#include <Utils.h>

#include "PitchShiftTransientFftObj.h"

#define DEBUG_GRAPH 0
#if DEBUG_GRAPH
#include <DebugGraph.h>

#define GRAPH_INPUT_CURVE 0
#define GRAPH_OUTPUT_CURVE 1
#define GRAPH_TRANSIENT_CURVE 2

#endif

// KEEP_SYNTHESIS_ENERGY and VARIABLE_HANNING are tightliy bounded !
//
// KEEP_SYNTHESIS_ENERGY=1 + VARIABLE_HANNING=1
// Not so bad, but we loose frequencies
//
// KEEP_SYNTHESIS_ENERGY=0 + VARIABLE_HANNING=0: GOOD


// When set to 0, we will keep transient boost !
#define KEEP_SYNTHESIS_ENERGY 0

// When set to 0, we are accurate for frequencies
#define VARIABLE_HANNING 0

// Smoothing: we set to 0 to have a fine loking bell curve
#define TRANSIENT_PRECISION 0.0 //0.9

// NOTE: can saturate if greater than 1.0
#define TRANSIENT_BOOST_FACTOR 1.0

// EXPE that doesnt work
//
// Ignore new phases when transients are high
// Good for more transient precision, but
// makes some brief oscillations in frequencies
// (maybe due to the value of the threshold param...
#define PHASE_IGNORE_EXPE 0

// For DEBUG
//#define FIFO_DECIM_COEFF 1.0 // No scroll
#define FIFO_DECIM_COEFF 256.0 // Scrolls slowly


PitchShiftTransientFftObj::PitchShiftTransientFftObj(int bufferSize, int overlapping, int freqRes,
                                                     double sampleRate)
: FftObj(bufferSize, overlapping, freqRes,
         AnalysisMethodWindow, SynthesisMethodWindow,
         KEEP_SYNTHESIS_ENERGY, // keep energy
         VARIABLE_HANNING,
         false) // skip fft)
{
    mTransObj = new TransientShaperFftObj2(bufferSize, overlapping, freqRes,
                                           // only detect transients, do not apply
                                           false);
    
    // This configuration seems good (only amp)
    mTransObj->SetFreqsToTrans(false);
    mTransObj->SetAmpsToTrans(true);
    
    mPitchObj = new PitchShiftFftObj2(bufferSize, overlapping, freqRes,
                                      sampleRate);
    
#if DEBUG_GRAPH
    int maxNumPoints = BUFFER_SIZE/4;
    double precision = TRANSIENT_PRECISION;
    double decimFactor = 1.0/FIFO_DECIM_COEFF;
    
    mTransObj->SetTrackIO(maxNumPoints, decimFactor, true, false);
    mPitchObj->SetTrackIO(maxNumPoints, decimFactor, false, true);
    
    SetDebugMode(true);
#endif
}

PitchShiftTransientFftObj::~PitchShiftTransientFftObj()
{
    delete mTransObj;
    delete mPitchObj;
}

void
PitchShiftTransientFftObj::Reset(int overlapping, int freqRes, double sampleRate)
{
    FftProcessObj13::Reset(overlapping, freqRes);
    
    mTransObj->Reset(overlapping, freqRes);
    mPitchObj->Reset(overlapping, freqRes, sampleRate);
}

void
PitchShiftTransientFftObj::SetPitchFactor(double factor)
{
    mPitchObj->SetPitchFactor(factor);
}

void
PitchShiftTransientFftObj::SetTransBoost(double factor)
{
    mTransObj->SetSoftHard(factor);
}

void
PitchShiftTransientFftObj::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                                            const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer)
{
    // Not so bad (but a bit worse...)
#if PHASE_IGNORE_EXPE
#define PHASE_IGNORE_EXPE_FACTOR 8.0
#endif
    
    // BAD !
    // => make frequencies vibrate
    //
    // ... and potentially revert the freqAdjustObj
#define USE_PHASE_RESTORE 0
    
#if PHASE_IGNORE_EXPE
    WDL_TypedBuf<double> origMagns;
    WDL_TypedBuf<double> origPhases;
    Utils::ComplexToMagnPhase(&origMagns, &origPhases, *ioBuffer);
    
#if USE_PHASE_RESTORE)
    mPitchObj->SavePhasesState();
#endif
#endif
    
    
    //
    // Normal method
    //
    mPitchObj->ProcessFftBuffer(ioBuffer, scBuffer);
    
    
#if PHASE_IGNORE_EXPE
    WDL_TypedBuf<double> transientness;
    mTransObj->GetCurrentTransientness(&transientness);
    
    // max ?
    double avg = Utils::ComputeAvg(transientness);
    avg *= PHASE_IGNORE_EXPE_FACTOR;
    
    // Better: use by frequencies instead of avg ?
    if (avg > mTransThreshold)
    {
        WDL_TypedBuf<double> newMagns;
        WDL_TypedBuf<double> newPhases;
        Utils::ComplexToMagnPhase(&newMagns, &newPhases, *ioBuffer);
        
        // Set the old phases !
        Utils::MagnPhaseToComplex(ioBuffer, newMagns, origPhases);
        
#if USE_PHASE_RESTORE
        mPitchObj->RestorePhasesState();
#endif
    }
#endif
}

void
PitchShiftTransientFftObj::ProcessSamplesBuffer(WDL_TypedBuf<double> *ioBuffer,
                                                WDL_TypedBuf<double> *scBuffer)
{
    WDL_TypedBuf<double> transientness;
    mTransObj->GetCurrentTransientness(&transientness);
    
    // Transient boost
    Utils::MultValues(&transientness, TRANSIENT_BOOST_FACTOR);
    //Utils::ClipMax(&transientness, 1.0);
    mTransObj->ApplyTransientness(ioBuffer, transientness);
    
    // DEBUG...
#if DEBUG_GRAPH
    WDL_TypedBuf<double> input;
    mTransObj->GetCurrentInput(&input);
    DebugGraph::SetCurveValues(input, GRAPH_INPUT_CURVE,
                               -2.0, 2.0, // Y ( x 2 just for display clarity)
                               1.0, // line width
                               0, 0, 255);
    
    // HACK !!! => should design correctly for PostProcess to ba called !!
    // and it should be called before PitchShiftTransientFftObj::ProcessSamplesBuffer() !!
    mPitchObj->PostProcessSamplesBuffer(ioBuffer);
    
    WDL_TypedBuf<double> output;
    mPitchObj->GetCurrentOutput(&output);
    DebugGraph::SetCurveValues(output, GRAPH_OUTPUT_CURVE,
                               -2.0, 2.0, // Y ( x 2 just for display clarity)
                               1.0, // line width
                               128, 128, 255);
    
    WDL_TypedBuf<double> transientnessGraph;
    Utils::DecimateSamples(&transientnessGraph, transientness, 1.0/4.0);
    
    DebugGraph::SetCurveValues(transientnessGraph, GRAPH_TRANSIENT_CURVE,
                               0.0, 1.0, // Y
                               1.0, // line width
                               240, 240, 255);
#endif
}

void
PitchShiftTransientFftObj::ProcessOneBuffer(const WDL_TypedBuf<double> &inBuffer,
                                            const WDL_TypedBuf<double> &inScBuffer,
                                            WDL_TypedBuf<double> *outBuffer)
{
    WDL_TypedBuf<double> outBufferTrans;
    mTransObj->ProcessOneBuffer(inBuffer, inScBuffer, &outBufferTrans);
    
    FftProcessObj13::ProcessOneBuffer(inBuffer, inScBuffer, outBuffer);
}
