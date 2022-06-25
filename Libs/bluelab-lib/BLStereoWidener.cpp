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
//  BLStereoWidener.cpp
//  BL
//
//  Created by applematuer on 1/3/20.
//
//

#include <BLWidthAdjuster.h>

#include <BLUtils.h>
#include <BLUtilsMath.h>
#include <BLUtilsComp.h>

#include <ParamSmoother2.h>

#include "BLStereoWidener.h"

// FIX: On Protools, when width parameter was reset to 0,
// by using alt + click, the gain was increased compared to bypass
//
// On Protools, when alt + click to reset parameter,
// Protools manages all, and does not reset the parameter exactly to the default value
// (precision 1e-8).
// Then the coeff is not exactly 1, then the gain coeff is applied.
#define FIX_WIDTH_PRECISION 1
#define WIDTH_PARAM_PRECISION 1e-6
#define WIDTH_PARAM_PRECISION_INV 1e6

#define MAX_WIDTH_FACTOR 2.0

BLStereoWidener::BLStereoWidener(BL_FLOAT sampleRate)
{
    // See: http://www.musicdsp.org/showArchiveComment.php?ArchiveID=256
    
    mAngle0 = ComputeAngle0();
    mAngle1 = ComputeAngle1();

    BL_FLOAT defaultStereoWidth = 0.0;
    mStereoWidthSmoother = new ParamSmoother2(sampleRate, defaultStereoWidth);
}

BLStereoWidener::~BLStereoWidener()
{
    delete mStereoWidthSmoother;
}

void
BLStereoWidener::Reset(BL_FLOAT sampleRate)
{
    if (mStereoWidthSmoother != NULL)
        mStereoWidthSmoother->Reset(sampleRate);
}

void
BLStereoWidener::StereoWiden(BL_FLOAT *l, BL_FLOAT *r, BL_FLOAT widthFactor) const
{
    BL_FLOAT width = ComputeFactor(widthFactor, MAX_WIDTH_FACTOR);
    
#if FIX_WIDTH_PRECISION
    width *= WIDTH_PARAM_PRECISION_INV;
    width = bl_round(width);
    width *= WIDTH_PARAM_PRECISION;
#endif
    
    StereoWiden(l, r, width, mAngle0, mAngle1);
}

void
BLStereoWidener::StereoWiden(BL_FLOAT *left, BL_FLOAT *right, BL_FLOAT widthFactor,
                             const WDL_FFT_COMPLEX &angle0,
                             const WDL_FFT_COMPLEX &angle1) const
{
    if (mStereoWidthSmoother != NULL)
    {
        mStereoWidthSmoother->SetTargetValue(widthFactor);
        widthFactor = mStereoWidthSmoother->Process();
    }
        
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

void
BLStereoWidener::StereoWiden(vector<WDL_TypedBuf<BL_FLOAT> * > *ioSamples,
                             BL_FLOAT widthFactor) const
{
    BL_FLOAT width = ComputeFactor(widthFactor, MAX_WIDTH_FACTOR);
    
#if FIX_WIDTH_PRECISION
    width *= WIDTH_PARAM_PRECISION_INV;
    width = bl_round(width);
    width *= WIDTH_PARAM_PRECISION;
#endif
    
    // Then do this per sample
    for (int i = 0; i < (*ioSamples)[0]->GetSize(); i++)
    {
        BL_FLOAT left = (*ioSamples)[0]->Get()[i];
        BL_FLOAT right = (*ioSamples)[1]->Get()[i];
        
        StereoWiden(&left, &right, width, mAngle0, mAngle1);
        
        (*ioSamples)[0]->Get()[i] = left;
        (*ioSamples)[1]->Get()[i] = right;
    }
}

// With new width adjuster
// Compute good width sample by sample
void
BLStereoWidener::StereoWiden(vector<WDL_TypedBuf<BL_FLOAT> * > *ioSamples,
                             BLWidthAdjuster *widthAdjuster) const
{
    // Do this per sample
    for (int i = 0; i < (*ioSamples)[0]->GetSize(); i++)
    {
        // Compute
        BL_FLOAT left = (*ioSamples)[0]->Get()[i];
        BL_FLOAT right = (*ioSamples)[1]->Get()[i];
        
        // Get current width
        widthAdjuster->Update(left, right);
        BL_FLOAT widthFactor = widthAdjuster->GetLimitedWidth();
        BL_FLOAT width = ComputeFactor(widthFactor, MAX_WIDTH_FACTOR);
        
#if FIX_WIDTH_PRECISION
        width *= WIDTH_PARAM_PRECISION_INV;
        width = bl_round(width);
        width *= WIDTH_PARAM_PRECISION;
#endif
        
        StereoWiden(&left, &right, width, mAngle0, mAngle1);
        
        (*ioSamples)[0]->Get()[i] = left;
        (*ioSamples)[1]->Get()[i] = right;
    }
}

WDL_FFT_COMPLEX
BLStereoWidener::ComputeAngle0()
{
    WDL_FFT_COMPLEX result;
    
    BL_FLOAT magn = 1.0;
    BL_FLOAT phase = -M_PI/4.0;
    
    MAGN_PHASE_COMP(magn, phase, result);
    
    return result;
}

WDL_FFT_COMPLEX
BLStereoWidener::ComputeAngle1()
{
    WDL_FFT_COMPLEX result;
    
    BL_FLOAT magn = 1.0;
    BL_FLOAT phase = M_PI/4.0;
    
    MAGN_PHASE_COMP(magn, phase, result);
    
    return result;
}

BL_FLOAT
BLStereoWidener::ComputeFactor(BL_FLOAT normVal, BL_FLOAT maxVal) const
{
    BL_FLOAT res = 0.0;
    if (normVal < 0.0)
        res = normVal + 1.0;
    else
        res = normVal*(maxVal - 1.0) + 1.0;
    
    return res;
}
