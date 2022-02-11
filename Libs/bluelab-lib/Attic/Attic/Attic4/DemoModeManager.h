//
//  DemoModeManager.h
//  Spatializer
//
//  Created by Apple m'a Tuer on 21/04/17.
//
//

#ifndef Spatializer_DemoModeManager_h
#define Spatializer_DemoModeManager_h

#include "IPlug_include_in_plug_hdr.h"
#include "IControl.h"

using namespace iplug;
using namespace iplug::igraphics;

// If defined,  the plugins will be compiled in demo mode
//#define DEMO_MODE

// Demo Label bitmap
#define DEMO_LABEL_ID 200
#define DEMO_LABEL_FN "resources/img/demo.png"

// Do not use singleton anymore because a singleton is shared between all the plugins
// of the same class.
// Then the bitmap provided to the demo manager can be destroyed by another
// plugin that exits.
class DemoModeManager
{
public:
    DemoModeManager();
    
    virtual ~DemoModeManager();
    
    void Init(Plugin *pPlug, IGraphics *pGraphics, bool isV2 = false);
    
    bool IsDemoMode();
    
    bool MustProcess();
    
    void Process(double **outputs, int nFrames);
    
protected:
    IBitmapControl *mBitmapControl;
};

#endif
