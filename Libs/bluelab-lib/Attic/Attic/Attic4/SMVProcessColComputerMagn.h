//
//  SMVProcessColComputerMagn.h
//  BL-SoundMetaViewer
//
//  Created by applematuer on 3/24/20.
//
//

#ifndef __BL_SoundMetaViewer__SMVProcessColComputerMagn__
#define __BL_SoundMetaViewer__SMVProcessColComputerMagn__

#include <SMVProcessColComputer.h>

// Magnitudes
class SMVProcessColComputerMagn : public SMVProcessColComputer
{
public:
    SMVProcessColComputerMagn();
    
    virtual ~SMVProcessColComputerMagn();
    
    virtual void Reset(BL_FLOAT sampleRate) {};
    
    void ComputeCol(const WDL_TypedBuf<BL_FLOAT> magns[2],
                    const WDL_TypedBuf<BL_FLOAT> phases[2],
                    const WDL_TypedBuf<BL_FLOAT> phasesUnwrap[2],
                    WDL_TypedBuf<BL_FLOAT> *resultCol);
};

#endif /* defined(__BL_SoundMetaViewer__SMVProcessColComputerMagn__) */
