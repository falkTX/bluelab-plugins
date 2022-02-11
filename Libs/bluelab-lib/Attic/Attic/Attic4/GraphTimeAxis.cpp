//
//  GraphTimeAxis.cpp
//  BL-InfrasonicViewer
//
//  Created by applematuer on 11/22/19.
//
//

#include <GraphControl11.h>

#include "GraphTimeAxis.h"

#define MAX_NUM_LABELS 128

GraphTimeAxis::GraphTimeAxis()
{
    mGraph = NULL;
}

GraphTimeAxis::~GraphTimeAxis() {}

void
GraphTimeAxis::Init(GraphControl11 *graph, int bufferSize,
                    BL_FLOAT timeDuration, int numLabels,
                    int yOffset)
{
    mGraph = graph;
    mBufferSize = bufferSize;
    mTimeDuration = timeDuration;
    mNumLabels = numLabels;
    
    //
    
    // Horizontal axis: time
#define MIN_HAXIS_VALUE 0.0
#define MAX_HAXIS_VALUE 1.0
    
#define NUM_HAXIS_DATA 6
    static char *HAXIS_DATA [NUM_HAXIS_DATA][2] =
    {
        { "0.0", "" },
        { "0.2", "" },
        { "0.4", "" },
        { "0.6", "" },
        { "0.8", "" },
        { "1.0", "" },
    };
    
    int hAxisColor[4] = { 48, 48, 48, /*255*/0 }; // invisible vertical bars
    // Choose maximum brightness color for labels,
    // to see them well over clear spectrograms
    int hAxisLabelColor[4] = { 255, 255, 255, 255 };
    int hAxisOverlayColor[4] = { 48, 48, 48, 255 };
    
    mGraph->AddHAxis(HAXIS_DATA, NUM_HAXIS_DATA, false, hAxisColor, hAxisLabelColor,
                     yOffset,
                     hAxisOverlayColor);
}

void
GraphTimeAxis::Reset(int bufferSize, BL_FLOAT timeDuration)
{
    mBufferSize = bufferSize;
    mTimeDuration = timeDuration;
}

void
GraphTimeAxis::Update(BL_FLOAT currentTime)
{
    // Just in case
    if (mGraph == NULL)
        return;
        
    // Make a cool axis, with values sliding as we zoom
    // and units changing when necessary
    // (the border values have decimals, the middle values are rounded and slide)
#define EPS 1e-15
    
    BL_FLOAT startTime = currentTime - mTimeDuration;
    BL_FLOAT endTime = currentTime;
        
    BL_FLOAT duration = endTime - startTime;
        
    // Convert to milliseconds
    startTime *= 1000.0;
    duration *= 1000.0;
    
    // NOTE: this should be fixed
//#define NUM_HAXIS_DATA 24 //12 //6
        
    char *hAxisData[MAX_NUM_LABELS/*NUM_HAXIS_DATA*/][2];
        
    // Allocate
#define LABEL_MAX_SIZE 64
    for (int i = 0; i < mNumLabels/*NUM_HAXIS_DATA*/; i++)
    {
        hAxisData[i][0] = new char[LABEL_MAX_SIZE];
        memset(hAxisData[i][0], '\0', LABEL_MAX_SIZE);
            
        hAxisData[i][1] = new char[LABEL_MAX_SIZE];
        memset(hAxisData[i][1], '\0', LABEL_MAX_SIZE);
    }
        
    BL_FLOAT prevT = 0.0;
    for (int i = 0; i < mNumLabels/*NUM_HAXIS_DATA*/; i++)
    {
        BL_FLOAT t = ((BL_FLOAT)i)/(mNumLabels/*NUM_HAXIS_DATA*/ - 1);
            
        BL_FLOAT tm = startTime + t*duration;
            
        BL_FLOAT newT = t;
            
        if ((i > 0) && (i < mNumLabels/*NUM_HAXIS_DATA*/ - 1))
            // Middle values
        {
            if (duration >= 1000.0)
            {
                tm = ((int)(tm/1000.0))*1000.0;
            }
            else if (duration >= 100.0)
            {
                tm = ((int)(tm/100.0))*100.0;
            }
            else if (duration >= 10.0)
            {
                tm = ((int)(tm/10.0))*10.0;
            }
                
            newT = (tm - startTime)/duration;
                
            // Avoid repeating the same value
            if (std::fabs(newT - prevT) < EPS)
                continue;
        }
            
        prevT = newT;
        sprintf(hAxisData[i][0], "%g", newT);
            
        if ((i > 0) && (i < mNumLabels/*NUM_HAXIS_DATA*/ - 1)) // Squeeze the borders
        {
            int seconds = (int)tm/1000;
            int millis = tm - seconds*1000;
            
            // Default
            sprintf(hAxisData[i][1], "0s", seconds, millis);
            
            if ((seconds != 0) && (millis != 0))
                sprintf(hAxisData[i][1], "%ds %dms", seconds, millis);
            else
                if ((seconds == 0) && (millis != 0))
                    sprintf(hAxisData[i][1], "%dms", millis);
                else
                    if ((seconds != 0) && (millis == 0))
                        sprintf(hAxisData[i][1], "%ds", seconds);
        }
    }
        
    mGraph->ReplaceHAxis(hAxisData, mNumLabels/*NUM_HAXIS_DATA*/);
        
    // Free
    for (int i = 0; i < mNumLabels/*NUM_HAXIS_DATA*/; i++)
    {
        delete []hAxisData[i][0];
        delete []hAxisData[i][1];
    }
}

BL_FLOAT
GraphTimeAxis::ComputeTimeDuration(int numBuffers, int bufferSize,
                                   int oversampling, BL_FLOAT sampleRate)
{
    int numSamples = (numBuffers*bufferSize)/oversampling;
    
    BL_FLOAT timeDuration = numSamples/sampleRate;
    
    return timeDuration;
}
