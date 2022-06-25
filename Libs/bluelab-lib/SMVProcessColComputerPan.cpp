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
//  SMVProcessColComputerPan.cpp
//  BL-SoundMetaViewer
//
//  Created by applematuer on 3/24/20.
//
//

#include <BLUtils.h>
#include <BLUtilsMath.h>

#include "SMVProcessColComputerPan.h"


SMVProcessColComputerPan::SMVProcessColComputerPan() {}

SMVProcessColComputerPan::~SMVProcessColComputerPan() {}

void
SMVProcessColComputerPan::ComputeCol(const WDL_TypedBuf<BL_FLOAT> magns[2],
                                      const WDL_TypedBuf<BL_FLOAT> phases[2],
                                      const WDL_TypedBuf<BL_FLOAT> phasesUnwrap[2],
                                      WDL_TypedBuf<BL_FLOAT> *resultCol)
{
    resultCol->Resize(magns[0].GetSize());
    
    WDL_TypedBuf<BL_FLOAT> pans;
    ComputePans(magns, &pans);
    
    // dB
    //WDL_TypedBuf<BL_FLOAT> pansDB;
    //BLUtils::AmpToDBNorm(&pansDB, pans, 1e-15, -120.0);
    //pans = pansDB;
    
    // Compute
    for (int i = 0; i < magns[0].GetSize(); i++)
    {
        BL_FLOAT pan = pans.Get()[i];
        
        resultCol->Get()[i] = pan;
    }
}

void
SMVProcessColComputerPan::ComputePans(const WDL_TypedBuf<BL_FLOAT> magns[2],
                                      WDL_TypedBuf<BL_FLOAT> *pans)
{
    pans->Resize(magns[0].GetSize());
    
    for (int i = 0; i < magns[0].GetSize(); i++)
    {
        BL_FLOAT l = magns[0].Get()[i];
        BL_FLOAT r = magns[1].Get()[i];
        
        BL_FLOAT angle = std::atan2(r, l);
        
        // Adjust
        angle = -angle;
        angle -= M_PI/4.0;
        
        if (angle < 0.0)
            angle += M_PI;
        
        if (angle > M_PI)
            angle -= M_PI;
        
        // Normalize angle
        angle = angle*(1.0/M_PI);
        
        pans->Get()[i] = angle;
    }
}
