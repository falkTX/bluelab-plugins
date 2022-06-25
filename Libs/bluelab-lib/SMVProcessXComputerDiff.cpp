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
//  SMVProcessXComputerDiff.cpp
//  BL-SoundMetaViewer
//
//  Created by applematuer on 3/24/20.
//
//

#include <BLUtils.h>

#include "SMVProcessXComputerDiff.h"

#ifndef M_PI
#define M_PI 3.1415926535897932384
#endif


SMVProcessXComputerDiff::SMVProcessXComputerDiff() {}

SMVProcessXComputerDiff::~SMVProcessXComputerDiff() {}

void
SMVProcessXComputerDiff::ComputeX(const WDL_TypedBuf<BL_FLOAT> samples[2],
                                  const WDL_TypedBuf<BL_FLOAT> magns[2],
                                  const WDL_TypedBuf<BL_FLOAT> phases[2],
                                  WDL_TypedBuf<BL_FLOAT> *resultX,
                                  WDL_TypedBuf<BL_FLOAT> *resultY,
                                  bool *isPolar, BL_FLOAT polarCenter[2],
                                  bool *isScalable)
{
    // Init
    if (isPolar != NULL)
        *isPolar = true;
    
    if (polarCenter != NULL)
    {
        polarCenter[0] = 0.0;
        polarCenter[1] = 0.0;
    }
    
    if (isScalable != NULL)
        *isScalable = false;
    
    resultX->Resize(magns[0].GetSize());
    if (resultY != NULL)
        resultY->Resize(magns[0].GetSize());
    
#define MAGNS_DIFF_COEFF 1000.0
    WDL_TypedBuf<BL_FLOAT> diffMagns;
    BLUtils::ComputeDiff(&diffMagns, magns[0], magns[1]);
    BLUtils::MultValues(&diffMagns, (BL_FLOAT)(-MAGNS_DIFF_COEFF*M_PI/2.0));
    BLUtils::AddValues(&diffMagns, (BL_FLOAT)(M_PI/2.0));
    BLUtils::ClipMin(&diffMagns, (BL_FLOAT)0.0);
    BLUtils::ClipMax(&diffMagns, (BL_FLOAT)M_PI);
    
    WDL_TypedBuf<BL_FLOAT> thetas = diffMagns;
    
    for (int i = 0; i < magns[0].GetSize(); i++)
    {
        BL_FLOAT theta = thetas.Get()[i];
        
        BL_FLOAT x = std::cos(theta);
        BL_FLOAT y = std::sin(theta);
        
        // Avoid overbounds
        //x *= 0.5;
        
        resultX->Get()[i] = x;
        resultY->Get()[i] = y;
    }
}

Axis3D *
SMVProcessXComputerDiff::CreateAxis()
{
    return NULL; // TODO
}
