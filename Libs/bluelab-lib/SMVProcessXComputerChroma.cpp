//
//  SMVProcessXComputerChroma.cpp
//  BL-SoundMetaViewer
//
//  Created by applematuer on 3/24/20.
//
//

#ifdef IGRAPHICS_NANOVG

#include <BLUtils.h>

#include <Axis3DFactory2.h>

#include "SMVProcessXComputerChroma.h"


SMVProcessXComputerChroma::SMVProcessXComputerChroma(Axis3DFactory2 *axisFactory,
                                                     BL_FLOAT sampleRate)
{
    mAxisFactory = axisFactory;
    
    mSampleRate = sampleRate;
}

SMVProcessXComputerChroma::~SMVProcessXComputerChroma() {}

void
SMVProcessXComputerChroma::Reset(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
}

void
SMVProcessXComputerChroma::ComputeX(const WDL_TypedBuf<BL_FLOAT> samples[2],
                                    const WDL_TypedBuf<BL_FLOAT> magns[2],
                                    const WDL_TypedBuf<BL_FLOAT> phases[2],
                                    WDL_TypedBuf<BL_FLOAT> *resultX,
                                    WDL_TypedBuf<BL_FLOAT> *resultY,
                                    bool *isPolar, BL_FLOAT polarCenter[2],
                                    bool *isScalable)
{
#define A_TUNE 440.0
    
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
    
    WDL_TypedBuf<BL_FLOAT> chromaBins;
    BLUtils::BinsToChromaBins(magns[0].GetSize(), &chromaBins, mSampleRate, (BL_FLOAT)A_TUNE);
    
    // Compute
    BL_FLOAT sizeInv = 1.0;
    if (chromaBins.GetSize() > 1)
        sizeInv = 1.0/(chromaBins.GetSize() - 1);
    for (int i = 0; i < chromaBins.GetSize(); i++)
    {
        BL_FLOAT binVal = chromaBins.Get()[i];
        
        BL_FLOAT x = 0.0;
        if (chromaBins.GetSize() > 1)
            //x = binVal/(chromaBins.GetSize() - 1) - 0.5;
            x = binVal*sizeInv - 0.5;
        
        resultX->Get()[i] = x;
    }
}

Axis3D *
SMVProcessXComputerChroma::CreateAxis()
{
    Axis3D *axis = mAxisFactory->CreateChromaAxis(Axis3DFactory2::ORIENTATION_X);
    
    return axis;
}

#endif // IGRAPHICS_NANOVG
