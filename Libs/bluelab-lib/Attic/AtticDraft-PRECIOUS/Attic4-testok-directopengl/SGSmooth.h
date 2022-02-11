//
//  Smooth.h
//  Denoiser
//
//  Created by Apple m'a Tuer on 08/05/17.
//
//

#ifndef Denoiser_Smooth_h
#define Denoiser_Smooth_h

// Savitzky-Golay smoothing

extern "C" {
double *calc_sgsmooth(const int ndat, double *input,
                      const int window, const int order);
}

#endif
