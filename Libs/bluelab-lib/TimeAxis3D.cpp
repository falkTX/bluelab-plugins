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
//  TimeAxis3D.cpp
//  BL-InfrasonicViewer
//
//  Created by applematuer on 11/22/19.
//
//

#ifdef IGRAPHICS_NANOVG

#include <math.h>
#include <string.h>

#include <Axis3D.h>

#include "TimeAxis3D.h"

#define MAX_NUM_LABELS 128

// FIX: Clear the time axis if we change the time window
#define FIX_RESET_TIME_AXIS 1


TimeAxis3D::TimeAxis3D(Axis3D *axis)
{
    mAxis = axis;
    
    mCurrentTime = 0.0;
    
    mBufferSize = 2048;
    mTimeDuration = 1.0;
    mSpacingSeconds = 1.0;
}

TimeAxis3D::~TimeAxis3D() {}

void
TimeAxis3D::Init(int bufferSize,
                 BL_FLOAT timeDuration, BL_FLOAT spacingSeconds,
                 int yOffset)
{
    mBufferSize = bufferSize;
    mTimeDuration = timeDuration;
    mSpacingSeconds = spacingSeconds;
    
    //
    
    // Horizontal axis: time
#define MIN_AXIS_VALUE 0.0
#define MAX_AXIS_VALUE 1.0
    
#define NUM_AXIS_DATA 6
    static char *AXIS_LABELS[NUM_AXIS_DATA] = { "", "", "", "", "", "" };
    
    static BL_FLOAT AXIS_NORM_POS[NUM_AXIS_DATA] = { 0.0, 0.2, 0.4, 0.6, 0.8, 1.0 };
    
    mAxis->UpdateLabels(AXIS_LABELS, AXIS_NORM_POS, NUM_AXIS_DATA);
}

void
TimeAxis3D::Reset(int bufferSize, BL_FLOAT timeDuration,
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
TimeAxis3D::Update(BL_FLOAT currentTime)
{
    // Just in case
    if (mAxis == NULL)
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
        
    char *axisLabels[MAX_NUM_LABELS];
    BL_FLOAT axisPos[MAX_NUM_LABELS];
    
    // Allocate
#define LABEL_MAX_SIZE 64
    for (int i = 0; i < MAX_NUM_LABELS; i++)
    {
        axisLabels[i] = new char[LABEL_MAX_SIZE];
        memset(axisLabels[i], '\0', LABEL_MAX_SIZE);
            
        axisPos[i] = 0.0;
    }
    
    //
    BL_FLOAT tm = startTime;
    
    // Align to 0 seconds
    tm = tm - fmod(tm, mSpacingSeconds*1000.0);
    
    for (int i = 0; i < MAX_NUM_LABELS; i++)
    {        
        // Parameter
        BL_FLOAT t = (tm - startTime)/duration;
        
        axisPos[i] = t;
            
        if ((i > 0) && (i < MAX_NUM_LABELS - 1)) // Squeeze the borders
        {
            int seconds = (int)tm/1000;
            int millis = tm - seconds*1000;

            bool negativeMillis = false;
            if (millis < 0)
            {
                millis = -millis;
                negativeMillis = true;
            }
            
            int minutes = seconds/60;
            seconds = seconds % 60;
            
            int hours = minutes/60;
            minutes = minutes % 60;
            
            // Default
            sprintf(axisLabels[i], "0s");
            
            // New formatting3: manage minutes and hours, formating like hh:mm:ss.ms
            //if ((hours != 0) && (minutes != 0) && (seconds != 0) && (millis != 0))
            if (hours != 0)
            {
                if (minutes < 0)
                    minutes = -minutes;
                if (seconds < 0)
                    seconds = -seconds;
                if (millis < 0)
                    millis = -millis;
                
                sprintf(axisLabels[i], "%d:%02d:%d.%d", hours, minutes, seconds, millis/100);
            }
            //else if ((minutes != 0) && (seconds != 0) && (millis != 0))
            else if (minutes != 0)
            {
                if (seconds < 0)
                    seconds = -seconds;
                if (millis < 0)
                    millis = -millis;
                
                sprintf(axisLabels[i], "%d:%02d.%d", minutes, seconds, millis/100);
            }
            //else if ((seconds != 0) && (millis != 0))
            else if (seconds != 0)
            {
                if (millis < 0)
                    millis = -millis;

                sprintf(axisLabels[i], "%d.%ds", seconds, millis/100);
            }
            else if (millis != 0)
            {
                if (negativeMillis)
                    sprintf(axisLabels[i], "-0.%ds", millis/100);
                else
                    sprintf(axisLabels[i], "0.%ds", millis/100);
            }
        }
        
        tm += mSpacingSeconds*1000.0;
        
        if (tm > startTime + duration)
            break;
    }
    
    // Update the axis
    mAxis->UpdateLabels(axisLabels, axisPos, MAX_NUM_LABELS);
        
    // Free
    for (int i = 0; i < MAX_NUM_LABELS; i++)
    {
        delete []axisLabels[i];
    }
}

BL_FLOAT
TimeAxis3D::ComputeTimeDuration(int numBuffers, int bufferSize,
                                    int oversampling, BL_FLOAT sampleRate)
{
    int numSamples = (numBuffers*bufferSize)/oversampling;
    
    BL_FLOAT timeDuration = numSamples/sampleRate;
    
    return timeDuration;
}

#endif // IGRAPHICS_NANOVG
