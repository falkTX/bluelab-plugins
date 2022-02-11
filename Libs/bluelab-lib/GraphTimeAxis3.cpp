//
//  GraphTimeAxis3.cpp
//  BL-InfrasonicViewer
//
//  Created by applematuer on 11/22/19.
//
//

#ifdef IGRAPHICS_NANOVG

#include <GraphControl11.h>

#include "GraphTimeAxis3.h"

#define MAX_NUM_LABELS 128

// FIX: Clear the time axis if we change the time window
#define FIX_RESET_TIME_AXIS 1

// InfrasonicViwer: with values likes -0.5s, or 0.8s for example
// this was displayed "0s" each time
#define FIX_ZERO_SECONDS_ROUND 1

// Avoid -1:-30.10 for example
#define FIX_NEGATIVE_MIDDLE_VALUES 1


GraphTimeAxis3::GraphTimeAxis3()
{
    mGraph = NULL;
    
    mCurrentTime = 0.0;
}

GraphTimeAxis3::~GraphTimeAxis3() {}

void
GraphTimeAxis3::Init(GraphControl11 *graph, int bufferSize,
                     BL_FLOAT timeDuration, BL_FLOAT spacingSeconds,
                     int yOffset)
{
    mGraph = graph;
    mBufferSize = bufferSize;
    mTimeDuration = timeDuration;
    mSpacingSeconds = spacingSeconds;
    
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
GraphTimeAxis3::Reset(int bufferSize, BL_FLOAT timeDuration,
                      BL_FLOAT spacingSeconds)
{
    mBufferSize = bufferSize;
    mTimeDuration = timeDuration;
    
    mSpacingSeconds = spacingSeconds;
    
#if FIX_RESET_TIME_AXIS
    // Reset the labels
    Update(mCurrentTime);
#endif
}

void
GraphTimeAxis3::Update(BL_FLOAT currentTime)
{
    // Just in case
    if (mGraph == NULL)
        return;
    
    mCurrentTime = currentTime;
    
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
        
    char *hAxisData[MAX_NUM_LABELS][2];
        
    // Allocate
#define LABEL_MAX_SIZE 64
    for (int i = 0; i < MAX_NUM_LABELS; i++)
    {
        hAxisData[i][0] = new char[LABEL_MAX_SIZE];
        memset(hAxisData[i][0], '\0', LABEL_MAX_SIZE);
            
        hAxisData[i][1] = new char[LABEL_MAX_SIZE];
        memset(hAxisData[i][1], '\0', LABEL_MAX_SIZE);
    }
    
    BL_FLOAT tm = startTime;
    
    // Align to 0 seconds
    tm = tm - fmod(tm, mSpacingSeconds*1000.0);
    
    for (int i = 0; i < MAX_NUM_LABELS; i++)
    {
#if 0 // Bad if we want to display e.g 0.5s
        // Check the "units"
        if ((i > 0) && (i < mNumLabels - 1))
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
        }
#endif
        
        // Parameter
        BL_FLOAT t = (tm - startTime)/duration;
        
        sprintf(hAxisData[i][0], "%g", t);
            
        if ((i > 0) && (i < MAX_NUM_LABELS - 1)) // Squeeze the borders
        {
            int seconds = (int)tm/1000;
            int millis = tm - seconds*1000;
            
#if !FIX_ZERO_SECONDS_ROUND
            if (millis < 0)
            {
                millis = -millis;
            }
#else
            bool negativeMillis = false;
            if (millis < 0)
            {
                millis = -millis;
                negativeMillis = true;
            }
#endif
            
            int minutes = seconds/60;
            seconds = seconds % 60;
            
            int hours = minutes/60;
            minutes = minutes % 60;
            
            // Default
            sprintf(hAxisData[i][1], "0s");
            
#if 0 // Prev formatting
            if ((seconds != 0) && (millis != 0))
                sprintf(hAxisData[i][1], "%ds %dms", seconds, millis);
            else
                if ((seconds == 0) && (millis != 0))
                    sprintf(hAxisData[i][1], "%dms", millis);
                else
                    if ((seconds != 0) && (millis == 0))
                        sprintf(hAxisData[i][1], "%ds", seconds);
#endif
            
#if 0 // New formatting: s/ms
            if ((seconds != 0) && (millis != 0))
                sprintf(hAxisData[i][1], "%d.%ds", seconds, millis/100);
            else
                if ((seconds == 0) && (millis != 0))
                    sprintf(hAxisData[i][1], "%dms", millis);
                else
                    if ((seconds != 0) && (millis == 0))
                        sprintf(hAxisData[i][1], "%ds", seconds);
#endif
            
#if 0 // New formatting2: manage minutes and hours, formating like hh:mm:ss.ms
            if (hours != 0)
            {
                sprintf(hAxisData[i][1], "%d:%d:%d.%d", hours, minutes, seconds, millis/100);
            }
            else if (minutes != 0)
            {
                sprintf(hAxisData[i][1], "%d:%d.%d", minutes, seconds, millis/100);
            }
            else if (seconds != 0)
            {
                sprintf(hAxisData[i][1], "%d.%ds", seconds, millis/100);
            }
            else if (millis != 0)
            {
                sprintf(hAxisData[i][1], "%dms", millis/100);
            }            
#endif
            
#if 1 // New formatting3: manage minutes and hours, formating like hh:mm:ss.ms
            //if ((hours != 0) && (minutes != 0) && (seconds != 0) && (millis != 0))
            if (hours != 0)
            {
#if FIX_NEGATIVE_MIDDLE_VALUES
                if (minutes < 0)
                    minutes = -minutes;
                if (seconds < 0)
                    seconds = -seconds;
                if (millis < 0)
                    millis = -millis;
#endif
                sprintf(hAxisData[i][1], "%d:%02d:%d.%d", hours, minutes, seconds, millis/100);
            }
            //else if ((minutes != 0) && (seconds != 0) && (millis != 0))
            else if (minutes != 0)
            {
#if FIX_NEGATIVE_MIDDLE_VALUES
                if (seconds < 0)
                    seconds = -seconds;
                if (millis < 0)
                    millis = -millis;
#endif

                
                sprintf(hAxisData[i][1], "%d:%02d.%d", minutes, seconds, millis/100);
            }
            //else if ((seconds != 0) && (millis != 0))
            else if (seconds != 0)
            {
#if FIX_NEGATIVE_MIDDLE_VALUES
                if (millis < 0)
                    millis = -millis;
#endif

                sprintf(hAxisData[i][1], "%d.%ds", seconds, millis/100);
            }
#if FIX_ZERO_SECONDS_ROUND
            else if (millis != 0)
            {
                if (negativeMillis)
                    sprintf(hAxisData[i][1], "-0.%ds", millis/100);
                else
                    sprintf(hAxisData[i][1], "0.%ds", millis/100);
            }
#endif

#endif
        }
        
        tm += mSpacingSeconds*1000.0;
        
        if (tm > startTime + duration)
            break;
    }
        
    mGraph->ReplaceHAxis(hAxisData, MAX_NUM_LABELS);
        
    // Free
    for (int i = 0; i < MAX_NUM_LABELS; i++)
    {
        delete []hAxisData[i][0];
        delete []hAxisData[i][1];
    }
}

BL_FLOAT
GraphTimeAxis3::ComputeTimeDuration(int numBuffers, int bufferSize,
                                    int oversampling, BL_FLOAT sampleRate)
{
    int numSamples = (numBuffers*bufferSize)/oversampling;
    
    BL_FLOAT timeDuration = numSamples/sampleRate;
    
    return timeDuration;
}

#endif // IGRAPHICS_NANOVG
