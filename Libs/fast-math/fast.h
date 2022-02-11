#ifndef FAST_H
#define FAST_H

// See: https://github.com/ekmett/approximate/blob/master/cbits/fast.c

extern double exp_fast_lb(double a);

/* 4607182418800017408 + 1 */
extern double exp_fast_ub(double a);

extern double exp_fast(double a);

extern double better_exp_fast(double a);


/* Schraudolph's published algorithm */
extern double exp_fast_schraudolph(double a);

/* 1065353216 + 1 */
extern float expf_fast_ub(float a);

/* Schraudolph's published algorithm with John's constants */
/* 1065353216 - 486411 = 1064866805 */
extern float expf_fast(float a);

//  1056478197 
extern double better_expf_fast(float a);

/* 1065353216 - 722019 */
extern float expf_fast_lb(float a);

/* Ankerl's inversion of Schraudolph's published algorithm, converted to explicit multiplication */
extern double log_fast_ankerl(double a);

extern double log_fast_ub(double a);

/* Ankerl's inversion of Schraudolph's published algorithm with my constants */
extern double log_fast(double a);

extern double log_fast_lb(double a);

/* 1065353216 - 722019 */
extern float logf_fast_ub(float a);

/* Ankerl's adaptation of Schraudolph's published algorithm with John's constants */
/* 1065353216 - 486411 = 1064866805 */
extern float logf_fast(float a);

/* 1065353216 + 1 */
extern float logf_fast_lb(float a);

/* Ankerl's version of Schraudolph's approximation. */
extern double pow_fast_ankerl(double a, double b);

/*
 These constants are based loosely on the following comment off of Ankerl's blog:

 "I have used the same trick for float, not double, with some slight modification to the constants to suite IEEE754 float format. The first constant for float is 1<<23/log(2) and the second is 127<<23 (for double they are 1<<20/log(2) and 1023<<20)." -- John
*/

/* 1065353216 + 1      = 1065353217 ub */
/* 1065353216 - 486411 = 1064866805 min RMSE */
/* 1065353216 - 722019 = 1064631197 lb */
extern float powf_fast(float a, float b);

extern float powf_fast_lb(float a, float b);

extern float powf_fast_ub(float a, float b);

/*
  Now that 64 bit arithmetic is cheap we can (try to) improve on Ankerl's algorithm.

 double long long approximation: round 1<<52/log(2) 6497320848556798,
  mask = 0x3ff0000000000000LL = 4607182418800017408LL

>>> round (2**52 * log (3 / (8 * log 2) + 1/2) / log 2 - 1/2)
261140389990638
>>> 0x3ff0000000000000 - round (2**52 * log (3 / (8 * log 2) + 1/2) / log 2 - 1/2)
4606921278410026770

*/

extern double pow_fast_ub(double a, double b);

extern double pow_fast(double a, double b);

extern double pow_fast_lb(double a, double b);

/* should be much more precise with large b, still ~3.3x faster. */
extern double pow_fast_precise_ankerl(double a, double b);
  
/* should be much more precise with large b, still ~3.3x faster. */
extern double pow_fast_precise(double a, double b);

extern double better_pow_fast_precise(double a, double b);

/* should be much more precise with large b */
extern float powf_fast_precise(float a, float b);

/* should be much more precise with large b */
extern float better_powf_fast_precise(float a, float b);

#endif
