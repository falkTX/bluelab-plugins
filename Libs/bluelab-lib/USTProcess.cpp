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
//  USTProcess.cpp
//  UST
//
//  Created by applematuer on 7/28/19.
//
//

#include <cmath>

#include <BLUtils.h>
#include <BLUtilsComp.h>

//#include <USTWidthAdjuster4.h> // old
#include <USTWidthAdjuster5.h>
#include <DelayObj4.h>

#include <FftProcessObj16.h>

#include <ParamSmoother.h>

#include "USTProcess.h"

// Stereo Widen
// For info, see: http://www.rs-met.com/documents/tutorials/StereoProcessing.pdf
// and https://www.kvraudio.com/forum/viewtopic.php?t=212587
// and https://www.irisa.fr/prive/kadi/Sujets_CTR/Emmanuel/Vincent_sujet1_article_avendano.pdf

#ifndef M_PI
#define M_PI 3.141592653589
#endif

// Polar samples
//
#define CLIP_DISTANCE 0.95
#define CLIP_KEEP     1

// Lissajous
#define LISSAJOUS_CLIP_DISTANCE 1.0

// StereoWiden
//

// Stereo widen computation method

//
#define STEREO_WIDEN_GONIO 0
#define STEREO_WIDEN_VOL_ADJUSTED       0
#define STEREO_WIDEN_SQRT               0
#define STEREO_WIDEN_SQRT_CUSTOM        0 // VERY GOOD
#define STEREO_WIDEN_SQRT_CUSTOM_OPTIM  1 // Same as previous, but optimized

#define MAX_WIDTH_FACTOR 2.0 //8.0 // not for sqrt method


// FIX: On Protools, when width parameter was reset to 0,
// by using alt + click, the gain was increased compared to bypass
//
// On Protools, when alt + click to reset parameter,
// Protools manages all, and does not reset the parameter exactly to the default value
// (precision 1e-8).
// Then the coeff is not exactly 1, then the gain coeff is applied.
#define FIX_WIDTH_PRECISION 1
#define WIDTH_PARAM_PRECISION 1e-6

// Correlation computation method
//

// See: https://dsp.stackexchange.com/questions/1671/calculate-phase-angle-between-two-signals-i-e-digital-phase-meter
#define COMPUTE_CORRELATION_ATAN 0

// See: http://www.rs-met.com/documents/tutorials/StereoProcessing.pdf
// (may be more correct)
#define COMPUTE_CORRELATION_CROSS 1

#define SQR2_INV 0.70710678118655


// See: https://dsp.stackexchange.com/questions/1671/calculate-phase-angle-between-two-signals-i-e-digital-phase-meter
template <typename FLOAT_TYPE>
void
USTProcess::ComputePolarSamples(const WDL_TypedBuf<FLOAT_TYPE> samples[2],
                                WDL_TypedBuf<FLOAT_TYPE> polarSamples[2])
{
    polarSamples[0].Resize(samples[0].GetSize());
    polarSamples[1].Resize(samples[0].GetSize());
     
    for (int i = 0; i < samples[0].GetSize(); i++)
    {
        FLOAT_TYPE l = samples[0].Get()[i];
        FLOAT_TYPE r = samples[1].Get()[i];
        
        FLOAT_TYPE angle = std::atan2(r, l);
        FLOAT_TYPE dist = std::sqrt(l*l + r*r);
        
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
        FLOAT_TYPE x = dist*std::cos(angle);
        FLOAT_TYPE y = dist*std::sin(angle);

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
template void USTProcess::ComputePolarSamples(const WDL_TypedBuf<float> samples[2],
                                              WDL_TypedBuf<float> polarSamples[2]);
template void USTProcess::ComputePolarSamples(const WDL_TypedBuf<double> samples[2],
                                              WDL_TypedBuf<double> polarSamples[2]);

template <typename FLOAT_TYPE>
void
USTProcess::ComputePolarLevels(const WDL_TypedBuf<FLOAT_TYPE> samples[2],
                               int numBins,
                               WDL_TypedBuf<FLOAT_TYPE> *levels,
                               enum PolarLevelMode mode)
{    
    // Accumulate levels depending on the angles
    levels->Resize(numBins);
    BLUtils::FillAllZero(levels);
    
    WDL_TypedBuf<int> numValues;
    numValues.Resize(numBins);
    BLUtils::FillAllZero(&numValues);
    
    for (int i = 0; i < samples[0].GetSize(); i++)
    {
        FLOAT_TYPE l = samples[0].Get()[i];
        FLOAT_TYPE r = samples[1].Get()[i];
        
        FLOAT_TYPE dist = std::sqrt(l*l + r*r);
        FLOAT_TYPE angle = std::atan2(r, l);
        
        // Adjust
        angle = -angle;
        angle -= M_PI/4.0;
        
        // Bound to [-Pi, Pi]
        angle = std::fmod(angle, (FLOAT_TYPE)(2.0*M_PI));
        if (angle < 0.0)
            angle += 2.0*M_PI;
        
        angle -= M_PI;
     
        // Set to 1 for Fireworks
#if 1
        if (angle < 0.0)
            angle += M_PI;
#endif
        
        int binNum = (angle/M_PI)*numBins;
        
        if (binNum < 0)
            binNum = 0;
        if (binNum > numBins - 1)
            binNum = numBins - 1;
        
        if (mode == MAX)
        {
            // Better to take max, will avoid increase in the borders and decrease in the center
            if (dist > levels->Get()[binNum])
                levels->Get()[binNum] = dist;
        }
        
        if (mode == AVG)
        {
            levels->Get()[binNum] += dist;
            numValues.Get()[binNum]++;
        }
    }
    
    if (mode == AVG)
    {
        for (int i = 0; i < levels->GetSize(); i++)
        {
            int nv = numValues.Get()[i];
            if (nv > 0)
                levels->Get()[i] /= nv;
        }
    }
    
    for (int i = 0; i < levels->GetSize(); i++)
    {
        FLOAT_TYPE dist = levels->Get()[i];
        
        // Clip so it will stay in the half circle
        if (dist > CLIP_DISTANCE)
        {
            dist = CLIP_DISTANCE;
        }
        
        levels->Get()[i] = dist;
    }
}
template void USTProcess::ComputePolarLevels(const WDL_TypedBuf<float> samples[2],
                                             int numBins,
                                             WDL_TypedBuf<float> *levels,
                                             enum PolarLevelMode mode);
template void USTProcess::ComputePolarLevels(const WDL_TypedBuf<double> samples[2],
                                             int numBins,
                                             WDL_TypedBuf<double> *levels,
                                             enum PolarLevelMode mode);

template <typename FLOAT_TYPE>
void
USTProcess::SmoothPolarLevels(WDL_TypedBuf<FLOAT_TYPE> *ioLevels,
                              WDL_TypedBuf<FLOAT_TYPE> *prevLevels,
                              bool smoothMinMax,
                              FLOAT_TYPE smoothCoeff)
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
template void USTProcess::SmoothPolarLevels(WDL_TypedBuf<float> *ioLevels,
                                            WDL_TypedBuf<float> *prevLevels,
                                            bool smoothMinMax,
                                            float smoothCoeff);
template void USTProcess::SmoothPolarLevels(WDL_TypedBuf<double> *ioLevels,
                                            WDL_TypedBuf<double> *prevLevels,
                                            bool smoothMinMax,
                                            double smoothCoeff);


template <typename FLOAT_TYPE>
void
USTProcess::ComputePolarLevelPoints(const  WDL_TypedBuf<FLOAT_TYPE> &levels,
                                    WDL_TypedBuf<FLOAT_TYPE> polarLevelSamples[2])
{
    // Convert to (x, y)
    polarLevelSamples[0].Resize(levels.GetSize());
    BLUtils::FillAllZero(&polarLevelSamples[0]);
    
    polarLevelSamples[1].Resize(levels.GetSize());
    BLUtils::FillAllZero(&polarLevelSamples[1]);
    
    for (int i = 0; i < levels.GetSize(); i++)
    {
        FLOAT_TYPE angle = (((FLOAT_TYPE)i)/(levels.GetSize() - 1))*M_PI;
        FLOAT_TYPE dist = levels.Get()[i];
        
        // Compute (x, y)
        FLOAT_TYPE x = dist*std::cos(angle);
        FLOAT_TYPE y = dist*std::sin(angle);
        
#if 0 // Disabled for Fireworks
        // DEBUG
        // (correlation -1 and 1 are out of screen)
#define OFFSET_Y 0.04
        y += OFFSET_Y;
#endif
        
        polarLevelSamples[0].Get()[i] = x;
        polarLevelSamples[1].Get()[i] = y;
    }
}
template void USTProcess::ComputePolarLevelPoints(const  WDL_TypedBuf<float> &levels,
                                                  WDL_TypedBuf<float> polarLevelSamples[2]);
template void USTProcess::ComputePolarLevelPoints(const  WDL_TypedBuf<double> &levels,
                                                  WDL_TypedBuf<double> polarLevelSamples[2]);

template <typename FLOAT_TYPE>
void
USTProcess::ComputeLissajous(const WDL_TypedBuf<FLOAT_TYPE> samples[2],
                             WDL_TypedBuf<FLOAT_TYPE> lissajousSamples[2],
                             bool fitInSquare)
{
    lissajousSamples[0].Resize(samples[0].GetSize());
    lissajousSamples[1].Resize(samples[0].GetSize());
    
    for (int i = 0; i < samples[0].GetSize(); i++)
    {
        FLOAT_TYPE l = samples[0].Get()[i];
        FLOAT_TYPE r = samples[1].Get()[i];
        
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
        FLOAT_TYPE angle = std::atan2(r, l);
        FLOAT_TYPE dist = std::sqrt(l*l + r*r);
        
        if (fitInSquare)
        {
            FLOAT_TYPE coeff = SQR2_INV;
            dist *= coeff;
        }
        
        angle = -angle;
        angle -= M_PI/4.0;
        
        FLOAT_TYPE x = dist*std::cos(angle);
        FLOAT_TYPE y = dist*std::sin(angle);
        
#if 0 // Keep [-1, 1] bounds
        // Center
        x = (x + 1.0)*0.5;
        y = (y + 1.0)*0.5;
#endif
        
        lissajousSamples[0].Get()[i] = x;
        lissajousSamples[1].Get()[i] = y;
    }
}
template void USTProcess::ComputeLissajous(const WDL_TypedBuf<float> samples[2],
                                           WDL_TypedBuf<float> lissajousSamples[2],
                                           bool fitInSquare);
template void USTProcess::ComputeLissajous(const WDL_TypedBuf<double> samples[2],
                                           WDL_TypedBuf<double> lissajousSamples[2],
                                           bool fitInSquare);

// See: https://dsp.stackexchange.com/questions/1671/calculate-phase-angle-between-two-signals-i-e-digital-phase-meter
#if COMPUTE_CORRELATION_ATAN
template <typename FLOAT_TYPE>
FLOAT_TYPE
USTProcess::ComputeCorrelation(const WDL_TypedBuf<FLOAT_TYPE> samples[2])
{
    FLOAT_TYPE result = 0.0;
    FLOAT_TYPE sumDist = 0.0;
    
    for (int i = 0; i < samples[0].GetSize(); i++)
    {
        FLOAT_TYPE l = samples[0].Get()[i];
        FLOAT_TYPE r = samples[1].Get()[i];
        
        FLOAT_TYPE angle = std::atan2(r, l);
        FLOAT_TYPE dist = std::sqrt(l*l + r*r);
        
        // Adjust
        angle = -angle;
        angle -= M_PI/4.0;
        
        // Flip vertically, to keep only one half-circle
        if (angle < 0.0)
            angle = -angle;
        
        // Flip horizontally to keep only one quarter slice
        if (angle > M_PI/2.0)
            angle = M_PI - angle;
        
        // Normalize into [0, 1]
        FLOAT_TYPE corr = angle/(M_PI/2.0);
        
        // Set into [-1, 1]
        corr = (corr * 2.0) - 1.0;
        
        // Ponderate with distance;
        corr *= dist;
        
        result += corr;
        sumDist += dist;
    }
    
    if (sumDist > 0.0)
        result /= sumDist;
    
    return result;
}
template float USTProcess::ComputeCorrelation(const WDL_TypedBuf<float> samples[2]);
template BL_FLOAT USTProcess::ComputeCorrelation(const WDL_TypedBuf<double> samples[2]);
#endif

// See: http://www.rs-met.com/documents/tutorials/StereoProcessing.pdf
#if COMPUTE_CORRELATION_CROSS
template <typename FLOAT_TYPE>
FLOAT_TYPE
USTProcess::ComputeCorrelation(const WDL_TypedBuf<FLOAT_TYPE> samples[2])
{
    FLOAT_TYPE sumProduct = 0.0;
    FLOAT_TYPE sumL2 = 0.0;
    FLOAT_TYPE sumR2 = 0.0;
    
    for (int i = 0; i < samples[0].GetSize(); i++)
    {
        FLOAT_TYPE l = samples[0].Get()[i];
        FLOAT_TYPE r = samples[1].Get()[i];
        
        FLOAT_TYPE product = l*r;
        sumProduct += product;
        
        sumL2 += l*l;
        sumR2 += r*r;
    }
    
    FLOAT_TYPE correl = 0.0;
    int numValues = samples[0].GetSize();
    if (numValues > 0)
    {
        FLOAT_TYPE num = sumProduct/numValues;
        
        FLOAT_TYPE denom2 = (sumL2/numValues)*(sumR2/numValues);
        if (denom2 > 0.0)
        {
            FLOAT_TYPE denom = std::sqrt(denom2);
        
            correl = num/denom;
        }
    }
    
    return correl;
}
template float USTProcess::ComputeCorrelation(const WDL_TypedBuf<float> samples[2]);
template double USTProcess::ComputeCorrelation(const WDL_TypedBuf<double> samples[2]);
#endif

//#if COMPUTE_CORRELATION_CROSS
template <typename FLOAT_TYPE>
void
USTProcess::ComputeCorrelationMinMax(const WDL_TypedBuf<FLOAT_TYPE> samples[2],
                                     int windowSize,
                                     FLOAT_TYPE *minCorr, FLOAT_TYPE *maxCorr)
{
#define INF 1e15
    *minCorr = INF;
    *maxCorr = -INF;
    
    // NOTE: the effect changes quite a lot when changing from 10 to 50
//#define NUM_SAMPLES 50 //10
    
    int startIndex = 0;
    while(startIndex < samples[0].GetSize())
    {
        FLOAT_TYPE sumProduct = 0.0;
        FLOAT_TYPE sumL2 = 0.0;
        FLOAT_TYPE sumR2 = 0.0;
        
        int endIndex = startIndex + windowSize/*NUM_SAMPLES*/;
        if (endIndex > samples[0].GetSize())
            endIndex = samples[0].GetSize();
        
        for (int i = startIndex; i < endIndex; i++)
        {
            FLOAT_TYPE l = samples[0].Get()[i];
            FLOAT_TYPE r = samples[1].Get()[i];
        
            FLOAT_TYPE product = l*r;
            sumProduct += product;
        
            sumL2 += l*l;
            sumR2 += r*r;
        }
    
        FLOAT_TYPE correl = 0.0;
        int numValues = endIndex - startIndex;
        if (numValues > 0)
        {
            FLOAT_TYPE num = sumProduct/numValues;
        
            FLOAT_TYPE denom2 = (sumL2/numValues)*(sumR2/numValues);
            if (denom2 > 0.0)
            {
	      FLOAT_TYPE denom = std::sqrt(denom2);
            
                correl = num/denom;
            }
        }
        
        if (correl < *minCorr)
            *minCorr = correl;
        
        if (correl > *maxCorr)
            *maxCorr = correl;
        
        startIndex += windowSize/*NUM_SAMPLES*/;
    }
}
//#endif

template <typename FLOAT_TYPE>
FLOAT_TYPE
USTProcess::ComputeCorrelation(const WDL_TypedBuf<FLOAT_TYPE> samples[2],
                               int index, int num)
{
    if (index - num/2 < 0)
        return 0.0;
    
    if (index + num/2 >= samples[0].GetSize())
        return 0.0;
    
    WDL_TypedBuf<FLOAT_TYPE> s[2];
    s[0].Add(&samples[0].Get()[index - num/2], num);
    s[1].Add(&samples[1].Get()[index - num/2], num);
    
    FLOAT_TYPE correl = ComputeCorrelation(s);
    
    return correl;
}

// Latest version
// Replace by USTStereoWidener
#if 0
template <typename FLOAT_TYPE>
void
USTProcess::StereoWiden(vector<WDL_TypedBuf<FLOAT_TYPE> * > *ioSamples, FLOAT_TYPE widthFactor)
{
    FLOAT_TYPE width = ComputeFactor(widthFactor, MAX_WIDTH_FACTOR);
    
#if FIX_WIDTH_PRECISION
    width /= WIDTH_PARAM_PRECISION;
    width = bl_round(width);
    width *= WIDTH_PARAM_PRECISION;
#endif
    
    // See: http://www.musicdsp.org/showArchiveComment.php?ArchiveID=256
    
    WDL_FFT_COMPLEX angle0 = USTProcess::StereoWidenComputeAngle0();
    WDL_FFT_COMPLEX angle1 = USTProcess::StereoWidenComputeAngle1();
    
    // Then do this per sample
    for (int i = 0; i < (*ioSamples)[0]->GetSize(); i++)
    {
        FLOAT_TYPE left = (*ioSamples)[0]->Get()[i];
        FLOAT_TYPE right = (*ioSamples)[1]->Get()[i];
        
        //StereoWiden(&left, &right, width);
        StereoWiden(&left, &right, width, angle0, angle1);
        
        (*ioSamples)[0]->Get()[i] = left;
        (*ioSamples)[1]->Get()[i] = right;
    }
}

#if 0
// With width adjuster
template <typename FLOAT_TYPE>
void
USTProcess::StereoWiden(vector<WDL_TypedBuf<FLOAT_TYPE> * > *ioSamples,
                        USTWidthAdjuster4 *widthAdjuster)
{
    WDL_FFT_COMPLEX angle0 = USTProcess::StereoWidenComputeAngle0();
    WDL_FFT_COMPLEX angle1 = USTProcess::StereoWidenComputeAngle1();
    
    // Then do this per sample
    for (int i = 0; i < (*ioSamples)[0]->GetSize(); i++)
    {
        // Get current width
        widthAdjuster->Update();
        FLOAT_TYPE widthFactor = widthAdjuster->GetLimitedWidth();
        FLOAT_TYPE width = ComputeFactor(widthFactor, MAX_WIDTH_FACTOR);
        
#if FIX_WIDTH_PRECISION
        width /= WIDTH_PARAM_PRECISION;
        width = bl_round(width);
        width *= WIDTH_PARAM_PRECISION;
#endif
        
        // Compute
        FLOAT_TYPE left = (*ioSamples)[0]->Get()[i];
        FLOAT_TYPE right = (*ioSamples)[1]->Get()[i];
        
        //StereoWiden(&left, &right, width);
        StereoWiden(&left, &right, width, angle0, angle1);
        
        (*ioSamples)[0]->Get()[i] = left;
        (*ioSamples)[1]->Get()[i] = right;
    }
}
#endif

// With new width adjuster
// Compute good width sample by sample
template <typename FLOAT_TYPE>
void
USTProcess::StereoWiden(vector<WDL_TypedBuf<FLOAT_TYPE> * > *ioSamples,
                        USTWidthAdjuster5 *widthAdjuster)
{
    WDL_FFT_COMPLEX angle0 = USTProcess::StereoWidenComputeAngle0();
    WDL_FFT_COMPLEX angle1 = USTProcess::StereoWidenComputeAngle1();
    
    // Then do this per sample
    for (int i = 0; i < (*ioSamples)[0]->GetSize(); i++)
    {
        // Compute
        FLOAT_TYPE left = (*ioSamples)[0]->Get()[i];
        FLOAT_TYPE right = (*ioSamples)[1]->Get()[i];
        
        // Get current width
        widthAdjuster->Update(left, right);
        FLOAT_TYPE widthFactor = widthAdjuster->GetLimitedWidth();
        FLOAT_TYPE width = ComputeFactor(widthFactor, MAX_WIDTH_FACTOR);
        
#if FIX_WIDTH_PRECISION
        width /= WIDTH_PARAM_PRECISION;
        width = bl_round(width);
        width *= WIDTH_PARAM_PRECISION;
#endif
        
        //StereoWiden(&left, &right, width);
        StereoWiden(&left, &right, width, angle0, angle1);
        
        (*ioSamples)[0]->Get()[i] = left;
        (*ioSamples)[1]->Get()[i] = right;
    }
}

template <typename FLOAT_TYPE>
void
USTProcess::StereoWiden(FLOAT_TYPE *l, FLOAT_TYPE *r, FLOAT_TYPE widthFactor)
{
    FLOAT_TYPE width = ComputeFactor(widthFactor, MAX_WIDTH_FACTOR);
    
#if FIX_WIDTH_PRECISION
    width /= WIDTH_PARAM_PRECISION;
    width = bl_round(width);
    width *= WIDTH_PARAM_PRECISION;
#endif
    
    // See: http://www.musicdsp.org/showArchiveComment.php?ArchiveID=256
    
    WDL_FFT_COMPLEX angle0 = USTProcess::StereoWidenComputeAngle0();
    WDL_FFT_COMPLEX angle1 = USTProcess::StereoWidenComputeAngle1();
    
    //StereoWiden(&left, &right, width);
    StereoWiden(l, r, width, angle0, angle1);
}
#endif

// Correct formula for balance
// (Le livre des techniques du son - Tome 2 - p227)
//
// Balance do not use pan law
//
// Center position: 0dB atten for both channel
// Extrement positions: (+3dB, -inf)
//
// Results:
// - center position: no gain
// - extreme position: (+3dB, -inf)
// - when balancing a mono signal, the line moves constantly on the vectorscope

// For implementation, see: https://www.kvraudio.com/forum/viewtopic.php?t=235347

template <typename FLOAT_TYPE>
void
USTProcess::Balance(vector<WDL_TypedBuf<FLOAT_TYPE> * > *ioSamples,
                    FLOAT_TYPE balance)
{
    FLOAT_TYPE p = M_PI*(balance + 1.0)/4.0;
	FLOAT_TYPE gl = std::cos(p);
	FLOAT_TYPE gr = std::sin(p);
    
    // Coefficients to have no gain when center, and +3dB for extreme pan pos
    gl *= std::sqrt((FLOAT_TYPE)2.0);
    gr *= std::sqrt((FLOAT_TYPE)2.0);
    
    for (int i = 0; i < (*ioSamples)[0]->GetSize(); i++)
    {
        FLOAT_TYPE l = (*ioSamples)[0]->Get()[i];
        FLOAT_TYPE r = (*ioSamples)[1]->Get()[i];
        
        l *= gl;
        r *= gr;
        
        (*ioSamples)[0]->Get()[i] = l;
        (*ioSamples)[1]->Get()[i] = r;
    }
}
template void USTProcess::Balance(vector<WDL_TypedBuf<float> * > *ioSamples,
                                  float balance);
template void USTProcess::Balance(vector<WDL_TypedBuf<double> * > *ioSamples,
                                  double balance);

template <typename FLOAT_TYPE>
void
USTProcess::Balance(vector<WDL_TypedBuf<FLOAT_TYPE> * > *ioSamples,
                    ParamSmoother *balanceSmoother)
{
    for (int i = 0; i < (*ioSamples)[0]->GetSize(); i++)
    {
        balanceSmoother->Update();
        FLOAT_TYPE balance = balanceSmoother->GetCurrentValue();
        
        FLOAT_TYPE p = M_PI*(balance + 1.0)/4.0;
        FLOAT_TYPE gl = std::cos(p);
        FLOAT_TYPE gr = std::sin(p);
        
        // Coefficients to have no gain when center, and +3dB for extreme pan pos
        gl *= std::sqrt((FLOAT_TYPE)2.0);
        gr *= std::sqrt((FLOAT_TYPE)2.0);
        
        FLOAT_TYPE l = (*ioSamples)[0]->Get()[i];
        FLOAT_TYPE r = (*ioSamples)[1]->Get()[i];
        
        l *= gl;
        r *= gr;
        
        (*ioSamples)[0]->Get()[i] = l;
        (*ioSamples)[1]->Get()[i] = r;
    }
}
template void USTProcess::Balance(vector<WDL_TypedBuf<float> * > *ioSamples,
                                  ParamSmoother *balanceSmoother);
template void USTProcess::Balance(vector<WDL_TypedBuf<double> * > *ioSamples,
                                  ParamSmoother *balanceSmoother);

// TODO: remove this
#if 1
// See: https://www.kvraudio.com/forum/viewtopic.php?t=235347
template <typename FLOAT_TYPE>
void
USTProcess::Balance0(vector<WDL_TypedBuf<FLOAT_TYPE> * > *ioSamples,
                    FLOAT_TYPE balance)
{
    FLOAT_TYPE p = M_PI*(balance + 1.0)/4.0;
    FLOAT_TYPE gl = std::cos(p);
    FLOAT_TYPE gr = std::sin(p);
    
    for (int i = 0; i < (*ioSamples)[0]->GetSize(); i++)
    {
        FLOAT_TYPE l = (*ioSamples)[0]->Get()[i];
        FLOAT_TYPE r = (*ioSamples)[1]->Get()[i];
        
        l *= gl;
        r *= gr;
        
        (*ioSamples)[0]->Get()[i] = l;
        (*ioSamples)[1]->Get()[i] = r;
    }
}
#endif

template <typename FLOAT_TYPE>
void
USTProcess::Balance3(vector<WDL_TypedBuf<FLOAT_TYPE> * > *ioSamples,
                     FLOAT_TYPE balance)
{
    FLOAT_TYPE p = M_PI*(balance + 1.0)/4.0;
    FLOAT_TYPE gl = std::cos(p);
    FLOAT_TYPE gr = std::sin(p);
    
    // TODO: check this
	gl = std::sqrt(gl);
	gr = std::sqrt(gr);
    
    for (int i = 0; i < (*ioSamples)[0]->GetSize(); i++)
    {
        FLOAT_TYPE l = (*ioSamples)[0]->Get()[i];
        FLOAT_TYPE r = (*ioSamples)[1]->Get()[i];
        
        l *= gl;
        r *= gr;
        
        (*ioSamples)[0]->Get()[i] = l;
        (*ioSamples)[1]->Get()[i] = r;
    }
}
template void USTProcess::Balance3(vector<WDL_TypedBuf<float> * > *ioSamples,
                                   float balance);
template void USTProcess::Balance3(vector<WDL_TypedBuf<double> * > *ioSamples,
                                   double balance);

#if 0 // "Haas effect" method
      // Problem: the sound is panned
template <typename FLOAT_TYPE>
void
USTProcess::MonoToStereo(vector<WDL_TypedBuf<FLOAT_TYPE> > *samplesVec,
                         DelayObj4 *delayObj)
{
    if (samplesVec->empty())
        return;
    
    StereoToMono(samplesVec);
    
    delayObj->ProcessSamples(&(*samplesVec)[1]);
}
#endif

// Latest version
// Replaced by USTPseudoStereoObj
#if 0
// See: http://www.native-instruments.com/forum/attachments/stereo-solutions-pdf.13883
// and: http://www.csounds.com/journal/issue14/PseudoStereo.html
// and: http://csoundjournal.com/issue14/PseudoStereo.html
//
// Result:
// - almost same level (mono2stereo, then out mono => the level is the same as mono only)
// - result seems like IZotope Ozone Imager
//
// NOTE: there are some improved algorithms in the paper
template <typename FLOAT_TYPE>
void
USTProcess::MonoToStereo(vector<WDL_TypedBuf<FLOAT_TYPE> > *samplesVec,
                         DelayObj4 *delayObj)
{
    if (samplesVec->empty())
        return;
    
    StereoToMono(samplesVec);
    
    // Lauridsen
    
    WDL_TypedBuf<FLOAT_TYPE> delayMono = (*samplesVec)[0];
    delayObj->ProcessSamples(&delayMono);
    
    // L = mono + delayMono
    BLUtils::AddValues(&(*samplesVec)[0], delayMono);
    
    // R = delayMono - mono
    BLUtils::MultValues(&(*samplesVec)[1], -1.0);
    BLUtils::AddValues(&(*samplesVec)[1], delayMono);
    
    // Adjust the width by default
#define LAURIDSEN_WIDTH_ADJUST -0.70
    vector<WDL_TypedBuf<FLOAT_TYPE> * > samples;
    samples.push_back(&(*samplesVec)[0]);
    samples.push_back(&(*samplesVec)[1]);
    
    USTProcess::StereoWiden(&samples, LAURIDSEN_WIDTH_ADJUST);
}
template void USTProcess::MonoToStereo(vector<WDL_TypedBuf<float> > *samplesVec,
                                       DelayObj4 *delayObj);
template void USTProcess::MonoToStereo(vector<WDL_TypedBuf<double> > *samplesVec,
                                       DelayObj4 *delayObj);
#endif

template <typename FLOAT_TYPE>
void
USTProcess::StereoToMono(vector<WDL_TypedBuf<FLOAT_TYPE> > *samplesVec)
{
    // First, set to bi-mono channels
    if (samplesVec->size() == 1)
    {
        // Duplicate mono channel
        samplesVec->push_back((*samplesVec)[0]);
    }
    else if (samplesVec->size() == 2)
    {
        // Set to mono
        WDL_TypedBuf<FLOAT_TYPE> mono;
        BLUtils::StereoToMono(&mono, (*samplesVec)[0], (*samplesVec)[1]);
        
        (*samplesVec)[0] = mono;
        (*samplesVec)[1] = mono;
    }
}
template void USTProcess::StereoToMono<float>(vector<WDL_TypedBuf<float> > *samplesVec);
template void USTProcess::StereoToMono<double>(vector<WDL_TypedBuf<double> > *samplesVec);

template <typename FLOAT_TYPE>
void
USTProcess::DecimateSamplesCorrelation(vector<WDL_TypedBuf<FLOAT_TYPE> > *samples,
                                       FLOAT_TYPE decimFactor, FLOAT_TYPE sampleRate)
{
    if (samples->size() != 2)
        return;
    
    vector<WDL_TypedBuf<FLOAT_TYPE> > samplesCopy = *samples;
    
    int decimFactor0 = (int)(decimFactor*(sampleRate/44100.0));
    
    int newSize = (*samples)[0].GetSize()/decimFactor0;
    
    (*samples)[0].Resize(newSize);
    (*samples)[1].Resize(newSize);
    
    FLOAT_TYPE bestSamples[2];
    FLOAT_TYPE bestDiff;
    for (int i = 0; i < samplesCopy[0].GetSize(); i += decimFactor0)
    {
        // Search for the maximum diff
        for (int k = 0; k < decimFactor0; k++)
        {
            if (k == 0)
            {
                bestSamples[0] = samplesCopy[0].Get()[i];
                bestSamples[1] = samplesCopy[1].Get()[i];
                
                bestDiff = std::fabs(bestSamples[0] - bestSamples[1]);
                
                continue;
            }
            
            FLOAT_TYPE samp0 = samplesCopy[0].Get()[i + k];
            FLOAT_TYPE samp1 = samplesCopy[1].Get()[i + k];
            
            FLOAT_TYPE diff = std::fabs(samp0 - samp1);
            // Keep the maximum diff
            if (diff > bestDiff)
            {
                bestDiff = diff;
                
                bestSamples[0] = samp0;
                bestSamples[1] = samp1;
            }
        }
        
        // Result
        
        // Check out of bounds, just in case
        if (i/decimFactor0 >= (*samples)[0].GetSize())
            break;
        
        (*samples)[0].Get()[i/decimFactor0] = bestSamples[0];
        (*samples)[1].Get()[i/decimFactor0] = bestSamples[1];
    }
}
template void
USTProcess::DecimateSamplesCorrelation(vector<WDL_TypedBuf<float> > *samples,
                                       float decimFactor, float sampleRate);
template void
USTProcess::DecimateSamplesCorrelation(vector<WDL_TypedBuf<double> > *samples,
                                       double decimFactor, double sampleRate);


#if (!STEREO_WIDEN_SQRT || STEREO_WIDEN_SQRT_CUSTOM || STEREO_WIDEN_SQRT_CUSTOM_OPTIM)
template <typename FLOAT_TYPE>
FLOAT_TYPE
USTProcess::ComputeFactor(FLOAT_TYPE normVal, FLOAT_TYPE maxVal)
{
    FLOAT_TYPE res = 0.0;
    if (normVal < 0.0)
        res = normVal + 1.0;
    else
        res = normVal*(maxVal - 1.0) + 1.0;
    
    return res;
}
template float USTProcess::ComputeFactor(float normVal, float maxVal);
template double USTProcess::ComputeFactor(double normVal, double maxVal);

#endif

#if STEREO_WIDEN_SQRT
template <typename FLOAT_TYPE>
FLOAT_TYPE
USTProcess::ComputeFactor(FLOAT_TYPE normVal, FLOAT_TYPE maxVal)
{
    FLOAT_TYPE res = (normVal + 1.0)*0.5;
    
    return res;
}
template float USTProcess::ComputeFactor(float normVal, float maxVal);
template BL_FLOAT USTProcess::ComputeFactor(BL_FLOAT normVal, BL_FLOAT maxVal);
#endif

// See: https://www.musicdsp.org/en/latest/Effects/256-stereo-width-control-obtained-via-transfromation-matrix.html?highlight=stereo
// Initial version (goniometer)
#if STEREO_WIDEN_GONIO
template <typename FLOAT_TYPE>
void
USTProcess::StereoWiden(FLOAT_TYPE *left, FLOAT_TYPE *right, FLOAT_TYPE widthFactor)
{
   // Initial version (goniometer)
    
    // Calculate scale coefficient
    FLOAT_TYPE coef_S = widthFactor*0.5;
    
    FLOAT_TYPE m = (*left + *right)*0.5;
    FLOAT_TYPE s = (*right - *left )*coef_S;
    
    *left = m - s;
    *right = m + s;
    
#if 0 // First correction of volume (not the best)
    *left  /= 0.5 + coef_S;
    *right /= 0.5 + coef_S;
#endif
}
template void USTProcess::StereoWiden(float *left, float *right, float widthFactor);
template void USTProcess::StereoWiden(BL_FLOAT *left, double *right, double widthFactor);
#endif

// See: https://www.musicdsp.org/en/latest/Effects/256-stereo-width-control-obtained-via-transfromation-matrix.html?highlight=stereo
// Volume adjusted version
// GOOD (loose a bit some volume when increasing width)
#if STEREO_WIDEN_VOL_ADJUSTED
template <typename FLOAT_TYPE>
void
USTProcess::StereoWiden(FLOAT_TYPE *left, FLOAT_TYPE *right, FLOAT_TYPE widthFactor)
{
    // Volume adjusted version
    //
    
    // Calc coefs
    FLOAT_TYPE tmp = 1.0/MAX(1.0 + widthFactor, 2.0);
    FLOAT_TYPE coef_M = 1.0 * tmp;
    FLOAT_TYPE coef_S = widthFactor * tmp;
    
    // Then do this per sample
    FLOAT_TYPE m = (*left + *right)*coef_M;
    FLOAT_TYPE s = (*right - *left )*coef_S;
    
    *left = m - s;
    *right = m + s;
}
template void USTProcess::StereoWiden(float *left, float *right, float widthFactor);
template void USTProcess::StereoWiden(double *left, double *right, double widthFactor);
#endif

// See: https://www.kvraudio.com/forum/viewtopic.php?t=212587
// and: http://www.rs-met.com/documents/tutorials/StereoProcessing.pdf
#if STEREO_WIDEN_SQRT
template <typename FLOAT_TYPE>
void
USTProcess::StereoWiden(FLOAT_TYPE *left, FLOAT_TYPE *right, FLOAT_TYPE widthFactor)
{
  FLOAT_TYPE M = (*left + *right)/std::sqrt((FLOAT_TYPE)2.0);   // obtain mid-signal from left and right
  FLOAT_TYPE S = (*left - *right)/std::sqrt((FLOAT_TYPE)2.0);   // obtain side-signal from left and right
    
    // amplify mid and side signal seperately:
    M *= 2.0*(1.0 - widthFactor);
    S *= 2.0*widthFactor;
    
    *left = (M + S)/std::sqrt((FLOAT_TYPE)2.0);   // obtain left signal from mid and side
    *right = (M - S)/std::sqrt((FLOAT_TYPE)2.0);   // obtain right signal from mid and side
}
template void USTProcess::StereoWiden(float *left, float *right, float widthFactor);
template void USTProcess::StereoWiden(double *left, double *right, double widthFactor);
#endif

// VERY GOOD:
// - presever loudness when decreasing stereo width
// - seems transparent if we set to mono after (no loudness change)
// - sound very well (comparable to IZotope Ozone)

// Custom version
//
// Algorithm:
// - samples to polar
// - rotate polar to have then on horizontal direction
// - scale them over y
// - rotate them back
// - polar to sample
//
#if STEREO_WIDEN_SQRT_CUSTOM
template <typename FLOAT_TYPE>
void
USTProcess::StereoWiden(FLOAT_TYPE *left, FLOAT_TYPE *right, FLOAT_TYPE widthFactor)
{
    FLOAT_TYPE l = *left;
    FLOAT_TYPE r = *right;
    
    // Samples to polar
    FLOAT_TYPE angle = std::atan2(r, l);
    FLOAT_TYPE dist = std::sqrt(l*l + r*r);
    
    // Rotate
    angle -= M_PI/4.0;
    
    // Get x, y coordinates
    FLOAT_TYPE x = dist*std::cos(angle);
    FLOAT_TYPE y = dist*std::sin(angle);
    
    // NOTE: sounds is bad with normalization
    // Normalization (1)
    //FLOAT_TYPE rad0 = std::sqrt(x*x + y*y);
    
    // Scale over y
    y *= widthFactor;
    
    // Back to polar
    dist = std::sqrt(x*x + y*y);
    angle = std::atan2(y, x);
    
    // Normalization (2)
    //dist = rad0;
    
    // Rotate back
    angle += M_PI/4.0;
    
    // Polar to sample
    l = dist*std::cos(angle);
    r = dist*std::sin(angle);
    
    // Result
    *left = l;
    *right = r;
}
template void USTProcess::StereoWiden(float *left, float *right, float widthFactor);
template void USTProcess::StereoWiden(double *left, double *right, double widthFactor);
#endif

#if STEREO_WIDEN_SQRT_CUSTOM_OPTIM
template <typename FLOAT_TYPE>
WDL_FFT_COMPLEX
USTProcess::StereoWidenComputeAngle0()
{
    WDL_FFT_COMPLEX result;
    
    FLOAT_TYPE magn = 1.0;
    FLOAT_TYPE phase = -M_PI/4.0;
    
    MAGN_PHASE_COMP(magn, phase, result);
    
    return result;
}
template WDL_FFT_COMPLEX USTProcess::StereoWidenComputeAngle0<float>();
template WDL_FFT_COMPLEX USTProcess::StereoWidenComputeAngle0<double>();

template <typename FLOAT_TYPE>
WDL_FFT_COMPLEX
USTProcess::StereoWidenComputeAngle1()
{
    WDL_FFT_COMPLEX result;
    
    FLOAT_TYPE magn = 1.0;
    FLOAT_TYPE phase = M_PI/4.0;
    
    MAGN_PHASE_COMP(magn, phase, result);
    
    return result;
}
template WDL_FFT_COMPLEX USTProcess::StereoWidenComputeAngle1<float>();
template WDL_FFT_COMPLEX USTProcess::StereoWidenComputeAngle1<double>();

template <typename FLOAT_TYPE>
void
USTProcess::StereoWiden(FLOAT_TYPE *left, FLOAT_TYPE *right, FLOAT_TYPE widthFactor,
                        const WDL_FFT_COMPLEX &angle0, WDL_FFT_COMPLEX &angle1)
{
    // Init
    WDL_FFT_COMPLEX signal0;
    signal0.re = *left;
    signal0.im = *right;
    
    // Rotate
    WDL_FFT_COMPLEX signal1;
    COMP_MULT(signal0, angle0, signal1);
    
    // Scale over y
    signal1.im *= widthFactor;
    
    // Rotate back
    WDL_FFT_COMPLEX signal2;
    COMP_MULT(signal1, angle1, signal2);
    
    // Result
    *left = signal2.re;
    *right = signal2.im;
}
template void USTProcess::StereoWiden(float *left, float *right, float widthFacator,
                                      const WDL_FFT_COMPLEX &angle0, WDL_FFT_COMPLEX &angle1);
template void USTProcess::StereoWiden(double *left, double *right, double widthFacator,
                                      const WDL_FFT_COMPLEX &angle0, WDL_FFT_COMPLEX &angle1);
#endif

template <typename FLOAT_TYPE>
WDL_FFT_COMPLEX
USTProcess::ComputeComplexRotation(FLOAT_TYPE angle)
{
    WDL_FFT_COMPLEX result;
    
    FLOAT_TYPE magn = 1.0;
    FLOAT_TYPE phase = angle;
    
    MAGN_PHASE_COMP(magn, phase, result);
    
    return result;
}
template WDL_FFT_COMPLEX USTProcess::ComputeComplexRotation(float angle);
template WDL_FFT_COMPLEX USTProcess::ComputeComplexRotation(double angle);

template <typename FLOAT_TYPE>
void
USTProcess::RotateSound(FLOAT_TYPE *left, FLOAT_TYPE *right, WDL_FFT_COMPLEX &rotation)
{
    // Init
    WDL_FFT_COMPLEX signal0;
    signal0.re = *left;
    signal0.im = *right;
    
    // Rotate
    WDL_FFT_COMPLEX signal1;
    COMP_MULT(signal0, rotation, signal1);
    
    // Result
    *left = signal1.re;
    *right = signal1.im;
}
template void USTProcess::RotateSound(float *left, float *right, WDL_FFT_COMPLEX &rotation);
template void USTProcess::RotateSound(double *left, double *right, WDL_FFT_COMPLEX &rotation);

template <typename FLOAT_TYPE>
FLOAT_TYPE
USTProcess::ApplyAngleShape(FLOAT_TYPE angle, FLOAT_TYPE shape)
{
    // Angle is in [0, Pi]
    FLOAT_TYPE angleShape = (angle - M_PI*0.5)/(M_PI*0.5);
    
    bool invert = false;
    if (angleShape < 0.0)
    {
        angleShape = -angleShape;
        
        invert = true;
    }
    
    angleShape = BLUtils::ApplyParamShape(angleShape, shape);
    
    if (invert)
        angleShape = -angleShape;
    
    angle = angleShape*M_PI*0.5 + M_PI*0.5;

    return angle;
}
template float USTProcess::ApplyAngleShape(float angle, float shape);
template double USTProcess::ApplyAngleShape(double angle, double shape);
