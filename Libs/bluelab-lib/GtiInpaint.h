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
//  GtiInpaint.h
//  BL-Ghost
//
//  Created by Pan on 09/07/18.
//
//

#ifndef __BL_Ghost__GtiInpaint__
#define __BL_Ghost__GtiInpaint__

#include <iostream>

#include <PbFft.h>

class GtiInpaint
{
public:
    // This object can be used only one time
    // After, create another !
    GtiInpaint(int w, int h);
    
    virtual ~GtiInpaint();
    
    void SetMask(BL_FLOAT *mask, const BL_FLOAT *im);
    
    BL_FLOAT* Gausstexinpaint(const BL_FLOAT *image, const BL_FLOAT *mask,
                            BL_FLOAT ep, int imax, int condw);

    int Ic(int i, int j, int c);
    
protected:
    void InitFft();
    
    void FftFft2d(BL_FLOAT *inRe, BL_FLOAT *outRe, BL_FLOAT *outIm);
    
    void FftFft2dInv(BL_FLOAT *inRe, BL_FLOAT *inIm, BL_FLOAT *outRe);

    void TermFft();
    
    void SetValue(BL_FLOAT *A, BL_FLOAT value, int size);
    
    void Adsn(BL_FLOAT *meanv, BL_FLOAT *v);
    
    void ConvolSingleChannel(BL_FLOAT *u, BL_FLOAT *w, BL_FLOAT *v);

    void Convcov(BL_FLOAT *u,BL_FLOAT *v,int *dom);
    
    void GetConditioningPoints(const BL_FLOAT *mask,int *cond,int condw);
    
    void EstimateAdsnModel(BL_FLOAT *u, const BL_FLOAT *mask,
                           BL_FLOAT *meanu, BL_FLOAT *t);
    
    void Cgd(int *cond,BL_FLOAT *rhs,BL_FLOAT *x,BL_FLOAT ep,int imax);
    
protected:
    // Height of the image.
    int height;
    
    //Width of the image.
    int width;
    
    //BL_FLOAT Height of the image.
    int height2;
    
    //BL_FLOAT Width of the image.
    int width2;
    
    // Size of Fourier transforms
    int ftsize;
    
    // output of Fourier transform
    PbFft::COMPLEX *fftOut;
    
    // input of Fourier transform
    BL_FLOAT *fftIn;
    
    // plans for direct and inverse FFT
    //static fftwf_plan fftw_plan_direct,fftw_plan_inverse;
    
    // BL_FLOAT arrays to store separately real and imaginary parts of FFT
    BL_FLOAT *re;
    BL_FLOAT *im;
    BL_FLOAT *ret;
    BL_FLOAT *imt;
    BL_FLOAT *retmp;
    BL_FLOAT *imtmp;
};

#endif /* defined(__BL_Ghost__GtiInpaint__) */
