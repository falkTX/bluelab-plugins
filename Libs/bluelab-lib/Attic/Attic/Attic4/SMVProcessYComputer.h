//
//  SMVProcessYComputer.h
//  BL-SoundMetaViewer
//
//  Created by applematuer on 3/24/20.
//
//

#ifndef __BL_SoundMetaViewer__SMVProcessYComputer__
#define __BL_SoundMetaViewer__SMVProcessYComputer__

#include "IPlug_include_in_plug_hdr.h"

class Axis3D;

class SMVProcessYComputer
{
public:
    virtual ~SMVProcessYComputer() {};
    
    virtual void Reset(BL_FLOAT sampleRate) = 0;
    
    // Return the y coordinates.
    virtual void ComputeY(const WDL_TypedBuf<BL_FLOAT> magns[2],
                          const WDL_TypedBuf<BL_FLOAT> phases[2],
                          const WDL_TypedBuf<BL_FLOAT> phasesUnwrap[2],
                          WDL_TypedBuf<BL_FLOAT> *resultY) = 0;
    
    virtual Axis3D *CreateAxis() = 0;
};

#endif /* defined(__BL_SoundMetaViewer__SMVProcessYComputer__) */
