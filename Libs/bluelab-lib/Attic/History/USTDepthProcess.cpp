//
//  USTDepthProcess.cpp
//  UST
//
//  Created by applematuer on 12/3/19.
//
//

#include <EarlyReflect.h>

#include <BLUtils.h>

#include "USTDepthProcess.h"

// Like StageOne
#define MIN_DB 0.0
#define MAX_DB 9.0

// 8ms => 2.7m
#define COMMON_DELAY_MS 8.0 // 15ms ?
// 0.11ms => 38mm
#define DELAY_L_R_MS 0.11

USTDepthProcess::USTDepthProcess(BL_FLOAT sampleRate)
{
    BL_FLOAT factor = 1.0;
    BL_FLOAT width = -1.0; //0.0;
    
    mRev = new EarlyReflect(sampleRate, factor, width);
}

USTDepthProcess::~USTDepthProcess()
{
    delete mRev;
}

void
USTDepthProcess::Reset(BL_FLOAT sampleRate)
{
    mRev->Reset(sampleRate);
}

void
USTDepthProcess::Process(const WDL_TypedBuf<BL_FLOAT> &input,
                         WDL_TypedBuf<BL_FLOAT> *outputL,
                         WDL_TypedBuf<BL_FLOAT> *outputR,
                         BL_FLOAT depth)
{
    outputL->Resize(input.GetSize());
    outputR->Resize(input.GetSize());
    
    
    // Reverb
    mRev->Process(input, outputL, outputR);
    
    // Gain
    BL_FLOAT gainDB = (1.0 - depth)*MIN_DB + depth*MAX_DB;
    BL_FLOAT gain = BLUtils::DBToAmp(gainDB);

    gain = gain - 1.0;
    if (gain < 0.0)
        gain = 0.0;
    
    for (int i = 0; i < input.GetSize(); i++)
    {
        BL_FLOAT outL = outputL->Get()[i];
        BL_FLOAT outR = outputR->Get()[i];
        
        outL *= gain;
        outR *= gain;
        
        outputL->Get()[i] = outL;
        outputR->Get()[i] = outR;
    }
}
