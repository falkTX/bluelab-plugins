//
//  MyBitmapControl.h
//  Transient
//
//  Created by Apple m'a Tuer on 20/05/17.
//
//

#ifndef __Transient__MyBitmapControl__
#define __Transient__MyBitmapControl__

#include "IControl.h"

#include <ParamSmoother.h>

#define MIN_LEVEL_DB -60.0

// Derive class, to be able to use Idle functionality.
class VuMeterControl : public IBitmapControl
{
public:
    VuMeterControl(IPlugBase* pPlug, int x, int y, int paramIdx, IBitmap* pBitmap, IChannelBlend::EBlendMethod blendMethod = IChannelBlend::kBlendNone);
    
    virtual ~VuMeterControl();
    
    void OnGUIIdle();
    
    void UpdateVuMeter(double **inputs, int nInputs, int nFrames, int plugParam);
    
private:
    ParamSmoother mParamSmoother;
};

#endif /* defined(__Transient__MyBitmapControl__) */
