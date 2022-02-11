//
//  USTBrillance.cpp
//  UST
//
//  Created by applematuer on 12/3/19.
//
//

//#include <CrossoverSplitterNBands3.h>
#include <CrossoverSplitterNBands4.h>

#include <BLUtils.h>

#include "USTBrillance.h"

// See: https://en.wikipedia.org/wiki/Audio_frequency
#define BRILLANCE_MIN_FREQ 8192.0
#define BRILLANCE_MAX_FREQ 16384.0
#define BRILLANCE_MAX_DB 12.0


USTBrillance::USTBrillance(BL_FLOAT sampleRate)
{
    BL_FLOAT brillanceFreqs[2] = { BRILLANCE_MIN_FREQ, BRILLANCE_MAX_FREQ };
    for (int i = 0; i < MAX_NUM_CHANNELS; i++)
    {
        mSplitters[i] = new CrossoverSplitterNBands4(3, brillanceFreqs, sampleRate);
    }
    
    for (int i = 0; i < MAX_NUM_CHANNELS; i++)
    {
        mBypassSplitters[i] = new CrossoverSplitterNBands4(3, brillanceFreqs, sampleRate);
    }
}

USTBrillance::~USTBrillance()
{
    for (int i = 0; i < MAX_NUM_CHANNELS; i++)
    {
        delete mSplitters[i];
    }
    
    for (int i = 0; i < MAX_NUM_CHANNELS; i++)
    {
        delete mBypassSplitters[i];
    }
}

void
USTBrillance::Reset(BL_FLOAT sampleRate)
{
    for (int i = 0; i < MAX_NUM_CHANNELS; i++)
        mSplitters[i]->Reset(sampleRate);
    
    for (int i = 0; i < MAX_NUM_CHANNELS; i++)
        mBypassSplitters[i]->Reset(sampleRate);
}

void
USTBrillance::Process(vector<WDL_TypedBuf<BL_FLOAT> > *samples,
                      BL_FLOAT brillance)
{
    for (int i = 0; i < samples->size(); i++)
    {
        if (i >= MAX_NUM_CHANNELS)
            break;

        // Split
        WDL_TypedBuf<BL_FLOAT> bands[3];
        mSplitters[i]->Split((*samples)[i], bands);
    
        // Increase center band
        BL_FLOAT brillanceDb = brillance*BRILLANCE_MAX_DB;
        BL_FLOAT brillanceAmp = BLUtils::DBToAmp(brillanceDb);
        BLUtils::MultValues(&bands[1], brillanceAmp);
        
        // Resynth
        BLUtils::FillAllZero(&(*samples)[i]);
        for (int j = 0; j < 3; j++)
        {
            BLUtils::AddValues(&(*samples)[i], bands[j]);
        }
    }
}

void
USTBrillance::BypassProcess(vector<WDL_TypedBuf<BL_FLOAT> > *samples)
{
    for (int i = 0; i < samples->size(); i++)
    {
        if (i >= MAX_NUM_CHANNELS)
            break;
        
        // Split
        WDL_TypedBuf<BL_FLOAT> bands[3];
        mBypassSplitters[i]->Split((*samples)[i], bands);
        
        // Resynth
        BLUtils::FillAllZero(&(*samples)[i]);
        for (int j = 0; j < 3; j++)
        {
            BLUtils::AddValues(&(*samples)[i], bands[j]);
        }
    }
}
