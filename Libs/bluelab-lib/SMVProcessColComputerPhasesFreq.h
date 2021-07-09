//
//  SMVProcessColComputerPhasesFreq.h
//  BL-SoundMetaViewer
//
//  Created by applematuer on 3/24/20.
//
//

#ifndef __BL_SoundMetaViewer__SMVProcessColComputerPhasesFreq__
#define __BL_SoundMetaViewer__SMVProcessColComputerPhasesFreq__

#include <SMVProcessColComputer.h>

//
class SMVProcessColComputerPhasesFreq : public SMVProcessColComputer
{
public:
    SMVProcessColComputerPhasesFreq();
    
    virtual ~SMVProcessColComputerPhasesFreq();
    
    virtual void Reset(BL_FLOAT sampleRate) override {};
    
    void ComputeCol(const WDL_TypedBuf<BL_FLOAT> magns[2],
                    const WDL_TypedBuf<BL_FLOAT> phases[2],
                    const WDL_TypedBuf<BL_FLOAT> phasesUnwrap[2],
                    WDL_TypedBuf<BL_FLOAT> *resultCol) override;
};


#endif /* defined(__BL_SoundMetaViewer__SMVProcessColComputerPhasesFreq__) */
