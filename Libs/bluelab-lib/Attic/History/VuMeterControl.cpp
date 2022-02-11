//
//  MyBitmapControl.cpp
//  Transient
//
//  Created by Apple m'a Tuer on 20/05/17.
//
//

#include <BLUtils.h>

#include "VuMeterControl.h"

#define EPS_DB 1e-15

using namespace iplug::igraphics;

VuMeterControl::VuMeterControl(IPluginBase* pPlug, int x, int y, int paramIdx,
                               IBitmap* pBitmap,
                               // #bl-iplug2
                               //IChannelBlend::EBlendMethod blendMethod,
							   bool decreaseWhenIdle,
                               bool informHostParamChange)
//: IBitmapControl(pPlug, x, y, paramIdx, pBitmap, blendMethod),
// #bl-iplug2
: IBitmapControl(x, y, *pBitmap, paramIdx),
mParamSmoother(MIN_LEVEL_DB)
{
	mDecreaseWhenIdle = decreaseWhenIdle;
    
    mInformHostParamChange = informHostParamChange;
    
    mIsLocked = false;
}

VuMeterControl::~VuMeterControl() {}

void
VuMeterControl::SetInformHostParamChange(bool flag)
{
    mInformHostParamChange = flag;
}

void
VuMeterControl::OnGUIIdle()
{
    // FIX: Freeze with Waveform Tracktion
    if (mInformHostParamChange)
    {
        // #bl-iplug2
#if 0
        // Force host update param, for automation
        mPlug->BeginInformHostOfParamChange(mParamIdx);
        mPlug->GetGUI()->SetParameterFromPlug(mParamIdx, mPlug->GetParam(mParamIdx)->GetNormalized(), true);
        mPlug->InformHostOfParamChange(mParamIdx, mPlug->GetParam(mParamIdx)->GetNormalized());
        mPlug->EndInformHostOfParamChange(mParamIdx);
        
#endif
    }
    
	if (!mDecreaseWhenIdle)
		// Do not decrease the level progressively automatically
		// Before this, there was jittering with autogain level metter
		return;

    // #bl-iplug2
#if 0
    
    // Make the vumeters goes to zero progressively
    // when there is no sound anymore.
    BL_FLOAT val = mPlug->GetParam(mParamIdx)->Value();
    
    val -= 1.2;
    
    if (val < MIN_LEVEL_DB)
    {
        val = MIN_LEVEL_DB;
        
        mParamSmoother.Reset(MIN_LEVEL_DB);
    }
    mPlug->GetParam(mParamIdx)->Set(val);
    
    BL_FLOAT norm = mPlug->GetParam(mParamIdx)->GetNormalized();
    SetValueFromPlug(norm);
    
#endif
}

void
VuMeterControl::UpdateVuMeter(BL_FLOAT **inputs, int nInputs, int nFrames, int plugParam)
{
    // Signal
    BL_FLOAT avg = BLUtils::ComputeRMSAvg(inputs[0], nFrames);
    if (/*!*/nInputs > 1)
    {
        avg += BLUtils::ComputeRMSAvg(inputs[1], nFrames);
        avg /= 2.0;
    }

    // convert to dB
    avg = BLUtils::AmpToDB(avg, (BL_FLOAT)MIN_LEVEL_DB, (BL_FLOAT)EPS_DB);
    if (avg < MIN_LEVEL_DB)
        avg = MIN_LEVEL_DB;
    
    // Smooth
    mParamSmoother.SetNewValue(avg);
    mParamSmoother.Update();
    avg = mParamSmoother.GetCurrentValue();
    
    // bl-iplug2
#if 0
    
    mPlug->GetParam(plugParam)->Set(avg);
    
    // Normalize to send to widget
    BL_FLOAT avgNorm = mPlug->GetParam(plugParam)->GetNormalized();
    
    SetValueFromPlug(avgNorm);
    
#endif
}

void
VuMeterControl::SetValueFromPlug(BL_FLOAT value)
{
    if (mIsLocked)
        return;
    
    // bl-iplug2
#if 0
    
    IBitmapControl::SetValueFromPlug(value);
    
#endif
}

void
VuMeterControl::SetValueFromGUI(BL_FLOAT value)
{
    if (mIsLocked)
        return;
    
    IBitmapControl::SetValueFromUserInput(value);
}

void
VuMeterControl::SetLocked(bool flag)
{
    mIsLocked = flag;
}

bool
VuMeterControl::IsLocked()
{
    return mIsLocked;
}
