//
//  MyBitmapControl.cpp
//  Transient
//
//  Created by Apple m'a Tuer on 20/05/17.
//
//

#include <Utils.h>

#include "VuMeterControl.h"

VuMeterControl::VuMeterControl(IPlugBase* pPlug, int x, int y, int paramIdx, IBitmap* pBitmap, IChannelBlend::EBlendMethod blendMethod)
: IBitmapControl(pPlug, x, y, paramIdx, pBitmap, blendMethod),
mParamSmoother(MIN_LEVEL_DB) {}

VuMeterControl::~VuMeterControl() {}

void
VuMeterControl::OnGUIIdle()
{
    // Make the vumeters goes to zero progressively
    // when there is no sound anymore.
    double val = mPlug->GetParam(mParamIdx)->Value();
    
    val -= 1.2;
    
    if (val < MIN_LEVEL_DB)
    {
        val = MIN_LEVEL_DB;
        
        mParamSmoother.Reset(MIN_LEVEL_DB);
    }
    mPlug->GetParam(mParamIdx)->Set(val);
    
    double norm = mPlug->GetParam(mParamIdx)->GetNormalized();
    SetValueFromPlug(norm);
}

void
VuMeterControl::UpdateVuMeter(double **inputs, int nInputs, int nFrames, int plugParam)
{
    // Signal
    double avg = Utils::ComputeRMSAvg(inputs[0], nFrames);
    if (/*!*/nInputs > 1)
    {
        avg += Utils::ComputeRMSAvg(inputs[1], nFrames);
        avg /= 2.0;
    }

    // convert to dB
    avg = Utils::ampToDB(avg, MIN_LEVEL_DB);
    if (avg < MIN_LEVEL_DB)
        avg = MIN_LEVEL_DB;
    
    // Smooth
    mParamSmoother.SetNewValue(avg);
    mParamSmoother.Update();
    avg = mParamSmoother.GetCurrentValue();
    
    mPlug->GetParam(plugParam)->Set(avg);
    
    // Normalize to send to widget
    double avgNorm = mPlug->GetParam(plugParam)->GetNormalized();
    
    SetValueFromPlug(avgNorm);
}