//
//  SMVProcessYComputerPhasesTime.h
//  BL-SoundMetaViewer
//
//  Created by applematuer on 3/24/20.
//
//

#ifndef __BL_SoundMetaViewer__SMVProcessYComputerPhasesTime__
#define __BL_SoundMetaViewer__SMVProcessYComputerPhasesTime__

#include <SMVProcessYComputer.h>

//
class Axis3DFactory2;
class SMVProcessYComputerPhasesTime : public SMVProcessYComputer
{
public:
    SMVProcessYComputerPhasesTime(Axis3DFactory2 *axisFactory);
    
    virtual ~SMVProcessYComputerPhasesTime();
    
    virtual void Reset(BL_FLOAT sampleRate) override {};
    
    void ComputeY(const WDL_TypedBuf<BL_FLOAT> magns[2],
                  const WDL_TypedBuf<BL_FLOAT> phases[2],
                  const WDL_TypedBuf<BL_FLOAT> phasesUnwrap[2],
                  WDL_TypedBuf<BL_FLOAT> *resultY) override;
    
    Axis3D *CreateAxis() override;
    
protected:
    Axis3DFactory2 *mAxisFactory;
};


#endif /* defined(__BL_SoundMetaViewer__SMVProcessYComputerPhasesTime__) */
