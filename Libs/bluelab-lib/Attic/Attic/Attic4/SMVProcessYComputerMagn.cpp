//
//  SMVProcessYComputerMagn.cpp
//  BL-SoundMetaViewer
//
//  Created by applematuer on 3/24/20.
//
//

#include <BLUtils.h>

#include <Axis3DFactory2.h>

#include "SMVProcessYComputerMagn.h"


SMVProcessYComputerMagn::SMVProcessYComputerMagn(Axis3DFactory2 *axisFactory)
{
    mAxisFactory = axisFactory;
}

SMVProcessYComputerMagn::~SMVProcessYComputerMagn() {}

void
SMVProcessYComputerMagn::ComputeY(const WDL_TypedBuf<BL_FLOAT> magns[2],
                                  const WDL_TypedBuf<BL_FLOAT> phases[2],
                                  const WDL_TypedBuf<BL_FLOAT> phasesUnwrap[2],
                                  WDL_TypedBuf<BL_FLOAT> *resultY)
{
    resultY->Resize(magns[0].GetSize());
    
    WDL_TypedBuf<BL_FLOAT> monoMagns;
    BLUtils::StereoToMono(&monoMagns, magns[0], magns[1]);
    
    // dB
    WDL_TypedBuf<BL_FLOAT> monoMagnsDB;
    BLUtils::AmpToDBNorm(&monoMagnsDB, monoMagns, (BL_FLOAT)1e-15, (BL_FLOAT)-120.0);
    monoMagns = monoMagnsDB;
    
    // Compute
    for (int i = 0; i < magns[0].GetSize(); i++)
    {
        BL_FLOAT y = monoMagns.Get()[i];
        
        resultY->Get()[i] = y;
    }
}

Axis3D *
SMVProcessYComputerMagn::CreateAxis()
{
    Axis3D *axis = mAxisFactory->CreateAmpDbAxis(Axis3DFactory2::ORIENTATION_Y);
    
    return axis;
}
