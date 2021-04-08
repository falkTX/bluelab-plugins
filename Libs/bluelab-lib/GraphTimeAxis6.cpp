//
//  GraphTimeAxis6.cpp
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

#include <BLDebug.h>

#include <Scale.h>

#include <GraphControl12.h>

#include <BLTransport.h>

#include "GraphTimeAxis6.h"

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

#define ROUND_TO_INT_LABELS 1

GraphTimeAxis6::GraphTimeAxis6(bool displayLines,
                               bool squeezeBorderLabels)
{
    mGraph = NULL;
    
    mGraphAxis = NULL;
    
    mCurrentTime = 0.0;
    
    mDisplayLines = displayLines;
    
    mSqueezeBorderLabels = squeezeBorderLabels;

    mAxisDataAllocated = false;

    mTransport = NULL;
}

GraphTimeAxis6::~GraphTimeAxis6()
{
    if (mAxisDataAllocated)
    {
        for (int i = 0; i < MAX_NUM_LABELS; i++)
        {
            delete []mHAxisData[i][0];
            delete []mHAxisData[i][1];
        }
    }
}

void
GraphTimeAxis6::SetTransport(BLTransport *transport)
{
    mTransport = transport;
}

void
GraphTimeAxis6::Init(GraphControl12 *graph,
                     GraphAxis2 *graphAxis,
                     GUIHelper12 *guiHelper,
                     int bufferSize,
                     BL_FLOAT timeDuration,
                     int maxNumLabels,
                     BL_FLOAT yOffset)
{
    mGraph = graph;
    
    graph->SetGraphTimeAxis(this);
    
    mGraphAxis = graphAxis;
    
    mBufferSize = bufferSize;
    mTimeDuration = timeDuration;
    mMaxNumLabels = maxNumLabels;
    
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

    // Be sure to not align the first and last labels to the borders of the graph
    mGraphAxis->SetAlignBorderLabels(false);
}

void
GraphTimeAxis6::Reset(int bufferSize, BL_FLOAT timeDuration,
                      int maxNumLabels, BL_FLOAT timeOffsetSec)
{
    mBufferSize = bufferSize;
    mTimeDuration = timeDuration;
    
    mMaxNumLabels = maxNumLabels;

    mTimeOffsetSec = timeOffsetSec;
    
#if FIX_RESET_TIME_AXIS
    // Reset the labels
    //Update(mCurrentTime);
    
    // FIX: GhostViewer: start with the plug not bypassed
    // => the time axis dislays bad values at starting
#define SS_COEFF 0.9
    // SS_COEFF => Hack, so that at starting,
    // there is no missing label on the left of the axis
    BL_FLOAT oneLabelSeconds = (1.0/mMaxNumLabels)*timeDuration;
    Update(oneLabelSeconds*SS_COEFF);
#endif
}

void
GraphTimeAxis6::UpdateFromDraw()
{
    if ((mTransport == NULL) || !mTransport->IsTransportPlaying())
        return;

    BL_FLOAT transportValue = mTransport->GetTransportValueSecLoop();
    Update(transportValue);
}

void
GraphTimeAxis6::GetMinMaxTime(BL_FLOAT *minTimeSec, BL_FLOAT *maxTimeSec)
{
    *minTimeSec = mCurrentTime - mTimeDuration + mTimeOffsetSec;
    *maxTimeSec = mCurrentTime + mTimeOffsetSec;
}

void
GraphTimeAxis6::Update(BL_FLOAT currentTime)
{
    // Just in case
    if (mGraphAxis == NULL)
        return;

    currentTime += mTimeOffsetSec;
    
    mCurrentTime = currentTime;
    
    BL_FLOAT startTime = currentTime - mTimeDuration;
    BL_FLOAT endTime = currentTime;
    
    BL_FLOAT timeDuration = endTime - startTime;
    
    // Convert to milliseconds
    startTime *= 1000.0;
    timeDuration *= 1000.0;
    
    // Allocate
#define LABEL_MAX_SIZE 64
    if (!mAxisDataAllocated)
    {
        for (int i = 0; i < MAX_NUM_LABELS; i++)
        {
            mHAxisData[i][0] = new char[LABEL_MAX_SIZE];
            memset(mHAxisData[i][0], '\0', LABEL_MAX_SIZE);
        
            mHAxisData[i][1] = new char[LABEL_MAX_SIZE];
            memset(mHAxisData[i][1], '\0', LABEL_MAX_SIZE);
        }

        mAxisDataAllocated = true;
    }
    else
    {
        for (int i = 0; i < MAX_NUM_LABELS; i++)
        {
            memset(mHAxisData[i][0], '\0', LABEL_MAX_SIZE);
            memset(mHAxisData[i][1], '\0', LABEL_MAX_SIZE);
        }
    }
    
    BL_FLOAT tm = startTime;

    BL_FLOAT step = timeDuration/mMaxNumLabels;
    int mod = 1;
    if (timeDuration > 1000)
        mod = 100;
    else if (timeDuration > 500)
        mod = 50;
    else if (timeDuration > 100)
        mod = 10;
    else if (timeDuration > 50)
        mod = 5;
    else if (timeDuration > 10) // For big zoom
        mod = 1;
    else if (timeDuration > 1) // For big zoom
        mod = 0;
    
    if (mod > 0) // Test for big zoom
        step = ((int)(step/mod))*mod;
    else
        // Step: 0.1
        step = ((int)(step*10.0))*0.1;
    
    // HACK
    // Without this, mMaxNumLabels is in fact min num labels,
    // and max num labels is 2*mMaxNumLabels
    step *= 2.0;
    
    // Start label
    if (mod > 0) // Test for big zoom ????
    {
        if (step > BL_EPS)
            tm = ((int)(tm/step))*step;
    }
    else
    {
        if (step > BL_EPS)
            tm = ((int)(tm/step))*step;
    }

    // Count exactly the right number of labels to send to the graph
    // Will avoid many empty values
    int labelNum = 0;
    
    BL_FLOAT timeDurationInv = 1.0/timeDuration;
    for (int i = 0; i < MAX_NUM_LABELS; i++)
    {
        // Parameter
        BL_FLOAT t = 0.0;
        if (timeDuration > BL_EPS)
            t = (tm - startTime)*timeDurationInv;

        // Do not fill the labels if out of bounds
        // (will be useful later to not display nvgText if not label)
        if ((t < 0.0) || (t > 1.0))
        {
            tm += step;
            
            continue;
        }
        
        sprintf(mHAxisData[labelNum][0], "%g", t);
        
#if FIX_ZERO_SECONDS_MILLIS
        sprintf(mHAxisData[labelNum][1], "0s");
#endif
        
        bool squeeze = (mSqueezeBorderLabels &&
                        ((labelNum == 0) ||
                         // Uper bound is incorrect
                         (labelNum == MAX_NUM_LABELS - 1))); 
        
        //if ((i > 0) && (i < MAX_NUM_LABELS - 1)) // Squeeze the borders
        if (!squeeze)
        {
            int seconds = (int)tm/1000;
            int millis = tm - seconds*1000;
            int decMillis = tm*10 - millis*10 - seconds*10000; // For big zooms
            
            bool negMillis = false;
            if (millis < 0)
            {
                millis = -millis;

                //decMillis = -decMillis;
                
                negMillis = true;
            }
            
            int minutes = seconds/60;
            seconds = seconds % 60;
            
            int hours = minutes/60;
            minutes = minutes % 60;
            
            // Default
            sprintf(mHAxisData[labelNum][1], "0s");
            
            // New formatting3: manage minutes and hours, formating like hh:mm:ss.ms
            if (hours != 0)
            {
                sprintf(mHAxisData[labelNum][1], "%d:%02d:%d.%d",
                        hours, minutes, seconds, millis/100);
            }
            else if (minutes != 0)
            {
                sprintf(mHAxisData[labelNum][1], "%d:%02d.%d",
                        minutes, seconds, millis/100);
            }
            else if (seconds != 0)
            {
                BL_FLOAT oneLabelSeconds = (1.0/mMaxNumLabels)*timeDuration*0.001;
                int div = 1;
                if (oneLabelSeconds >= 0.1)
                    div = 100;
                else if (oneLabelSeconds >= 0.01)
                    div = 10;
                else
                {
                    // HACK: limit the precision to 10ms (it is sufficient!)
                    div = 10;

                    if (mod <= 1)
                        div = 1; // For Ghost and big zoom
                }

                if (step >= 1)
                {
                    if (millis == 0)
                        sprintf(mHAxisData[labelNum][1], "%ds", seconds);
                
                    if ((millis > 0) && (millis < 10))
                        sprintf(mHAxisData[labelNum][1], "%d.00%ds",
                                seconds, millis/div);
                
                    if ((millis >= 10) && (millis < 100))
                        sprintf(mHAxisData[labelNum][1], "%d.0%ds",
                                seconds, millis/div);
                    
                    if (millis >= 100)
                        sprintf(mHAxisData[labelNum][1], "%d.%ds",
                                seconds, millis/div);
                }
                else
                {
                    // Display tenth of millis
                    int decMillis0 = millis*10 + decMillis;
                    
                    if (decMillis0 == 0)
                        sprintf(mHAxisData[labelNum][1], "%ds", seconds);
                
                    if ((decMillis0 > 0) && (decMillis0 < 10))
                        sprintf(mHAxisData[labelNum][1], "%d.000%ds",
                                seconds, decMillis0);
                
                    if ((decMillis0 >= 10) && (decMillis0 < 100))
                        sprintf(mHAxisData[labelNum][1], "%d.00%ds",
                                seconds, decMillis0);
                    
                    if ((decMillis0 >= 100) && (decMillis0 < 1000))
                        sprintf(mHAxisData[labelNum][1], "%d.0%ds",
                                seconds, decMillis0);

                    if (decMillis0 >= 1000)
                        sprintf(mHAxisData[labelNum][1], "%d.%ds",
                                seconds, decMillis0);
                }
            }
#if FIX_DISPLAY_MS
            else if (millis != 0)
            {
                if (negMillis)
                {
                    millis = -millis;
                    decMillis = -decMillis;
                }

                if (timeDuration >= 5.0)
                    // Origin
                    sprintf(mHAxisData[labelNum][1], "%dms", millis);
                else
                    sprintf(mHAxisData[labelNum][1], "%d.%dms", millis, decMillis);
            }
#endif
            else if (decMillis != 0)
            {
                if (decMillis >= 0)
                    sprintf(mHAxisData[labelNum][1], "0.%dms", decMillis);
                else
                    sprintf(mHAxisData[labelNum][1], "-0.%dms", -decMillis);
            }
        }
        
#if SQUEEZE_LAST_CROPPED_LABEL
        BL_FLOAT normSpacing = mSpacingSeconds/mTimeDuration;
        if (t > 1.0 - normSpacing)
        {
            sprintf(mHAxisData[labelNum][1], "");
        }
#endif

        tm += step;

        labelNum++;
    }

    // Send exactly the right number of labels to send to the graph
    // (and not many additional dummy labels)
    mGraphAxis->SetData(mHAxisData, labelNum/*MAX_NUM_LABELS*/);
    
    // Free
    /*for (int i = 0; i < MAX_NUM_LABELS; i++)
      {
      delete []hAxisData[i][0];
      delete []hAxisData[i][1];
      }*/
}

BL_FLOAT
GraphTimeAxis6::ComputeTimeDuration(int numBuffers, int bufferSize,
                                    int oversampling, BL_FLOAT sampleRate)
{
    int numSamples = (numBuffers*bufferSize)/oversampling;
    
    BL_FLOAT timeDuration = numSamples/sampleRate;
    
    return timeDuration;
}

#endif
