//
//  DemoModeManager.cpp
//  Spatializer
//
//  Created by Apple m'a Tuer on 21/04/17.
//
//

#include <stdlib.h>
#include <stdio.h> 
#include <time.h>

#ifndef WIN32
#include <sys/time.h>
#else
#include "GetTimeOfDay.h"
#endif

#include "IControl.h"

#include "DemoModeManager.h"

// Demo Label bitmap
#define DEMO_LABEL_X 180
#define DEMO_LABEL_Y 18

// For version 2 of the interface
#define DEMO_LABEL_X_V2 166
#define DEMO_LABEL_Y_V2 18


DemoModeManager *DemoModeManager::mInstance = NULL;

void
DemoModeManager::Init(IPlugBase *pPlug, IGraphics *pGraphics, bool isV2)
{
    if (mInstance == NULL)
        mInstance = new DemoModeManager(pPlug, pGraphics, isV2);
}

DemoModeManager *DemoModeManager::Get()
{
    return mInstance;
}

DemoModeManager::~DemoModeManager() {}

bool
DemoModeManager::IsDemoMode()
{
#ifdef DEMO_MODE
    return true;
#endif
    
    return false;
}

bool
DemoModeManager::MustProcess()
{
#ifndef DEMO_MODE
    return false;
#endif
    
    // Mute the sound for 1 second every 15 seconds
    time_t t = time(NULL);
    int sec = t % 60;
    
    // Every 15 seconds...
    sec = sec % 15;
    
    if (sec == 0)
        // Must mute
        return true;
    
    // Show the demo bitmap normally if we don't mute
    mBitmapControl->Hide(false);
    
    return false;
}

void
DemoModeManager::Process(double **outputs, int nFrames)
{
#ifdef DEMO_MODE
    // Mute
    for (int i = 0; i < nFrames; i++)
    {
        outputs[0][i] = 0.0;
        
        if (outputs[1] != NULL)
            outputs[1][i] = 0.0;
    }
    
    // Make the demo flag to blink
    struct timeval tp;
    gettimeofday(&tp, NULL);
    long int ms = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    
    // Blink 4 times each second
    ms /= 250;
    
    if (ms % 2 == 0)
        mBitmapControl->Hide(false);
    else
        mBitmapControl->Hide(true);
#endif
}

DemoModeManager::DemoModeManager(IPlugBase *pPlug, IGraphics *pGraphics, bool isV2)
{
#ifdef DEMO_MODE
    IBitmap bitmap = pGraphics->LoadIBitmap(DEMO_LABEL_ID, DEMO_LABEL_FN, 1);
    
    if (!isV2)
    {
        mBitmapControl = new IBitmapControl(pPlug, DEMO_LABEL_X, DEMO_LABEL_Y, &bitmap);
    }
    else
    {
        mBitmapControl = new IBitmapControl(pPlug, DEMO_LABEL_X_V2, DEMO_LABEL_Y_V2, &bitmap);
    }
    
    pGraphics->AttachControl(mBitmapControl);
#endif
}
