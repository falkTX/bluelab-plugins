//
//  BLVectorscopeProcess.cpp
//  UST
//
//  Created by applematuer on 7/28/19.
//
//

#include <FftProcessObj16.h>

#include <BLUtils.h>
#include <BLUtilsMath.h>

#include "BLVectorscopeProcess.h"

#ifndef M_PI
#define M_PI 3.141592653589
#endif

#define PI_DIV4 0.785398163397448
#define SQR2_INV 0.70710678118655

// Polar samples
//
#define CLIP_DISTANCE 0.95
#define CLIP_KEEP     1

// Lissajous
#define LISSAJOUS_CLIP_DISTANCE 1.0


// See: https://dsp.stackexchange.com/questions/1671/calculate-phase-angle-between-two-signals-i-e-digital-phase-meter
void
BLVectorscopeProcess::ComputePolarSamples(const WDL_TypedBuf<BL_FLOAT> samples[2],
                                          WDL_TypedBuf<BL_FLOAT> polarSamples[2])
{
    polarSamples[0].Resize(samples[0].GetSize());
    polarSamples[1].Resize(samples[0].GetSize());

    int numSamples = samples[0].GetSize();
    BL_FLOAT *samples0Buf = samples[0].Get();
    BL_FLOAT *samples1Buf = samples[1].Get();

    BL_FLOAT *polarSamples0Buf = polarSamples[0].Get();
    BL_FLOAT *polarSamples1Buf = polarSamples[1].Get();
    for (int i = 0; i < numSamples; i++)
    {
        BL_FLOAT l = samples0Buf[i];
        BL_FLOAT r = samples1Buf[i];
        
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
            polarSamples0Buf[i] = -1.0;
            polarSamples1Buf[i] = -1.0;
            
            continue;
#else
            // Keep in the border
            dist = CLIP_DISTANCE;
#endif
        }
        
        // Adjust
        angle = -angle;
        angle -= PI_DIV4;
        
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
        
        polarSamples0Buf[i] = x;
        polarSamples1Buf[i] = y;
    }
}

void
BLVectorscopeProcess::ComputePolarLevels(const WDL_TypedBuf<BL_FLOAT> samples[2],
                                         int numBins,
                                         WDL_TypedBuf<BL_FLOAT> *levels,
                                         enum PolarLevelMode mode)
{    
    // Accumulate levels depending on the angles
    levels->Resize(numBins);
    BLUtils::FillAllZero(levels);
    
    WDL_TypedBuf<int> numValues;
    numValues.Resize(numBins);
    BLUtils::FillAllZero(&numValues);

    int samplesSize = samples[0].GetSize();
    BL_FLOAT *samples0Buf = samples[0].Get();
    BL_FLOAT *samples1Buf = samples[1].Get();

    int levelsSize = levels->GetSize();
    BL_FLOAT *levelsBuf = levels->Get();
    int *numValuesBuf = numValues.Get();
        
    for (int i = 0; i < samplesSize; i++)
    {
        BL_FLOAT l = samples0Buf[i];
        BL_FLOAT r = samples1Buf[i];
        
        BL_FLOAT dist = std::sqrt(l*l + r*r);
        BL_FLOAT angle = std::atan2(r, l);
        
        // Adjust
        angle = -angle;
        angle -= PI_DIV4;
        
        // Bound to [-Pi, Pi]
        angle = fmod(angle, 2.0*M_PI);
        if (angle < 0.0)
            angle += 2.0*M_PI;
        
        angle -= M_PI;
     
        // Set to 1 for Fireworks
#if 1
        if (angle < 0.0)
            angle += M_PI;
#endif
        
        //int binNum = (angle/M_PI)*numBins;
        int binNum = (angle*M_PI_INV)*numBins;
        
        if (binNum < 0)
            binNum = 0;
        if (binNum > numBins - 1)
            binNum = numBins - 1;
        
        if (mode == MAX)
        {
            // Better to take max, will avoid increase in the borders and decrease in the center
            if (dist > levelsBuf[binNum])
                levelsBuf[binNum] = dist;
        }
        
        if (mode == AVG)
        {
            levelsBuf[binNum] += dist;
            numValuesBuf[binNum]++;
        }
    }
        
    if (mode == AVG)
    {
        for (int i = 0; i < levelsSize; i++)
        {
            int nv = numValuesBuf[i];
            if (nv > 0)
                levels->Get()[i] /= nv;
        }
    }
    
    for (int i = 0; i < levelsSize; i++)
    {
        BL_FLOAT dist = levelsBuf[i];
        
        // Clip so it will stay in the half circle
        if (dist > CLIP_DISTANCE)
        {
            dist = CLIP_DISTANCE;
        }
        
        levelsBuf[i] = dist;
    }
}

void
BLVectorscopeProcess::SmoothPolarLevels(WDL_TypedBuf<BL_FLOAT> *ioLevels,
                                        WDL_TypedBuf<BL_FLOAT> *prevLevels,
                                        bool smoothMinMax,
                                        BL_FLOAT smoothCoeff)
{
    // Smooth
    if (prevLevels != NULL)
    {
        if (prevLevels->GetSize() != ioLevels->GetSize())
        {
            *prevLevels = *ioLevels;
        }
        else
        {
            if (!smoothMinMax)
                BLUtils::Smooth(ioLevels, prevLevels, smoothCoeff);
            else
                BLUtils::SmoothMax(ioLevels, prevLevels, smoothCoeff);
        }
    }
}

void
BLVectorscopeProcess::
ComputePolarLevelPoints(const  WDL_TypedBuf<BL_FLOAT> &levels,
                        WDL_TypedBuf<BL_FLOAT> polarLevelSamples[2])
{
    // Convert to (x, y)
    polarLevelSamples[0].Resize(levels.GetSize());
    BLUtils::FillAllZero(&polarLevelSamples[0]);
    
    polarLevelSamples[1].Resize(levels.GetSize());
    BLUtils::FillAllZero(&polarLevelSamples[1]);

    int levelsSize = levels.GetSize();
    BL_FLOAT *levelsBuf = levels.Get();
    
    BL_FLOAT coeff = M_PI;
    if (levelsSize > 1)
        coeff = M_PI/(levelsSize - 1);

    BL_FLOAT *polarLevelSamplesBuf0 = polarLevelSamples[0].Get();
    BL_FLOAT *polarLevelSamplesBuf1 = polarLevelSamples[1].Get();
        
    for (int i = 0; i < levelsSize; i++)
    {
        BL_FLOAT angle = i*coeff;
        BL_FLOAT dist = levelsBuf[i];
        
        // Compute (x, y)
        BL_FLOAT x = dist*cos(angle);
        BL_FLOAT y = dist*sin(angle);
        
#if 0 // Disabled for Fireworks
        // DEBUG
        // (correlation -1 and 1 are out of screen)
#define OFFSET_Y 0.04
        y += OFFSET_Y;
#endif
        
        polarLevelSamplesBuf0[i] = x;
        polarLevelSamplesBuf1[i] = y;
    }
}

void
BLVectorscopeProcess::ComputeLissajous(const WDL_TypedBuf<BL_FLOAT> samples[2],
                                       WDL_TypedBuf<BL_FLOAT> lissajousSamples[2],
                                       bool fitInSquare)
{
    lissajousSamples[0].Resize(samples[0].GetSize());
    lissajousSamples[1].Resize(samples[0].GetSize());

    int samples0Size = samples[0].GetSize();
    BL_FLOAT *samples0Buf = samples[0].Get();
    BL_FLOAT *samples1Buf = samples[1].Get();

    BL_FLOAT *lissajousSamples0Buf = lissajousSamples[0].Get();
    BL_FLOAT *lissajousSamples1Buf = lissajousSamples[1].Get();
        
    for (int i = 0; i < samples0Size; i++)
    {
        BL_FLOAT l = samples0Buf[i];
        BL_FLOAT r = samples1Buf[i];
        
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
        angle -= PI_DIV4;
        
        BL_FLOAT x = dist*cos(angle);
        BL_FLOAT y = dist*sin(angle);
        
#if 0 // Keep [-1, 1] bounds
        // Center
        x = (x + 1.0)*0.5;
        y = (y + 1.0)*0.5;
#endif
        
        lissajousSamples0Buf[i] = x;
        lissajousSamples1Buf[i] = y;
    }
}
