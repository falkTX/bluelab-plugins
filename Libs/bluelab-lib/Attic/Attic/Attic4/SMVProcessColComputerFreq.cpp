//
//  SMVProcessColComputerFreq.cpp
//  BL-SoundMetaViewer
//
//  Created by applematuer on 3/24/20.
//
//

#include <BLUtils.h>

#include "SMVProcessColComputerFreq.h"

#define LOG_SCALE_FACTOR 64.0 //128.0


SMVProcessColComputerFreq::SMVProcessColComputerFreq() {}

SMVProcessColComputerFreq::~SMVProcessColComputerFreq() {}

void
SMVProcessColComputerFreq::ComputeCol(const WDL_TypedBuf<BL_FLOAT> magns[2],
                                      const WDL_TypedBuf<BL_FLOAT> phases[2],
                                      const WDL_TypedBuf<BL_FLOAT> phasesUnwrap[2],
                                      WDL_TypedBuf<BL_FLOAT> *resultCol)
{
    // Init
    resultCol->Resize(magns[0].GetSize());
    
    // Compute
    for (int i = 0; i < magns[0].GetSize(); i++)
    {
        BL_FLOAT col = 0.0;
        if (magns[0].GetSize() > 1)
            //col = ((BL_FLOAT)i)/(magns[0].GetSize() - 1) - 0.5;
            col = ((BL_FLOAT)i)/(magns[0].GetSize() - 1);
            
        resultCol->Get()[i] = col;
    }
    
#if 1 // Log scale
    //BLUtils::AddValues(resultCol, 0.5);
    BLUtils::LogScaleNorm2(resultCol, (BL_FLOAT)LOG_SCALE_FACTOR);
    //BLUtils::AddValues(resultCol, -0.5);
#endif
}
