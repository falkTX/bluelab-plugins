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
//  SMVProcessXComputerLissajous.cpp
//  BL-SoundMetaViewer
//
//  Created by applematuer on 3/24/20.
//
//

#ifdef IGRAPHICS_NANOVG

#include <BLUtils.h>
#include <BLUtilsMath.h>

#include <Axis3DFactory2.h>

#include "SMVProcessXComputerLissajous.h"

#if 0
NOTE: ComputePolarSamples() should be applied on samples, here it is applied on fft samples
#endif

#define SQR2_INV 0.70710678118655

// Lissajous
#define LISSAJOUS_CLIP_DISTANCE 1.0

SMVProcessXComputerLissajous::SMVProcessXComputerLissajous(Axis3DFactory2 *axisFactory)
{
    mAxisFactory = axisFactory;
}

SMVProcessXComputerLissajous::~SMVProcessXComputerLissajous() {}

void
SMVProcessXComputerLissajous::ComputeX(const WDL_TypedBuf<BL_FLOAT> samples[2],
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
        polarCenter[1] = 0.0; //0.5;
    }
    
    if (isScalable != NULL)
        *isScalable = false;
    
    resultX->Resize(magns[0].GetSize());
    if (resultY != NULL)
        resultY->Resize(magns[0].GetSize());
    
    // Compute
    WDL_TypedBuf<BL_FLOAT> lissajousSamples[2];
    ComputeLissajous(samples, lissajousSamples, true);
    
    // Warning: samples size is twice the magns size
    // So take only half of the samples
    // (otherwise we would have no display after)
    for (int i = 0; i < magns[0].GetSize(); i++)
    {
        BL_FLOAT x = lissajousSamples[0].Get()[i];
        BL_FLOAT y = lissajousSamples[1].Get()[i];
        
        resultX->Get()[i] = x;
        
        if (resultY != NULL)
            resultY->Get()[i] = y;
    }
    
    // We need less fat in x
    BLUtils::MultValues(resultX, (BL_FLOAT)0.5);
    
    // We need positive y
    BLUtils::AddValues(resultY, (BL_FLOAT)1.0);
    BLUtils::MultValues(resultY, (BL_FLOAT)0.5);
    
    //BLDebug::DumpData("x.txt", *resultX);
    //BLDebug::DumpData("y.txt", *resultY);
}

Axis3D *
SMVProcessXComputerLissajous::CreateAxis()
{
    Axis3D *axis = mAxisFactory->CreateLeftRightAxis(Axis3DFactory2::ORIENTATION_X);
    
    return axis;
}

// From USTProcess
void
SMVProcessXComputerLissajous::ComputeLissajous(const WDL_TypedBuf<BL_FLOAT> samples[2],
                                               WDL_TypedBuf<BL_FLOAT> lissajousSamples[2],
                                               bool fitInSquare)
{
    lissajousSamples[0].Resize(samples[0].GetSize());
    lissajousSamples[1].Resize(samples[0].GetSize());
    
    for (int i = 0; i < samples[0].GetSize(); i++)
    {
        BL_FLOAT l = samples[0].Get()[i];
        BL_FLOAT r = samples[1].Get()[i];
        
        if (fitInSquare)
        {
            if (l < -LISSAJOUS_CLIP_DISTANCE)
                l = -LISSAJOUS_CLIP_DISTANCE;
            if (l > LISSAJOUS_CLIP_DISTANCE)
                l = LISSAJOUS_CLIP_DISTANCE;
            
            if (r < -LISSAJOUS_CLIP_DISTANCE)
                r = -LISSAJOUS_CLIP_DISTANCE;
            if (r > LISSAJOUS_CLIP_DISTANCE)
                r = LISSAJOUS_CLIP_DISTANCE;
        }
        
        // Rotate
        BL_FLOAT angle = std::atan2(r, l);
        BL_FLOAT dist = std::sqrt(l*l + r*r);
        
        if (fitInSquare)
        {
            BL_FLOAT coeff = SQR2_INV;
            dist *= coeff;
        }
        
        angle = -angle;
        angle -= M_PI/4.0;
        
        BL_FLOAT x = dist*cos(angle);
        BL_FLOAT y = dist*sin(angle);
        
#if 0 // Keep [-1, 1] bounds
        // Center
        x = (x + 1.0)*0.5;
        y = (y + 1.0)*0.5;
#endif
        
        lissajousSamples[0].Get()[i] = x;
        lissajousSamples[1].Get()[i] = y;
    }
}

#endif // IGRAPHICS_NANOVG
