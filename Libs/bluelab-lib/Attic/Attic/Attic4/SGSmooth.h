//
//  Smooth.h
//  Denoiser
//
//  Created by Apple m'a Tuer on 08/05/17.
//
//

#ifndef Denoiser_Smooth_h
#define Denoiser_Smooth_h

#include <BLTypes.h>

// Savitzky-Golay smoothing

extern "C" {
BL_FLOAT *calc_sgsmooth(const int ndat, BL_FLOAT *input,
                      const int window, const int order);
}

#endif
