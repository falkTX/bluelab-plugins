//
//  BLVectorscope.cpp
//  UST
//
//  Created by applematuer on 7/27/19.
//
//

#include <math.h>

#include <GraphControl11.h>

#include <BLCircleGraphDrawer.h>
#include <BLLissajousGraphDrawer.h>
#include <BLUpmixGraphDrawer.h>
#include <StereoWidthGraphDrawer3.h>

#include <BLVectorscopeProcess.h>

#include <BLFireworks.h>

#include <SourceComputer.h>

#include <BLUtils.h>

#include "BLVectorscope.h"

#ifndef M_PI
#define M_PI 3.141592653589
#endif

// NOTE: can gain 10% CPU by setting to 1024
// (but the display will be a bit too quick
#define NUM_POINTS 4096

#define NUM_CURVES_GRAPH0 1
#define NUM_CURVES_GRAPH1 1
#define NUM_CURVES_GRAPH2 2
#define NUM_CURVES_GRAPH3 0
#define NUM_CURVES_GRAPH4 1

// Bounds
//

// Graph0
#define GRAPH0_MIN_X -1.0
#define GRAPH0_MAX_X 1.0

#define GRAPH0_MIN_Y 0.0
#define GRAPH0_MAX_Y 1.0

// Graph1
#define GRAPH1_MIN_X -1.0
#define GRAPH1_MAX_X 1.0

#define GRAPH1_MIN_Y 0.0
#define GRAPH1_MAX_Y 1.0


// Graph2
#define GRAPH2_MIN_X -1.0
#define GRAPH2_MAX_X 1.0

#define GRAPH2_MIN_Y -1.0
#define GRAPH2_MAX_Y 1.0

// Graph4
#define GRAPH4_MIN_X -1.0
#define GRAPH4_MAX_X 1.0

#define GRAPH4_MIN_Y 0.0
#define GRAPH4_MAX_Y 1.0

//
#define POINT_SIZE_MODE0 5.0
#define POINT_SIZE_MODE1 2.0
#define LINE_SIZE_MODE2 2.0 //1.0
#define LINE_SIZE2_MODE2 3.0
#define POINT_SIZE_MODE4 5.0

#define POINT_ALPHA 0.25 //1.0

// Curve to display points
#define CURVE_POINTS0 0
#define CURVE_POINTS1 1

#define SQR2_INV 0.70710678118655

// Same as BLVectorscopeProcess::CLIP_DISTANCE
// So when saturate, the saturation line will be exactly on the circle drawer
#define SCALE_POLAR_Y 0.95


//
#define LISSAJOUS_SCALE 0.8

// Crash Protools (mac), when the plugin is just inserted
#define FIX_PROTOOLS_CRASH_AT_INSERT 1


BLVectorscope::BLVectorscope(BLVectorscopePlug *plug,
                             BL_FLOAT sampleRate)
{
    mPlug = plug;
    
    for (int i = 0; i < NUM_MODES; i++)
        mGraphs[i] = NULL;
    
    mFireworks = new BLFireworks(sampleRate);
    
    mSourceComputer = new SourceComputer(sampleRate);
    
    mMode = POLAR_SAMPLE;
}

BLVectorscope::~BLVectorscope()
{
    delete mFireworks;
    
    delete mSourceComputer;
}

void
BLVectorscope::Reset(BL_FLOAT sampleRate)
{
    mFireworks->Reset(sampleRate);
    mSourceComputer->Reset(sampleRate);
}

void
BLVectorscope::Reset()
{
    mFireworks->Reset();
    mSourceComputer->Reset();
}

void
BLVectorscope::SetMode(Mode mode)
{
    mMode = mode;
    
    for (int i = 0; i < NUM_MODES; i++)
    {
        if (mGraphs[i] != NULL)
        {
            bool enabled = (i == mode);
            mGraphs[i]->SetEnabled(enabled);
            
            if (enabled)
            {
                mGraphs[i]->SetInteractionDisabled(false);
                mGraphs[i]->SetDirty(false);//
            }
            else
            {
                mGraphs[i]->SetInteractionDisabled(true);
                mGraphs[i]->SetClean();
            }
            
            // NEW
#if !FIX_PROTOOLS_CRASH_AT_INSERT
            //mGraphs[i]->SetDirty(true);
#else
            //mGraphs[i]->SetDirty(false);
#endif
        }
    }
    
    mFireworks->Reset();
    mSourceComputer->Reset();
}

void
BLVectorscope::OnMouseDown(int x, int y, IMouseMod* pMod)
{
    if (mMode == UPMIX)
    {
        // Upmix graph drawer also manage control
        mUpmixDrawer->OnMouseDown(x, y, pMod);
    }
}

void
BLVectorscope::OnMouseUp(int x, int y, IMouseMod* pMod)
{
    if (mMode == UPMIX)
    {
        // Upmix graph drawer also manage control
        mUpmixDrawer->OnMouseUp(x, y, pMod);
        
        return;
    }
}

void
BLVectorscope::OnMouseDrag(int x, int y, int dX, int dY, IMouseMod* pMod)
{
    if (mMode == UPMIX)
    {
        // Upmix graph drawer also manage control
        mUpmixDrawer->OnMouseDrag(x, y, dX, dY, pMod);
        
        return;
    }
    
//#define COEFF 0.01
//#define FINE_COEFF 0.1
    
#define COEFF 0.005
#define FINE_COEFF 0.2
    
    BL_FLOAT dpan = 0.0;
    BL_FLOAT dwidth = 0.0;
    
    if (!pMod->A)
        dpan = ((BL_FLOAT)dX)*COEFF;
    else
        dwidth = -((BL_FLOAT)dY)*COEFF;
    
    if (pMod->S)
    {
        dpan *= FINE_COEFF;
        dwidth *= FINE_COEFF;
    }
    
    mPlug->VectorscopeUpdateDPanCB(dpan);
    mPlug->VectorscopeUpdateDWidthCB(dwidth);
}

int
BLVectorscope::GetNumCurves(int graphNum)
{
    if (graphNum == POLAR_SAMPLE_MODE_ID)
        return NUM_CURVES_GRAPH0;
    
    if (graphNum == LISSAJOUS_MODE_ID)
        return NUM_CURVES_GRAPH1;
    
    if (graphNum == FIREWORKS_MODE_ID)
        return NUM_CURVES_GRAPH2;
    
    if (graphNum == UPMIX_MODE_ID)
        return NUM_CURVES_GRAPH3;
    
    if (graphNum == SOURCE_MODE_ID)
        return NUM_CURVES_GRAPH4;
    
    return -1;
}

int
BLVectorscope::GetNumPoints(int graphNum)
{
    return NUM_POINTS;
}

void
BLVectorscope::SetGraphs(GraphControl11 *graph0,
                         GraphControl11 *graph1,
                         GraphControl11 *graph2,
                         GraphControl11 *graph3,
                         GraphControl11 *graph4)
{
    mGraphs[POLAR_SAMPLE_MODE_ID] = graph0;
    mGraphs[LISSAJOUS_MODE_ID] = graph1;
    mGraphs[FIREWORKS_MODE_ID] = graph2;
    mGraphs[UPMIX_MODE_ID] = graph3;
    mGraphs[SOURCE_MODE_ID] = graph4;
    
    //
    if (mGraphs[POLAR_SAMPLE_MODE_ID] != NULL)
        mGraphs[POLAR_SAMPLE_MODE_ID]->AddCustomControl(this);
    if (mGraphs[LISSAJOUS_MODE_ID] != NULL)
        mGraphs[LISSAJOUS_MODE_ID]->AddCustomControl(this);
    if (mGraphs[FIREWORKS_MODE_ID] != NULL)
        mGraphs[FIREWORKS_MODE_ID]->AddCustomControl(this);
    if (mGraphs[UPMIX_MODE_ID] != NULL)
        mGraphs[UPMIX_MODE_ID]->AddCustomControl(this);
    if (mGraphs[SOURCE_MODE_ID] != NULL)
        mGraphs[SOURCE_MODE_ID]->AddCustomControl(this);
    
    // Graph0
    if (mGraphs[POLAR_SAMPLE_MODE_ID] != NULL)
    {
        mGraphs[POLAR_SAMPLE_MODE_ID]->SetBounds(0.0, 0.0, 1.0, 1.0);
        mGraphs[POLAR_SAMPLE_MODE_ID]->SetClearColor(0, 0, 0, 255);
    
        mCircleDrawerPolarSamples = new BLCircleGraphDrawer("POLAR SAMPLES");
        mGraphs[POLAR_SAMPLE_MODE_ID]->AddCustomDrawer(mCircleDrawerPolarSamples);
    
        // Style
        //
        int pointColor[3] = { 113, 130, 182 };
        //int pointColor[3] = { 153, 176, 246 };
        
        // To display lines instead of points
        bool pointsAsLines = false;
        BL_FLOAT alpha = POINT_ALPHA;
        bool overlay = true;
        
        bool bevelFlag = false;
        
        SetCurveStyle(mGraphs[POLAR_SAMPLE_MODE_ID],
                      CURVE_POINTS0,
                      GRAPH0_MIN_X, GRAPH0_MAX_X,
                      GRAPH0_MIN_Y, GRAPH0_MAX_Y,
                      true, POINT_SIZE_MODE0,
                      bevelFlag,
                      pointColor[0], pointColor[1], pointColor[2],
                      false, alpha, pointsAsLines, overlay, false);
        
        mGraphs[POLAR_SAMPLE_MODE_ID]->SetDisablePointOffsetHack(true);
    }
    
    // Graph for Lissajouss
    if (mGraphs[LISSAJOUS_MODE_ID] != NULL)
    {
        mGraphs[LISSAJOUS_MODE_ID]->SetBounds(0.0, 0.0, 1.0, 1.0);
        mGraphs[LISSAJOUS_MODE_ID]->SetClearColor(0, 0, 0, 255);
        
        mLissajousDrawer = new BLLissajousGraphDrawer(LISSAJOUS_SCALE, "LISSAJOUS");
        mGraphs[LISSAJOUS_MODE_ID]->AddCustomDrawer(mLissajousDrawer);
        
        // Style
        //
        int pointColor[3] = { 113, 130, 182 };
        //int pointColor[3] = { 153, 176, 246 };
        
        // To display lines instead of points
        bool pointsAsLines = false;
        BL_FLOAT alpha = POINT_ALPHA;
        bool overlay = true;
        bool bevelFlag = false;
        
        SetCurveStyle(mGraphs[LISSAJOUS_MODE_ID],
                      CURVE_POINTS0,
                      GRAPH2_MIN_X, GRAPH2_MAX_X,
                      GRAPH2_MIN_Y, GRAPH2_MAX_Y,
                      true, POINT_SIZE_MODE1,
                      bevelFlag,
                      pointColor[0], pointColor[1], pointColor[2],
                      false, alpha, pointsAsLines, overlay, false);
        
        mGraphs[LISSAJOUS_MODE_ID]->SetDisablePointOffsetHack(true);
    }
    
    // Graph for fireworks
    if (mGraphs[FIREWORKS_MODE_ID] != NULL)
    {
        mGraphs[FIREWORKS_MODE_ID]->SetBounds(0.0, 0.0, 1.0, 1.0);
        mGraphs[FIREWORKS_MODE_ID]->SetClearColor(0, 0, 0, 255);
        
        mCircleDrawerFireworks = new BLCircleGraphDrawer("FLAME");
        mGraphs[FIREWORKS_MODE_ID]->AddCustomDrawer(mCircleDrawerFireworks);
        
        // Style
        //
        //int pointColor0[3] = { 113, 130, 182 };
        int pointColor0[3] = { 153, 176, 246 };
        int pointColor1[3] = { 255, 255, 255 };
        
        // To display lines instead of points
        bool pointsAsLines = true;
        BL_FLOAT alpha = 1.0;
        bool overlayFlag = false;
        
        bool fillFlag0 = false;
        bool linesPolarFlag0 = true;
        bool linesPolarFlag1 = false;
        bool bevelFlag0 = false;
        
        SetCurveStyle(mGraphs[FIREWORKS_MODE_ID],
                      CURVE_POINTS0,
                      GRAPH1_MIN_X, GRAPH1_MAX_X,
                      GRAPH1_MIN_Y, GRAPH1_MAX_Y,
                      true/*false*/, LINE_SIZE_MODE2,
                      bevelFlag0,
                      pointColor0[0], pointColor0[1], pointColor0[2],
                      fillFlag0, alpha, pointsAsLines, overlayFlag, linesPolarFlag0);
        
        bool fillFlag1 = false;
        bool bevelFlag1 = true;
        SetCurveStyle(mGraphs[FIREWORKS_MODE_ID],
                      CURVE_POINTS1,
                      GRAPH1_MIN_X, GRAPH1_MAX_X,
                      GRAPH1_MIN_Y, GRAPH1_MAX_Y,
                      true/*false*/, LINE_SIZE2_MODE2,
                      bevelFlag1,
                      pointColor1[0], pointColor1[1], pointColor1[2],
                      fillFlag1, alpha, pointsAsLines, overlayFlag, linesPolarFlag1);
        
        mGraphs[FIREWORKS_MODE_ID]->SetDisablePointOffsetHack(true);
    }
    
    // Graph for upmix
    if (mGraphs[UPMIX_MODE_ID] != NULL)
    {
        mGraphs[UPMIX_MODE_ID]->SetBounds(0.0, 0.0, 1.0, 1.0);
        mGraphs[UPMIX_MODE_ID]->SetClearColor(0, 0, 0, 255);
        
        mUpmixDrawer = new BLUpmixGraphDrawer(mPlug, mGraphs[UPMIX_MODE_ID], "GRID");
        mGraphs[UPMIX_MODE_ID]->AddCustomDrawer(mUpmixDrawer);
        
        // Style
        //
        int pointColor0[3] = { 193, 229, 237 };
        //int pointColor1[3] = { 113, 130, 182 };
        int pointColor1[3] = { 153, 176, 246 };
        
        // To display lines instead of points
        bool pointsAsLines = false;
        BL_FLOAT alpha = 1.0;
        bool overlayFlag = false;
    
        bool fillFlag0 = false;
        bool linesPolarFlag0 = true;
        bool linesPolarFlag1 = false;
        bool bevelFlag0 = false;
        
        SetCurveStyle(mGraphs[UPMIX_MODE_ID],
                      CURVE_POINTS0,
                      GRAPH1_MIN_X, GRAPH1_MAX_X,
                      GRAPH1_MIN_Y, GRAPH1_MAX_Y,
                      true/*false*/, LINE_SIZE_MODE2,
                      bevelFlag0,
                      pointColor0[0], pointColor0[1], pointColor0[2],
                      fillFlag0, alpha, pointsAsLines, overlayFlag, linesPolarFlag0);
        
        bool fillFlag1 = false;
        bool bevelFlag1 = true;
        SetCurveStyle(mGraphs[UPMIX_MODE_ID],
                      CURVE_POINTS1,
                      GRAPH1_MIN_X, GRAPH1_MAX_X,
                      GRAPH1_MIN_Y, GRAPH1_MAX_Y,
                      true/*false*/, LINE_SIZE2_MODE2,
                      bevelFlag1,
                      pointColor1[0], pointColor1[1], pointColor1[2],
                      fillFlag1, alpha, pointsAsLines, overlayFlag, linesPolarFlag1);
        
        mGraphs[UPMIX_MODE_ID]->SetDisablePointOffsetHack(true);
    }
    
    // Graph4
    if (mGraphs[SOURCE_MODE_ID] != NULL)
    {
        mGraphs[SOURCE_MODE_ID]->SetBounds(0.0, 0.0, 1.0, 1.0);
        mGraphs[SOURCE_MODE_ID]->SetClearColor(0, 0, 0, 255);
        
        mCircleDrawerSource = new StereoWidthGraphDrawer3("SOURCE");
        
        mGraphs[SOURCE_MODE_ID]->AddCustomDrawer(mCircleDrawerSource);
        
        // Style
        //
        int pointColor[3] = { 113, 130, 182 };
        //int pointColor[3] = { 153, 176, 246 };
        
        // To display lines instead of points
        bool pointsAsLines = false;
        BL_FLOAT alpha = POINT_ALPHA;
        bool overlay = true;
        
        bool bevelFlag = false;
        
        SetCurveStyle(mGraphs[SOURCE_MODE_ID],
                      CURVE_POINTS0,
                      GRAPH4_MIN_X, GRAPH4_MAX_X,
                      GRAPH4_MIN_Y, GRAPH4_MAX_Y,
                      true, POINT_SIZE_MODE4,
                      bevelFlag,
                      pointColor[0], pointColor[1], pointColor[2],
                      false, alpha, pointsAsLines, overlay, false);
        
        mGraphs[SOURCE_MODE_ID]->SetDisablePointOffsetHack(true);
    }
    
    SetMode(mMode); // TEST
}

void
BLVectorscope::AddSamples(vector<WDL_TypedBuf<BL_FLOAT> > samples)
{
    if (samples.size() != 2)
        return;
    
    for (int i = 0; i < 2; i++)
    {
        mSamples[i].Add(samples[i].Get(), samples[i].GetSize());
        
        int numToConsume = mSamples[i].GetSize() - NUM_POINTS;
        if (numToConsume > 0)
        {
            BLUtils::ConsumeLeft(&mSamples[i], numToConsume);
        }
    }
    
    if (mMode == POLAR_SAMPLE)
    {
        if (mGraphs[POLAR_SAMPLE_MODE_ID] != NULL)
        {
            WDL_TypedBuf<BL_FLOAT> polarSamples[2];
            BLVectorscopeProcess::ComputePolarSamples(mSamples, polarSamples);
            
            // Adjust to the circle graph drawer
            BLUtils::MultValues(&polarSamples[1], (BL_FLOAT)SCALE_POLAR_Y);
            
            mGraphs[POLAR_SAMPLE_MODE_ID]->SetCurveValuesPoint(CURVE_POINTS0, polarSamples[0], polarSamples[1]);
        }
    }
    
    if (mMode == LISSAJOUS)
    {
        if (mGraphs[LISSAJOUS_MODE_ID] != NULL)
        {
            WDL_TypedBuf<BL_FLOAT> lissajousSamples[2];
            BLVectorscopeProcess::ComputeLissajous(mSamples, lissajousSamples, true);
            
            // Scale so that we stay in the square (even in diagonal)
            BLUtils::MultValues(&lissajousSamples[0], (BL_FLOAT)LISSAJOUS_SCALE);
            BLUtils::MultValues(&lissajousSamples[1], (BL_FLOAT)LISSAJOUS_SCALE);
            
            mGraphs[LISSAJOUS_MODE_ID]->SetCurveValuesPointEx(CURVE_POINTS0,
                                                              lissajousSamples[0],
                                                              lissajousSamples[1],
                                                              true, false, true);
        }
    }
    
    if (mMode == FIREWORKS)
    {
        if (mGraphs[FIREWORKS_MODE_ID] != NULL)
        {
            WDL_TypedBuf<BL_FLOAT> samplesIn[2] = { mSamples[0], mSamples[1] };
            
            WDL_TypedBuf<BL_FLOAT> polarSamples[2];
            WDL_TypedBuf<BL_FLOAT> polarSamplesMax[2];
            mFireworks->ComputePoints(samplesIn, polarSamples, polarSamplesMax);
            
            // Adjust to the circle graph drawer
            BLUtils::MultValues(&polarSamples[1], (BL_FLOAT)SCALE_POLAR_Y);
            BLUtils::MultValues(&polarSamplesMax[1], (BL_FLOAT)SCALE_POLAR_Y);
            
            mGraphs[FIREWORKS_MODE_ID]->SetCurveValuesPoint(CURVE_POINTS0,
                                            polarSamples[0], polarSamples[1]);
            
            mGraphs[FIREWORKS_MODE_ID]->SetCurveValuesPoint(CURVE_POINTS1,
                                            polarSamplesMax[0], polarSamplesMax[1]);
        }
    }
    
    if (mMode == UPMIX)
    {
        // Nothing to do
    }
    
    if (mMode == SOURCE)
    {
        if (mGraphs[SOURCE_MODE_ID] != NULL)
        {
            WDL_TypedBuf<BL_FLOAT> samplesIn[2] = { mSamples[0], mSamples[1] };
            
            WDL_TypedBuf<BL_FLOAT> polarSamples[2];
            mSourceComputer->ComputePoints(samplesIn, polarSamples);
            
            // Adjust to the circle graph drawer
            BLUtils::MultValues(&polarSamples[1], (BL_FLOAT)SCALE_POLAR_Y);
            
            mGraphs[SOURCE_MODE_ID]->SetCurveValuesPoint(CURVE_POINTS0,
                                                         polarSamples[0], polarSamples[1]);
        }
    }
}

BLUpmixGraphDrawer *
BLVectorscope::GetUpmixGraphDrawer()
{
    return mUpmixDrawer;
}

void
BLVectorscope::SetCurveStyle(GraphControl11 *graph,
                               int curveNum,
                               BL_FLOAT minX, BL_FLOAT maxX,
                               BL_FLOAT minY, BL_FLOAT maxY,
                               bool pointFlag,
                               BL_FLOAT strokeSize,
                               bool bevelFlag,
                               int r, int g, int b,
                               bool curveFill, BL_FLOAT curveFillAlpha,
                               bool pointsAsLines,
                               bool pointOverlay, bool linesPolarFlag)
{
    if (graph == NULL)
        return;
    
    graph->SetCurveXScale(curveNum, false, minX, maxX);
    graph->SetCurveYScale(curveNum, false, minY, maxY);
    
    if (!pointFlag)
        graph->SetCurveLineWidth(curveNum, strokeSize);
    
    //
    if (pointFlag)
    {
        graph->SetCurvePointSize(curveNum, strokeSize);
        graph->SetCurvePointStyle(curveNum, true, linesPolarFlag, pointsAsLines);
        graph->SetCurvePointOverlay(curveNum, pointOverlay);
    }
    
    graph->SetCurveBevel(curveNum, bevelFlag);
                          
    graph->SetCurveColor(curveNum, r, g, b);
    
    if (curveFill)
    {
        graph->SetCurveFill(curveNum, curveFill);
    }
            
    graph->SetCurveFillAlpha(curveNum, curveFillAlpha);
}
