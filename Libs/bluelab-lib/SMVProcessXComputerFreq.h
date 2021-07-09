//
//  SMVProcessXComputerFreq.h
//  BL-SoundMetaViewer
//
//  Created by applematuer on 3/24/20.
//
//

#ifndef __BL_SoundMetaViewer__SMVProcessXComputerFreq__
#define __BL_SoundMetaViewer__SMVProcessXComputerFreq__

#include <SMVProcessXComputer.h>

//
class Axis3DFactory2;
class SMVProcessXComputerFreq : public SMVProcessXComputer
{
public:
    SMVProcessXComputerFreq(Axis3DFactory2 *axisFactory);
    
    virtual ~SMVProcessXComputerFreq();
    
    virtual void Reset(BL_FLOAT sampleRate) override {};
    
    void ComputeX(const WDL_TypedBuf<BL_FLOAT> samples[2],
                  const WDL_TypedBuf<BL_FLOAT> magns[2],
                  const WDL_TypedBuf<BL_FLOAT> phases[2],
                  WDL_TypedBuf<BL_FLOAT> *resultX,
                  WDL_TypedBuf<BL_FLOAT> *resultY = NULL,
                  bool *isPolar = NULL, BL_FLOAT polarCenter[2] = NULL,
                  bool *isScalable = NULL) override;
    
    Axis3D *CreateAxis() override;
    
protected:
    Axis3DFactory2 *mAxisFactory;
};

#endif /* defined(__BL_SoundMetaViewer__SMVProcessXComputerFreq__) */
