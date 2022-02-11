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
DitherMaker::Dither(double *samples, int nFrames)
{
    //double coeff = DBL_MIN*1000;
    double coeff = 0.00000001;
    
    for (int i = 0; i < nFrames; i++)
    {
        double sample = samples[i];
        
        double rnd = ((double)rand())/RAND_MAX;
        
        sample = sample + rnd*coeff;
        
        samples[i] = sample;
    }
}