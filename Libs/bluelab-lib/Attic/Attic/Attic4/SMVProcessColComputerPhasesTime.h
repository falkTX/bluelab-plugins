//
//  SMVProcessColComputerPhasesTime.h
//  BL-SoundMetaViewer
//
//  Created by applematuer on 3/24/20.
//
//

#ifndef __BL_SoundMetaViewer__SMVProcessColComputerPhasesTime__
#define __BL_SoundMetaViewer__SMVProcessColComputerPhasesTime__

#include <SMVProcessColComputer.h>

//
class SMVProcessColComputerPhasesTime : public SMVProcessColComputer
{
public:
    SMVProcessColComputerPhasesTime();
    
    virtual ~SMVProcessColComputerPhasesTime();
    
    virtual void Reset(BL_FLOAT sampleRate) {};
    
    void ComputeCol(const WDL_TypedBuf<BL_FLOAT> magns[2],
                    const WDL_TypedBuf<BL_FLOAT> phases[2],
                    const WDL_TypedBuf<BL_FLOAT> phasesUnwrap[2],
                    WDL_TypedBuf<BL_FLOAT> *resultCol);
};


#endif /* defined(__BL_SoundMetaViewer__SMVProcessColComputerPhasesTime__) */
