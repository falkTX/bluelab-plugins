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
//  SMVProcessYComputerChroma.cpp
//  BL-SoundMetaViewer
//
//  Created by applematuer on 3/24/20.
//
//

#ifdef IGRAPHICS_NANOVG

#include <BLUtils.h>

#include <Axis3DFactory2.h>

#include "SMVProcessYComputerChroma.h"


SMVProcessYComputerChroma::SMVProcessYComputerChroma(Axis3DFactory2 *axisFactory,
                                                     BL_FLOAT sampleRate)
{
    mAxisFactory = axisFactory;
    
    mSampleRate = sampleRate;
}

SMVProcessYComputerChroma::~SMVProcessYComputerChroma() {}

void
SMVProcessYComputerChroma::Reset(BL_FLOAT sampleRate)
{
    mSampleRate = sampleRate;
}

void
SMVProcessYComputerChroma::ComputeY(const WDL_TypedBuf<BL_FLOAT> magns[2],
                                    const WDL_TypedBuf<BL_FLOAT> phases[2],
                                    const WDL_TypedBuf<BL_FLOAT> phasesUnwrap[2],
                                    WDL_TypedBuf<BL_FLOAT> *resultY)
{
#define A_TUNE 440.0
    
    // Init
    resultY->Resize(magns[0].GetSize());
    
    WDL_TypedBuf<BL_FLOAT> chromaBins;
    BLUtils::BinsToChromaBins(magns[0].GetSize(), &chromaBins, mSampleRate, (BL_FLOAT)A_TUNE);
    
    // Compute
    BL_FLOAT sizeInv = 1.0;
    if (chromaBins.GetSize() > 1)
        sizeInv = 1.0/(chromaBins.GetSize() - 1);
    for (int i = 0; i < chromaBins.GetSize(); i++)
    {
        BL_FLOAT binVal = chromaBins.Get()[i];
        
        BL_FLOAT y = 0.0;
        if (chromaBins.GetSize() > 1)
            //y = binVal/(chromaBins.GetSize() - 1) - 0.25/*- 0.5*/;
            //y = binVal/(chromaBins.GetSize() - 1);
            y = binVal*sizeInv;
        
        resultY->Get()[i] = y;
    }
}

Axis3D *
SMVProcessYComputerChroma::CreateAxis()
{
    Axis3D *axis = mAxisFactory->CreateChromaAxis(Axis3DFactory2::ORIENTATION_Y);
    
    return axis;
}

#endif // IGRAPHICS_NANOVG
