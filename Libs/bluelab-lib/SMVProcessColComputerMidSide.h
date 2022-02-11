//
//  SMVProcessColComputerMidSide.h
//  BL-SoundMetaViewer
//
//  Created by applematuer on 3/24/20.
//
//

#ifndef __BL_SoundMetaViewer__SMVProcessColComputerMidSide__
#define __BL_SoundMetaViewer__SMVProcessColComputerMidSide__

#include <SMVProcessColComputer.h>

// Magnitudes
class SMVProcessColComputerMidSide : public SMVProcessColComputer
{
public:
    SMVProcessColComputerMidSide();
    
    virtual ~SMVProcessColComputerMidSide();
    
    virtual void Reset(BL_FLOAT sampleRate) override {};
    
    void ComputeCol(const WDL_TypedBuf<BL_FLOAT> magns[2],
                    const WDL_TypedBuf<BL_FLOAT> phases[2],
                    const WDL_TypedBuf<BL_FLOAT> phasesUnwrap[2],
                    WDL_TypedBuf<BL_FLOAT> *resultCol) override;
    
protected:
    void ComputeMidSide(const WDL_TypedBuf<BL_FLOAT> magns[2],
                        WDL_TypedBuf<BL_FLOAT> *ms);

};

#endif /* defined(__BL_SoundMetaViewer__SMVProcessColComputerMidSide__) */
