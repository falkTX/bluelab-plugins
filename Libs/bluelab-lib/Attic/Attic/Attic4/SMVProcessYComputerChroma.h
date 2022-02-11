//
//  SMVProcessYComputerChroma.h
//  BL-SoundMetaViewer
//
//  Created by applematuer on 3/24/20.
//
//

#ifndef __BL_SoundMetaViewer__SMVProcessYComputerChroma__
#define __BL_SoundMetaViewer__SMVProcessYComputerChroma__

#include <SMVProcessYComputer.h>

//
class Axis3DFactory2;
class SMVProcessYComputerChroma : public SMVProcessYComputer
{
public:
    SMVProcessYComputerChroma(Axis3DFactory2 *axisFactory, BL_FLOAT sampleRate);
    
    virtual ~SMVProcessYComputerChroma();
    
    virtual void Reset(BL_FLOAT sampleRate);
    
    void ComputeY(const WDL_TypedBuf<BL_FLOAT> magns[2],
                  const WDL_TypedBuf<BL_FLOAT> phases[2],
                  const WDL_TypedBuf<BL_FLOAT> phasesUnwrap[2],
                  WDL_TypedBuf<BL_FLOAT> *resultY);
    
    Axis3D *CreateAxis();
    
protected:
    Axis3DFactory2 *mAxisFactory;
    
    BL_FLOAT mSampleRate;
};

#endif /* defined(__BL_SoundMetaViewer__SMVProcessYComputerChroma__) */
