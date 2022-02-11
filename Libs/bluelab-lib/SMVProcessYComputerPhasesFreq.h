//
//  SMVProcessYComputerPhasesFreq.h
//  BL-SoundMetaViewer
//
//  Created by applematuer on 3/24/20.
//
//

#ifndef __BL_SoundMetaViewer__SMVProcessYComputerPhasesFreq__
#define __BL_SoundMetaViewer__SMVProcessYComputerPhasesFreq__

#include <SMVProcessYComputer.h>

//
class Axis3DFactory2;
class SMVProcessYComputerPhasesFreq : public SMVProcessYComputer
{
public:
    SMVProcessYComputerPhasesFreq(Axis3DFactory2 *axisFactory);
    
    virtual ~SMVProcessYComputerPhasesFreq();
    
    virtual void Reset(BL_FLOAT sampleRate) override {};
    
    void ComputeY(const WDL_TypedBuf<BL_FLOAT> magns[2],
                  const WDL_TypedBuf<BL_FLOAT> phases[2],
                  const WDL_TypedBuf<BL_FLOAT> phasesUnwrap[2],
                  WDL_TypedBuf<BL_FLOAT> *resultY) override;
    
    Axis3D *CreateAxis() override;
    
protected:
    Axis3DFactory2 *mAxisFactory;
};


#endif /* defined(__BL_SoundMetaViewer__SMVProcessYComputerPhasesFreq__) */
