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
//  DitherMaker.cpp
//  Denoiser
//
//  Created by Apple m'a Tuer on 21/04/17.
//
//

#include <stdlib.h>
#include <float.h>

#include "DitherMaker.h"

void
DitherMaker::Dither(BL_FLOAT *samples, int nFrames)
{
    //BL_FLOAT coeff = DBL_MIN*1000;
    BL_FLOAT coeff = 0.00000001;
    
    for (int i = 0; i < nFrames; i++)
    {
        BL_FLOAT sample = samples[i];
        
        BL_FLOAT rnd = ((BL_FLOAT)rand())/RAND_MAX;
        
        sample = sample + rnd*coeff;
        
        samples[i] = sample;
    }
}
