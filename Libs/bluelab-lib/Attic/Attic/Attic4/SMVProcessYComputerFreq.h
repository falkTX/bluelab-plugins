//
//  SMVProcessYComputerFreq.h
//  BL-SoundMetaViewer
//
//  Created by applematuer on 3/24/20.
//
//

#ifndef __BL_SoundMetaViewer__SMVProcessYComputerFreq__
#define __BL_SoundMetaViewer__SMVProcessYComputerFreq__

#include <SMVProcessYComputer.h>

//
class Axis3DFactory2;
class SMVProcessYComputerFreq : public SMVProcessYComputer
{
public:
    SMVProcessYComputerFreq(Axis3DFactory2 *axisFactory);
    
    virtual ~SMVProcessYComputerFreq();
    
    virtual void Reset(BL_FLOAT sampleRate) {};
    
    void ComputeY(const WDL_TypedBuf<BL_FLOAT> magns[2],
                  const WDL_TypedBuf<BL_FLOAT> phases[2],
                  const WDL_TypedBuf<BL_FLOAT> phasesUnwrap[2],
                  WDL_TypedBuf<BL_FLOAT> *resultY);
    
     Axis3D *CreateAxis();
    
protected:
    Axis3DFactory2 *mAxisFactory;
};


#endif /* defined(__BL_SoundMetaViewer__SMVProcessYComputerFreq__) */
