//
//  GraphFreqAxis2.cpp
//  BL-InfrasonicViewer
//
//  Created by applematuer on 11/22/19.
//
//

//#include <BLDefs.h>

#include <GraphAxis2.h>
#include <GUIHelper12.h>
#include <BLUtils.h>
//#include <Scale.h>

#include "GraphFreqAxis2.h"

#ifdef IGRAPHICS_NANOVG

GraphFreqAxis2::GraphFreqAxis2(bool displayLines,
                               Scale::Type scale)
{
    mScale = scale;
    
    mGraphAxis = NULL;
    
    mBufferSize = 2048;
    mSampleRate = 44100.0;
    
    mDisplayLines = displayLines;
}

GraphFreqAxis2::~GraphFreqAxis2() {}

void
GraphFreqAxis2::Init(GraphAxis2 *graphAxis, GUIHelper12 *guiHelper,
                     bool horizontal,
                     int bufferSize, BL_FLOAT sampleRate,
                     int graphWidth)
{
    mGraphAxis = graphAxis;
    
    mBufferSize = bufferSize;
    mSampleRate = sampleRate;
    
    //
    int axisColor[4];
    guiHelper->GetGraphAxisColor(axisColor);
    
    int axisLabelColor[4];
    guiHelper->GetGraphAxisLabelColor(axisLabelColor);
    
    int axisLabelOverlayColor[4];
    guiHelper->GetGraphAxisLabelOverlayColor(axisLabelOverlayColor);
    
    BL_GUI_FLOAT lineWidth = guiHelper->GetGraphAxisLineWidth();
    
    if (!mDisplayLines)
    {
        axisColor[3] = 0;
    }
    
    //
    if (horizontal)
    {
#if 0
#if !USE_DEFAULT_SCALE_MEL
        mGraphAxis->InitHAxis(Scale::LOG_FACTOR,
                              0.0, sampleRate*0.5,
                              axisColor, axisLabelColor,
                              lineWidth,
                              0.0,
                              axisLabelOverlayColor);
#else
        mGraphAxis->InitHAxis(Scale::MEL,
                              0.0, sampleRate*0.5,
                              axisColor, axisLabelColor,
                              lineWidth,
                              0.0,
                              axisLabelOverlayColor);
#endif
#endif
        
        mGraphAxis->InitHAxis(mScale,
                              0.0, sampleRate*0.5,
                              axisColor, axisLabelColor,
                              lineWidth,
                              0.0,
                              axisLabelOverlayColor);
    }
    else
    {
#if 0
#if !USE_DEFAULT_SCALE_MEL
        mGraphAxis->InitVAxis(Scale::LOG_FACTOR,
                              0.0, sampleRate*0.5,
                              axisColor, axisLabelColor,
                              lineWidth,
                              //0.0, graphWidth - 40.0,
                              graphWidth - 40.0, 0.0,
                              axisLabelOverlayColor);
#else
        mGraphAxis->InitVAxis(Scale::MEL,
                              0.0, sampleRate*0.5,
                              axisColor, axisLabelColor,
                              lineWidth,
                              //0.0, graphWidth - 40.0,
                              graphWidth - 40.0, 0.0,
                              axisLabelOverlayColor);
#endif
#endif
        
        mGraphAxis->InitVAxis(mScale,
                              0.0, sampleRate*0.5,
                              axisColor, axisLabelColor,
                              lineWidth,
                              //0.0, graphWidth - 40.0,
                              //graphWidth - 40.0, 0.0,
                              1.0 - 40.0/graphWidth, 0.0,
                              axisLabelOverlayColor);
    }
    
    //
    Update();
}

void
GraphFreqAxis2::SetBounds(BL_FLOAT bounds[2])
{
    mGraphAxis->SetBounds(bounds);
}

void
GraphFreqAxis2::Reset(int bufferSize, BL_FLOAT sampleRate)
{
    mBufferSize = bufferSize;
    mSampleRate = sampleRate;
    
    mGraphAxis->SetMinMaxValues(0.0, sampleRate*0.5);
    
    Update();
}

void
GraphFreqAxis2::Update()
{
    // Just in case
    if (mGraphAxis == NULL)
        return;
    
#define NUM_AXIS_DATA 12
    char *AXIS_DATA [NUM_AXIS_DATA][2];
    for (int i = 0; i < NUM_AXIS_DATA; i++)
    {
        AXIS_DATA[i][0] = (char *)malloc(255);
        AXIS_DATA[i][1] = (char *)malloc(255);
    }
    
    sprintf(AXIS_DATA[0][1], "");
    sprintf(AXIS_DATA[1][1], ""); // Do not display 50Hz for Scale::LOG_FACTOR2
    sprintf(AXIS_DATA[2][1], "100Hz");
    sprintf(AXIS_DATA[3][1], "500Hz");
    sprintf(AXIS_DATA[4][1], "1KHz");
    sprintf(AXIS_DATA[5][1], "2KHz");
    sprintf(AXIS_DATA[6][1], "5KHz");
    sprintf(AXIS_DATA[7][1], "10KHz");
    sprintf(AXIS_DATA[8][1], "20KHz");
    sprintf(AXIS_DATA[9][1], "40KHz");
    sprintf(AXIS_DATA[10][1], "80KHz");
    sprintf(AXIS_DATA[11][1], "");
    
    BL_FLOAT freqs[NUM_AXIS_DATA] =
                    { 25.0, 50.0, 100.0, 500.0, 1000.0, 2000.0, 5000.0,
                      10000.0, 20000.0, 40000.0, 80000.0, 176400.0 };

    BL_FLOAT minHzValue;
    BL_FLOAT maxHzValue;
    BLUtils::GetMinMaxFreqAxisValues(&minHzValue, &maxHzValue,
                                     mBufferSize, mSampleRate);
    
    // Avoid a shift
    minHzValue = 0.0;
    
    // Normalize
    for (int i = 0; i < NUM_AXIS_DATA; i++)
    {
        sprintf(AXIS_DATA[i][0], "%g", freqs[i]);
        
        // We are over the sample rate, make empty label
        if ((freqs[i] < minHzValue) || (freqs[i] > maxHzValue))
            sprintf(AXIS_DATA[i][1], "");
    }
    
    mGraphAxis->SetData(AXIS_DATA, NUM_AXIS_DATA);
    
    for (int i = 0; i < NUM_AXIS_DATA; i++)
    {
        free(AXIS_DATA[i][0]);
        free(AXIS_DATA[i][1]);
    }
}

#endif

