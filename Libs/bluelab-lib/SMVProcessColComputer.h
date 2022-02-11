//
//  SMVProcessYComputer.h
//  BL-SoundMetaViewer
//
//  Created by applematuer on 3/24/20.
//
//

#ifndef __BL_SoundMetaViewer__SMVProcessColComputer__
#define __BL_SoundMetaViewer__SMVProcessColComputer__

#include "IPlug_include_in_plug_hdr.h"

class SMVProcessColComputer
{
public:
    virtual ~SMVProcessColComputer() {};
    
    // Return the bormalized color.
    virtual void ComputeCol(const WDL_TypedBuf<BL_FLOAT> magns[2],
                            const WDL_TypedBuf<BL_FLOAT> phases[2],
                            const WDL_TypedBuf<BL_FLOAT> phasesUnwrap[2],
                            WDL_TypedBuf<BL_FLOAT> *resultCol) = 0;
    
    virtual void Reset(BL_FLOAT sampleRate) = 0;
};

#endif /* defined(__BL_SoundMetaViewer__SMVProcessColComputer__) */
