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
//  GraphFreqAxis.cpp
//  BL-InfrasonicViewer
//
//  Created by applematuer on 11/22/19.
//
//

#ifdef IGRAPHICS_NANOVG

#include <GraphControl11.h>
#include <BLUtils.h>

#include "GraphFreqAxis.h"

#define Y_AXIS_CURVE_HZ 0


GraphFreqAxis::GraphFreqAxis()
{
    mGraph = NULL;
    
    mBufferSize = 2048;
    mSampleRate = 44100.0;
}

GraphFreqAxis::~GraphFreqAxis() {}

void
GraphFreqAxis::Init(GraphControl11 *graph,
                    int bufferSize, BL_FLOAT sampleRate)
{
    mGraph = graph;
    
    mBufferSize = bufferSize;
    mSampleRate = sampleRate;
  
    //
#define NUM_DUMMY_AXIS_DATA 9
    char *DUMMY_AXIS_DATA [NUM_DUMMY_AXIS_DATA][2];
    for (int i = 0; i < NUM_DUMMY_AXIS_DATA; i++)
    {
        DUMMY_AXIS_DATA[i][0] = (char *)malloc(255);
        DUMMY_AXIS_DATA[i][1] = (char *)malloc(255);
    }
    
    sprintf(DUMMY_AXIS_DATA[0][1], " ");
    sprintf(DUMMY_AXIS_DATA[1][1], "100Hz");
    sprintf(DUMMY_AXIS_DATA[2][1], "500Hz");
    sprintf(DUMMY_AXIS_DATA[3][1], "1KHz");
    sprintf(DUMMY_AXIS_DATA[4][1], "2KHz");
    sprintf(DUMMY_AXIS_DATA[5][1], "5KHz");
    sprintf(DUMMY_AXIS_DATA[6][1], "10KHz");
    sprintf(DUMMY_AXIS_DATA[7][1], "20KHz");
    sprintf(DUMMY_AXIS_DATA[8][1], " ");
    
    BL_FLOAT freqs[NUM_DUMMY_AXIS_DATA] =
                        { 50.0, 100.0, 500.0, 1000.0, 2000.0,
                          5000.0, 10000.0, 20000.0, 40000.0 };
    
    BL_FLOAT minHzValue;
    BL_FLOAT maxHzValue;
    BLUtils::GetMinMaxFreqAxisValues(&minHzValue, &maxHzValue,
                                     mBufferSize, mSampleRate);
    
    
    // Avoid a shift
    minHzValue = 0.0;
    
    // Normalize
    for (int i = 0; i < NUM_DUMMY_AXIS_DATA; i++)
    {
        freqs[i] = (freqs[i] - minHzValue)/(maxHzValue - minHzValue);
        freqs[i] = BLUtils::LogScaleNormInv(freqs[i],
                                            (BL_FLOAT)1.0,
                                            (BL_FLOAT)Y_LOG_SCALE_FACTOR);
        
        sprintf(DUMMY_AXIS_DATA[i][0], "%g", freqs[i]);
    }
    
    // Dummy Y axis curve, for the scale of the axis
    mGraph->SetCurveYScale(Y_AXIS_CURVE_HZ, false, 0.0, 1.0);
    
    int axisColor[4] = { 48, 48, 48, /*255*/0 }; // invisible horizontal bars
    
    // Choose maximum brightness color for labels,
    // to see them well over clear spectrograms
    int axisLabelColor[4] = { 255, 255, 255, 255 };
    int axisOverlayColor[4] = { 48, 48, 48, 255 };
    
    int width = mGraph->GetRECT().W();
    mGraph->AddVAxis(DUMMY_AXIS_DATA, NUM_DUMMY_AXIS_DATA,
                         axisColor, axisLabelColor,
                         0.0, width - 40.0,
                         axisOverlayColor);
    
    for (int i = 0; i < NUM_DUMMY_AXIS_DATA; i++)
    {
        free(DUMMY_AXIS_DATA[i][0]);
        free(DUMMY_AXIS_DATA[i][1]);
    }
    
    //
    Update();
}

void
GraphFreqAxis::Reset(int bufferSize, BL_FLOAT sampleRate)
{
    mBufferSize = bufferSize;
    mSampleRate = sampleRate;
    
    Update();
}

void
GraphFreqAxis::Update()
{
    // Just in case
    if (mGraph == NULL)
        return;
    
#define NUM_AXIS_DATA 11
    char *AXIS_DATA [NUM_AXIS_DATA][2];
    for (int i = 0; i < NUM_AXIS_DATA; i++)
    {
        AXIS_DATA[i][0] = (char *)malloc(255);
        AXIS_DATA[i][1] = (char *)malloc(255);
    }
    
    sprintf(AXIS_DATA[0][1], " ");
    sprintf(AXIS_DATA[1][1], "100Hz");
    sprintf(AXIS_DATA[2][1], "500Hz");
    sprintf(AXIS_DATA[3][1], "1KHz");
    sprintf(AXIS_DATA[4][1], "2KHz");
    sprintf(AXIS_DATA[5][1], "5KHz");
    sprintf(AXIS_DATA[6][1], "10KHz");
    sprintf(AXIS_DATA[7][1], "20KHz");
    sprintf(AXIS_DATA[8][1], "40KHz");
    sprintf(AXIS_DATA[9][1], "80KHz");
    sprintf(AXIS_DATA[10][1], " ");
    
    BL_FLOAT freqs[NUM_AXIS_DATA] =
                    { 50.0, 100.0, 500.0, 1000.0, 2000.0, 5000.0,
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
        freqs[i] = (freqs[i] - minHzValue)/(maxHzValue - minHzValue);
        
        freqs[i] = BLUtils::LogScaleNormInv(freqs[i],
                                            (BL_FLOAT)1.0,
                                            (BL_FLOAT)Y_LOG_SCALE_FACTOR);
        
        sprintf(AXIS_DATA[i][0], "%g", freqs[i]);
        
        // We are over the sample rate, make empty label
        if (freqs[i] > 1.0)
            sprintf(AXIS_DATA[i][1], " ");
    }
    
    mGraph->ReplaceVAxis(AXIS_DATA, NUM_AXIS_DATA);
    
    for (int i = 0; i < NUM_AXIS_DATA; i++)
    {
        free(AXIS_DATA[i][0]);
        free(AXIS_DATA[i][1]);
    }
}

#endif // IGRAPHICS_NANOVG
