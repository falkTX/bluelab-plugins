//
//  USTClipperDisplay4.cpp
//  UST
//
//  Created by applematuer on 7/30/19.
//
//

#include <GraphControl11.h>

#include <BLUtils.h>

#include "USTClipperDisplay4.h"

// We display 2 seconds of samples
#define NUM_SECONDS 4.0 //2.0
#define GRAPH_NUM_POINTS 256

// Curves
#define AXIS_CURVE 0

#define WAVEFORM_UP_CURVE 1
#define WAVEFORM_DOWN_CURVE 2

#define WAVEFORM_CLIP_UP_CURVE 3
#define WAVEFORM_CLIP_DOWN_CURVE 4

#define CLIP_LO_CURVE 6 //5
#define CLIP_HI_CURVE 7 //6

#define SWEEP_BAR_CURVE 5 //7

//#define CURVE_FILL_ALPHA 0.2

#define ORANGE_COLOR_SCHEME 0
#define BLUE_COLOR_SCHEME 1

#define FILL_CLIP_LINES 1 //0
#define CURVE_FILL_ALPHA 0.1

// On this version, scrolling display is not debugged well (it jitters)
// So, let's bet on sweep update !
#define SWEEP_UPDATE 1

// Use a kind of gamma curve for the zoom (so 1 remains 1, but small values are incerased)
// Smart zoom.
#define GAMMA_ZOOM 0 //1


USTClipperDisplay4::USTClipperDisplay4(GraphControl11 *graph, BL_GUI_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    
    mZoom = 1.0;
    mCurrentClipValue = 0.0;
    
    mGraph = graph;
    
#if SWEEP_UPDATE
    mSweepPos = 0;
    mSweepPosClip = 0;
#endif
    
    if (mGraph != NULL)
    {
        mGraph->SetBounds(0.0, 0.0, 1.0, 1.0);
        //mGraph->SetClearColor(255, 0, 255, 255); // DEBUG
        mGraph->SetClearColor(0, 0, 0, 255);
    
        // Orizontal axis
        //mGraph->SetCurveColor(AXIS_CURVE, 232, 110, 36);
        
#if ORANGE_COLOR_SCHEME
        mGraph->SetCurveColor(AXIS_CURVE, 252, 79, 36);
#endif
        
#if BLUE_COLOR_SCHEME
        mGraph->SetCurveColor(AXIS_CURVE, 170, 202, 209);
#endif
        
        mGraph->SetCurveAlpha(AXIS_CURVE, 1.0);
        mGraph->SetCurveLineWidth(AXIS_CURVE, 2.0);
        mGraph->SetCurveFill(AXIS_CURVE, false);
        mGraph->SetCurveFillAlpha(AXIS_CURVE, CURVE_FILL_ALPHA);
        mGraph->SetCurveYScale(AXIS_CURVE, false, 0.0, 1.0);
        mGraph->SetCurveSingleValueH(AXIS_CURVE, true);
        mGraph->SetCurveYScale(AXIS_CURVE, false, -2.0, 2.0);
        mGraph->SetCurveSingleValueH(AXIS_CURVE, (BL_GUI_FLOAT)0.0);
        
        // Waveform curves
        //
    
#define FILL_ORIGIN_Y_OFFSET 0.001

        // Waveform up
    
        //mGraph->SetCurveDescription(WAVEFORM_UP_CURVE, "input", descrColor);
#if (!ORANGE_COLOR_SCHEME && !BLUE_COLOR_SCHEME)
        mGraph->SetCurveColor(WAVEFORM_UP_CURVE, 128, 128, 255);
        
        // Dummy
        mGraph->SetCurveColor(WAVEFORM_CLIP_UP_CURVE, 252, 228, 205);
#endif
        
#if ORANGE_COLOR_SCHEME
        //mGraph->SetCurveColor(WAVEFORM_CURVE, 252, 228, 205);
        mGraph->SetCurveColor(WAVEFORM_UP_CURVE, 252, 79, 36);
        mGraph->SetCurveColor(WAVEFORM_DOWN_CURVE, 252, 79, 36);
        
        mGraph->SetCurveColor(WAVEFORM_CLIP_UP_CURVE, 252, 228, 205);
#endif

#if BLUE_COLOR_SCHEME
        // Blue
        //mGraph->SetCurveColor(WAVEFORM_UP_CURVE, 170, 202, 209);
        //mGraph->SetCurveColor(WAVEFORM_DOWN_CURVE, 170, 202, 209);
        
        // Orange
        mGraph->SetCurveColor(WAVEFORM_UP_CURVE, 234, 101, 0);
        mGraph->SetCurveColor(WAVEFORM_DOWN_CURVE, 234, 101, 0);
        
        mGraph->SetCurveColor(WAVEFORM_CLIP_UP_CURVE, 113, 130, 182);
        mGraph->SetCurveColor(WAVEFORM_CLIP_DOWN_CURVE, 113, 130, 182);
#endif

        mGraph->SetCurveAlpha(WAVEFORM_UP_CURVE, 1.0);
        mGraph->SetCurveLineWidth(WAVEFORM_UP_CURVE, -1.0); // Disable draw line over fill
        mGraph->SetCurveFill(WAVEFORM_UP_CURVE, true, 0.5 - FILL_ORIGIN_Y_OFFSET);
        mGraph->SetCurveFillAlpha(WAVEFORM_UP_CURVE, 1.0);
        mGraph->SetCurveYScale(WAVEFORM_UP_CURVE, false, -2.0, 2.0);
    
        // Waveform down
        
        //mGraph->SetCurveColor(WAVEFORM_DOWN_CURVE, 252, 79, 36);
        mGraph->SetCurveAlpha(WAVEFORM_DOWN_CURVE, 1.0);
        mGraph->SetCurveLineWidth(WAVEFORM_DOWN_CURVE, -1.0);
        mGraph->SetCurveFillAlpha(WAVEFORM_DOWN_CURVE, 1.0);
        mGraph->SetCurveFill(WAVEFORM_DOWN_CURVE, true, 0.5 + FILL_ORIGIN_Y_OFFSET);
        mGraph->SetCurveYScale(WAVEFORM_DOWN_CURVE, false, -2.0, 2.0);
        
        // Waveform clip up
        //mGraph->SetCurveColor(WAVEFORM_CLIP_UP_CURVE, 252, 228, 205);
        mGraph->SetCurveAlpha(WAVEFORM_CLIP_UP_CURVE, 1.0);
        mGraph->SetCurveLineWidth(WAVEFORM_CLIP_UP_CURVE, -1.0);
        mGraph->SetCurveFillAlpha(WAVEFORM_CLIP_UP_CURVE, 1.0);
        mGraph->SetCurveFill(WAVEFORM_CLIP_UP_CURVE, true, 0.5 - FILL_ORIGIN_Y_OFFSET);
        mGraph->SetCurveYScale(WAVEFORM_CLIP_UP_CURVE, false, -2.0, 2.0);
        
        // Waveform clip down
        //mGraph->SetCurveColor(WAVEFORM_CLIP_DOWN_CURVE, 252, 228, 205);
        mGraph->SetCurveAlpha(WAVEFORM_CLIP_DOWN_CURVE, 1.0);
        mGraph->SetCurveLineWidth(WAVEFORM_CLIP_DOWN_CURVE, -1.0);
        mGraph->SetCurveFillAlpha(WAVEFORM_CLIP_DOWN_CURVE, 1.0);
        mGraph->SetCurveFill(WAVEFORM_CLIP_DOWN_CURVE, true, 0.5 + FILL_ORIGIN_Y_OFFSET);
        mGraph->SetCurveYScale(WAVEFORM_CLIP_DOWN_CURVE, false, -2.0, 2.0);
        
        // Clip Lo
    
        //mGraph->SetCurveDescription(GRAPH_THRESHOLD_CURVE,
        //                            "threshold", descrColor);
        
#if (!ORANGE_COLOR_SCHEME && !BLUE_COLOR_SCHEME)
        mGraph->SetCurveColor(CLIP_LO_CURVE, 64, 64, 255);
        mGraph->SetCurveColor(CLIP_HI_CURVE, 64, 64, 255);
#endif
        
#if ORANGE_COLOR_SCHEME
        mGraph->SetCurveColor(CLIP_LO_CURVE, 232, 110, 36);
        mGraph->SetCurveColor(CLIP_HI_CURVE, 232, 110, 36);
#endif

#if BLUE_COLOR_SCHEME
        //mGraph->SetCurveColor(CLIP_LO_CURVE, 113, 130, 182);
        
        // Blue
        mGraph->SetCurveColor(CLIP_LO_CURVE, 255, 255, 255);
        mGraph->SetCurveColor(CLIP_HI_CURVE, 255, 255, 255);
        
        // Orange
        //mGraph->SetCurveColor(CLIP_LO_CURVE, 234, 101, 0);
        //mGraph->SetCurveColor(CLIP_HI_CURVE, 234, 101, 0);
#endif

        
        mGraph->SetCurveAlpha(CLIP_LO_CURVE, 1.0);
        mGraph->SetCurveLineWidth(CLIP_LO_CURVE, 2.0);
        
#if FILL_CLIP_LINES
        mGraph->SetCurveFill(CLIP_LO_CURVE, true);
#endif
        
        //mGraph->SetCurveFillAlpha(CLIP_LO_CURVE, CURVE_FILL_ALPHA);
        mGraph->SetCurveFillAlphaUp(CLIP_LO_CURVE, CURVE_FILL_ALPHA); //
        
        mGraph->SetCurveYScale(CLIP_LO_CURVE, false, 0.0, 1.0);
        mGraph->SetCurveSingleValueH(CLIP_LO_CURVE, true);
    
        mGraph->SetCurveYScale(CLIP_LO_CURVE, false, -2.0, 2.0);
        mGraph->SetCurveSingleValueH(CLIP_LO_CURVE, (BL_GUI_FLOAT)-1.0);
    
        // Clip Hi
    
        //mGraph->SetCurveDescription(GRAPH_THRESHOLD_CURVE,
        //                            "threshold", descrColor);
        
        mGraph->SetCurveAlpha(CLIP_HI_CURVE, 1.0);
        mGraph->SetCurveLineWidth(CLIP_HI_CURVE, 2.0);
        
#if FILL_CLIP_LINES
        mGraph->SetCurveFill(CLIP_HI_CURVE, true);
#endif
        
        //mGraph->SetCurveFillAlpha(CLIP_HI_CURVE, CURVE_FILL_ALPHA);
        //mGraph->SetCurveFillAlphaUp(CLIP_HI_CURVE, CURVE_FILL_ALPHA);
        mGraph->SetCurveFillAlpha(CLIP_HI_CURVE, CURVE_FILL_ALPHA);//
        
        mGraph->SetCurveYScale(CLIP_HI_CURVE, false, 0.0, 1.0);
        mGraph->SetCurveSingleValueH(CLIP_HI_CURVE, true);
        mGraph->SetCurveYScale(CLIP_HI_CURVE, false, -2.0, 2.0);
        mGraph->SetCurveSingleValueH(CLIP_HI_CURVE, (BL_GUI_FLOAT)1.0);
        
        // Sweep bar
        mGraph->SetCurveColor(SWEEP_BAR_CURVE, 255, 255, 255);
        mGraph->SetCurveAlpha(SWEEP_BAR_CURVE, 1.0);
        mGraph->SetCurveLineWidth(SWEEP_BAR_CURVE, 1.0);
        mGraph->SetCurveSingleValueV(SWEEP_BAR_CURVE, true);
        //mGraph->SetCurveXScale(SWEEP_BAR_CURVE, false);
    }
    
    Reset(sampleRate);
}

USTClipperDisplay4::~USTClipperDisplay4() {}

void
USTClipperDisplay4::Reset(BL_GUI_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    
    WDL_TypedBuf<BL_GUI_FLOAT> zeros;
    BLUtils::ResizeFillZeros(&zeros, GRAPH_NUM_POINTS);
    
    // To bufferize
    mCurrentSamples.Resize(0);
    mCurrentClippedSamples.Resize(0);
    
    // To display
    mCurrentDecimValuesUp = zeros;
    mCurrentDecimValuesDown = zeros;
    
    mCurrentDecimValuesUpClip = zeros;
    mCurrentDecimValuesDownClip = zeros;
    //
    
#if SWEEP_UPDATE
    mSweepPos = 0;
    mSweepPosClip = 0;
#endif

    if (mGraph != NULL)
    {
        mGraph->Resize(GRAPH_NUM_POINTS);
        
        mGraph->SetCurveValues3(WAVEFORM_UP_CURVE, &mCurrentDecimValuesUp);
        mGraph->SetCurveValues3(WAVEFORM_DOWN_CURVE, &mCurrentDecimValuesDown);
        
        mGraph->SetCurveValues3(WAVEFORM_CLIP_UP_CURVE, &mCurrentDecimValuesUpClip);
        mGraph->SetCurveValues3(WAVEFORM_CLIP_DOWN_CURVE, &mCurrentDecimValuesDownClip);
    }
}

void
USTClipperDisplay4::SetClipValue(BL_GUI_FLOAT clipValue)
{
    if (mGraph == NULL)
        return;
    
#if 0
    if (clipValue > 1.0)
        clipValue = 1.0;
    if (clipValue < -1.0)
        clipValue = -1.0;
#endif
    
    //clipValue = 1.0 - clipValue;
    
    mCurrentClipValue = clipValue;
    
    SetClipValueZoom();
}

void
USTClipperDisplay4::AddSamples(const WDL_TypedBuf<BL_FLOAT> &samplesIn)
{
    WDL_TypedBuf<BL_GUI_FLOAT> samples;
    BLUtils::ConvertToGUIFloatType(&samples, samplesIn);
    
    mCurrentSamples.Add(samples.Get(), samples.GetSize());
    
    int numSamples = GetNumSamples();
    long oneLineSizeSamples = numSamples/GRAPH_NUM_POINTS;
    
    while(mCurrentSamples.GetSize() >= oneLineSizeSamples)
    {
        // Buffer management
        WDL_TypedBuf<BL_GUI_FLOAT> bufSamples;
        bufSamples.Add(mCurrentSamples.Get(), oneLineSizeSamples);
        BLUtils::ConsumeLeft(&mCurrentSamples, oneLineSizeSamples);
    
        // Find min and max (equaivalent to decimation to get a single value)
        BL_GUI_FLOAT decimLineMin;
        BL_GUI_FLOAT decimLineMax;
        DecimateSamplesOneLine(bufSamples, &decimLineMin, &decimLineMax);

        // Cut
        if (decimLineMin > 0.0)
            decimLineMin = 0.0;
        
        if (decimLineMax < 0.0)
            decimLineMax = 0.0;
    
        // For zoom
#if !SWEEP_UPDATE
        mCurrentDecimValuesUp.Add(&decimLineMax, 1);
        BLUtils::ConsumeLeft(&mCurrentDecimValuesUp, 1);
        
        mCurrentDecimValuesDown.Add(&decimLineMin, 1);
        BLUtils::ConsumeLeft(&mCurrentDecimValuesDown, 1);
#else
        mCurrentDecimValuesUp.Get()[mSweepPos] = decimLineMax;;
        mCurrentDecimValuesDown.Get()[mSweepPos] = decimLineMin;
        
        mSweepPos = (mSweepPos + 1) % GRAPH_NUM_POINTS;
#endif
        
        AddSamplesZoom();
        
        UpdateSweepBar();
    }
}

void
USTClipperDisplay4::AddClippedSamples(const WDL_TypedBuf<BL_FLOAT> &samplesIn)
{
    WDL_TypedBuf<BL_GUI_FLOAT> samples;
    BLUtils::ConvertToGUIFloatType(&samples, samplesIn);
    
    mCurrentClippedSamples.Add(samples.Get(), samples.GetSize());
    
    int numSamples = GetNumSamples();
    long oneLineSizeSamples = numSamples/GRAPH_NUM_POINTS;
    
    while(mCurrentClippedSamples.GetSize() >= oneLineSizeSamples)
    {
        // Buffer management
        WDL_TypedBuf<BL_GUI_FLOAT> bufSamples;
        bufSamples.Add(mCurrentClippedSamples.Get(), oneLineSizeSamples);
        BLUtils::ConsumeLeft(&mCurrentClippedSamples, oneLineSizeSamples);
        
        // Find min and max (equaivalent to decimation to get a single value)
        BL_GUI_FLOAT decimLineMin;
        BL_GUI_FLOAT decimLineMax;
        DecimateSamplesOneLine(bufSamples, &decimLineMin, &decimLineMax);
        
        // Cut
        if (decimLineMin > 0.0)
            decimLineMin = 0.0;
        
        if (decimLineMax < 0.0)
            decimLineMax = 0.0;
        
        // For zoom
#if !SWEEP_UPDATE
        mCurrentDecimValuesUpClip.Add(&decimLineMax, 1);
        BLUtils::ConsumeLeft(&mCurrentDecimValuesUpClip, 1);
        
        mCurrentDecimValuesDownClip.Add(&decimLineMin, 1);
        BLUtils::ConsumeLeft(&mCurrentDecimValuesDownClip, 1);
#else
        mCurrentDecimValuesUpClip.Get()[mSweepPosClip] = decimLineMax;
        mCurrentDecimValuesDownClip.Get()[mSweepPosClip] = decimLineMin;
        
        mSweepPosClip = (mSweepPosClip + 1) % GRAPH_NUM_POINTS;
#endif
        
        //
        AddSamplesZoomClip();
    }
}

void
USTClipperDisplay4::SetDirty()
{
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
}

void
USTClipperDisplay4::SetZoom(BL_GUI_FLOAT zoom)
{
    mZoom = zoom;
    
#if GAMMA_ZOOM
    // Smaller zoom value if we use param shape
    mZoom = 1.0 + (mZoom - 1.0)*0.5;
#endif
    
    AddSamplesZoom();
    AddSamplesZoomClip();
    
    SetClipValueZoom();
    
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
}

void
USTClipperDisplay4::AddSamplesZoom()
{
    // Graph
    if (mGraph != NULL)
    {
        WDL_TypedBuf<BL_GUI_FLOAT> decimValuesUp = mCurrentDecimValuesUp;
        WDL_TypedBuf<BL_GUI_FLOAT> decimValuesDown = mCurrentDecimValuesDown;
        
#if !GAMMA_ZOOM
        BLUtils::MultValues(&decimValuesUp, mZoom);
        BLUtils::MultValues(&decimValuesDown, mZoom);
#else
        BLUtils::ApplyParamShapeWaveform(&decimValuesUp, mZoom);
        BLUtils::ApplyParamShapeWaveform(&decimValuesDown, mZoom);
#endif
        
        mGraph->SetCurveValues3(WAVEFORM_UP_CURVE, &decimValuesUp);
        mGraph->SetCurveValues3(WAVEFORM_DOWN_CURVE, &decimValuesDown);
    }
}

void
USTClipperDisplay4::AddSamplesZoomClip()
{
    if (mGraph != NULL)
    {
        WDL_TypedBuf<BL_GUI_FLOAT> decimValuesUp = mCurrentDecimValuesUpClip;
        WDL_TypedBuf<BL_GUI_FLOAT> decimValuesDown = mCurrentDecimValuesDownClip;
        
#if !GAMMA_ZOOM
        BLUtils::MultValues(&decimValuesUp, mZoom);
        BLUtils::MultValues(&decimValuesDown, mZoom);
#else
        BLUtils::ApplyParamShapeWaveform(&decimValuesUp, mZoom);
        BLUtils::ApplyParamShapeWaveform(&decimValuesDown, mZoom);
#endif
        
        mGraph->SetCurveValues3(WAVEFORM_CLIP_UP_CURVE, &decimValuesUp);
        mGraph->SetCurveValues3(WAVEFORM_CLIP_DOWN_CURVE, &decimValuesDown);
    }
}

void
USTClipperDisplay4::SetClipValueZoom()
{
    BL_GUI_FLOAT clipValue = mCurrentClipValue;
    
#if !GAMMA_ZOOM
    clipValue *= mZoom;
#else
    bool neg = (clipValue < 0.0);
    if (neg)
        clipValue = -clipValue;
    
    clipValue = BLUtils::ApplyParamShape(clipValue, mZoom);
    
    if (neg)
        clipValue = -clipValue;
#endif
    
    mGraph->SetCurveSingleValueH(CLIP_LO_CURVE, clipValue /*- 2.0*/);
    mGraph->SetCurveSingleValueH(CLIP_HI_CURVE, /*2.0*/ - clipValue);
}

long
USTClipperDisplay4::GetNumSamples()
{
    long numSamples = mSampleRate*NUM_SECONDS;
    
    return numSamples;
}

// Find min and max (equaivalent to decimation to get a single value)
void
USTClipperDisplay4::DecimateSamplesOneLine(const WDL_TypedBuf<BL_GUI_FLOAT> &bufSamples,
                                           BL_GUI_FLOAT *decimLineMin, BL_GUI_FLOAT *decimLineMax)
{
    BL_GUI_FLOAT minVal = BLUtils::ComputeMin(bufSamples);
    BL_GUI_FLOAT maxVal = BLUtils::ComputeMax(bufSamples);
    
    *decimLineMin = minVal;
    *decimLineMax = maxVal;
}

void
USTClipperDisplay4::UpdateSweepBar()
{
    BL_GUI_FLOAT pos = ((BL_GUI_FLOAT)mSweepPos)/GRAPH_NUM_POINTS;
    mGraph->SetCurveSingleValueV(SWEEP_BAR_CURVE, pos);
}
