#ifndef REBALANCE_DEFS
#define REBALANCE_DEFS

//
//  Rebalance_defs.h
//  BL-Rebalance
//
//  Created by applematuer on 5/17/20.
//
//

//#ifndef BL_Rebalance_Rebalance_defs_h
//#define BL_Rebalance_Rebalance_defs_h

#define PROFILE_RNN 0 //1

// FIX: result is not good for 88200Hz for example
// So resample input data to 44100, process, then resample to
// current sample rate
//
// And better perfs for 88100Hz !
// 44100: 75% CPU
// 88200 (old): 120% CPU
// 88200 (new): 78% CPU
//
#define FORCE_SAMPLE_RATE 1 //0 //1
#define SAMPLE_RATE 44100.0

// Soft masks
#define USE_SOFTMASK_CHECKBOX 0 //1
#define SOFT_MASK_HISTO_SIZE 8

// Do not predict other, but keep the remaining
#define OTHER_IS_REST 0 //1 //0

// FIX: when using 192000Hz, with buffer size 64,
// the spectrogram was scaled horizontally progressively
//
// This was due to accumulated "rounding" error on the input buffer size
//
// GOOD: must be kept to 1
#define FIX_ADJUST_IN_RESAMPLING 1

// GOOD: must be kepts to 1 too
#define FIX_ADJUST_OUT_RESAMPLING 1

// Resample buffers and masks
//
// e.g 4 will make buffers of 256
//
#define RESAMPLE_FACTOR 1 //4

#define NUM_INPUT_COLS 32 //20
#define NUM_OUTPUT_COLS 32 //20

// Listen to what the network is listening
#define DEBUG_LISTEN_BUFFER 0

// Same config as in article Simpson & Roma
#define BUFFER_SIZE 2048
#define OVERSAMPLING 4

#define FREQ_RES 1

#define KEEP_SYNTHESIS_ENERGY 0

// When set to 0, we are accurate for frequencies
#define VARIABLE_HANNING 0

//#endif

// Reshape correctly to convert from 4 spectrogram colf of 356 freqs
// to a square of 32x32
#define RESHAPE_32_X_32 0 //1

#define DBG_PPM_COLOR_COEFF 64.0 //1.0

// Models
//#define MODEL_4X "rebalance-4x"
//#define MODEL_8X "rebalance-8x"
//#define MODEL_16X "rebalance-16x"
//#define MODEL_32X "rebalance-32x"

#define MODEL_NAME "rebalance"

#define USE_DEBUG_RADIO_BUTTONS 0 //1

enum RebalanceMode
{
    SOFT = 0,
    HARD
};

#endif
