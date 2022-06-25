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
//  SMVProcessXComputerScopeFlat.cpp
//  BL-SoundMetaViewer
//
//  Created by applematuer on 3/24/20.
//
//

#ifdef IGRAPHICS_NANOVG

#include <SMVProcessXComputerScope.h>

#include <BLUtils.h>

#include <Axis3DFactory2.h>

#include "SMVProcessXComputerScopeFlat.h"

#ifndef M_PI
#define M_PI 3.1415926535897932384
#endif

SMVProcessXComputerScopeFlat::SMVProcessXComputerScopeFlat(Axis3DFactory2 *axisFactory)
{
    mAxisFactory = axisFactory;
    
    mComputerScope = new SMVProcessXComputerScope(axisFactory);
}

SMVProcessXComputerScopeFlat::~SMVProcessXComputerScopeFlat()
{
    delete mComputerScope;
}

void
SMVProcessXComputerScopeFlat::ComputeX(const WDL_TypedBuf<BL_FLOAT> samples[2],
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
    
    bool dummyPolar;
    WDL_TypedBuf<BL_FLOAT> resultX0;
    WDL_TypedBuf<BL_FLOAT> resultY0;
    mComputerScope->ComputeX(samples, magns, phases,
                             &resultX0, &resultY0,
                             &dummyPolar);
    
#if 0
    // (resultX0, resultY0) dir is normalized
    for (int i = 0; i < magns[0].GetSize(); i++)
    {
        BL_FLOAT x = resultX0.Get()[i];
        BL_FLOAT angle = acos(x);
        
        BL_FLOAT normAngle = (angle/M_PI) - 0.5;
        
        resultX->Get()[i] = normAngle;
    }
#endif
    
    BLUtils::CartesianToPolarFlat(&resultX0, &resultY0);
    
    for (int i = 0; i < resultX0.GetSize(); i++)
    {
        BL_FLOAT x = resultX0.Get()[i];
        
        // Ivert
        x = -x;
        
        resultX0.Get()[i] = x;
    }
    
    *resultX = resultX0;
}

Axis3D *
SMVProcessXComputerScopeFlat::CreateAxis()
{
    Axis3D *axis = mAxisFactory->CreateAngleAxis(Axis3DFactory2::ORIENTATION_X);
    
    return axis;
}

#endif // IGRAPHICS_NANOVG
