//
//  SMVProcessXComputerFreq.cpp
//  BL-SoundMetaViewer
//
//  Created by applematuer on 3/24/20.
//
//

#ifdef IGRAPHICS_NANOVG

#include <BLUtils.h>

#include <Axis3DFactory2.h>

#include "SMVProcessXComputerFreq.h"


#define LOG_SCALE_FACTOR 64.0 //128.0


SMVProcessXComputerFreq::SMVProcessXComputerFreq(Axis3DFactory2 *axisFactory)
{
    mAxisFactory = axisFactory;
}

SMVProcessXComputerFreq::~SMVProcessXComputerFreq() {}

void
SMVProcessXComputerFreq::ComputeX(const WDL_TypedBuf<BL_FLOAT> samples[2],
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
    
    // Compute
    BL_FLOAT sizeInv = 1.0;
    if (magns[0].GetSize() > 1)
        sizeInv = 1.0/(magns[0].GetSize() - 1); //
    for (int i = 0; i < magns[0].GetSize(); i++)
    {
        BL_FLOAT x = 0.0;
        if (magns[0].GetSize() > 1)
            //x = ((BL_FLOAT)i)/(magns[0].GetSize() - 1) - 0.5;
            x = ((BL_FLOAT)i)*sizeInv - 0.5;
        
        resultX->Get()[i] = x;
    }
    
#if 1 // Log scale
    BLUtils::AddValues(resultX, (BL_FLOAT)0.5);
    BLUtils::LogScaleNorm2(resultX, (BL_FLOAT)LOG_SCALE_FACTOR);
    BLUtils::AddValues(resultX, (BL_FLOAT)-0.5);
#endif
}

Axis3D *
SMVProcessXComputerFreq::CreateAxis()
{
    Axis3D *axis = mAxisFactory->CreateFreqAxis(Axis3DFactory2::ORIENTATION_X);
    
    return axis;
}

#endif // IGRAPHICS_NANOVG
