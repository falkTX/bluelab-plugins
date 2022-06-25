/* Copyright (C) 2022 Nicolas Dittlo <deadlab.plugins@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this software; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
 
//
//  USTClipperDisplay4.cpp
//  UST
//
//  Created by applematuer on 7/30/19.
//
//

#ifdef IGRAPHICS_NANOVG

#include <GraphControl12.h>

#include <BLUtils.h>

#include "USTClipperDisplay4.h"

// We display 2 seconds of samples
#define NUM_SECONDS 4.0 //2.0
#define GRAPH_NUM_POINTS 256

// Curves
/*#define AXIS_CURVE 0

#define WAVEFORM_UP_CURVE 1
#define WAVEFORM_DOWN_CURVE 2

#define WAVEFORM_CLIP_UP_CURVE 3
#define WAVEFORM_CLIP_DOWN_CURVE 4

#define CLIP_LO_CURVE 6 //5
#define CLIP_HI_CURVE 7 //6

#define SWEEP_BAR_CURVE 5 //7
*/

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


USTClipperDisplay4::USTClipperDisplay4(BL_GUI_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    
    mZoom = 1.0;
    mCurrentClipValue = 0.0;
    
    //mGraph = graph;
    
#if SWEEP_UPDATE
    mSweepPos = 0;
    mSweepPosClip = 0;
#endif
    
    mGraph = NULL;

    // Curves
    mAxisCurve = NULL;
    mWaveformUpCurve = NULL;
    mWaveformDownCurve = NULL;
    mWaveformClipUpCurve = NULL;
    mWaveformClipDownCurve = NULL;
    mClipLoCurve = NULL;
    mClipHiCurve = NULL;
    mSweepBarCurve = NULL;

    CreateCurves();
    
    Reset(sampleRate);
}

USTClipperDisplay4::~USTClipperDisplay4()
{
  delete mAxisCurve;
  delete mWaveformUpCurve;
  delete mWaveformDownCurve;
  delete mWaveformClipUpCurve;
  delete mWaveformClipDownCurve;
  delete mClipLoCurve;
  delete mClipHiCurve;
  delete mSweepBarCurve;
}

void
USTClipperDisplay4::SetGraph(GraphControl12 *graph)
{
    mGraph = graph;
    
    if (mGraph != NULL)
    {
        mGraph->SetBounds(0.0, 0.0, 1.0, 1.0);
        //mGraph->SetClearColor(255, 0, 255, 255); // DEBUG
        mGraph->SetClearColor(0, 0, 0, 255);
        
        // Orizontal axis
        //mGraph->SetCurveColor(AXIS_CURVE, 232, 110, 36);
        
#if ORANGE_COLOR_SCHEME
        mAxisCurve->SetColor(252, 79, 36);
#endif
        
#if BLUE_COLOR_SCHEME
        mAxisCurve->SetColor(170, 202, 209);
#endif
	
        mAxisCurve->SetAlpha(1.0);
        mAxisCurve->SetLineWidth(2.0);
        mAxisCurve->SetFill(false);
        mAxisCurve->SetFillAlpha(CURVE_FILL_ALPHA);
        //mAxisCurve->SetYScale(false, 0.0, 1.0);
        mAxisCurve->SetSingleValueH(true);
        //mAxisCurve->SetYScale(false, -2.0, 2.0);
        mAxisCurve->SetYScale(Scale::LINEAR, -2.0, 2.0);
        
        // Must set view size before value...
        int width;
        int height;
        mGraph->GetSize(&width, &height);
        mAxisCurve->SetViewSize(width, height);
        
        mAxisCurve->SetSingleValueH((BL_GUI_FLOAT)0.0);
        
        // Waveform curves
        //
        
#define FILL_ORIGIN_Y_OFFSET 0.001
        
        // Waveform up
        
        //mGraph->SetCurveDescription(WAVEFORM_UP_CURVE, "input", descrColor);
#if (!ORANGE_COLOR_SCHEME && !BLUE_COLOR_SCHEME)
        mWaveformUpCurve->SetColor(128, 128, 255);
        
        // Dummy
        mWaveformClipUpCurve->SetColor(252, 228, 205);
#endif
        
#if ORANGE_COLOR_SCHEME
        //mGraph->SetCurveColor(WAVEFORM_CURVE, 252, 228, 205);
        mWaveformUpCurve->SetColor(252, 79, 36);
        mWaveformDownCurve->SetColor(252, 79, 36);
        
        mWaveformClipUpCurve->SetColor(252, 228, 205);
#endif
        
#if BLUE_COLOR_SCHEME
        // Blue
        //mGraph->SetCurveColor(WAVEFORM_UP_CURVE, 170, 202, 209);
        //mGraph->SetCurveColor(WAVEFORM_DOWN_CURVE, 170, 202, 209);
        
        // Orange
        mWaveformUpCurve->SetColor(234, 101, 0);
        mWaveformDownCurve->SetColor(234, 101, 0);
        
        mWaveformClipUpCurve->SetColor(113, 130, 182);
        mWaveformClipDownCurve->SetColor(113, 130, 182);
#endif
        
        mWaveformUpCurve->SetAlpha(1.0);
        mWaveformUpCurve->SetLineWidth(-1.0); // Disable draw line over fill
        mWaveformUpCurve->SetFill(true, 0.5 - FILL_ORIGIN_Y_OFFSET);
        mWaveformUpCurve->SetFillAlpha(1.0);
        mWaveformUpCurve->SetYScale(Scale::LINEAR, -2.0, 2.0);
        
        // Waveform down
        
        //mGraph->SetCurveColor(WAVEFORM_DOWN_CURVE, 252, 79, 36);
        mWaveformDownCurve->SetAlpha(1.0);
        mWaveformDownCurve->SetLineWidth(-1.0);
        mWaveformDownCurve->SetFillAlpha(1.0);
        mWaveformDownCurve->SetFill(true, 0.5 + FILL_ORIGIN_Y_OFFSET);
        mWaveformDownCurve->SetYScale(Scale::LINEAR, -2.0, 2.0);
        
        // Waveform clip up
        //mGraph->SetCurveColor(WAVEFORM_CLIP_UP_CURVE, 252, 228, 205);
        mWaveformClipUpCurve->SetAlpha(1.0);
        mWaveformClipUpCurve->SetLineWidth(-1.0);
        mWaveformClipUpCurve->SetFillAlpha(1.0);
        mWaveformClipUpCurve->SetFill(true, 0.5 - FILL_ORIGIN_Y_OFFSET);
        mWaveformClipUpCurve->SetYScale(Scale::LINEAR, -2.0, 2.0);
        
        // Waveform clip down
        //mGraph->SetCurveColor(WAVEFORM_CLIP_DOWN_CURVE, 252, 228, 205);
        mWaveformClipDownCurve->SetAlpha(1.0);
        mWaveformClipDownCurve->SetLineWidth(-1.0);
        mWaveformClipDownCurve->SetFillAlpha(1.0);
        mWaveformClipDownCurve->SetFill(true, 0.5 + FILL_ORIGIN_Y_OFFSET);
        mWaveformClipDownCurve->SetYScale(Scale::LINEAR, -2.0, 2.0);
        
        // Clip Lo
        
        //mGraph->SetCurveDescription(GRAPH_THRESHOLD_CURVE,
        //                            "threshold", descrColor);
        
#if (!ORANGE_COLOR_SCHEME && !BLUE_COLOR_SCHEME)
        mClipLoCurve->SetColor(64, 64, 255);
        mClipHiCurveh->SetColor(64, 64, 255);
#endif
        
#if ORANGE_COLOR_SCHEME
        mClipLoCurve->SetColor(232, 110, 36);
        mClipHiCurve->SetColor(232, 110, 36);
#endif
        
#if BLUE_COLOR_SCHEME
        //mGraph->SetCurveColor(CLIP_LO_CURVE, 113, 130, 182);
        
        // Blue
        mClipLoCurve->SetColor(255, 255, 255);
        mClipHiCurve->SetColor(255, 255, 255);
        
        // Orange
        //mGraph->SetCurveColor(CLIP_LO_CURVE, 234, 101, 0);
        //mGraph->SetCurveColor(CLIP_HI_CURVE, 234, 101, 0);
#endif
        
        
        mClipLoCurve->SetAlpha(1.0);
        mClipLoCurve->SetLineWidth(2.0);
        
#if FILL_CLIP_LINES
        mClipLoCurve->SetFill(true);
#endif
        
        //mGraph->SetCurveFillAlpha(CLIP_LO_CURVE, CURVE_FILL_ALPHA);

        // Doesn't fill low
        mClipLoCurve->SetFillAlpha(0.0);
        // Fill up only
        mClipLoCurve->SetFillAlphaUp(CURVE_FILL_ALPHA); //
        
        //mClipLoCurve->SetYScale(Scale::LINEAR, 0.0, 1.0);
        mClipLoCurve->SetSingleValueH(true);
        
        //mClipLoCurve->SetYScale(Scale::LINEAR, -2.0, 2.0);
        mClipLoCurve->SetYScale(Scale::LINEAR, -2.0, 2.0); // NEW
        
        // Must set view size before value...
        mClipLoCurve->SetViewSize(width, height);
        mClipLoCurve->SetSingleValueH((BL_GUI_FLOAT)-1.0);
        
        // Clip Hi
        
        //mGraph->SetCurveDescription(GRAPH_THRESHOLD_CURVE,
        //                            "threshold", descrColor);
        
        mClipHiCurve->SetAlpha(1.0);
        mClipHiCurve->SetLineWidth(2.0);
        
#if FILL_CLIP_LINES
        mClipHiCurve->SetFill(true);
#endif
        
        //mGraph->SetCurveFillAlpha(CLIP_HI_CURVE, CURVE_FILL_ALPHA);
        //mGraph->SetCurveFillAlphaUp(CLIP_HI_CURVE, CURVE_FILL_ALPHA);
        mClipHiCurve->SetFillAlpha(CURVE_FILL_ALPHA);//
        
        //mClipHiCurve->SetYScale(Scale::LINEAR, 0.0, 1.0);
        mClipHiCurve->SetSingleValueH(true);
        //mClipHiCurve->SetYScale(Scale::LINEAR, -2.0, 2.0);
        mClipHiCurve->SetYScale(Scale::LINEAR, -2.0, 2.0); // NEW
        
        // Must set view size before value...
        mClipHiCurve->SetViewSize(width, height);
        mClipHiCurve->SetSingleValueH((BL_GUI_FLOAT)1.0);
	  
        // Sweep bar
        mSweepBarCurve->SetColor(255, 255, 255);
        mSweepBarCurve->SetAlpha(1.0);
        mSweepBarCurve->SetLineWidth(1.0);
        mSweepBarCurve->SetSingleValueV(true);
        //mGraph->SetCurveXScale(SWEEP_BAR_CURVE, false);
        
        // Add curved
        mGraph->AddCurve(mAxisCurve);
        mGraph->AddCurve(mWaveformUpCurve);
        mGraph->AddCurve(mWaveformDownCurve);
        mGraph->AddCurve(mWaveformClipUpCurve);
        mGraph->AddCurve(mWaveformClipDownCurve);
        mGraph->AddCurve(mClipLoCurve);
        mGraph->AddCurve(mClipHiCurve);
        mGraph->AddCurve(mSweepBarCurve);
    }
}

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
        //mGraph->Resize(GRAPH_NUM_POINTS);
        
        mWaveformUpCurve->SetValues4(mCurrentDecimValuesUp);
        mWaveformDownCurve->SetValues4(mCurrentDecimValuesDown);
        
        mWaveformClipUpCurve->SetValues4(mCurrentDecimValuesUpClip);
        mWaveformClipDownCurve->SetValues4(mCurrentDecimValuesDownClip);
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
    if (mGraph != NULL)
    {
        //mGraph->SetDirty(true);
        mGraph->SetDataChanged();
    }
}

void
USTClipperDisplay4::SetEnabled(bool flag)
{
    if (!flag)
    {
        // Rewind sweep line
        if (mSweepBarCurve != NULL)
            mSweepBarCurve->SetSingleValueV((BL_FLOAT)0.0);
        
        if (mGraph != NULL)
        {
            mGraph->SetDataChanged();
        }
    }
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
    
    if (mGraph != NULL)
    {
        //mGraph->SetDirty(true);
        mGraph->SetDataChanged();
    }
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
        
        mWaveformUpCurve->SetValues4(decimValuesUp);
        mWaveformDownCurve->SetValues4(decimValuesDown);
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
        
        mWaveformClipUpCurve->SetValues4(decimValuesUp);
        mWaveformClipDownCurve->SetValues4(decimValuesDown);
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
    
    if (mGraph != NULL)
    {
        mClipLoCurve->SetSingleValueH(clipValue /*- 2.0*/);
        mClipHiCurve->SetSingleValueH(/*2.0*/ - clipValue);
    }
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
    if (mGraph != NULL)
    {
        BL_GUI_FLOAT pos = ((BL_GUI_FLOAT)mSweepPos)/GRAPH_NUM_POINTS;
        mSweepBarCurve->SetSingleValueV(pos);
    }
}

void
USTClipperDisplay4::CreateCurves()
{
  mAxisCurve = new GraphCurve5(GRAPH_NUM_POINTS);
  mWaveformUpCurve = new GraphCurve5(GRAPH_NUM_POINTS);
  mWaveformDownCurve = new GraphCurve5(GRAPH_NUM_POINTS);
  mWaveformClipUpCurve = new GraphCurve5(GRAPH_NUM_POINTS);
  mWaveformClipDownCurve = new GraphCurve5(GRAPH_NUM_POINTS);
  mClipLoCurve = new GraphCurve5(GRAPH_NUM_POINTS);
  mClipHiCurve = new GraphCurve5(GRAPH_NUM_POINTS);
  mSweepBarCurve = new GraphCurve5(GRAPH_NUM_POINTS);
}

#endif // IGRAPHICS_NANOVG
