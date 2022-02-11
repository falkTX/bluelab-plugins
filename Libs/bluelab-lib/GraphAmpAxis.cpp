//
//  GraphAmpAxis.cpp
//  BL-InfrasonicViewer
//
//  Created by applematuer on 11/22/19.
//
//

#include <GraphAxis2.h>
#include <GUIHelper12.h>
#include <BLUtils.h>

#include "GraphAmpAxis.h"

#ifdef IGRAPHICS_NANOVG

GraphAmpAxis::GraphAmpAxis(bool displayLines, Density density)
{
    mGraphAxis = NULL;
    
    mMinDB = -60.0;
    mMaxDB = 0.0;
    
    mDisplayLines = displayLines;
    
    mDensity = density;
}

GraphAmpAxis::~GraphAmpAxis() {}

void
GraphAmpAxis::Init(GraphAxis2 *graphAxis,
                   GUIHelper12 *guiHelper,
                   BL_FLOAT minDB, BL_FLOAT maxDB,
                   int graphWidth)
{
    mMinDB = minDB;
    mMaxDB = maxDB;
    
    mGraphAxis = graphAxis;
    
    int axisColor[4];
    guiHelper->GetGraphAxisColor(axisColor);
    
    int axisLabelColor[4];
    guiHelper->GetGraphAxisLabelColor(axisLabelColor);
    
    int axisLabelOverlayColor[4];
    guiHelper->GetGraphAxisLabelOverlayColor(axisLabelOverlayColor);
    
    BL_GUI_FLOAT lineWidth = guiHelper->GetGraphAxisLineWidth();
    //BL_GUI_FLOAT lineWidth = guiHelper->GetGraphAxisLineWidthBold();
    
    // NOTE: this is important to set the labels linearly
    // (the data must be set with db scale however)
    mGraphAxis->InitVAxis(//Scale::DB,
                          Scale::LINEAR,
                          minDB, maxDB,
                          axisColor, axisLabelColor,
                          lineWidth,
                          ///0.0,
                          /////graphWidth - 40.0, // on the right
                          ///4.0, //40.0, // on the left
                          4.0/graphWidth, 0.0,
                          axisLabelOverlayColor);
    
    //
    Update();
}

void
GraphAmpAxis::Reset(BL_FLOAT minDB, BL_FLOAT maxDB)
{
    mMinDB = minDB;
    mMaxDB = maxDB;
    
    mGraphAxis->SetMinMaxValues(minDB, maxDB);
    
    Update();
}

void
GraphAmpAxis::Update()
{
    if (mDensity == DENSITY_20DB)
        UpdateDensity20dB();
    else if (mDensity == DENSITY_10DB)
        UpdateDensity10dB();
}

void
GraphAmpAxis::UpdateDensity20dB()
{
    // Just in case
    if (mGraphAxis == NULL)
        return;
    
#define NUM_AXIS_DATA_20DB 11
    char *AXIS_DATA [NUM_AXIS_DATA_20DB][2];
    for (int i = 0; i < NUM_AXIS_DATA_20DB; i++)
    {
        AXIS_DATA[i][0] = (char *)malloc(255);
        AXIS_DATA[i][1] = (char *)malloc(255);
    }
    
    sprintf(AXIS_DATA[0][1], "-160dB");
    sprintf(AXIS_DATA[1][1], "-140dB");
    sprintf(AXIS_DATA[2][1], "-120dB");
    sprintf(AXIS_DATA[3][1], "-100dB");
    sprintf(AXIS_DATA[4][1], "-80dB");
    sprintf(AXIS_DATA[5][1], "-60dB");
    sprintf(AXIS_DATA[6][1], "-40dB");
    sprintf(AXIS_DATA[7][1], "-20dB");
    sprintf(AXIS_DATA[8][1], "0dB");
    sprintf(AXIS_DATA[9][1], "20dB");
    sprintf(AXIS_DATA[10][1], "40dB"); // For EQHack
    
    BL_FLOAT amps[NUM_AXIS_DATA_20DB] =
                    { -160.0, -140.0, -120.0, -100.0, -80.0,
                      -60.0, -40.0, -20.0, 0.0, 20.0, 40.0 };

    for (int i = 0; i < NUM_AXIS_DATA_20DB; i++)
    {
        // Do not normalize here, set the values as they are!
        if ((amps[i] < mMinDB) || (amps[i] > mMaxDB))
            sprintf(AXIS_DATA[i][1], " ");
        
        sprintf(AXIS_DATA[i][0], "%g", amps[i]);
    }
    
    mGraphAxis->SetData(AXIS_DATA, NUM_AXIS_DATA_20DB);
    
    for (int i = 0; i < NUM_AXIS_DATA_20DB; i++)
    {
        free(AXIS_DATA[i][0]);
        free(AXIS_DATA[i][1]);
    }
}

void
GraphAmpAxis::UpdateDensity10dB()
{
    // Just in case
    if (mGraphAxis == NULL)
        return;
    
#define NUM_AXIS_DATA_10DB 21
    char *AXIS_DATA [NUM_AXIS_DATA_10DB][2];
    for (int i = 0; i < NUM_AXIS_DATA_10DB; i++)
    {
        AXIS_DATA[i][0] = (char *)malloc(255);
        AXIS_DATA[i][1] = (char *)malloc(255);
    }
    
    sprintf(AXIS_DATA[0][1], "-160dB");
    sprintf(AXIS_DATA[1][1], "-150dB");
    sprintf(AXIS_DATA[2][1], "-140dB");
    sprintf(AXIS_DATA[3][1], "-130dB");
    sprintf(AXIS_DATA[4][1], "-120dB");
    sprintf(AXIS_DATA[5][1], "-110dB");
    sprintf(AXIS_DATA[6][1], "-100dB");
    sprintf(AXIS_DATA[7][1], "-90dB");
    sprintf(AXIS_DATA[8][1], "-80dB");
    sprintf(AXIS_DATA[9][1], "-70dB");
    sprintf(AXIS_DATA[10][1], "-60dB");
    sprintf(AXIS_DATA[11][1], "-50dB");
    sprintf(AXIS_DATA[12][1], "-40dB");
    sprintf(AXIS_DATA[13][1], "-30dB");
    sprintf(AXIS_DATA[14][1], "-20dB");
    sprintf(AXIS_DATA[15][1], "-10dB");
    sprintf(AXIS_DATA[16][1], "0dB");
    sprintf(AXIS_DATA[17][1], "10dB");
    sprintf(AXIS_DATA[18][1], "20dB");
    sprintf(AXIS_DATA[19][1], "30dB");
    sprintf(AXIS_DATA[20][1], "40dB"); // For EQHack
    
    BL_FLOAT amps[NUM_AXIS_DATA_10DB] =
                    { -160.0, -150.0, -140.0, -130.0, -120.0,
                      -110.0, -100.0, -90.0, -80.0, -70.0, -60.0,
                      -50.0, -40.0, -30.0, -20.0, -10.0,
                        0.0, 10.0, 20.0, 30.0, 40.0 };
    
    for (int i = 0; i < NUM_AXIS_DATA_10DB; i++)
    {
        // Do not normalize here, set the values as they are!
        if ((amps[i] < mMinDB) || (amps[i] > mMaxDB))
            sprintf(AXIS_DATA[i][1], " ");
        
        sprintf(AXIS_DATA[i][0], "%g", amps[i]);
    }
    
    mGraphAxis->SetData(AXIS_DATA, NUM_AXIS_DATA_10DB);
    
    for (int i = 0; i < NUM_AXIS_DATA_10DB; i++)
    {
        free(AXIS_DATA[i][0]);
        free(AXIS_DATA[i][1]);
    }
}

#endif

