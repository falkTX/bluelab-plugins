#include <stdio.h>
#include <math.h>

#include <BLUtilsPlug.h>

#include "PitchShifterPV.h"

using namespace stekyne;

//
PitchShifterPV::PitchShifterPV()
{
    for (int i = 0; i < 2; i++)
        mPitchObjs[i] = new PitchShifter();
}

PitchShifterPV::~PitchShifterPV()
{
    for (int i = 0; i < 2; i++)
    {
        if (mPitchObjs[i] != NULL)
            delete mPitchObjs[i];
    }
}

void
PitchShifterPV::Process(vector<WDL_TypedBuf<BL_FLOAT> > &in,
                        vector<WDL_TypedBuf<BL_FLOAT> > *out)
{
    if (in.empty())
        return;

    for (int i = 0; i < in.size(); i++)
    {
        if (i >= out.size())
            break;

        if (mPitchObjs[i] == NULL)
            break;

        (*out)[i] = in[i];
        
        mPitchObjs[i]->process((*out)[i].Get(), (*out)[i].GetSize());
    }
}
    
void
PitchShifterPV::SetFactor(BL_FLOAT factor)
{
    for (int i = 0; i < 2; i++)
    {
        if (mPitchObjs[i] != NULL)
            mPitchObjs[i]->setPitchRatio(factor);
    }
}

int
PitchShifterPV::ComputeLatency(int blockSize)
{
    int latency = 0;

    if (mPitchObjs[0] != NULL)
        latency = mPitchObjs[0].getLatencyInSamples();
    
    return latency;
}
