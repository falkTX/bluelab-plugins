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
//  SMVProcessYComputerPhasesTime.cpp
//  BL-SoundMetaViewer
//
//  Created by applematuer on 3/24/20.
//
//

#ifdef IGRAPHICS_NANOVG

#include <BLUtils.h>

#include <Axis3DFactory2.h>

#include "SMVProcessYComputerPhasesTime.h"

SMVProcessYComputerPhasesTime::SMVProcessYComputerPhasesTime(Axis3DFactory2 *axisFactory)
{
    mAxisFactory = axisFactory;
}

SMVProcessYComputerPhasesTime::~SMVProcessYComputerPhasesTime() {}

void
SMVProcessYComputerPhasesTime::ComputeY(const WDL_TypedBuf<BL_FLOAT> magns[2],
                                        const WDL_TypedBuf<BL_FLOAT> phases[2],
                                        const WDL_TypedBuf<BL_FLOAT> phasesUnwrap[2],
                                        WDL_TypedBuf<BL_FLOAT> *resultY)
{
    // Init
    resultY->Resize(phasesUnwrap[1].GetSize());
    
    // Compute
    for (int i = 0; i < phasesUnwrap[1].GetSize(); i++)
    {
        // phases[1] time should be already normalized!
        BL_FLOAT y = phasesUnwrap[1].Get()[i];
        resultY->Get()[i] = y;
    }
}

Axis3D *
SMVProcessYComputerPhasesTime::CreateAxis()
{
    Axis3D *axis = mAxisFactory->CreatePercentAxis(Axis3DFactory2::ORIENTATION_Y);
    
    return axis;
}

#endif // IGRAPHICS_NANOVG
