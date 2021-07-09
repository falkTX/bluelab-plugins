//
//  SMVProcessColComputerPan.h
//  BL-SoundMetaViewer
//
//  Created by applematuer on 3/24/20.
//
//

#ifndef __BL_SoundMetaViewer__SMVProcessColComputerPan__
#define __BL_SoundMetaViewer__SMVProcessColComputerPan__

#include <SMVProcessColComputer.h>

// Magnitudes
class SMVProcessColComputerPan : public SMVProcessColComputer
{
public:
    SMVProcessColComputerPan();
    
    virtual ~SMVProcessColComputerPan();
    
    virtual void Reset(BL_FLOAT sampleRate) override {};
    
    void ComputeCol(const WDL_TypedBuf<BL_FLOAT> magns[2],
                    const WDL_TypedBuf<BL_FLOAT> phases[2],
                    const WDL_TypedBuf<BL_FLOAT> phasesUnwrap[2],
                    WDL_TypedBuf<BL_FLOAT> *resultCol) override;
    
protected:
    void ComputePans(const WDL_TypedBuf<BL_FLOAT> magns[2],
                     WDL_TypedBuf<BL_FLOAT> *pans);

};

#endif /* defined(__BL_SoundMetaViewer__SMVProcessColComputerPan__) */
