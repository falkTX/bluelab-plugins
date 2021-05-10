#include <stdio.h>
#include <math.h>

#include <BLUtilsPlug.h>

#include "PitchShifterPV.h"

// With 1024, we miss some frequencies
#define BUFFER_SIZE 2048

#define OVERSAMPLING_0 4
#define OVERSAMPLING_1 8
#define OVERSAMPLING_2 16
#define OVERSAMPLING_3 32

//
PitchShifterPV::PitchShifterPV()
{
    //mPitchObjs[0] = NULL;
    //mPitchObjs[1] = NULL;
    
    //
    mOversampling = OVERSAMPLING_0;
    
    mSampleRate = 44100.0;
    mFactor = 1.0;

    Init(mSampleRate);
}

PitchShifterPV::~PitchShifterPV()
{
    for (int i = 0; i < 2; i++)
    {
        //if (mPitchObjs[i] != NULL)
        //    delete mPitchObjs[i];
    }
}

void
PitchShifterPV::Reset(BL_FLOAT sampleRate, int blockSize)
{
    mSampleRate = sampleRate;

    // Sample rate has changed, and we can have variable buffer size
    Init(mSampleRate);
}

void
PitchShifterPV::Process(vector<WDL_TypedBuf<BL_FLOAT> > &in,
                        vector<WDL_TypedBuf<BL_FLOAT> > *out)
{
    if (in.empty())
        return;

    // TODO
}
    
void
PitchShifterPV::SetFactor(BL_FLOAT factor)
{
    mFactor = factor;
}

void
PitchShifterPV::SetQuality(int quality)
{
    switch(quality)
    {
        case 0:
            mOversampling = 4;
            break;
            
        case 1:
            mOversampling = 8;
            break;
            
        case 2:
            mOversampling = 16;
            break;
            
        case 3:
            mOversampling = 32;
            break;
            
        default:
            break;
    }

    Init(mSampleRate);
}

int
PitchShifterPV::ComputeLatency(int blockSize)
{
    // TODO
    int latency = 0;

    return latency;
}

void
PitchShifterPV::Init(BL_FLOAT sampleRate)
{
    // TODO
}
