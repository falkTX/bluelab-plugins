/* Copyright (C) 2022 Nicolas Dittlo <deadlab.plugins@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this software; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
 
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
