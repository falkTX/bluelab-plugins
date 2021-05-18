//
//  BLTypes.h
//  UST
//
//  Created by applematuer on 8/18/20.
//
//

#ifndef UST_BLTypes_h
#define UST_BLTypes_h

#include <float.h>
//#include <limits.h>

#ifdef __cplusplus
#include <ScopedNoDenormals2.h>
#endif

#include "bl_config.h"

//
#ifndef BL_GUI_TYPE_FLOAT
#define BL_GUI_TYPE_FLOAT 0
#endif

#if BL_GUI_TYPE_FLOAT
#define BL_GUI_FLOAT float
#else
#define BL_GUI_FLOAT double
#endif

//
#ifndef BL_TYPE_FLOAT
#define BL_TYPE_FLOAT 0
#endif

// For external .c files
#if !BL_TYPE_FLOAT
#define POW pow
#define SQRT sqrt
#define TAN tan
#define SIN sin
#define COS cos
#define SINH sinh
#define FLOOR floor
#define FABS fabs
#define LOG10 log10
#define ASIN asin
#define EXP exp
#else
#define POW powf
#define SQRT sqrtf
#define TAN tanf
#define SIN sinf
#define COS cosf
#define SINH sinhf
#define FLOOR floorf
#define FABS fabsf
#define LOG10 log10f
#define ASIN asinf
#define EXP expf
#endif

#if BL_TYPE_FLOAT
#define BL_FLOAT float
#define WDL_FFT_REALSIZE 4 // WDL fft
#define WDL_RESAMPLE_TYPE float // WDL resampler
#define SAMPLE_TYPE float // Crossover
#define RTC_FLOAT_TYPE float // RTConvolve
#define SNDFILTER_FLOAT float // sndfilter
#define FV_FLOAT float // freeverb
//#define R8B_FLOAT float // does not work with float // r8brain reampler
#define R8B_FLOAT double // r8brain reampler
#define DECIMATOR_SAMPLE float
#define WDL_RESAMPLE_TYPE float
#define DCT_FLOAT float
#define WDL_SIMPLEPITCHSHIFT_SAMPLETYPE float
#else
#define BL_FLOAT double
#define WDL_FFT_REALSIZE 8 // WDL fft
#define WDL_RESAMPLE_TYPE double // WDL resampler
#define SAMPLE_TYPE double // Crossover
#define RTC_FLOAT_TYPE double // RTConvolve
#define SNDFILTER_FLOAT double // sndfilter
#define FV_FLOAT double // freeverb
#define R8B_FLOAT double // r8brain reampler
#define DECIMATOR_SAMPLE double
#define WDL_RESAMPLE_FULL_SINC_PRECISION
#define WDL_RESAMPLE_TYPE double
#define DCT_FLOAT double
#define WDL_SIMPLEPITCHSHIFT_SAMPLETYPE double
#endif

// When we inject samples of value 0, after having injected non-zero signal,
// the results becomes little and little, and we go under the value DBL_MIN,
// making double numbers that are not correct anymore.
// See: https://docs.microsoft.com/en-us/cpp/c-runtime-library/data-type-constants?view=vs-2019
// and DBL_TRUE_MIN
#if !BL_FIX_FLT_DENORMAL_OBJ

// Must check everywhere for denormal

// Set to 0 a bit before the denorm limit.
// Otherwise we are too close to the limit,
// and we should have checked every line of computation

// DLB_MIN*10
// See: https://docs.microsoft.com/en-us/cpp/c-runtime-library/data-type-constants?view=vs-2019
#define MY_DBL_MIN 2.2250738585072014e-307
#define MY_FLT_MIN 1.175494351e-37

#if !BL_TYPE_FLOAT
// NOTE: stdlib isnormal() uses a similar code as below i.e (fabs < DLB_MIN)
#define FIX_FLT_DENORMAL(__sample__) \
if ((__sample__ >= -MY_DBL_MIN) && (__sample__ <= MY_DBL_MIN)) \
__sample__ = 0.0;

#else

#define FIX_FLT_DENORMAL(__sample__) \
if ((__sample__ >= -MY_FLT_MIN) && (__sample__ <= MY_FLT_MIN)) \
__sample__ = 0.0f;
#endif

#define FIX_FLT_DENORMAL_INIT()

#else

// Define denormal obj only once (it writes to the CPU)

#define FIX_FLT_DENORMAL(__sample__)

#ifdef __cplusplus
#define FIX_FLT_DENORMAL_INIT() ScopedNoDenormals2 noDenormalsObj;
#else
#define FIX_FLT_DENORMAL_INIT()
#endif

#endif

#endif
