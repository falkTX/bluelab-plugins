//
//  SMVProcessXComputerScope.h
//  BL-SoundMetaViewer
//
//  Created by applematuer on 3/24/20.
//
//

#ifndef __BL_SoundMetaViewer__SMVProcessXComputerScope__
#define __BL_SoundMetaViewer__SMVProcessXComputerScope__

#include <SMVProcessXComputer.h>

// Polar vectorscope
class Axis3DFactory2;
class SMVProcessXComputerScope : public SMVProcessXComputer
{
public:
    SMVProcessXComputerScope(Axis3DFactory2 *axisFactory);
    
    virtual ~SMVProcessXComputerScope();
    
    virtual void Reset(BL_FLOAT sampleRate) {};
    
    void ComputeX(const WDL_TypedBuf<BL_FLOAT> samples[2],
                  const WDL_TypedBuf<BL_FLOAT> magns[2],
                  const WDL_TypedBuf<BL_FLOAT> phases[2],
                  WDL_TypedBuf<BL_FLOAT> *resultX,
                  WDL_TypedBuf<BL_FLOAT> *resultY = NULL,
                  bool *isPolar = NULL, BL_FLOAT polarCenter[2] = NULL,
                  bool *isScalable = NULL);
    
    Axis3D *CreateAxis();
    
protected:
    void ComputePolarSamples(const WDL_TypedBuf<BL_FLOAT> samples[2],
                             WDL_TypedBuf<BL_FLOAT> polarSamples[2]);
    
    //
    Axis3DFactory2 *mAxisFactory;
};

#endif /* defined(__BL_SoundMetaViewer__SMVProcessXComputerScope__) */
