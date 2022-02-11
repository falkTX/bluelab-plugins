//
//  Denoiser_defs.h
//  BL-Denoiser
//
//  Created by applematuer on 6/25/20.
//
//

#ifndef BL_Denoiser_Denoiser_defs_h
#define BL_Denoiser_Denoiser_defs_h

// Set to 1, in addition, it seems to fix a bug:
//
// Learn at 44100Hz (make a peak at 500Hz with BL-Sine
// Then switch to 88200 => the peak is no more at 500Hz
#define USE_VARIABLE_BUFFER_SIZE 1

#define USE_AUTO_RES_NOISE 1

#define USE_RESIDUAL_DENOISE 1

#define MIN_DB -120.0
#define MAX_DB 0.0
#define EPS_DB 1e-15

#endif
