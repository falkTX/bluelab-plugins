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
//  SMVProcessColComputerFreq.cpp
//  BL-SoundMetaViewer
//
//  Created by applematuer on 3/24/20.
//
//

#include <BLUtils.h>

#include "SMVProcessColComputerFreq.h"

#define LOG_SCALE_FACTOR 64.0 //128.0


SMVProcessColComputerFreq::SMVProcessColComputerFreq() {}

SMVProcessColComputerFreq::~SMVProcessColComputerFreq() {}

void
SMVProcessColComputerFreq::ComputeCol(const WDL_TypedBuf<BL_FLOAT> magns[2],
                                      const WDL_TypedBuf<BL_FLOAT> phases[2],
                                      const WDL_TypedBuf<BL_FLOAT> phasesUnwrap[2],
                                      WDL_TypedBuf<BL_FLOAT> *resultCol)
{
    // Init
    resultCol->Resize(magns[0].GetSize());
    
    // Compute
    for (int i = 0; i < magns[0].GetSize(); i++)
    {
        BL_FLOAT col = 0.0;
        if (magns[0].GetSize() > 1)
            //col = ((BL_FLOAT)i)/(magns[0].GetSize() - 1) - 0.5;
            col = ((BL_FLOAT)i)/(magns[0].GetSize() - 1);
            
        resultCol->Get()[i] = col;
    }
    
#if 1 // Log scale
    //BLUtils::AddValues(resultCol, 0.5);
    BLUtils::LogScaleNorm2(resultCol, (BL_FLOAT)LOG_SCALE_FACTOR);
    //BLUtils::AddValues(resultCol, -0.5);
#endif
}
