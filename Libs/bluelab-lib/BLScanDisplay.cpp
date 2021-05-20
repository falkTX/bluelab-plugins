//
//  BLScanDisplay.cpp
//  UST
//
//  Created by applematuer on 7/30/19.
//
//

#ifdef IGRAPHICS_NANOVG

#include <GraphControl12.h>

#include <BLUtils.h>

#include "BLScanDisplay.h"

// We display 2 seconds of samples
#define NUM_SECONDS 4.0 //2.0
//#define NUM_SECONDS 12.0 //16.0 //8.0
#define GRAPH_NUM_POINTS 256

// Curves
#define BLUE_COLOR_SCHEME 1

#define CURVE_FILL_ALPHA 0.1

// On this version, scrolling display is not debugged well (it jitters)
// So, let's bet on sweep update !
#define SWEEP_UPDATE 1

// Use a kind of gamma curve for the zoom (so 1 remains 1, but small values are incerased)
// Smart zoom.
#define GAMMA_ZOOM 0 //1


BLScanDisplay::BLScanDisplay(int numCurves, BL_GUI_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    
    mZoom = 1.0;
    
#if SWEEP_UPDATE
    mSweepPos = 0;
#endif
    
    mGraph = NULL;

    // Curves
    mAxisCurve = NULL;
    mSweepBarCurve = NULL;

    CreateCurves(numCurves);
    
    Reset(sampleRate);

    mIsEnabled = false;
}

BLScanDisplay::~BLScanDisplay()
{
    DeleteCurves();  
}

void
BLScanDisplay::SetGraph(GraphControl12 *graph)
{        
    mGraph = graph;
    
    if (mGraph != NULL)
    {
        mGraph->SetBounds(0.0, 0.0, 1.0, 1.0);
        mGraph->SetClearColor(0, 0, 0, 255);
                
#if BLUE_COLOR_SCHEME
        mAxisCurve->SetColor(170, 202, 209);
#endif
	
        mAxisCurve->SetAlpha(1.0);
        mAxisCurve->SetLineWidth(2.0);
        mAxisCurve->SetFill(false);
        mAxisCurve->SetFillAlpha(CURVE_FILL_ALPHA);
        mAxisCurve->SetSingleValueH(true);
        mAxisCurve->SetYScale(Scale::LINEAR, -2.0, 2.0);
        
        // Must set view size before value...
        int width;
        int height;
        mGraph->GetSize(&width, &height);
        mAxisCurve->SetViewSize(width, height);
        
        mAxisCurve->SetSingleValueH((BL_GUI_FLOAT)0.0);
                        
        // Sweep bar
        mSweepBarCurve->SetColor(255, 255, 255);
        mSweepBarCurve->SetAlpha(1.0);
        mSweepBarCurve->SetLineWidth(1.0);
        mSweepBarCurve->SetSingleValueV(true);
        
        // Add curved
        mGraph->AddCurve(mAxisCurve);

        for (int i = 0; i < mCurves.size(); i++)
        {
            Curve &c = mCurves[i];

            mGraph->AddCurve(c.mCurves[0]);
            if (c.mIsSampleCurve)
                mGraph->AddCurve(c.mCurves[1]);
        }

        mGraph->AddCurve(mSweepBarCurve);
    }
}

void
BLScanDisplay::Reset(BL_GUI_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
    
    WDL_TypedBuf<BL_GUI_FLOAT> zeros;
    BLUtils::ResizeFillZeros(&zeros, GRAPH_NUM_POINTS);

    for (int i = 0; i < mCurves.size(); i++)
    {
        Curve &curve = mCurves[i];
        
        // To bufferize
        curve.mCurrentSamples.Resize(0);
    
        // To display
        curve.mCurrentDecimValues[0] = zeros;
        if (curve.mIsSampleCurve)
            curve.mCurrentDecimValues[1] = zeros;

        if (mGraph != NULL)
        {        
            curve.mCurves[0]->SetValues5(curve.mCurrentDecimValues[0]);
            if (curve.mIsSampleCurve)
                curve.mCurves[1]->SetValues5(curve.mCurrentDecimValues[1]);
        }
    }
    
#if SWEEP_UPDATE
    mSweepPos = 0;
#endif
}

void
BLScanDisplay::AddSamples(int curveNum,
                          const WDL_TypedBuf<BL_FLOAT> &samplesIn,
                          bool advanceSweep)
{
    if (!mIsEnabled)
        return;

    if (curveNum >= mCurves.size())
        return;

    Curve &curve = mCurves[curveNum];
        
    WDL_TypedBuf<BL_GUI_FLOAT> samples;
    BLUtils::ConvertToGUIFloatType(&samples, samplesIn);
    
    curve.mCurrentSamples.Add(samples.Get(), samples.GetSize());
    
    int numSamples = GetNumSamples();
    long oneLineSizeSamples = numSamples/GRAPH_NUM_POINTS;

    long sweepPosSave = mSweepPos;
    while(curve.mCurrentSamples.GetSize() >= oneLineSizeSamples)
    {
        // Buffer management
        WDL_TypedBuf<BL_GUI_FLOAT> bufSamples;
        bufSamples.Add(curve.mCurrentSamples.Get(), oneLineSizeSamples);
        BLUtils::ConsumeLeft(&curve.mCurrentSamples, oneLineSizeSamples);
        
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
        curve.mCurrentDecimValues[0].Add(&decimLineMax, 1);
        BLUtils::ConsumeLeft(&curve.mCurrentDecimValues[0], 1);
        
        if (curve.mIsSampleCurve)
        {
            curve.mCurrentDecimValues[1].Add(&decimLineMin, 1);
            BLUtils::ConsumeLeft(&curve.mCurrentDecimValues[1], 1);
        }
#else
        curve.mCurrentDecimValues[0].Get()[mSweepPos] = decimLineMax;;
        if (curve.mIsSampleCurve)
            curve.mCurrentDecimValues[1].Get()[mSweepPos] = decimLineMin;

        //
        mSweepPos = (mSweepPos + 1) % GRAPH_NUM_POINTS;
    
        AddSamplesZoom(curveNum);
        
        UpdateSweepBar();
    }

    if (!advanceSweep)
        mSweepPos = sweepPosSave;
#endif
}

void
BLScanDisplay::SetDirty()
{
    if (mGraph != NULL)
        mGraph->SetDataChanged();
}

void
BLScanDisplay::SetEnabled(bool flag)
{
#if 0
    if (flag == mIsEnabled)
        return;
    
    if (!flag)
    {
        ResetSweepBar();
    }
#endif
    
    mIsEnabled = flag;
}

void
BLScanDisplay::SetZoom(BL_GUI_FLOAT zoom)
{
    mZoom = zoom;
    
#if GAMMA_ZOOM
    // Smaller zoom value if we use param shape
    mZoom = 1.0 + (mZoom - 1.0)*0.5;
#endif

    for (int i = 0; i < mCurves.size(); i++)
        AddSamplesZoom(i);
    
    if (mGraph != NULL)
        mGraph->SetDataChanged();
}

void
BLScanDisplay::ResetSweepBar()
{
    // Rewind sweep line
    if (mSweepBarCurve != NULL)
        mSweepBarCurve->SetSingleValueV((BL_FLOAT)0.0);
    
    if (mGraph != NULL)
    {
        mGraph->SetDataChanged();
    }
}

void
BLScanDisplay::SetCurveStyle(int curveNum,
                             const char *description, int descrColor[4],
                             bool isSampleCurve, BL_GUI_FLOAT lineWidth,
                             bool fillFlag, int color[4])
{
#define FILL_ORIGIN_Y_OFFSET 0.001
    
    if (curveNum >= mCurves.size())
        return;

    Curve &curve = mCurves[curveNum];
    curve.mIsSampleCurve = isSampleCurve;
    
    int size = 1;
    if (isSampleCurve)
        size = 2;
    for (int i = 0; i < size; i++)
    {
        GraphCurve5 *c = curve.mCurves[i];

        if ((i == 0) && (description != NULL))
            c->SetDescription(description, descrColor);
        
        c->SetAlpha(1.0);
        if (!fillFlag)
            c->SetLineWidth(lineWidth);
        else
            // Do not draw line curve over fill
            c->SetLineWidth(-lineWidth);
        
        c->SetFillAlpha(1.0);
        c->SetFill(fillFlag, 0.5 - FILL_ORIGIN_Y_OFFSET);
        c->SetYScale(Scale::LINEAR, -2.0, 2.0);

        c->SetColor(color[0], color[1], color[2]);
    }
}

void
BLScanDisplay::AddSamplesZoom(int curveNum)
{
    if (curveNum >= mCurves.size())
        return;
    
    // Graph
    if (mGraph != NULL)
    {    
        Curve &curve = mCurves[curveNum];
        
        WDL_TypedBuf<BL_GUI_FLOAT> decimValues[2];
        decimValues[0] = curve.mCurrentDecimValues[0];
        decimValues[1] = curve.mCurrentDecimValues[1];
        
#if !GAMMA_ZOOM
        BLUtils::MultValues(&decimValues[0], mZoom);
        BLUtils::MultValues(&decimValues[1], mZoom);
#else
        BLUtils::ApplyParamShapeWaveform(&decimValues[0], mZoom);
        BLUtils::ApplyParamShapeWaveform(&decimValues[1], mZoom);
#endif
        
        curve.mCurves[0]->SetValues5(decimValues[0]);
        if (curve.mIsSampleCurve)
            curve.mCurves[1]->SetValues5(decimValues[1]);
    }
}

long
BLScanDisplay::GetNumSamples()
{
    long numSamples = mSampleRate*NUM_SECONDS;
    
    return numSamples;
}

// Find min and max (equivalent to decimation to get a single value)
void
BLScanDisplay::DecimateSamplesOneLine(const WDL_TypedBuf<BL_GUI_FLOAT> &bufSamples,
                                      BL_GUI_FLOAT *decimLineMin,
                                      BL_GUI_FLOAT *decimLineMax)
{
    BL_GUI_FLOAT minVal = BLUtils::ComputeMin(bufSamples);
    BL_GUI_FLOAT maxVal = BLUtils::ComputeMax(bufSamples);
    
    *decimLineMin = minVal;
    *decimLineMax = maxVal;
}

void
BLScanDisplay::UpdateSweepBar()
{
    if (!mIsEnabled)
        return;
    
    if (mGraph != NULL)
    {
        BL_GUI_FLOAT pos = ((BL_GUI_FLOAT)mSweepPos)/GRAPH_NUM_POINTS;
        mSweepBarCurve->SetSingleValueV(pos);
    }
}

void
BLScanDisplay::CreateCurves(int numCurves)
{
    mAxisCurve = new GraphCurve5(GRAPH_NUM_POINTS);
    mSweepBarCurve = new GraphCurve5(GRAPH_NUM_POINTS);
    
    mCurves.resize(numCurves);
    for (int i = 0; i < mCurves.size(); i++)
    {
        Curve &curve = mCurves[i];
        
        curve.mCurves[0] = new GraphCurve5(GRAPH_NUM_POINTS);
        curve.mCurves[1] = new GraphCurve5(GRAPH_NUM_POINTS);
    }
}

void
BLScanDisplay::DeleteCurves()
{
    delete mAxisCurve;
    delete mSweepBarCurve;
  
    for (int i = 0; i < mCurves.size(); i++)
    {
        Curve &curve = mCurves[i];
        
        if (curve.mCurves[0] != NULL)
        {
            delete curve.mCurves[0];
            curve.mCurves[0] = NULL;
        }
        
        if (curve.mCurves[1] != NULL)
        {
            delete curve.mCurves[1];
            curve.mCurves[1] = NULL;
        }
    }

    mCurves.resize(0);
}

#endif // IGRAPHICS_NANOVG
