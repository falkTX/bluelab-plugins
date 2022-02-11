//
//  USTVectorscope.cpp
//  UST
//
//  Created by applematuer on 7/27/19.
//
//

#include <math.h>

#include <GraphControl11.h>

#include <USTCircleGraphDrawer.h>
#include <USTProcess.h>

#include <BLUtils.h>

#include "USTVectorscope.h"

#ifndef M_PI
#define M_PI 3.141592653589
#endif

#define NUM_CURVES 1
#define NUM_POINTS 4096 //1024 //256

// Bounds
#define GRAPH_FADER_MIN_X -1.0
#define GRAPH_FADER_MAX_X 1.0

#define GRAPH_FADER_MIN_Y 0.0
#define GRAPH_FADER_MAX_Y 1.0

#define POINT_SIZE 5.0

#define ORANGE_COLOR_SCHEME 1


USTVectorscope::USTVectorscope()
{
    mIsEnabled = false;
    
    mGraph = NULL;
}

USTVectorscope::~USTVectorscope() {}

void
USTVectorscope::SetEnabled(bool flag)
{
    mIsEnabled = flag;
    
    if (mGraph != NULL)
        mGraph->SetEnabled(flag);
}

void
USTVectorscope::Reset() {}

void
USTVectorscope::SetMode(Mode mode)
{
    // TODO
    mMode = mode;
}

int
USTVectorscope::GetNumCurves()
{
    return NUM_CURVES;
}

int
USTVectorscope::GetNumPoints()
{
    return NUM_POINTS;
}

void
USTVectorscope::SetGraph(GraphControl11 *graph)
{
    mGraph = graph;
    if (mGraph == NULL)
        return;
    
    mGraph->SetBounds(0.0, 0.0, 1.0, 1.0);
    //mGraph->SetClearColor(255, 0, 255, 255); // DEBUG
    mGraph->SetClearColor(0, 0, 0, 255);
    
    mCircleDrawer = new USTCircleGraphDrawer();
    mGraph->AddCustomDrawer(mCircleDrawer);
    
    // Style
    //
#if !ORANGE_COLOR_SCHEME
    int pointColor[3] = { 64, 64, 255 };
#else
    int pointColor[3] = { 232, 110, 36 };
#endif
    
    // To display lines instead of points
    bool pointsAsLines = false;
    BL_FLOAT alpha = 0.25;
    
    SetPointStyle(GRAPH_FADER_MIN_X, GRAPH_FADER_MAX_X,
                  GRAPH_FADER_MIN_Y, GRAPH_FADER_MAX_Y,
                  POINT_SIZE, //0.5, //1.0, // point size
                  pointColor[0], pointColor[1], pointColor[2],
                  false, alpha, pointsAsLines);
}

void
USTVectorscope::AddSamples(vector<WDL_TypedBuf<BL_FLOAT> > samples)
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
    
    WDL_TypedBuf<BL_FLOAT> polarSamples[2];
    USTProcess::ComputePolarSamples(mSamples, polarSamples);
    
    if (mGraph != NULL)
        mGraph->SetCurveValuesPoint(0, polarSamples[0], polarSamples[1]);
}

void
USTVectorscope::SetPointStyle(BL_FLOAT minX, BL_FLOAT maxX,
                              BL_FLOAT minY, BL_FLOAT maxY,
                              BL_FLOAT pointSize,
                              int r, int g, int b,
                              bool curveFill, BL_FLOAT curveFillAlpha,
                              bool pointsAsLines)
{
    if (mGraph == NULL)
        return;
    
    mGraph->SetCurveXScale(0, false, minX, maxX);
    mGraph->SetCurveYScale(0, false, minY, maxY);
    mGraph->SetCurvePointSize(0, pointSize);
        
    //
    mGraph->SetCurvePointStyle(0, true, pointsAsLines);
        
    mGraph->SetCurveColor(0, r, g, b);
            
    if (curveFill)
    {
        mGraph->SetCurveFill(0, curveFill);
    }
            
    mGraph->SetCurveFillAlpha(0, curveFillAlpha);
}
