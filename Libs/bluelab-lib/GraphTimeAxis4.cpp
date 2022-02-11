//
//  GraphTimeAxis4.cpp
//  BL-InfrasonicViewer
//
//  Created by applematuer on 11/22/19.
//
//

#ifdef IGRAPHICS_NANOVG

#include <GraphControl11.h>
#include <BLUtils.h>

#include "GraphTimeAxis4.h"

#define MAX_NUM_LABELS 128

// FIX: Clear the time axis if we change the time window
#define FIX_RESET_TIME_AXIS 1

// Without this, when the time interval is smaller than 1 second
// every label is "0s"
#define FIX_DISPLAY_MS 1

// When we have only milliseconds, the label "0s" is not displayed
#define FIX_ZERO_SECONDS_MILLIS 1

// Avoid that the last label is partially displayed, and cropped
// (was the case with Reverb, and display 1 second interval)
//
// For GhostViewer, setting to 1 is bad
// => it makes the last label pop inside a previous large empty space
#define SQUEEZE_LAST_CROPPED_LABEL 0 //1


GraphTimeAxis4::GraphTimeAxis4()
{
    mGraph = NULL;
    
    mCurrentTime = 0.0;
    
    mTransportIsPlaying = false;
    mCurrentTimeTransport = 0.0;
    mTransportTimeStamp = 0;
}

GraphTimeAxis4::~GraphTimeAxis4() {}

void
GraphTimeAxis4::Init(GraphControl11 *graph, int bufferSize,
                     BL_FLOAT timeDuration, BL_FLOAT spacingSeconds,
                     int yOffset)
{
    mGraph = graph;
    mBufferSize = bufferSize;
    mTimeDuration = timeDuration;
    mSpacingSeconds = spacingSeconds;
    
    mGraph->SetGraphTimeAxis(this);
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
GraphTimeAxis4::Reset(int bufferSize, BL_FLOAT timeDuration,
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
GraphTimeAxis4::UpdateFromTransport(BL_FLOAT currentTime)
{
    mCurrentTimeTransport = currentTime;
    
    mTransportTimeStamp = BLUtils::GetTimeMillis();
    
    Update(mCurrentTimeTransport);
}

void
GraphTimeAxis4::Update()
{
    if (!mTransportIsPlaying)
        return;
    
    long int now = BLUtils::GetTimeMillis();
    BL_FLOAT elapsed = (now - mTransportTimeStamp)*0.001;
    
    Update(mCurrentTimeTransport + elapsed);
}

void
GraphTimeAxis4::SetTransportPlaying(bool flag)
{
    mTransportIsPlaying = flag;
}

void
GraphTimeAxis4::Update(BL_FLOAT currentTime)
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
        // Parameter
        BL_FLOAT t = (tm - startTime)/duration;
        
        sprintf(hAxisData[i][0], "%g", t);
        
#if FIX_ZERO_SECONDS_MILLIS
        sprintf(hAxisData[i][1], "0s");
#endif
        
        if ((i > 0) && (i < MAX_NUM_LABELS - 1)) // Squeeze the borders
        {
            int seconds = (int)tm/1000;
            int millis = tm - seconds*1000;
            bool negMillis = false;
            if (millis < 0)
            {
                millis = -millis;
                negMillis = true;
            }
            
            int minutes = seconds/60;
            seconds = seconds % 60;
            
            int hours = minutes/60;
            minutes = minutes % 60;
            
            // Default
            sprintf(hAxisData[i][1], "0s");
            
            // New formatting3: manage minutes and hours, formating like hh:mm:ss.ms
            if (hours != 0)
            {
                sprintf(hAxisData[i][1], "%d:%02d:%d.%d", hours, minutes, seconds, millis/100);
            }
            //else if ((minutes != 0) && (seconds != 0) && (millis != 0))
            else if (minutes != 0)
            {
                sprintf(hAxisData[i][1], "%d:%02d.%d", minutes, seconds, millis/100);
            }
            //else if ((seconds != 0) && (millis != 0))
            else if (seconds != 0)
            {
                sprintf(hAxisData[i][1], "%d.%ds", seconds, millis/100);
            }
#if FIX_DISPLAY_MS
            else if (millis != 0)
            {
                if (negMillis)
                    millis = -millis;
                
                sprintf(hAxisData[i][1], "%dms", millis);
            }
#endif
        }
        
#if SQUEEZE_LAST_CROPPED_LABEL
        BL_FLOAT normSpacing = mSpacingSeconds/mTimeDuration;
        if (t > 1.0 - normSpacing)
        {
            sprintf(hAxisData[i][1], "");
        }
#endif
        
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
GraphTimeAxis4::ComputeTimeDuration(int numBuffers, int bufferSize,
                                    int oversampling, BL_FLOAT sampleRate)
{
    int numSamples = (numBuffers*bufferSize)/oversampling;
    
    BL_FLOAT timeDuration = numSamples/sampleRate;
    
    return timeDuration;
}

#endif // IGRAPHICS_NANOVG
