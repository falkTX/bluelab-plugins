//
//  SMVProcessXComputerDiff.h
//  BL-SoundMetaViewer
//
//  Created by applematuer on 3/24/20.
//
//

#ifndef __BL_SoundMetaViewer__SMVProcessXComputerDiff__
#define __BL_SoundMetaViewer__SMVProcessXComputerDiff__

#include <SMVProcessXComputer.h>

// Polar vectorscope
class SMVProcessXComputerDiff : public SMVProcessXComputer
{
public:
    SMVProcessXComputerDiff();
    
    virtual ~SMVProcessXComputerDiff();
    
    virtual void Reset(BL_FLOAT sampleRate) {};
    
    void ComputeX(const WDL_TypedBuf<BL_FLOAT> samples[2],
                  const WDL_TypedBuf<BL_FLOAT> magns[2],
                  const WDL_TypedBuf<BL_FLOAT> phases[2],
                  WDL_TypedBuf<BL_FLOAT> *resultX,
                  WDL_TypedBuf<BL_FLOAT> *resultY = NULL,
                  bool *isPolar = NULL, BL_FLOAT polarCenter[2] = NULL,
                  bool *isScalable = NULL);
    
    // Dummy
    Axis3D *CreateAxis();
};

#endif /* defined(__BL_SoundMetaViewer__SMVProcessXComputerDiff__) */
