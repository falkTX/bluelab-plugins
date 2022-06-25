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
//  Window.h
//  Denoiser
//
//  Created by Apple m'a Tuer on 25/04/17.
//
//

#ifndef __Denoiser__Window__
#define __Denoiser__Window__

#include "IPlug_include_in_plug_hdr.h"

#include <BLTypes.h>

class Window
{
public:
    static void MakeHanning(int size, WDL_TypedBuf<BL_FLOAT> *result);
    
    // 2D window
    
    // Buggy
    static void MakeHanningKernel(int size, WDL_TypedBuf<BL_FLOAT> *result);
    // Fixed
    static void MakeHanningKernel2(int size, WDL_TypedBuf<BL_FLOAT> *result);
    
    static void MakeRootHanning(int size, WDL_TypedBuf<BL_FLOAT> *result);
    
    static void MakeRootHanning2(int size, WDL_TypedBuf<BL_FLOAT> *result);
    
    static void MakeRoot2Hanning(int size, WDL_TypedBuf<BL_FLOAT> *result);
    
    static void MakeAnalysisHanning(int size, WDL_TypedBuf<BL_FLOAT> *result);
    
    static void MakeSynthesisHanning(int size, WDL_TypedBuf<BL_FLOAT> *result);
    
    static void MakeTriangular(int size, WDL_TypedBuf<BL_FLOAT> *result);
    
    static BL_FLOAT Gaussian(BL_FLOAT sigma, BL_FLOAT x);

    // Good version
    static void MakeGaussian(int size, BL_FLOAT sigma,
                             WDL_TypedBuf<BL_FLOAT> *result);

    // Adjusted for overlap-add, so that the bounds are 0
    static void MakeGaussianConfined(int size, BL_FLOAT sigma,
                                     WDL_TypedBuf<BL_FLOAT> *result);
    
    // Not used anymore
    static void MakeGaussianWholeDomain(int size, BL_FLOAT sigma,
                                        WDL_TypedBuf<BL_FLOAT> *result);
    
    // Try to fix the prev version
    static void MakeGaussian2(int size, BL_FLOAT sigma,
                              WDL_TypedBuf<BL_FLOAT> *result);
    
    // overlap is for example 2, or 4, meaning 50% and 75%
    // return the sum when passed to a constant signal of 1
    static BL_FLOAT CheckCOLA(const WDL_TypedBuf<BL_FLOAT> *result,
                              int overlap, BL_FLOAT outTimeStretchFactor);
    
    static void MakeHanningPow(int size, BL_FLOAT factor,
                               WDL_TypedBuf<BL_FLOAT> *window);
    
    static void MakeSquare(int size, BL_FLOAT value, WDL_TypedBuf<BL_FLOAT> *window);
   
    static void MakeNormSinc(int size, WDL_TypedBuf<BL_FLOAT> *window);
    
    static void MakeNormSincFilter(int size, BL_FLOAT fcSr,
                                   WDL_TypedBuf<BL_FLOAT> *window);
    
    static void MakeBlackman(int size, WDL_TypedBuf<BL_FLOAT> *window);
    
    // Compute COLA and normalize with it
    static void NormalizeWindow(WDL_TypedBuf<BL_FLOAT> *window,
                                int oversampling,
                                BL_FLOAT outTimeStretchFactor = 1.0);
    
    // Normalize using a factor
    static void NormalizeWindow(WDL_TypedBuf<BL_FLOAT> *window, BL_FLOAT factor);

    static void Apply(const WDL_TypedBuf<BL_FLOAT> &window,
                      WDL_TypedBuf<BL_FLOAT> *buf);
    
protected:
    static void makehanning( BL_FLOAT *H, BL_FLOAT *A, BL_FLOAT *S,
                             int Nw, int N, int I, int odd );
};

#endif /* defined(__Denoiser__Window__) */
