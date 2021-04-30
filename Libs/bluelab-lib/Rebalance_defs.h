#ifndef REBALANCE_DEFS
#define REBALANCE_DEFS

//
//  Rebalance_defs.h
//  BL-Rebalance
//
//  Created by applematuer on 5/17/20.
//
//

// Soft masks
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

#define MODEL_NAME "rebalance"

#define NUM_STEM_SOURCES 4

#define REBALANCE_TARGET_SAMPLE_RATE 44100.0 //11025.0
#define REBALANCE_BUFFER_SIZE 2048
#define REBALANCE_TARGET_BUFFER_SIZE 2048 //512
#define REBALANCE_OVERLAPPING 4
#define REBALANCE_NUM_SPECTRO_FREQS 256
#define REBALANCE_NUM_SPECTRO_COLS 32
// Fastest
#define REBALANCE_PREDICT_MODULO_NUM 3

// Unused/Legacy, for old classes
#define REBALANCE_USE_MEL_FILTER_METHOD 1

// Use simple methond (loweer quality)
#define REBALANCE_MEL_METHOD_SIMPLE 0
// Use filter bank method (stairs effect)
#define REBALANCE_MEL_METHOD_FILTER 0 // 1
// Use Scale object with mel (best method, avoid stairs effect)
#define REBALANCE_MEL_METHOD_SCALE  1

#define USE_SOFT_MASKS 1

// NOTE: strangely, looks far better when set to 0
// (and will consume less CPU)
#define PROCESS_SIGNAL_DB 0 //1
// In theory, should be the same value as when training in darknet
// But in fact, in darkknet this is -60
// And here, we must set -120 to get beautiful spectrograms (visually)
#define PROCESS_SIGNAL_MIN_DB -120.0 //-60.0

// Threshold the masks (debug..)
#define USE_DBG_PREDICT_MASK_THRESHOLD 0 //1 // 0

// Basice sensivity
#define SENSIVITY_IS_SCALE 1

// Uunsed
enum RebalanceMode
{
    SOFT = 0,
    HARD
};

#endif
