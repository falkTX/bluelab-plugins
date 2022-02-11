//
//  SMVProcessXComputerDiffFlat.cpp
//  BL-SoundMetaViewer
//
//  Created by applematuer on 3/24/20.
//
//

#include <SMVProcessXComputerDiff.h>
#include <BLUtils.h>

#include "SMVProcessXComputerDiffFlat.h"

SMVProcessXComputerDiffFlat::SMVProcessXComputerDiffFlat()
{
    mComputerScope = new SMVProcessXComputerDiff();
}

SMVProcessXComputerDiffFlat::~SMVProcessXComputerDiffFlat()
{
    delete mComputerScope;
}

void
SMVProcessXComputerDiffFlat::ComputeX(const WDL_TypedBuf<BL_FLOAT> samples[2],
                                      const WDL_TypedBuf<BL_FLOAT> magns[2],
                                      const WDL_TypedBuf<BL_FLOAT> phases[2],
                                      WDL_TypedBuf<BL_FLOAT> *resultX,
                                      WDL_TypedBuf<BL_FLOAT> *resultY,
                                      bool *isPolar, BL_FLOAT polarCenter[2],
                                      bool *isScalable)
{
    // Init
    if (isPolar != NULL)
        *isPolar = false;
    
    if (polarCenter != NULL)
    {
        polarCenter[0] = 0.0;
        polarCenter[1] = 0.0;
    }
    
    if (isScalable != NULL)
        *isScalable = false;
    
    resultX->Resize(magns[0].GetSize());
    
    if (resultY != NULL)
        resultY->Resize(0);
    
    bool dummyPolar;
    WDL_TypedBuf<BL_FLOAT> resultX0;
    WDL_TypedBuf<BL_FLOAT> resultY0;
    mComputerScope->ComputeX(samples, magns, phases,
                             &resultX0, &resultY0,
                             &dummyPolar);
    
    BLUtils::CartesianToPolarFlat(&resultX0, &resultY0);
    
    for (int i = 0; i < resultX0.GetSize(); i++)
    {
        BL_FLOAT x = resultX0.Get()[i];
        
        // Ivert
        x = -x;
        
        resultX0.Get()[i] = x;
    }
    
    *resultX = resultX0;
}

Axis3D *
SMVProcessXComputerDiffFlat::CreateAxis()
{
    return NULL; // TODO
}
