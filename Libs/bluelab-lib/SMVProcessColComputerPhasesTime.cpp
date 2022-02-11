//
//  SMVProcessColComputerPhasesTime.cpp
//  BL-SoundMetaViewer
//
//  Created by applematuer on 3/24/20.
//
//

#include <BLUtils.h>

#include "SMVProcessColComputerPhasesTime.h"

SMVProcessColComputerPhasesTime::SMVProcessColComputerPhasesTime() {}

SMVProcessColComputerPhasesTime::~SMVProcessColComputerPhasesTime() {}

void
SMVProcessColComputerPhasesTime::ComputeCol(const WDL_TypedBuf<BL_FLOAT> magns[2],
                                            const WDL_TypedBuf<BL_FLOAT> phases[2],
                                            const WDL_TypedBuf<BL_FLOAT> phasesUnwrap[2],
                                            WDL_TypedBuf<BL_FLOAT> *resultCol)
{
    // Init
    resultCol->Resize(phasesUnwrap[1].GetSize());
    
    // Compute
    for (int i = 0; i < phasesUnwrap[1].GetSize(); i++)
    {
        // phases[1] time should be already normalized!
        BL_FLOAT col = phasesUnwrap[1].Get()[i];
        resultCol->Get()[i] = col;
    }
}
