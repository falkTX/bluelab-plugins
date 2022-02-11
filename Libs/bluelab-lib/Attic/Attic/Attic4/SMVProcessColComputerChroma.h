//
//  SMVProcessYComputerChroma.h
//  BL-SoundMetaViewer
//
//  Created by applematuer on 3/24/20.
//
//

#ifndef __BL_SoundMetaViewer__SMVProcessColComputerChroma__
#define __BL_SoundMetaViewer__SMVProcessColComputerChroma__

#include <SMVProcessColComputer.h>

//
class SMVProcessColComputerChroma : public SMVProcessColComputer
{
public:
    SMVProcessColComputerChroma(BL_FLOAT sampleRate);
    
    virtual ~SMVProcessColComputerChroma();
    
    virtual void Reset(BL_FLOAT sampleRate);
    
    void ComputeCol(const WDL_TypedBuf<BL_FLOAT> magns[2],
                    const WDL_TypedBuf<BL_FLOAT> phases[2],
                    const WDL_TypedBuf<BL_FLOAT> phasesUnwrap[2],
                    WDL_TypedBuf<BL_FLOAT> *resultCol);
    
protected:
    BL_FLOAT mSampleRate;
};

#endif /* defined(__BL_SoundMetaViewer__SMVProcessYComputerChroma__) */
