//
//  CMAParamSmooth.cpp
//  UST
//
//  Created by applematuer on 2/29/20.
//
//

#include "CMAParamSmooth.h"

CMAParamSmooth::CMAParamSmooth(BL_FLOAT smoothingTimeMs, BL_FLOAT samplingRate)
{
    mSmoothingTimeMs = smoothingTimeMs;
    mSampleRate = samplingRate;
    
    mWindowSize = smoothingTimeMs*0.001*samplingRate;
    
    mCurrentAvg = 0.0;
    
    Reset(samplingRate);
}

CMAParamSmooth::~CMAParamSmooth() {}

void
CMAParamSmooth::Reset(BL_FLOAT samplingRate)
{
    Reset(samplingRate, 0.0);
}

void
CMAParamSmooth::Reset(BL_FLOAT samplingRate, BL_FLOAT val)
{
    mSampleRate = samplingRate;
    mWindowSize = mSmoothingTimeMs*0.001*samplingRate;
    
    mCurrentAvg = val;
    
    mPrevValues.clear();
    
    // TEST
    Process(val);
}

void
CMAParamSmooth::SetSmoothTimeMs(BL_FLOAT smoothingTimeMs)
{
    mSmoothingTimeMs = smoothingTimeMs;
    
    Reset(mSampleRate);
}

BL_FLOAT
CMAParamSmooth::Process(BL_FLOAT inVal)
{
    if (mPrevValues.empty())
    {
        for (int i = 0; i < mWindowSize; i++)
            mPrevValues.push_back(inVal);
        
        return inVal;
    }
    
    mPrevValues.push_back(inVal);
    
    //if (mPrevValues.size() == 1)
    //    return inVal;
    
    if (!mPrevValues.empty())
    {
        BL_FLOAT firstValue = mPrevValues[0];
        mCurrentAvg += (1.0/mPrevValues.size())*(inVal - firstValue);
        
        if (mPrevValues.size() >= mWindowSize)
        {
            mPrevValues.pop_front();
        }
    }
    
    return mCurrentAvg;
}
