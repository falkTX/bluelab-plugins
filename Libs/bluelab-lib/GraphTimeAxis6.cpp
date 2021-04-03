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

#include <Scale.h>

#include <GraphControl12.h>

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
    mGraphAxis = NULL;
    
    mCurrentTime = 0.0;
    
    mTransportIsPlaying = false;
    mCurrentTimeTransport = 0.0;
    mTransportTimeStamp = 0;
    
    mDisplayLines = displayLines;
    
    mSqueezeBorderLabels = squeezeBorderLabels;
    
    mMustUpdate = true;

    mAxisDataAllocated = false;
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
GraphTimeAxis6::Init(GraphControl12 *graph,
                     GraphAxis2 *graphAxis,
                     GUIHelper12 *guiHelper,
                     int bufferSize,
                     BL_FLOAT timeDuration,
                     int maxNumLabels,
                     BL_FLOAT yOffset)
{
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
}

void
GraphTimeAxis6::Reset(int bufferSize, BL_FLOAT timeDuration,
                      int maxNumLabels)
{
    mBufferSize = bufferSize;
    mTimeDuration = timeDuration;
    
    mMaxNumLabels = maxNumLabels;
    
#if FIX_RESET_TIME_AXIS
    // Reset the labels
    //Update(mCurrentTime);
    
    // FIX: GhostViewer: start with the plug not bypassed
    // => the time axis dislays bad values at starting
#define SS_COEFF 0.9
    // SS_COEFF => Hack, so that at starting,
    // there is no missing label on the left of the axis
    BL_FLOAT oneLabelSeconds = (1.0/mMaxNumLabels)*timeDuration;
    //Update(spacingSeconds*SS_COEFF);
    Update(oneLabelSeconds*SS_COEFF);
#endif
}

void
GraphTimeAxis6::UpdateFromTransport(BL_FLOAT currentTime)
{
    mCurrentTimeTransport = currentTime;
    
    mTransportTimeStamp = BLUtils::GetTimeMillis();
    
    Update(mCurrentTimeTransport);
}

void
GraphTimeAxis6::Update()
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
GraphTimeAxis6::SetTransportPlaying(bool flag)
{
    mTransportIsPlaying = flag;
}

void
GraphTimeAxis6::SetMustUpdate()
{
    mMustUpdate = true;
}

void
GraphTimeAxis6::GetMinMaxTime(BL_FLOAT *minTimeSec, BL_FLOAT *maxTimeSec)
{
    *minTimeSec = mCurrentTime - mTimeDuration;
    *maxTimeSec = mCurrentTime;
}

#if 0 // ORIGIN
void
GraphTimeAxis6::Update(BL_FLOAT currentTime)
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
        
    BL_FLOAT timeDuration = endTime - startTime;
        
    // Convert to milliseconds
    startTime *= 1000.0;
    timeDuration *= 1000.0;
        
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
    BL_FLOAT oneLabelSeconds = (1.0/mMaxNumLabels)*timeDuration*0.001;
    tm = tm - fmod(tm, oneLabelSeconds*1000.0);
    
    BL_FLOAT prevT = 0.0;
    for (int i = 0; i < MAX_NUM_LABELS; i++)
    {
        // Parameter
        BL_FLOAT t = (tm - startTime)/timeDuration;
        
#if ROUND_TO_INT_LABELS
        // Choose labels with integer values
        // (to avoid evenly spaced labels, but with various values)
        if ((i > 0) && (i < MAX_NUM_LABELS - 1))
            // Middle values
        {
            if (timeDuration >= 1000.0)
            {
                tm = ((int)(tm/(1000.0/mMaxNumLabels)))*(1000.0/mMaxNumLabels);
            }
            else if (timeDuration >= 100.0)
            {
                tm = ((int)(tm/(100.0/mMaxNumLabels)))*(100.0/mMaxNumLabels);
            }
            else if (timeDuration >= 10.0)
            {
                tm = ((int)(tm/(10.0/mMaxNumLabels)))*(10.0/mMaxNumLabels);
            }
            
            t = (tm - startTime)/timeDuration;
                
            // Avoid repeating the same value
            if (fabs(t - prevT) < BL_EPS)
                continue;
                
            prevT = t;
        }
#endif
        
        sprintf(hAxisData[i][0], "%g", t);
        
#if FIX_ZERO_SECONDS_MILLIS
        sprintf(hAxisData[i][1], "0s");
#endif
        
        bool squeeze = (mSqueezeBorderLabels &&
                        ((i == 0) || (i == MAX_NUM_LABELS - 1)));
        
        //if ((i > 0) && (i < MAX_NUM_LABELS - 1)) // Squeeze the borders
        if (!squeeze)
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
                // Check the number of digits to display
                BL_FLOAT oneLabelSeconds = (1.0/mMaxNumLabels)*timeDuration*0.001;
                if (oneLabelSeconds > 0.1)
                    sprintf(hAxisData[i][1], "%d.%ds", seconds, millis/100);
                else if (oneLabelSeconds > 0.01)
                    sprintf(hAxisData[i][1], "%d.%ds", seconds, millis/10);
                else
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
        
        BL_FLOAT oneLabelSeconds = (1.0/mMaxNumLabels)*timeDuration*0.001;
        
        tm += oneLabelSeconds*1000.0;
        
        if (tm > startTime + timeDuration)
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
#endif

void
GraphTimeAxis6::Update(BL_FLOAT currentTime)
{
    // Just in case
    if (mGraphAxis == NULL)
        return;
    
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
    //tm = 0.0;
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
    
    BL_FLOAT timeDurationInv = 1.0/timeDuration;
    for (int i = 0; i < MAX_NUM_LABELS; i++)
    {
        // Parameter
        BL_FLOAT t = 0.0;
        if (timeDuration > BL_EPS)
            //t = (tm - startTime)/timeDuration;
            t = (tm - startTime)*timeDurationInv;

        // Do not fill the labls if out of bounds
        // (will be useful later to not display nvgText if not label)
        if ((t < 0.0) || (t > 1.0))
            continue;
        
        sprintf(mHAxisData[i][0], "%g", t);
        
#if FIX_ZERO_SECONDS_MILLIS
        sprintf(mHAxisData[i][1], "0s");
#endif
        
        bool squeeze = (mSqueezeBorderLabels &&
                        ((i == 0) || (i == MAX_NUM_LABELS - 1)));
        
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
            sprintf(mHAxisData[i][1], "0s");
            
            // New formatting3: manage minutes and hours, formating like hh:mm:ss.ms
            if (hours != 0)
            {
                sprintf(mHAxisData[i][1], "%d:%02d:%d.%d",
                        hours, minutes, seconds, millis/100);
            }
            else if (minutes != 0)
            {
                sprintf(mHAxisData[i][1], "%d:%02d.%d",
                        minutes, seconds, millis/100);
            }
            else if (seconds != 0)
            {
                // Check the number of digits to display
                /*BL_FLOAT oneLabelSeconds = (1.0/mMaxNumLabels)*timeDuration*0.001;
                if (oneLabelSeconds > 0.1)
                    sprintf(hAxisData[i][1], "%d.%ds", seconds, millis);
                else if (oneLabelSeconds > 0.01)
                    sprintf(hAxisData[i][1], "%d.%ds", seconds, millis);
                else
                    sprintf(hAxisData[i][1], "%d.%ds", seconds, millis);*/
                
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
                        sprintf(mHAxisData[i][1], "%ds", seconds);
                
                    if ((millis > 0) && (millis < 10))
                        sprintf(mHAxisData[i][1], "%d.00%ds", seconds, millis/div);
                
                    //if ((millis > 0) && (millis < 100))
                    if ((millis >= 10) && (millis < 100))
                        sprintf(mHAxisData[i][1], "%d.0%ds", seconds, millis/div);
                    
                    //if ((millis > 0) && (millis >= 100))
                    if (millis >= 100)
                        sprintf(mHAxisData[i][1], "%d.%ds", seconds, millis/div);
                }
                else
                {
                    // Display tenth of millis
                    int decMillis0 = millis*10 + decMillis;
                    
                    if (decMillis0 == 0)
                        sprintf(mHAxisData[i][1], "%ds", seconds);
                
                    if ((decMillis0 > 0) && (decMillis0 < 10))
                        sprintf(mHAxisData[i][1], "%d.000%ds", seconds, decMillis0);
                
                    if ((decMillis0 >= 10) && (decMillis0 < 100))
                        sprintf(mHAxisData[i][1], "%d.00%ds", seconds, decMillis0);
                    
                    if ((decMillis0 >= 100) && (decMillis0 < 1000))
                        sprintf(mHAxisData[i][1], "%d.0%ds", seconds, decMillis0);

                    if (decMillis0 >= 1000)
                        sprintf(mHAxisData[i][1], "%d.%ds", seconds, decMillis0);
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
                    sprintf(mHAxisData[i][1], "%dms", millis);
                else
                    sprintf(mHAxisData[i][1], "%d.%dms", millis, decMillis);
            }
#endif
            else if (decMillis != 0)
            {
                if (decMillis >= 0)
                    sprintf(mHAxisData[i][1], "0.%dms", decMillis);
                else
                    sprintf(mHAxisData[i][1], "-0.%dms", -decMillis);
            }
        }
        
#if SQUEEZE_LAST_CROPPED_LABEL
        BL_FLOAT normSpacing = mSpacingSeconds/mTimeDuration;
        if (t > 1.0 - normSpacing)
        {
            sprintf(mHAxisData[i][1], "");
        }
#endif
        
        tm += step;
    }
    
    mGraphAxis->SetData(mHAxisData, MAX_NUM_LABELS);
    
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

