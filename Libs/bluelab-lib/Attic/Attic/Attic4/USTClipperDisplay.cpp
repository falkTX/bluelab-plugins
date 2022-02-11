//
//  USTClipperDisplay.cpp
//  UST
//
//  Created by applematuer on 7/30/19.
//
//

#include <GraphControl11.h>
#include <FifoDecimator2.h>

#include "USTClipperDisplay.h"

// Fifo decim 
//#define FIFO_DECIM_MAX_SIZE 256
//#define FIFO_DECIM_FACTOR 0.25

// TODO: adapt to frequency
//#define FIFO_DECIM_MAX_SIZE 256
//#define FIFO_DECIM_FACTOR 0.25*256.0/44100.0 // 1 second

// TODO: adapt to samplerate
// For FifoDecimator2
#define NUM_SECONDS 2.0
#define FIFO_DECIM_MAX_SIZE 44100*NUM_SECONDS
#define FIFO_DECIM_FACTOR 256.0/(44100.0*NUM_SECONDS)

// Curves
#define WAVEFORM_CURVE 0
#define CLIP_LO_CURVE  1
#define CLIP_HI_CURVE  2

#define CURVE_FILL_ALPHA 0.2

#define ORANGE_COLOR_SCHEME 1


USTClipperDisplay::USTClipperDisplay(GraphControl11 *graph)
{
    mFifoDecim = new FifoDecimator2(FIFO_DECIM_MAX_SIZE, FIFO_DECIM_FACTOR, true);
    
    mGraph = graph;
    
    if (mGraph != NULL)
    {
        mGraph->SetBounds(0.0, 0.0, 1.0, 1.0);
        //mGraph->SetClearColor(255, 0, 255, 255); // DEBUG
        mGraph->SetClearColor(0, 0, 0, 255);
    
        // Waveform curve
        //
    
        // Waveform
    
        //mGraph->SetCurveDescription(WAVEFORM_CURVE, "input", descrColor);
#if !ORANGE_COLOR_SCHEME
        mGraph->SetCurveColor(WAVEFORM_CURVE, 128, 128, 255);
#else
        mGraph->SetCurveColor(WAVEFORM_CURVE, 252, 228, 205);
#endif
        
        mGraph->SetCurveAlpha(WAVEFORM_CURVE, 1.0);
        mGraph->SetCurveLineWidth(WAVEFORM_CURVE, 1.0);
        //mGraph->SetCurveFill(WAVEFORM_CURVE, true);
        //mGraph->SetCurveFillAlpha(WAVEFORM_CURVE, CURVE_FILL_ALPHA);
        mGraph->SetCurveYScale(WAVEFORM_CURVE, false, -2.0, 2.0);
    
        // Clip Lo
    
        //mGraph->SetCurveDescription(GRAPH_THRESHOLD_CURVE,
        //                            "threshold", descrColor);
        
#if !ORANGE_COLOR_SCHEME
        mGraph->SetCurveColor(CLIP_LO_CURVE, 64, 64, 255);
#else
        mGraph->SetCurveColor(CLIP_LO_CURVE, 232, 110, 36);
#endif
      
        // Zero is in the middle, since it is a waveform-like curve
        float curveFillOriginY = 0.5;
        
        mGraph->SetCurveAlpha(CLIP_LO_CURVE, 1.0);
        mGraph->SetCurveLineWidth(CLIP_LO_CURVE, 2.0);
        mGraph->SetCurveFill(CLIP_LO_CURVE, true, curveFillOriginY);
        mGraph->SetCurveFillAlpha(CLIP_LO_CURVE, CURVE_FILL_ALPHA);
        mGraph->SetCurveYScale(CLIP_LO_CURVE, false, 0.0, 1.0);
        mGraph->SetCurveSingleValueH(CLIP_LO_CURVE, true);
    
        mGraph->SetCurveYScale(CLIP_LO_CURVE, false, -2.0, 2.0);
        mGraph->SetCurveSingleValueH(CLIP_LO_CURVE, (BL_FLOAT)-1.0);
    
        // Clip Hi
    
        //mGraph->SetCurveDescription(GRAPH_THRESHOLD_CURVE,
        //                            "threshold", descrColor);
#if !ORANGE_COLOR_SCHEME
        mGraph->SetCurveColor(CLIP_HI_CURVE, 64, 64, 255);
#else
        mGraph->SetCurveColor(CLIP_HI_CURVE, 232, 110, 36);
#endif
        
        mGraph->SetCurveAlpha(CLIP_HI_CURVE, 1.0);
        mGraph->SetCurveLineWidth(CLIP_HI_CURVE, 2.0);
        mGraph->SetCurveFill(CLIP_HI_CURVE, true, curveFillOriginY);
        //mGraph->SetCurveFillAlpha(CLIP_HI_CURVE, CURVE_FILL_ALPHA);
        mGraph->SetCurveFillAlphaUp(CLIP_HI_CURVE, CURVE_FILL_ALPHA);
        mGraph->SetCurveYScale(CLIP_HI_CURVE, false, 0.0, 1.0);
        mGraph->SetCurveSingleValueH(CLIP_HI_CURVE, true);
        mGraph->SetCurveYScale(CLIP_HI_CURVE, false, -2.0, 2.0);
        mGraph->SetCurveSingleValueH(CLIP_HI_CURVE, (BL_FLOAT)1.0);
    }
}

USTClipperDisplay::~USTClipperDisplay()
{
    delete mFifoDecim;
}

void
USTClipperDisplay::SetClipValue(BL_FLOAT clipValue)
{
    if (mGraph == NULL)
        return;
    
    mGraph->SetCurveSingleValueH(CLIP_LO_CURVE, (BL_FLOAT)(clipValue - 2.0));
    
    mGraph->SetCurveSingleValueH(CLIP_HI_CURVE, (BL_FLOAT)(2.0 - clipValue));
}

void
USTClipperDisplay::AddSamples(const WDL_TypedBuf<BL_FLOAT> &samples)
{
    mFifoDecim->AddValues(samples);
    
    WDL_TypedBuf<BL_FLOAT> decimValues;
    mFifoDecim->GetValues(&decimValues);
    
    if (mGraph != NULL)
        mGraph->SetCurveValues3(WAVEFORM_CURVE, &decimValues);
}
