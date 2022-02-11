//
//  USTVectorscope3.cpp
//  UST
//
//  Created by applematuer on 7/27/19.
//
//

#include <math.h>

#include <GraphControl11.h>

#include <USTCircleGraphDrawer.h>
#include <USTLissajousGraphDrawer.h>

#include <USTProcess.h>

#include <USTPolarLevel.h>

#include <BLUtils.h>

#include "USTVectorscope3.h"

#ifndef M_PI
#define M_PI 3.141592653589
#endif

#define NUM_POINTS 4096 //1024 //256

#define NUM_CURVES_GRAPH0 1
#define NUM_CURVES_GRAPH1 2
#define NUM_CURVES_GRAPH2 1

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


//#define POINT_SIZE 2.0 //1.0 //5.0
#define POINT_SIZE_MODE0 5.0
#define LINE_SIZE_MODE1 1.0
#define LINE_SIZE2_MODE1 3.0
#define POINT_SIZE_MODE2 2.0

#define POINT_ALPHA 0.25 //1.0 //0.25

#define ORANGE_COLOR_SCHEME 1

// Curve to display points
#define CURVE_POINTS0 0
#define CURVE_POINTS1 1


#define LISSAJOUS_SCALE 0.8

#define SQR2_INV 0.70710678118655


USTVectorscope3::USTVectorscope3(BL_FLOAT sampleRate)
{
    mIsEnabled = false;
    
    for (int i = 0; i < 3; i++)
        mGraphs[i] = NULL;
    
    mPolarLevel = new USTPolarLevel(sampleRate);
}

USTVectorscope3::~USTVectorscope3()
{
    delete mPolarLevel;
}

void
USTVectorscope3::SetEnabled(bool flag)
{
    mIsEnabled = flag;
    
    for (int i = 0; i < 3; i++)
    {
        if (mGraphs[i] != NULL)
        {
            bool enabled = false;
            
            if (flag && i == mMode)
                enabled = true;
            
            mGraphs[i]->SetEnabled(enabled);
        }
    }
    
    mPolarLevel->Reset();
}

void
USTVectorscope3::Reset(BL_FLOAT sampleRate)
{
    mPolarLevel->Reset(sampleRate);
}

void
USTVectorscope3::SetMode(Mode mode)
{
    // TODO
    mMode = mode;
    
    for (int i = 0; i < 3; i++)
    {
        if (mGraphs[i] != NULL)
        {
            bool enabled = (i == mode);
            mGraphs[i]->SetEnabled(enabled);
        }
    }
    
    mPolarLevel->Reset();
}

int
USTVectorscope3::GetNumCurves(int graphNum)
{
    if (graphNum == 0)
        return NUM_CURVES_GRAPH0;
    
    if (graphNum == 1)
        return NUM_CURVES_GRAPH1;
    
    if (graphNum == 2)
        return NUM_CURVES_GRAPH2;
    
    return -1;
}

int
USTVectorscope3::GetNumPoints(int graphNum)
{
    return NUM_POINTS;
}

void
USTVectorscope3::SetGraphs(GraphControl11 *graph0,
                           GraphControl11 *graph1,
                           GraphControl11 *graph2)
{
    mGraphs[0] = graph0;
    mGraphs[1] = graph1;
    mGraphs[2] = graph2;
    
    // Graph0
    if (mGraphs[0] != NULL)
    {
        mGraphs[0]->SetBounds(0.0, 0.0, 1.0, 1.0);
        //mGraphs[0]->SetClearColor(255, 0, 255, 255); // DEBUG
        mGraphs[0]->SetClearColor(0, 0, 0, 255);
    
        mCircleDrawer0 = new USTCircleGraphDrawer();
        mGraphs[0]->AddCustomDrawer(mCircleDrawer0);
    
        // Style
        //
#if !ORANGE_COLOR_SCHEME
        int pointColor[3] = { 64, 64, 255 };
#else
        int pointColor[3] = { 232, 110, 36 };
#endif
    
        // To display lines instead of points
        bool pointsAsLines = false;
        BL_FLOAT alpha = POINT_ALPHA; //0.25;
        bool overlay = true;
        
        bool bevelFlag = false;
        
        SetCurveStyle(mGraphs[0],
                      CURVE_POINTS0,
                      GRAPH0_MIN_X, GRAPH0_MAX_X,
                      GRAPH0_MIN_Y, GRAPH0_MAX_Y,
                      true, POINT_SIZE_MODE0, //0.5, //1.0, // point size
                      bevelFlag,
                      pointColor[0], pointColor[1], pointColor[2],
                      false, alpha, pointsAsLines, overlay, false);
        
        mGraphs[0]->SetDisablePointOffsetHack(true);
    }
    
    // Graph1
    if (mGraphs[1] != NULL)
    {
        mGraphs[1]->SetBounds(0.0, 0.0, 1.0, 1.0);
        //mGraphs[1]->SetClearColor(255, 0, 255, 255); // DEBUG
        mGraphs[1]->SetClearColor(0, 0, 0, 255);
        
        mCircleDrawer1 = new USTCircleGraphDrawer();
        mGraphs[1]->AddCustomDrawer(mCircleDrawer1);
        
        // Style
        //
#if !ORANGE_COLOR_SCHEME
        int pointColor[3] = { 64, 64, 255 };
#else
        int pointColor[3] = { 232, 110, 36 };
#endif
        
        // To display lines instead of points
        bool pointsAsLines = true; //false;
        BL_FLOAT alpha = 1.0; //POINT_ALPHA; //0.25;
        bool overlayFlag = false; //true;
        
#if 0
        bool fillFlag0 = true; //false; //true; //GraphControl11 Fill is buggy
        bool linesPolarFlag = true;
#endif
        
#if 1
        bool fillFlag0 = false;
        bool linesPolarFlag = false;
        bool bevelFlag0 = false;
#endif
        
        SetCurveStyle(mGraphs[1],
                      CURVE_POINTS0,
                      GRAPH1_MIN_X, GRAPH1_MAX_X,
                      GRAPH1_MIN_Y, GRAPH1_MAX_Y,
                      true/*false*/, LINE_SIZE_MODE1, //0.5, //1.0, // point size
                      bevelFlag0,
                      pointColor[0], pointColor[1], pointColor[2],
                      fillFlag0, alpha, pointsAsLines, overlayFlag, linesPolarFlag);
        
        bool fillFlag1 = false;
        bool bevelFlag1 = true;
        SetCurveStyle(mGraphs[1],
                      CURVE_POINTS1,
                      GRAPH1_MIN_X, GRAPH1_MAX_X,
                      GRAPH1_MIN_Y, GRAPH1_MAX_Y,
                      true/*false*/, LINE_SIZE2_MODE1, //0.5, //1.0, // point size
                      bevelFlag1,
                      pointColor[0], pointColor[1], pointColor[2],
                      fillFlag1, alpha, pointsAsLines, overlayFlag, false);
        
        mGraphs[1]->SetDisablePointOffsetHack(true);
    }
    
    // Graph2
    if (mGraphs[2] != NULL)
    {
        mGraphs[2]->SetBounds(0.0, 0.0, 1.0, 1.0);
        //mGraphs[1]->SetClearColor(255, 0, 255, 255); // DEBUG
        mGraphs[2]->SetClearColor(0, 0, 0, 255);
        
        mLissajousDrawer = new USTLissajousGraphDrawer(LISSAJOUS_SCALE);
        mGraphs[2]->AddCustomDrawer(mLissajousDrawer);
        
        // Style
        //
#if !ORANGE_COLOR_SCHEME
        int pointColor[3] = { 64, 64, 255 };
#else
        int pointColor[3] = { 232, 110, 36 };
#endif
        
        // To display lines instead of points
        bool pointsAsLines = false;
        BL_FLOAT alpha = POINT_ALPHA; //0.25;
        bool overlay = true;
        bool bevelFlag = false;
        
        SetCurveStyle(mGraphs[2],
                      CURVE_POINTS0,
                      GRAPH2_MIN_X, GRAPH2_MAX_X,
                      GRAPH2_MIN_Y, GRAPH2_MAX_Y,
                      true, POINT_SIZE_MODE2, //0.5, //1.0, // point size
                      bevelFlag,
                      pointColor[0], pointColor[1], pointColor[2],
                      false, alpha, pointsAsLines, overlay, false);
        
        mGraphs[2]->SetDisablePointOffsetHack(true);
    }
}

void
USTVectorscope3::AddSamples(vector<WDL_TypedBuf<BL_FLOAT> > samples)
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
        if (mGraphs[0] != NULL)
        {
            WDL_TypedBuf<BL_FLOAT> polarSamples[2];
            USTProcess::ComputePolarSamples(mSamples, polarSamples);
            
            mGraphs[0]->SetCurveValuesPoint(CURVE_POINTS0, polarSamples[0], polarSamples[1]);
        }
    }
    
    if (mMode == POLAR_LEVEL)
    {
        if (mGraphs[1] != NULL)
        {
            WDL_TypedBuf<BL_FLOAT> samplesIn[2] = { mSamples[0], mSamples[1] };
            
            WDL_TypedBuf<BL_FLOAT> polarSamples[2];
            WDL_TypedBuf<BL_FLOAT> polarSamplesMax[2];
            
            mPolarLevel->ComputePoints(samplesIn, polarSamples, polarSamplesMax);
            
            
            mGraphs[1]->SetCurveValuesPoint(CURVE_POINTS0,
                                            polarSamples[0], polarSamples[1]);
            
            mGraphs[1]->SetCurveValuesPoint(CURVE_POINTS1,
                                            polarSamplesMax[0], polarSamplesMax[1]);
        }
    }
    
    if (mMode == LISSAJOUS)
    {
        if (mGraphs[2] != NULL)
        {
            WDL_TypedBuf<BL_FLOAT> lissajousSamples[2];
            USTProcess::ComputeLissajous(mSamples, lissajousSamples, true);
            
            // Scale so that we stay in the square (even in diagonal)
            //BLUtils::MultValues(&lissajousSamples[0], SQR2_INV);
            //BLUtils::MultValues(&lissajousSamples[1], SQR2_INV);
            
            BLUtils::MultValues(&lissajousSamples[0], LISSAJOUS_SCALE);
            BLUtils::MultValues(&lissajousSamples[1], LISSAJOUS_SCALE);
            
#if 1
            mGraphs[2]->SetCurveValuesPointEx(CURVE_POINTS0,
                                              lissajousSamples[0],
                                              lissajousSamples[1],
                                              true, false, true);
#endif
            
#if 0
            mGraphs[2]->SetCurveValuesPoint(CURVE_POINTS,
                                            lissajousSamples[0],
                                            lissajousSamples[1]);
#endif
        }
    }
}

void
USTVectorscope3::SetCurveStyle(GraphControl11 *graph,
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
