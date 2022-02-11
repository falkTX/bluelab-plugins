//
//  USTClipperDisplay3.cpp
//  UST
//
//  Created by applematuer on 7/30/19.
//
//

#ifdef IGRAPHICS_NANOVG

#include <GraphControl11.h>
#include <SamplesPyramid2.h>
#include <BLUtils.h>

#include "USTClipperDisplay3.h"

// We display 2 seconds of samples
#define NUM_SECONDS 2.0

// Curves
#define AXIS_CURVE 0

#define WAVEFORM_UP_CURVE 1
#define WAVEFORM_DOWN_CURVE 2

#define WAVEFORM_CLIP_UP_CURVE 3
#define WAVEFORM_CLIP_DOWN_CURVE 4

#define CLIP_LO_CURVE 5
#define CLIP_HI_CURVE 6

//#define CURVE_FILL_ALPHA 0.2

#define ORANGE_COLOR_SCHEME 0
#define BLUE_COLOR_SCHEME 1

#define FILL_CLIP_LINES 1 //0
#define CURVE_FILL_ALPHA 0.1

// NOTE: when playing a loop, Reaper doesn't respect the buffer size
// just at the limit of looping
// So it is better to manage fixed buffer size here, to avoid
// waveform jittering on loops
#define BUFFER_SIZE 1024

// BAD: no need, because the sample pyramid manages it correclty already
// Fix the increasing speed with high sample rates
#define FIX_SCOLL_SPEED_SAMPLERATE 0 //1


USTClipperDisplay3::USTClipperDisplay3(GraphControl11 *graph, BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    
    mPyramid = new SamplesPyramid2();
    mPyramidClip = new SamplesPyramid2();
    
    mZoom = 1.0;
    mCurrentClipValue = 0.0;
    
    mGraph = graph;
    
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
        mGraph->SetCurveSingleValueH(AXIS_CURVE, (BL_FLOAT)0.0);
        
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
        mGraph->SetCurveSingleValueH(CLIP_LO_CURVE, (BL_FLOAT)-1.0);
    
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
        mGraph->SetCurveSingleValueH(CLIP_HI_CURVE, (BL_FLOAT)1.0);
    }
    
    Reset(sampleRate);
}

USTClipperDisplay3::~USTClipperDisplay3()
{
    delete mPyramid;
    delete mPyramidClip;
}

void
USTClipperDisplay3::Reset(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    
    BL_FLOAT numSamples = GetPyramidSize();
    
    WDL_TypedBuf<BL_FLOAT> zeros;
    BLUtils::ResizeFillZeros(&zeros, numSamples);
    
    mPyramid->SetValues(zeros);
    mPyramidClip->SetValues(zeros);
    
    mCurrentSamples.Resize(0);
    mCurrentClippedSamples.Resize(0);
    
    if (mGraph != NULL)
    {
        int numPoints = mGraph->GetNumCurveValues();
        
        WDL_TypedBuf<BL_FLOAT> zeros;
        zeros.Resize(numPoints);
        BLUtils::FillAllZero(&zeros);
        
        mGraph->SetCurveValues3(WAVEFORM_UP_CURVE, &zeros);
        mGraph->SetCurveValues3(WAVEFORM_DOWN_CURVE, &zeros);
        
        mGraph->SetCurveValues3(WAVEFORM_CLIP_UP_CURVE, &zeros);
        mGraph->SetCurveValues3(WAVEFORM_CLIP_DOWN_CURVE, &zeros);
    }
}

void
USTClipperDisplay3::SetClipValue(BL_FLOAT clipValue)
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
USTClipperDisplay3::AddSamples(const WDL_TypedBuf<BL_FLOAT> &samplesIn)
{
    WDL_TypedBuf<BL_FLOAT> samples = samplesIn;
    
#if FIX_SCOLL_SPEED_SAMPLERATE
    BL_FLOAT decimFactor = 44100.0/mSampleRate;
    if (decimFactor < 1.0)
    {
        BLUtils::DecimateSamples(&samples, samplesIn, decimFactor);
    }
#endif
    
    mCurrentSamples.Add(samples.Get(), samples.GetSize());
    
    while(mCurrentSamples.GetSize() >= BUFFER_SIZE)
    {
        // Buffer management
        WDL_TypedBuf<BL_FLOAT> bufSamples;
        bufSamples.Add(mCurrentSamples.Get(), BUFFER_SIZE);
        BLUtils::ConsumeLeft(&mCurrentSamples, BUFFER_SIZE);
        
        // Pyramid
        mPyramid->PushValues(bufSamples);
        mPyramid->PopValues(bufSamples.GetSize());
    
        BL_FLOAT endPos = GetPyramidSize() - 1;
    
        WDL_TypedBuf<BL_FLOAT> decimValues;
        mPyramid->GetValues(0, endPos, 256, &decimValues);
    
        // Cut
        WDL_TypedBuf<BL_FLOAT> decimValuesUp = decimValues;
        BLUtils::CutHalfSamples(&decimValuesUp, true);
    
        WDL_TypedBuf<BL_FLOAT> decimValuesDown = decimValues;
        BLUtils::CutHalfSamples(&decimValuesDown, false);
    
        // For zoom
        mCurrentDecimValuesUp = decimValuesUp;
        mCurrentDecimValuesDown = decimValuesDown;
        
        AddSamplesZoom();
        
#if 0
        // Graph
        if (mGraph != NULL)
        {
            mGraph->SetCurveValues3(WAVEFORM_UP_CURVE, &decimValuesUp);
            mGraph->SetCurveValues3(WAVEFORM_DOWN_CURVE, &decimValuesDown);
        }
#endif
    }
}

void
USTClipperDisplay3::AddClippedSamples(const WDL_TypedBuf<BL_FLOAT> &samplesIn)
{
    WDL_TypedBuf<BL_FLOAT> samples = samplesIn;
    
#if FIX_SCOLL_SPEED_SAMPLERATE
    BL_FLOAT decimFactor = 44100.0/mSampleRate;
    if (decimFactor < 1.0)
    {
        BLUtils::DecimateSamples(&samples, samplesIn, decimFactor);
    }
#endif
    
    mCurrentClippedSamples.Add(samples.Get(), samples.GetSize());
    
    while(mCurrentClippedSamples.GetSize() >= BUFFER_SIZE)
    {
        // Buffer management
        WDL_TypedBuf<BL_FLOAT> bufSamples;
        bufSamples.Add(mCurrentClippedSamples.Get(), BUFFER_SIZE);
        BLUtils::ConsumeLeft(&mCurrentClippedSamples, BUFFER_SIZE);

        // Pyramid
        mPyramidClip->PushValues(bufSamples);
        mPyramidClip->PopValues(bufSamples.GetSize());
    
        BL_FLOAT endPos = GetPyramidSize() - 1;
    
        WDL_TypedBuf<BL_FLOAT> decimValues;
        mPyramidClip->GetValues(0, endPos, 256, &decimValues);
    
        // Cut
        WDL_TypedBuf<BL_FLOAT> decimValuesUp = decimValues;
        BLUtils::CutHalfSamples(&decimValuesUp, true);
    
        WDL_TypedBuf<BL_FLOAT> decimValuesDown = decimValues;
        BLUtils::CutHalfSamples(&decimValuesDown, false);
    
        // For zoom
        mCurrentDecimValuesUpClip = decimValuesUp;
        mCurrentDecimValuesDownClip = decimValuesDown;
        
        AddSamplesZoomClip();
#if 0
        // Graph
        if (mGraph != NULL)
        {
            mGraph->SetCurveValues3(WAVEFORM_CLIP_UP_CURVE, &decimValuesUp);
            mGraph->SetCurveValues3(WAVEFORM_CLIP_DOWN_CURVE, &decimValuesDown);
        }
#endif
    }
}

void
USTClipperDisplay3::SetDirty()
{
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
}

void
USTClipperDisplay3::SetZoom(BL_FLOAT zoom)
{
    mZoom = zoom;
    
    AddSamplesZoom();
    AddSamplesZoomClip();
    
    SetClipValueZoom();
    
    //mGraph->SetDirty(true);
    mGraph->SetDataChanged();
}

void
USTClipperDisplay3::AddSamplesZoom()
{
    // Graph
    if (mGraph != NULL)
    {
        WDL_TypedBuf<BL_FLOAT> decimValuesUp = mCurrentDecimValuesUp;
        WDL_TypedBuf<BL_FLOAT> decimValuesDown = mCurrentDecimValuesDown;
        
        BLUtils::MultValues(&decimValuesUp, mZoom);
        BLUtils::MultValues(&decimValuesDown, mZoom);
        
        mGraph->SetCurveValues3(WAVEFORM_UP_CURVE, &decimValuesUp);
        mGraph->SetCurveValues3(WAVEFORM_DOWN_CURVE, &decimValuesDown);
    }
}

void
USTClipperDisplay3::AddSamplesZoomClip()
{
    if (mGraph != NULL)
    {
        WDL_TypedBuf<BL_FLOAT> decimValuesUp = mCurrentDecimValuesUpClip;
        WDL_TypedBuf<BL_FLOAT> decimValuesDown = mCurrentDecimValuesDownClip;
        
        BLUtils::MultValues(&decimValuesUp, mZoom);
        BLUtils::MultValues(&decimValuesDown, mZoom);
        
        mGraph->SetCurveValues3(WAVEFORM_CLIP_UP_CURVE, &decimValuesUp);
        mGraph->SetCurveValues3(WAVEFORM_CLIP_DOWN_CURVE, &decimValuesDown);
    }
}

void
USTClipperDisplay3::SetClipValueZoom()
{
    BL_FLOAT clipValue = mCurrentClipValue;
    clipValue *= mZoom;
    
    mGraph->SetCurveSingleValueH(CLIP_LO_CURVE, clipValue /*- 2.0*/);
    mGraph->SetCurveSingleValueH(CLIP_HI_CURVE, /*2.0*/ - clipValue);
}

long
USTClipperDisplay3::GetPyramidSize()
{
    long numSamples = mSampleRate*NUM_SECONDS;
    
    // Avoid jittering when scrolling
    numSamples = BLUtils::NextPowerOfTwo(numSamples);
    
    return numSamples;
}

#endif // IGRAPHICS_NANOVG
