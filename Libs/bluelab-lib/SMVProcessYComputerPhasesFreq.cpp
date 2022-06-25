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
//  SMVProcessYComputerPhasesFreq.cpp
//  BL-SoundMetaViewer
//
//  Created by applematuer on 3/24/20.
//
//

#ifdef IGRAPHICS_NANOVG

#include <BLUtils.h>

#include <Axis3DFactory2.h>

#include "SMVProcessYComputerPhasesFreq.h"

#define LOG_SCALE_FACTOR 64.0 //128.0

SMVProcessYComputerPhasesFreq::SMVProcessYComputerPhasesFreq(Axis3DFactory2 *axisFactory)
{
    mAxisFactory = axisFactory;
}

SMVProcessYComputerPhasesFreq::~SMVProcessYComputerPhasesFreq() {}

void
SMVProcessYComputerPhasesFreq::ComputeY(const WDL_TypedBuf<BL_FLOAT> magns[2],
                                        const WDL_TypedBuf<BL_FLOAT> phases[2],
                                        const WDL_TypedBuf<BL_FLOAT> phasesUnwrap[2],
                                        WDL_TypedBuf<BL_FLOAT> *resultY)
{
    // Init
    resultY->Resize(phasesUnwrap[0].GetSize());
    
    // Compute
    for (int i = 0; i < phasesUnwrap[0].GetSize(); i++)
    {
        //BL_FLOAT y = phases[0].Get()[i]/(2.0*M_PI);
        
        // Phases are already normalized here
        BL_FLOAT y = phasesUnwrap[0].Get()[i];
        
        resultY->Get()[i] = y;
    }
    
#if 0 //1 // Log scale
    BLUtils::LogScaleNorm2(resultY, LOG_SCALE_FACTOR);
#endif
}

Axis3D *
SMVProcessYComputerPhasesFreq::CreateAxis()
{
    Axis3D *axis = mAxisFactory->CreatePercentAxis(Axis3DFactory2::ORIENTATION_Y);
    
    return axis;
}

#endif // IGRAPHICS_NANOVG
