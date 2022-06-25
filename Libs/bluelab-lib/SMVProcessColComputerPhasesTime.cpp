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
//  SMVProcessColComputerPhasesTime.cpp
//  BL-SoundMetaViewer
//
//  Created by applematuer on 3/24/20.
//
//

#include <BLUtils.h>

#include "SMVProcessColComputerPhasesTime.h"

SMVProcessColComputerPhasesTime::SMVProcessColComputerPhasesTime() {}

SMVProcessColComputerPhasesTime::~SMVProcessColComputerPhasesTime() {}

void
SMVProcessColComputerPhasesTime::ComputeCol(const WDL_TypedBuf<BL_FLOAT> magns[2],
                                            const WDL_TypedBuf<BL_FLOAT> phases[2],
                                            const WDL_TypedBuf<BL_FLOAT> phasesUnwrap[2],
                                            WDL_TypedBuf<BL_FLOAT> *resultCol)
{
    // Init
    resultCol->Resize(phasesUnwrap[1].GetSize());
    
    // Compute
    for (int i = 0; i < phasesUnwrap[1].GetSize(); i++)
    {
        // phases[1] time should be already normalized!
        BL_FLOAT col = phasesUnwrap[1].Get()[i];
        resultCol->Get()[i] = col;
    }
}
