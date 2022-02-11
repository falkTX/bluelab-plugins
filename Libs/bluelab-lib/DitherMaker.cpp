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
