//
//  DemoModeManager.h
//  Spatializer
//
//  Created by Apple m'a Tuer on 21/04/17.
//
//

#ifndef Spatializer_DemoModeManager_h
#define Spatializer_DemoModeManager_h

// If defined,  the plugins will be compiled in demo mode
#define DEMO_MODE

// Demo Label bitmap
#define DEMO_LABEL_ID 200
#define DEMO_LABEL_FN "resources/img/demo.png"

class IGraphics;
class IBitmapControl;
class IPlugBase;

class DemoModeManager
{
public:
    static void Init(IPlugBase *pPlug, IGraphics *pGraphics, bool isV2 = false);
    
    static DemoModeManager *Get();
    
    virtual ~DemoModeManager();
    
    bool IsDemoMode();
    
    bool MustProcess();
    
    void Process(double **outputs, int nFrames);
    
protected:
    DemoModeManager(IPlugBase *pPlug, IGraphics *pGraphics, bool isV2);
    
    IBitmapControl *mBitmapControl;
    
private:
    static DemoModeManager *mInstance;
};

#endif
