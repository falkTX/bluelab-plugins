//
//  CMAParamSmooth2.cpp
//  UST
//
//  Created by applematuer on 2/29/20.
//
//

#include "CMAParamSmooth2.h"

CMAParamSmooth2::CMAParamSmooth2(BL_FLOAT smoothingTimeMs, BL_FLOAT samplingRate)
{
    mSmoothers[0] = new CMAParamSmooth(smoothingTimeMs, samplingRate);
    mSmoothers[1] = new CMAParamSmooth(smoothingTimeMs, samplingRate);
}

CMAParamSmooth2::~CMAParamSmooth2()
{
    delete mSmoothers[0];
    delete mSmoothers[1];
}

void
CMAParamSmooth2::Reset(BL_FLOAT samplingRate)
{
    mSmoothers[0]->Reset(samplingRate, 0.0);
    mSmoothers[1]->Reset(samplingRate, 0.0);
}

void
CMAParamSmooth2::Reset(BL_FLOAT samplingRate, BL_FLOAT val)
{
    mSmoothers[0]->Reset(samplingRate, val);
    mSmoothers[1]->Reset(samplingRate, val);
}

void
CMAParamSmooth2::SetSmoothTimeMs(BL_FLOAT smoothingTimeMs)
{
    mSmoothers[0]->SetSmoothTimeMs(smoothingTimeMs);
    mSmoothers[1]->SetSmoothTimeMs(smoothingTimeMs);
}

BL_FLOAT
CMAParamSmooth2::Process(BL_FLOAT inVal)
{
    inVal = mSmoothers[0]->Process(inVal);
    BL_FLOAT result = mSmoothers[1]->Process(inVal);

    return result;
}
