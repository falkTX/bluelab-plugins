//
//  MyBitmapControl.h
//  Transient
//
//  Created by Apple m'a Tuer on 20/05/17.
//
//

#ifndef __Transient__MyBitmapControl__
#define __Transient__MyBitmapControl__

// #bl-iplug2
#include "IControl.h"
//#include "IPlug_include_in_plug_hdr.h"

#include <ParamSmoother.h>

using namespace iplug::igraphics;
using namespace iplug;

#define MIN_LEVEL_DB -60.0

// Derive class, to be able to use Idle functionality.
class VuMeterControl : public IBitmapControl
{
public:
	// decreaseWhenIdle was added for NIKO-WIN, to fix a problem of vu-meter jittering with AutoGain
	// When disabled, do not try to decrease the level progressively, when no signal comes
    VuMeterControl(IPluginBase* pPlug, int x, int y, int paramIdx, IBitmap* pBitmap,
                   // #bl-iplug2
				   /*IChannelBlend::EBlendMethod blendMethod = IChannelBlend::kBlendNone,*/
				   bool decreaseWhenIdle = true, bool informHostParamChange = false);
    
    virtual ~VuMeterControl();
    
	// FIX: Reason not reading automation correctly
    void SetInformHostParamChange(bool flag);
	
    void OnGUIIdle();
    
    void UpdateVuMeter(BL_FLOAT **inputs, int nInputs, int nFrames, int plugParam);
    
    // Hack, to avoid that the vumeter is modified both by
    // the gain change in ProcessBL_FLOATReplacing() and by
    // a possible automation
    
    // Overriden
    void SetValueFromPlug(BL_FLOAT value);
    
    // Niko
    void SetValueFromGUI(BL_FLOAT value);
    
    void SetLocked(bool flag);
    
    bool IsLocked();
    
private:
    ParamSmoother mParamSmoother;

	bool mDecreaseWhenIdle;
    
    bool mInformHostParamChange;
    
    bool mIsLocked;
};

#endif /* defined(__Transient__MyBitmapControl__) */
