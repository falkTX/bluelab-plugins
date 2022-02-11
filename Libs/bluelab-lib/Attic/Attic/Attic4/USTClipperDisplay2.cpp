//
//  USTClipperDisplay2.cpp
//  UST
//
//  Created by applematuer on 7/30/19.
//
//

#include <GraphControl11.h>
#include <FifoDecimator2.h>
#include <BLUtils.h>

#include "USTClipperDisplay2.h"

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
#define WAVEFORM_UP_CURVE 0
#define WAVEFORM_DOWN_CURVE 1

#define WAVEFORM_CLIP_UP_CURVE 2
#define WAVEFORM_CLIP_DOWN_CURVE 3

#define CLIP_LO_CURVE  4
#define CLIP_HI_CURVE  5


#define CURVE_FILL_ALPHA 0.2

#define ORANGE_COLOR_SCHEME 1


USTClipperDisplay2::USTClipperDisplay2(GraphControl11 *graph)
{
    mFifoDecim = new FifoDecimator2(FIFO_DECIM_MAX_SIZE, FIFO_DECIM_FACTOR, true);
    mFifoDecimClip = new FifoDecimator2(FIFO_DECIM_MAX_SIZE, FIFO_DECIM_FACTOR, true);
    
    mGraph = graph;
    
    if (mGraph != NULL)
    {
        mGraph->SetBounds(0.0, 0.0, 1.0, 1.0);
        //mGraph->SetClearColor(255, 0, 255, 255); // DEBUG
        mGraph->SetClearColor(0, 0, 0, 255);
    
        // Waveform curves
        //
    
#define FILL_ORIGIN_Y_OFFSET 0.001

        // Waveform up
    
        //mGraph->SetCurveDescription(WAVEFORM_UP_CURVE, "input", descrColor);
#if !ORANGE_COLOR_SCHEME
        mGraph->SetCurveColor(WAVEFORM_UP_CURVE, 128, 128, 255);
#else
        //mGraph->SetCurveColor(WAVEFORM_CURVE, 252, 228, 205);
        mGraph->SetCurveColor(WAVEFORM_UP_CURVE, 252, 79, 36);
#endif
        
        mGraph->SetCurveAlpha(WAVEFORM_UP_CURVE, 1.0);
        mGraph->SetCurveLineWidth(WAVEFORM_UP_CURVE, -1.0); // Disable draw line over fill
        mGraph->SetCurveFill(WAVEFORM_UP_CURVE, true, 0.5 - FILL_ORIGIN_Y_OFFSET);
        mGraph->SetCurveFillAlpha(WAVEFORM_UP_CURVE, 1.0);
        mGraph->SetCurveYScale(WAVEFORM_UP_CURVE, false, -2.0, 2.0);
    
        // Waveform down
        mGraph->SetCurveColor(WAVEFORM_DOWN_CURVE, 252, 79, 36);
        mGraph->SetCurveAlpha(WAVEFORM_DOWN_CURVE, 1.0);
        mGraph->SetCurveLineWidth(WAVEFORM_DOWN_CURVE, -1.0);
        mGraph->SetCurveFillAlpha(WAVEFORM_DOWN_CURVE, 1.0);
        mGraph->SetCurveFill(WAVEFORM_DOWN_CURVE, true, 0.5 + FILL_ORIGIN_Y_OFFSET);
        mGraph->SetCurveYScale(WAVEFORM_DOWN_CURVE, false, -2.0, 2.0);
        
        // Waveform clip up
        mGraph->SetCurveColor(WAVEFORM_CLIP_UP_CURVE, 252, 228, 205);
        mGraph->SetCurveAlpha(WAVEFORM_CLIP_UP_CURVE, 1.0);
        mGraph->SetCurveLineWidth(WAVEFORM_CLIP_UP_CURVE, -1.0);
        mGraph->SetCurveFillAlpha(WAVEFORM_CLIP_UP_CURVE, 1.0);
        mGraph->SetCurveFill(WAVEFORM_CLIP_UP_CURVE, true, 0.5 - FILL_ORIGIN_Y_OFFSET);
        mGraph->SetCurveYScale(WAVEFORM_CLIP_UP_CURVE, false, -2.0, 2.0);
        
        // Waveform clip down
        mGraph->SetCurveColor(WAVEFORM_CLIP_DOWN_CURVE, 252, 228, 205);
        mGraph->SetCurveAlpha(WAVEFORM_CLIP_DOWN_CURVE, 1.0);
        mGraph->SetCurveLineWidth(WAVEFORM_CLIP_DOWN_CURVE, -1.0);
        mGraph->SetCurveFillAlpha(WAVEFORM_CLIP_DOWN_CURVE, 1.0);
        mGraph->SetCurveFill(WAVEFORM_CLIP_DOWN_CURVE, true, 0.5 + FILL_ORIGIN_Y_OFFSET);
        mGraph->SetCurveYScale(WAVEFORM_CLIP_DOWN_CURVE, false, -2.0, 2.0);
        
        // Clip Lo
    
        //mGraph->SetCurveDescription(GRAPH_THRESHOLD_CURVE,
        //                            "threshold", descrColor);
        
#if !ORANGE_COLOR_SCHEME
        mGraph->SetCurveColor(CLIP_LO_CURVE, 64, 64, 255);
#else
        mGraph->SetCurveColor(CLIP_LO_CURVE, 232, 110, 36);
#endif
        
        mGraph->SetCurveAlpha(CLIP_LO_CURVE, 1.0);
        mGraph->SetCurveLineWidth(CLIP_LO_CURVE, 2.0);
        mGraph->SetCurveFill(CLIP_LO_CURVE, true);
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
        mGraph->SetCurveFill(CLIP_HI_CURVE, true);
        //mGraph->SetCurveFillAlpha(CLIP_HI_CURVE, CURVE_FILL_ALPHA);
        mGraph->SetCurveFillAlphaUp(CLIP_HI_CURVE, CURVE_FILL_ALPHA);
        mGraph->SetCurveYScale(CLIP_HI_CURVE, false, 0.0, 1.0);
        mGraph->SetCurveSingleValueH(CLIP_HI_CURVE, true);
        mGraph->SetCurveYScale(CLIP_HI_CURVE, false, -2.0, 2.0);
        mGraph->SetCurveSingleValueH(CLIP_HI_CURVE, (BL_FLOAT)1.0);
    }
}

USTClipperDisplay2::~USTClipperDisplay2()
{
    delete mFifoDecim;
    delete mFifoDecimClip;
}

void
USTClipperDisplay2::SetClipValue(BL_FLOAT clipValue)
{
    if (mGraph == NULL)
        return;
    
    mGraph->SetCurveSingleValueH(CLIP_LO_CURVE, (BL_FLOAT)(clipValue - 2.0));
    
    mGraph->SetCurveSingleValueH(CLIP_HI_CURVE, (BL_FLOAT)(2.0 - clipValue));
}

void
USTClipperDisplay2::AddSamples(const WDL_TypedBuf<BL_FLOAT> &samples)
{
    mFifoDecim->AddValues(samples);
    
    WDL_TypedBuf<BL_FLOAT> decimValues;
    mFifoDecim->GetValues(&decimValues);
    
    WDL_TypedBuf<BL_FLOAT> decimValuesUp = decimValues;
    BLUtils::CutHalfSamples(&decimValuesUp, true);
    
    WDL_TypedBuf<BL_FLOAT> decimValuesDown = decimValues;
    BLUtils::CutHalfSamples(&decimValuesDown, false);
    
    if (mGraph != NULL)
    {
        mGraph->SetCurveValues3(WAVEFORM_UP_CURVE, &decimValuesUp);
        mGraph->SetCurveValues3(WAVEFORM_DOWN_CURVE, &decimValuesDown);
    }
}

void
USTClipperDisplay2::AddClippedSamples(const WDL_TypedBuf<BL_FLOAT> &samples)
{
    mFifoDecimClip->AddValues(samples);
    
    WDL_TypedBuf<BL_FLOAT> decimValues;
    mFifoDecimClip->GetValues(&decimValues);
    
    WDL_TypedBuf<BL_FLOAT> decimValuesUp = decimValues;
    BLUtils::CutHalfSamples(&decimValuesUp, true);
    
    WDL_TypedBuf<BL_FLOAT> decimValuesDown = decimValues;
    BLUtils::CutHalfSamples(&decimValuesDown, false);
    
    if (mGraph != NULL)
    {
        mGraph->SetCurveValues3(WAVEFORM_CLIP_UP_CURVE, &decimValuesUp);
        mGraph->SetCurveValues3(WAVEFORM_CLIP_DOWN_CURVE, &decimValuesDown);
    }
}
