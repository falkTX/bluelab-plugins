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

// From 0Hz to sampleRate/2 Hz
#define NUM_AXIS_DATA_FULL 12
const char *labelsFull[NUM_AXIS_DATA_FULL] =
{
    "", "", "100Hz", "500Hz", "1KHz", "2KHz", "5KHz",
    "10KHz", "20KHz", "40KHz", "80KHz", ""
};
const BL_FLOAT freqsFull[NUM_AXIS_DATA_FULL] =
{
    25.0, 50.0, 100.0, 500.0, 1000.0, 2000.0, 5000.0,
    10000.0, 20000.0, 40000.0, 80000.0, 176400.0
};

// Use very close lines (to see well log scale), and add just some labels
// From 0Hz to sampleRate/2 Hz
#define NUM_AXIS_DATA_FULL2 39 //48
const char *labelsFull2[NUM_AXIS_DATA_FULL2] =
{
    //"", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "",
    "100Hz", "", "", "", "500Hz", "", "", "", "",
    "1KHz", "2KHz", "", "", "5KHz", "", "", "", "",
    "10KHz", "20KHz", "", "40KHz", "", "", "", "80KHz", "",
    "100Khz", "200KHz", ""
};
const BL_FLOAT freqsFull2[NUM_AXIS_DATA_FULL2] =
{
    //1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0,
    10.0, 20.0, 30.0, 40.0, 50.0, 60.0, 70.0, 80.0, 90.0,
    100.0, 200.0, 300.0, 400.0, 500.0, 600.0, 700.0, 800.0, 900.0,
    1000.0, 2000.0, 3000.0, 4000.0, 5000.0, 6000.0, 7000.0, 8000.0, 9000.0,
    10000.0, 20000.0, 30000.0, 40000.0, 50000.0, 60000.0, 70000.0, 80000.0, 90000.0,
    100000.0, 200000.0, 300000.0
};

// Log scale
//

// From 1Hz to 100Hz
#define NUM_AXIS_DATA_LOG100 13
const char *labelsLog100[NUM_AXIS_DATA_LOG100] =
{
    "", "1Hz", "2Hz", "4Hz", "6Hz", "8Hz", "10Hz",
    "20Hz", "40Hz", "60Hz", "80Hz", "100Hz", ""
};
const BL_FLOAT freqsLog100[NUM_AXIS_DATA_LOG100] =
{
    0.0, 1.0, 2.0, 4.0, 6.0, 8.0, 10.0, 20.0, 40.0,
    60.0, 80.0, 100.0, 120.0
};

// From 1Hz to 50Hz
#define NUM_AXIS_DATA_LOG50 12
const char *labelsLog50[NUM_AXIS_DATA_LOG50] =
{
    "", "1Hz", "2Hz", "4Hz", "6Hz", "8Hz", "10Hz",
    "20Hz", "30Hz", "40Hz", "50Hz", ""
};
const BL_FLOAT freqsLog50[NUM_AXIS_DATA_LOG50] =
{
    0.0, 1.0, 2.0, 4.0, 6.0, 8.0, 10.0, 20.0, 30.0,
    40.0, 50.0, 60.0
};

// From 1Hz to 25Hz
#define NUM_AXIS_DATA_LOG25 11
const char *labelsLog25[NUM_AXIS_DATA_LOG25] =
{
    "", "1Hz", "2Hz", "4Hz", "6Hz", "8Hz", "10Hz",
    "15Hz", "20Hz", "25Hz", ""
};
const BL_FLOAT freqsLog25[NUM_AXIS_DATA_LOG25] =
{
    0.0, 1.0, 2.0, 4.0, 6.0, 8.0, 10.0, 15.0,
    20.0, 25.0, 30.0
};

// Linear scale
//

// From 1Hz to 100Hz
#define NUM_AXIS_DATA_LIN100 15
const char *labelsLin100[NUM_AXIS_DATA_LIN100] =
{
    "", "8Hz", "16Hz", "24Hz", "32Hz", "40Hz", "48Hz",
    "56Hz", "64Hz", "72Hz", "80Hz", "88Hz", "96Hz", "104Hz", ""
};
const BL_FLOAT freqsLin100[NUM_AXIS_DATA_LIN100] =
{
    0.0, 8.0, 16.0, 24.0, 32.0, 40.0, 48.0, 56.0, 64.0,
    72.0, 80.0, 88.0, 96.0, 104.0, 112.0

};

// From 1Hz to 50Hz
#define NUM_AXIS_DATA_LIN50 15
const char *labelsLin50[NUM_AXIS_DATA_LIN50] =
{
    "", "4Hz", "8Hz", "12Hz", "16Hz", "20Hz", "24Hz",
    "28Hz", "32Hz", "36Hz", "40Hz", "44Hz", "48Hz", "52Hz", ""
};
const BL_FLOAT freqsLin50[NUM_AXIS_DATA_LIN50] =
{
    0.0, 4.0, 8.0, 12.0, 16.0, 20.0, 24.0, 28.0, 32.0,
    36.0, 40.0, 44.0, 48.0, 52.0, 56.0
};

// From 1Hz to 25Hz
#define NUM_AXIS_DATA_LIN25 15
const char *labelsLin25[NUM_AXIS_DATA_LIN25] =
{
    "", "2Hz", "4Hz", "6Hz", "8Hz", "10Hz", "12Hz",
    "14Hz", "16Hz", "18Hz", "20Hz", "22Hz", "24Hz", "26Hz", ""
};

const BL_FLOAT freqsLin25[NUM_AXIS_DATA_LIN25] =
{
    0.0, 2.0, 4.0, 6.0, 8.0, 10.0, 12.0, 14.0, 16.0,
    18.0, 20.0, 22.0, 24.0, 26.0, 28.0
};

// From 1Hz to 12Hz
#define NUM_AXIS_DATA_LIN12 14
const char *labelsLin12[NUM_AXIS_DATA_LIN12] =
{
    "", "1Hz", "2Hz", "3Hz", "4Hz", "5Hz", "6Hz",
    "7Hz", "8Hz", "9Hz", "10Hz", "11Hz", "12Hz", ""
};

const BL_FLOAT freqsLin12[NUM_AXIS_DATA_LIN12] =
{
    0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0,
    9.0, 10.0, 11.0, 12.0, 13.0
};

// From 1Hz to 6Hz
#define NUM_AXIS_DATA_LIN6 14
const char *labelsLin6[NUM_AXIS_DATA_LIN6] =
{
    "", "0.5Hz", "1.0Hz", "1.5Hz", "2.0Hz", "2.5Hz",
    "3.0Hz", "3.5Hz", "4.0Hz", "4.5Hz", "5.0Hz", "5.5Hz", "6.0Hz", ""
};

const BL_FLOAT freqsLin6[NUM_AXIS_DATA_LIN6] =
{
    0.0, 0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0,
    4.5, 5.0, 5.5, 6.0, 6.5
};

GraphFreqAxis2::GraphFreqAxis2(bool displayLines,
                               Scale::Type scale)
{
    mScale = scale;
    
    mGraphAxis = NULL;
    
    mBufferSize = 2048;
    mSampleRate = 44100.0;

    mMaxFreq = -1.0;
    
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
    //BL_GUI_FLOAT lineWidth = guiHelper->GetGraphAxisLineWidthBold();
    
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
    if (mGraphAxis != NULL)
        mGraphAxis->SetBounds(bounds);
}

void
GraphFreqAxis2::Reset(int bufferSize, BL_FLOAT sampleRate)
{
    mBufferSize = bufferSize;
    mSampleRate = sampleRate;
    
    if (mGraphAxis != NULL)
        mGraphAxis->SetMinMaxValues(0.0, sampleRate*0.5);
    
    Update();
}

void
GraphFreqAxis2::SetMaxFreq(BL_FLOAT maxFreq)
{
    mMaxFreq = maxFreq;

    if (mGraphAxis != NULL)
        mGraphAxis->SetMinMaxValues(0.0, mMaxFreq);
    
    Update();
}

void
GraphFreqAxis2::Update()
{
    // Just in case
    if (mGraphAxis == NULL)
        return;

    if (mMaxFreq < 0.0)
    {
        BL_FLOAT minHzValue;
        BL_FLOAT maxHzValue;
        BLUtils::GetMinMaxFreqAxisValues(&minHzValue, &maxHzValue,
                                         mBufferSize, mSampleRate);
    
        // Avoid a shift
        minHzValue = 0.0;

        //UpdateAxis(NUM_AXIS_DATA_FULL, freqsFull, labelsFull,
        //minHzValue, maxHzValue);
        UpdateAxis(NUM_AXIS_DATA_FULL2, freqsFull2, labelsFull2,
                   minHzValue, maxHzValue);
    }
    else if ((mScale == Scale::LOG) ||
             (mScale == Scale::LOG10) ||
             (mScale == Scale::LOG_FACTOR) ||
             (mScale == Scale::MEL))
    {
        if (mMaxFreq <= 30.0)
            UpdateAxis(NUM_AXIS_DATA_LOG25,
                       freqsLog25, labelsLog25,
                       0.0, 30.0);
        else if (mMaxFreq <= 60.0)
            UpdateAxis(NUM_AXIS_DATA_LOG50,
                       freqsLog50, labelsLog50,
                       0.0, 60.0);
        else if (mMaxFreq <= 120.0)
            UpdateAxis(NUM_AXIS_DATA_LOG100,
                       freqsLog100, labelsLog100,
                       0.0, 120.0);
    }
    else if (mScale == Scale::LINEAR)
    {
        if (mMaxFreq <= 6.5)
            UpdateAxis(NUM_AXIS_DATA_LIN6,
                       freqsLin6, labelsLin6,
                       0.0, 6.5);
        else if (mMaxFreq <= 13.0)
            UpdateAxis(NUM_AXIS_DATA_LIN12,
                       freqsLin12, labelsLin12,
                       0.0, 13.0);
        else if (mMaxFreq <= 28.0)
            UpdateAxis(NUM_AXIS_DATA_LIN25,
                       freqsLin25, labelsLin25,
                       0.0, 28.0);
        else if (mMaxFreq <= 56.0)
            UpdateAxis(NUM_AXIS_DATA_LIN50,
                       freqsLin50, labelsLin50,
                       0.0, 56.0);
        else if (mMaxFreq <= 112.0)
            UpdateAxis(NUM_AXIS_DATA_LIN100,
                       freqsLin100, labelsLin100,
                       0.0, 112.0);
    }
}

void
GraphFreqAxis2::UpdateAxis(int numAxisData,
                           const BL_FLOAT freqs[],
                           const char *labels[],
                           BL_FLOAT minHzValue, BL_FLOAT maxHzValue)
{
    //void SetData(char *data[][2], int numData);
    
    char **axisData = (char **)malloc(numAxisData*sizeof(char *)*2);
    
    for (int i = 0; i < numAxisData; i++)
    {
        axisData[i*2] = (char *)malloc(sizeof(char)*255);
        axisData[i*2 + 1] = (char *)malloc(sizeof(char)*255);
    }

    for (int i = 0; i < numAxisData; i++)
    {
        sprintf(axisData[i*2 + 1], "%s", labels[i]);
    }

    // Normalize
    for (int i = 0; i < numAxisData; i++)
    {
        sprintf(axisData[i*2], "%g", freqs[i]);
        
        // We are over the sample rate, make empty label
        if ((freqs[i] < minHzValue) || (freqs[i] > maxHzValue))
            //sprintf(axisData[i*2 + 1], " ");
            sprintf(axisData[i*2 + 1], "");
    }

    if (mGraphAxis != NULL)
        mGraphAxis->SetData((char *(*)[2])axisData, numAxisData);
    
    for (int i = 0; i < numAxisData; i++)
    {
        free(axisData[i*2]);
        free(axisData[i*2 + 1]);
    }

    free(axisData);
}

#endif
