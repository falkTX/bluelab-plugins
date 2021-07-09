//
//  SMVProcessXComputerChroma.h
//  BL-SoundMetaViewer
//
//  Created by applematuer on 3/24/20.
//
//

#ifndef __BL_SoundMetaViewer__SMVProcessXComputerChroma__
#define __BL_SoundMetaViewer__SMVProcessXComputerChroma__

#include <SMVProcessXComputer.h>

//
class Axis3DFactory2;
class SMVProcessXComputerChroma : public SMVProcessXComputer
{
public:
    SMVProcessXComputerChroma(Axis3DFactory2 *axisFactory, BL_FLOAT sampleRate);
    
    virtual ~SMVProcessXComputerChroma();
    
    virtual void Reset(BL_FLOAT sampleRate) override;
    
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
    
    BL_FLOAT mSampleRate;
};

#endif /* defined(__BL_SoundMetaViewer__SMVProcessXComputerChroma__) */
