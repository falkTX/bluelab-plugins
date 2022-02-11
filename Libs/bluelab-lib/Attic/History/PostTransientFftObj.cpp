//
//  PostTransientFftObj.cpp
//  BL-PitchShift
//
//  Created by Pan on 18/04/18.
//
//

#include <TransientShaperFftObj2.h>
#include <BLUtils.h>

#include "PostTransientFftObj.h"

#define DEBUG_GRAPH 0
#if DEBUG_GRAPH
#include <DebugGraph.h>

#define GRAPH_INPUT_CURVE 0
#define GRAPH_OUTPUT_CURVE 1
#define GRAPH_TRANSIENT_CURVE 2

//#define FIFO_DECIM_COEFF 1.0 // No scroll
#define FIFO_DECIM_COEFF 256.0 // Scrolls slowly

#endif

// Keep this comment for reminding
//
// KEEP_SYNTHESIS_ENERGY and VARIABLE_HANNING are tightliy bounded !
//
// KEEP_SYNTHESIS_ENERGY=1 + VARIABLE_HANNING=1
// Not so bad, but we loose frequencies
//
// KEEP_SYNTHESIS_ENERGY=0 + VARIABLE_HANNING=0: GOOD

// When set to 0, we will keep transient boost !
//#define KEEP_SYNTHESIS_ENERGY 0

// When set to 0, we are accurate for frequencies
//#define VARIABLE_HANNING 0


// Smoothing: we set to 0 to have a fine loking bell curve
// In the current implementation, if we increase the precision,
// we will have blurry frequencies (there is no threshold !),
// and the result transient precision won't be increased !
#define TRANSIENT_PRECISION 0.0

// NOTE: can saturate if greater than 1.0
#define TRANSIENT_BOOST_FACTOR 1.0

// Above this threshold, we don't compute transients
// (performances gain)
#define TRANS_BOOST_EPS 1e-15


PostTransientFftObj::PostTransientFftObj(int bufferSize, int overlapping, int freqRes,
                                         bool keepSynthesisEnergy, bool variableHanning,
                                         FftObj *obj)
: FftObj(bufferSize, overlapping, freqRes,
         AnalysisMethodWindow, SynthesisMethodWindow,
         keepSynthesisEnergy,
         variableHanning,
         false) // skip fft
{
    mTransBoost = 0.0;
    
    mTransObj = new TransientShaperFftObj2(bufferSize, overlapping, freqRes,
                                           // only detect transients, do not apply
                                           false);
    
    // This configuration seems good (only amp)
    mTransObj->SetFreqsToTrans(false);
    mTransObj->SetAmpsToTrans(true);
    
    mTransObj->SetPrecision(TRANSIENT_PRECISION);
    
    mFftObj = obj;
    
#if DEBUG_GRAPH
    int maxNumPoints = BUFFER_SIZE/4;
    BL_FLOAT decimFactor = 1.0/FIFO_DECIM_COEFF;
    
    mTransObj->SetTrackIO(maxNumPoints, decimFactor, true, false);
    mFftObj->SetTrackIO(maxNumPoints, decimFactor, false, true);
    
    SetDebugMode(true);
#endif
}

PostTransientFftObj::~PostTransientFftObj()
{
    delete mTransObj;
    //delete mFftObj; // Don't delete !
}

void
PostTransientFftObj::Reset(int overlapping, int freqRes)
{
    FftObj::Reset(overlapping, freqRes);
    
    if ((overlapping > 0) && (freqRes > 0))
    {
        mTransObj->Reset(overlapping, freqRes);
        mFftObj->Reset(overlapping, freqRes);
    }
}

void
PostTransientFftObj::SetTransBoost(BL_FLOAT factor)
{
    mTransBoost = factor;
    
    mTransObj->SetSoftHard(factor);
}

void
PostTransientFftObj::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                                            const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer)
{
    // Normal processing
    mFftObj->ProcessFftBuffer(ioBuffer, scBuffer);
}

void
PostTransientFftObj::ProcessSamplesBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer,
                                                WDL_TypedBuf<BL_FLOAT> *scBuffer)
{
    // Do not apply transientness if not necessary
    if (mTransBoost <= TRANS_BOOST_EPS)
        return;
    
    WDL_TypedBuf<BL_FLOAT> transientness;
    mTransObj->GetCurrentTransientness(&transientness);
    
    // Transient boost
    BLUtils::MultValues(&transientness, TRANSIENT_BOOST_FACTOR);
    //BLUtils::ClipMax(&transientness, 1.0);
    mTransObj->ApplyTransientness(ioBuffer, transientness);
    
    // DEBUG...
#if DEBUG_GRAPH
    WDL_TypedBuf<BL_FLOAT> input;
    mTransObj->GetCurrentInput(&input);
    DebugGraph::SetCurveValues(input, GRAPH_INPUT_CURVE,
                               -2.0, 2.0, // Y ( x 2 just for display clarity)
                               1.0, // line width
                               0, 0, 255);
    
    // HACK !!! => should design correctly for PostProcess to ba called !!
    // and it should be called before PostTransientFftObj::ProcessSamplesBuffer() !!
    mFftObj->PostProcessSamplesBuffer(ioBuffer);
    
    WDL_TypedBuf<BL_FLOAT> output;
    mFftObj->GetCurrentOutput(&output);
    DebugGraph::SetCurveValues(output, GRAPH_OUTPUT_CURVE,
                               -2.0, 2.0, // Y ( x 2 just for display clarity)
                               1.0, // line width
                               128, 128, 255);

    WDL_TypedBuf<BL_FLOAT> transientnessGraph;
    BLUtils::DecimateSamples(&transientnessGraph, transientness, 1.0/4.0);
    
    DebugGraph::SetCurveValues(transientnessGraph, GRAPH_TRANSIENT_CURVE,
                               0.0, 1.0, // Y
                               1.0, // line width
                               240, 240, 255);
#endif
}

void
PostTransientFftObj::ProcessOneBuffer(const WDL_TypedBuf<BL_FLOAT> &inBuffer,
                                      const WDL_TypedBuf<BL_FLOAT> &inScBuffer,
                                      WDL_TypedBuf<BL_FLOAT> *outBuffer)
{
    // Do not compute transientness if not necessary
    if (mTransBoost > TRANS_BOOST_EPS)
    {
        WDL_TypedBuf<BL_FLOAT> outBufferTrans;
        mTransObj->ProcessOneBuffer(inBuffer, inScBuffer, &outBufferTrans);
    }
    
    FftObj::ProcessOneBuffer(inBuffer, inScBuffer, outBuffer);
}
