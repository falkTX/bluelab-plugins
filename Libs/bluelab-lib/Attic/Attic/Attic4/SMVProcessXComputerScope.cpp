//
//  SMVProcessXComputerScope.cpp
//  BL-SoundMetaViewer
//
//  Created by applematuer on 3/24/20.
//
//

#include <Axis3DFactory2.h>

#include "SMVProcessXComputerScope.h"

#if 0
NOTE: ComputePolarSamples() should be applied on samples, here it is applied on fft samples
#endif

SMVProcessXComputerScope::SMVProcessXComputerScope(Axis3DFactory2 *axisFactory)
{
    mAxisFactory = axisFactory;
}

SMVProcessXComputerScope::~SMVProcessXComputerScope() {}

void
SMVProcessXComputerScope::ComputeX(const WDL_TypedBuf<BL_FLOAT> samples[2],
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
        *isScalable = true;
    
    resultX->Resize(magns[0].GetSize());
    if (resultY != NULL)
        resultY->Resize(magns[0].GetSize());
    
    // Compute
    WDL_TypedBuf<BL_FLOAT> polarSamples[2];
    ComputePolarSamples(magns, polarSamples);
    
    for (int i = 0; i < magns[0].GetSize(); i++)
    {
        BL_FLOAT x = polarSamples[0].Get()[i];
        BL_FLOAT y = polarSamples[1].Get()[i];
        
        BL_FLOAT norm = std::sqrt(x*x + y*y);
        if (norm > 0.0)
        {
            BL_FLOAT normInv = 1.0/norm;
            x *= normInv;
            y *= normInv;
            
            //x /= norm;
            //y /= norm;
        }
        
        resultX->Get()[i] = x;
        
        if (resultY != NULL)
            resultY->Get()[i] = y;
    }
}

// See: https://dsp.stackexchange.com/questions/1671/calculate-phase-angle-between-two-signals-i-e-digital-phase-meter

#define CLIP_DISTANCE 0.95
#define CLIP_KEEP     1

// From USTProcess::ComputePolarSamples
void
SMVProcessXComputerScope::ComputePolarSamples(const WDL_TypedBuf<BL_FLOAT> samples[2],
                                              WDL_TypedBuf<BL_FLOAT> polarSamples[2])
{
    polarSamples[0].Resize(samples[0].GetSize());
    polarSamples[1].Resize(samples[0].GetSize());
    
    for (int i = 0; i < samples[0].GetSize(); i++)
    {
        BL_FLOAT l = samples[0].Get()[i];
        BL_FLOAT r = samples[1].Get()[i];
        
        BL_FLOAT angle = std::atan2(r, l);
        BL_FLOAT dist = std::sqrt(l*l + r*r);
        
        //dist *= 0.5;
        
        // Clip
        if (dist > CLIP_DISTANCE)
            // Point outside the circle
            // => set it ouside the graph
        {
#if !CLIP_KEEP
            // Discard
            polarSamples[0].Get()[i] = -1.0;
            polarSamples[1].Get()[i] = -1.0;
            
            continue;
#else
            // Keep in the border
            dist = CLIP_DISTANCE;
#endif
        }
        
        // Adjust
        angle = -angle;
        angle -= M_PI/4.0;
        
        // Compute (x, y)
        BL_FLOAT x = dist*cos(angle);
        BL_FLOAT y = dist*sin(angle);
        
#if 1 // NOTE: seems good !
        // Out of view samples add them by central symetry
        // Result: increase the directional perception (e.g when out of phase)
        if (y < 0.0)
        {
            x = -x;
            y = -y;
        }
#endif
        
        polarSamples[0].Get()[i] = x;
        polarSamples[1].Get()[i] = y;
    }
}

Axis3D *
SMVProcessXComputerScope::CreateAxis()
{
    Axis3D *axis = mAxisFactory->CreateLeftRightAxis(Axis3DFactory2::ORIENTATION_X);
    
    return axis;
}
