//
//  GraphTimeAxis5.cpp
//  BL-InfrasonicViewer
//
//  Created by applematuer on 11/22/19.
//
//

#ifdef IGRAPHICS_NANOVG

#include <GraphAxis2.h>
#include <GUIHelper12.h>

#include <BLUtils.h>
#include <BLUtilsMath.h>

#include <Scale.h>

#include <GraphControl12.h>

#include "GraphTimeAxis5.h"

#define MAX_NUM_LABELS 128

// FIX: Clear the time axis if we change the time window
#define FIX_RESET_TIME_AXIS 1

// Without this, when the time interval is smaller than 1 second
// every label is "0s"
#define FIX_DISPLAY_MS 1

// When we have only milliseconds, the label "0s" is not displayed
//
// NOTE: Disabled for GhostViewer: just at startup the first label 0s is false
#define FIX_ZERO_SECONDS_MILLIS 0 //1

// Avoid that the last label is partially displayed, and cropped
// (was the case with Reverb, and display 1 second interval)
//
// For GhostViewer, setting to 1 is bad
// => it makes the last label pop inside a previous large empty space
#define SQUEEZE_LAST_CROPPED_LABEL 0 //1

// TMP
#define FIX_COMPILATION 1

GraphTimeAxis5::GraphTimeAxis5(bool displayLines, bool roundToIntLabels)
{
    mGraphAxis = NULL;
    
    mCurrentTime = 0.0;
    
    mTransportIsPlaying = false;
    mCurrentTimeTransport = 0.0;
    mTransportTimeStamp = 0;
    
    mDisplayLines = displayLines;
    
    mRoundToIntLabels = roundToIntLabels;
    
    mMustUpdate = true;
}

GraphTimeAxis5::~GraphTimeAxis5() {}

void
GraphTimeAxis5::Init(GraphControl12 *graph,
                     GraphAxis2 *graphAxis,
                     GUIHelper12 *guiHelper,
                     int bufferSize,
                     BL_FLOAT timeDuration,
                     BL_FLOAT spacingSeconds,
                     BL_FLOAT yOffset)
{
#if !FIX_COMPILATION
    graph->SetGraphTimeAxis(this);
#endif
    
    mGraphAxis = graphAxis;
    
    mBufferSize = bufferSize;
    mTimeDuration = timeDuration;
    mSpacingSeconds = spacingSeconds;
    
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
    
    // NOTE: should be InitHAxis() ?
    mGraphAxis->InitVAxis(Scale::LINEAR, 0.0, 1.0,
                          axisColor, axisLabelColor,
                          lineWidth,
                          //yOffset, 0.0,
                          0.0, yOffset,
                          axisLabelOverlayColor);
}

void
GraphTimeAxis5::Reset(int bufferSize, BL_FLOAT timeDuration,
                      BL_FLOAT spacingSeconds)
{
    mBufferSize = bufferSize;
    mTimeDuration = timeDuration;
    
    mSpacingSeconds = spacingSeconds;
    
#if FIX_RESET_TIME_AXIS
    // Reset the labels
    //Update(mCurrentTime);
    
    // FIX: GhostViewer: start with the plug not bypassed
    // => the time axis dislays bad values at starting
#define SS_COEFF 0.9
    // SS_COEFF => Hack, so that at starting,
    // there is no missing label on the left of the axis
    Update(spacingSeconds*SS_COEFF);
#endif
}

void
GraphTimeAxis5::UpdateFromTransport(BL_FLOAT currentTime)
{
    mCurrentTimeTransport = currentTime;
    
    mTransportTimeStamp = BLUtils::GetTimeMillis();
    
    Update(mCurrentTimeTransport);
}

void
GraphTimeAxis5::Update()
{
    if (!mTransportIsPlaying)
        return;
    
    if (!mMustUpdate)
        return;
    
    long int now = BLUtils::GetTimeMillis();
    BL_FLOAT elapsed = (now - mTransportTimeStamp)*0.001;
    
    Update(mCurrentTimeTransport + elapsed);
    
    mMustUpdate = false;
}

void
GraphTimeAxis5::SetTransportPlaying(bool flag)
{
    mTransportIsPlaying = flag;
}

void
GraphTimeAxis5::SetMustUpdate()
{
    mMustUpdate = true;
}

void
GraphTimeAxis5::Update(BL_FLOAT currentTime)
{
    // Just in case
    if (mGraphAxis == NULL)
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
    
    BL_FLOAT prevT = 0.0;
    for (int i = 0; i < MAX_NUM_LABELS; i++)
    {
        // Parameter
        BL_FLOAT t = (tm - startTime)/duration;
        
        if (mRoundToIntLabels)
        {
            int numLabels = 0.001*duration/mSpacingSeconds;
            
            // Choose labels with integer values
            // (to avoid evenly spaced labels, but with various values)
            if ((i > 0) && (i < MAX_NUM_LABELS - 1))
                // Middle values
            {
                if (duration >= 1000.0)
                {
                    tm = ((int)(tm/(1000.0/numLabels)))*(1000.0/numLabels);
                }
                else if (duration >= 100.0)
                {
                    tm = ((int)(tm/(100.0/numLabels)))*(100.0/numLabels);
                }
                else if (duration >= 10.0)
                {
                    tm = ((int)(tm/(10.0/numLabels)))*(10.0/numLabels);
                }
                
                t = (tm - startTime)/duration;
                
                // Avoid repeating the same value
                if (fabs(t - prevT) < BL_EPS)
                    continue;
                
                prevT = t;
            }
        }
        
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
            else if (minutes != 0)
            {
                sprintf(hAxisData[i][1], "%d:%02d.%d", minutes, seconds, millis/100);
            }
            else if (seconds != 0)
            {
                //sprintf(hAxisData[i][1], "%d.%ds", seconds, millis/100);
                sprintf(hAxisData[i][1], "%d.%ds", seconds, millis);
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
    
    mGraphAxis->SetData(hAxisData, MAX_NUM_LABELS);
    
    // Free
    for (int i = 0; i < MAX_NUM_LABELS; i++)
    {
        delete []hAxisData[i][0];
        delete []hAxisData[i][1];
    }
}

BL_FLOAT
GraphTimeAxis5::ComputeTimeDuration(int numBuffers, int bufferSize,
                                    int oversampling, BL_FLOAT sampleRate)
{
    int numSamples = (numBuffers*bufferSize)/oversampling;
    
    BL_FLOAT timeDuration = numSamples/sampleRate;
    
    return timeDuration;
}

#endif

