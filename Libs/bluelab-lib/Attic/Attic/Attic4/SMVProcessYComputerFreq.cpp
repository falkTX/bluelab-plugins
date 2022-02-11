//
//  SMVProcessYComputerFreq.cpp
//  BL-SoundMetaViewer
//
//  Created by applematuer on 3/24/20.
//
//

#include <BLUtils.h>

#include <Axis3DFactory2.h>

#include "SMVProcessYComputerFreq.h"

#define LOG_SCALE_FACTOR 64.0 //128.0

SMVProcessYComputerFreq::SMVProcessYComputerFreq(Axis3DFactory2 *axisFactory)
{
    mAxisFactory = axisFactory;
}

SMVProcessYComputerFreq::~SMVProcessYComputerFreq() {}

void
SMVProcessYComputerFreq::ComputeY(const WDL_TypedBuf<BL_FLOAT> magns[2],
                                  const WDL_TypedBuf<BL_FLOAT> phases[2],
                                  const WDL_TypedBuf<BL_FLOAT> phasesUnwrap[2],
                                  WDL_TypedBuf<BL_FLOAT> *resultY)
{
    // Init
    resultY->Resize(magns[0].GetSize());
    
    // Compute
    for (int i = 0; i < magns[0].GetSize(); i++)
    {
        BL_FLOAT y = 0.0;
        if (magns[0].GetSize() > 1)
            y = ((BL_FLOAT)i)/(magns[0].GetSize() - 1) /*- 0.5*/;
        
        resultY->Get()[i] = y;
    }
    
#if 1 // Log scale
    BLUtils::LogScaleNorm2(resultY, (BL_FLOAT)LOG_SCALE_FACTOR);
#endif
}

Axis3D *
SMVProcessYComputerFreq::CreateAxis()
{
    Axis3D *axis = mAxisFactory->CreateFreqAxis(Axis3DFactory2::ORIENTATION_Y);
    
    return axis;
}
