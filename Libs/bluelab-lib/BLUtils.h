//
//  Utils.h
//  Denoiser
//
//  Created by Apple m'a Tuer on 06/05/17.
//
//

#ifndef __Denoiser__BLUtils__
#define __Denoiser__BLUtils__

#include <math.h>

#include <limits.h>

#include <cmath>

#include <vector>
#include <deque>
using namespace std;

//#include <lice.h>

#include "IPlug_include_in_plug_hdr.h"

#include "../../WDL/fft.h"
#include "../../WDL/fastqueue.h"

#include <BLTypes.h>

using namespace iplug;

#ifndef MAX_PATH
#define MAX_PATH 512
#endif

#ifndef M_PI
#define M_PI 3.1415926535897932384
#endif

#ifndef M_TWO_PI
#define M_TWO_PI 6.28318530717958647692
#endif

#ifndef MIN
#define MIN(__x__, __y__) ((__x__ < __y__) ? __x__ : __y__)
#endif

#ifndef MAX
#define MAX(__x__, __y__) ((__x__ > __y__) ? __x__ : __y__)
#endif

#define COMP_MAGN(__x__) (std::sqrt((__x__).re * (__x__).re + (__x__).im * (__x__).im))
#define COMP_PHASE(__x__) (std::atan2((__x__).im, (__x__).re))

#define COMP_MAGNF(__x__) (sqrtf((__x__).re * (__x__).re + (__x__).im * (__x__).im))
#define COMP_PHASEF(__x__) (atan2f((__x__).im, (__x__).re))


#define MAGN_PHASE_COMP(__MAGN__, __PHASE__, __COMP__) \
__COMP__.re = __MAGN__*cos(__PHASE__);                 \
__COMP__.im = __MAGN__*sin(__PHASE__);

#define COMP_ADD(__COMP0__, __COMP1__, __RESULT__)     \
__RESULT__.re = __COMP0__.re + __COMP1__.re;           \
__RESULT__.im = __COMP0__.im + __COMP1__.im;

#define COMP_MULT(__COMP0__, __COMP1__, __RESULT__)                      \
__RESULT__.re = __COMP0__.re*__COMP1__.re - __COMP0__.im*__COMP1__.im;  \
__RESULT__.im = __COMP0__.im*__COMP1__.re + __COMP0__.re*__COMP1__.im;

// See: https://courses.lumenlearning.com/ivytech-collegealgebra/chapter/multiply-and-divide-complex-numbers/
#define COMP_DIV(__COMP0__, __COMP1__, __RESULT__)                                  \
{ BL_FLOAT denom = __COMP1__.re*__COMP1__.re + __COMP1__.im*__COMP1__.im;             \
BL_FLOAT denomInv = 1.0/denom;                                                        \
__RESULT__.re = (__COMP0__.re*__COMP1__.re + __COMP0__.im*__COMP1__.im)*denomInv;   \
__RESULT__.im = (__COMP1__.re*__COMP0__.im - __COMP0__.re*__COMP1__.im)*denomInv; }

#define SET_COLOR_FROM_INT(__COLOR__, __R__, __G__, __B__, __A__) \
__COLOR__[0] = ((float)__R__)/255.0; \
__COLOR__[1] = ((float)__G__)/255.0; \
__COLOR__[2] = ((float)__B__)/255.0; \
__COLOR__[3] = ((float)__A__)/255.0;

//#define BUFFER_SIZE 2048

#define UTILS_VALUE_UNDEFINED -1e16

#define BL_EPS 1e-15
#define BL_EPS3 1e-3
#define BL_EPS6 1e-6
#define BL_EPS8 1e-8
#define BL_EPS10 1e-10

#define BL_INF 1e15
#define BL_INF8 1e8
#define BL_INFI 1e9


#define Y_LOG_SCALE_FACTOR 3.5

inline double
bl_round(double x)
{ return round(x); }

inline float
bl_round(float x)
{ return roundf(x); }

// TODO: reorder the cpp too
class CMA2Smoother;
class BLUtils
{
public:
    static void SetUseSimdFlag(bool flag);
    
    // Sound utilitied
    //static FLOAT_TYPE ampToDB(FLOAT_TYPE amp, FLOAT_TYPE minDB);
    
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE DBToAmp(FLOAT_TYPE dB);
    
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE AmpToDB(FLOAT_TYPE dB);
    
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE AmpToDB(FLOAT_TYPE sampleVal, FLOAT_TYPE eps, FLOAT_TYPE minDB);
    
    template <typename FLOAT_TYPE>
    static void AmpToDB(WDL_TypedBuf<FLOAT_TYPE> *dBBuf,
                        const WDL_TypedBuf<FLOAT_TYPE> &ampBuf,
                        FLOAT_TYPE eps, FLOAT_TYPE minDB);

    template <typename FLOAT_TYPE>
    static void AmpToDB(FLOAT_TYPE *dBBuf, const FLOAT_TYPE *ampBuf, int bufSize,
                        FLOAT_TYPE eps, FLOAT_TYPE minDB);
    
    // Version without check
    template <typename FLOAT_TYPE>
    static void AmpToDB(WDL_TypedBuf<FLOAT_TYPE> *dBBuf,
                        const WDL_TypedBuf<FLOAT_TYPE> &ampBuf);
    
    // OPTIM PROF Infra
    template <typename FLOAT_TYPE>
    static void AmpToDB(WDL_TypedBuf<FLOAT_TYPE> *ioBuf, FLOAT_TYPE eps, FLOAT_TYPE minDB);
    
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE AmpToDBClip(FLOAT_TYPE sampleVal, FLOAT_TYPE eps, FLOAT_TYPE minDB);
    
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE AmpToDBNorm(FLOAT_TYPE sampleVal, FLOAT_TYPE eps, FLOAT_TYPE minDB);
    
    template <typename FLOAT_TYPE>
    static void AmpToDBNorm(WDL_TypedBuf<FLOAT_TYPE> *dBBufNorm,
                            const WDL_TypedBuf<FLOAT_TYPE> &ampBuf,
                            FLOAT_TYPE eps, FLOAT_TYPE minDB);
    
    template <typename FLOAT_TYPE>
    static void AmpToDBNorm(WDL_TypedBuf<FLOAT_TYPE> *ioBuf,
                            FLOAT_TYPE eps, FLOAT_TYPE minDB);

    template <typename FLOAT_TYPE>
    static void DBToAmp(WDL_TypedBuf<FLOAT_TYPE> *ioBuf);
    
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE DBToAmpNorm(FLOAT_TYPE sampleVal, FLOAT_TYPE eps, FLOAT_TYPE minDB);
    
    template <typename FLOAT_TYPE>
    static void DBToAmpNorm(WDL_TypedBuf<FLOAT_TYPE> *ioBuf,
                            FLOAT_TYPE eps, FLOAT_TYPE minDB);
    
    // Convert the normalized x to dB, then renormalize it
    // So it is still normalized, but with a dB progression
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE NormalizedXTodB(FLOAT_TYPE x, FLOAT_TYPE mindB, FLOAT_TYPE maxdB);
    
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE NormalizedXTodBInv(FLOAT_TYPE x, FLOAT_TYPE mindB, FLOAT_TYPE maxdB);
    
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE NormalizedYTodB(FLOAT_TYPE y, FLOAT_TYPE mindB, FLOAT_TYPE maxdB);

    template <typename FLOAT_TYPE>
    static void NormalizedYTodB(const WDL_TypedBuf<FLOAT_TYPE> &yBuf,
                                FLOAT_TYPE mindB, FLOAT_TYPE maxdB,
                                WDL_TypedBuf<FLOAT_TYPE> *resBuf);
        
    // Variant for vertical graph axis
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE NormalizedYTodB2(FLOAT_TYPE y, FLOAT_TYPE mindB, FLOAT_TYPE maxdB);
    
    // Copy of NormalizedXTodB.
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE NormalizedYTodB3(FLOAT_TYPE y, FLOAT_TYPE mindB, FLOAT_TYPE maxdB);
    
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE NormalizedYTodBInv(FLOAT_TYPE y, FLOAT_TYPE mindB, FLOAT_TYPE maxdB);
    
    //
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE AverageYDB(FLOAT_TYPE y0, FLOAT_TYPE y1, FLOAT_TYPE mindB, FLOAT_TYPE maxdB);
    
    
    template <typename FLOAT_TYPE>
    static void StereoToMono(WDL_TypedBuf<FLOAT_TYPE> *monoResult,
                             const FLOAT_TYPE *in0, const FLOAT_TYPE *in1, int nFrames);
    
    template <typename FLOAT_TYPE>
    static void StereoToMono(WDL_TypedBuf<FLOAT_TYPE> *monoResult,
                             const WDL_TypedBuf<FLOAT_TYPE> &in0,
                             const WDL_TypedBuf<FLOAT_TYPE> &in1);
    
    template <typename FLOAT_TYPE>
    static void StereoToMono(WDL_TypedBuf<FLOAT_TYPE> *monoResult,
                             const vector< WDL_TypedBuf<FLOAT_TYPE> > &in0);

    template <typename FLOAT_TYPE>
    static void StereoToMono(vector<WDL_TypedBuf<FLOAT_TYPE> > *samplesVec);
    
    static void StereoToMono(WDL_TypedBuf<WDL_FFT_COMPLEX> *monoResult,
                             const vector< WDL_TypedBuf<WDL_FFT_COMPLEX> > &in0);
    
    template <typename FLOAT_TYPE>
    static void Mix(FLOAT_TYPE *output, FLOAT_TYPE *buf0, FLOAT_TYPE *buf1, int nFrames, FLOAT_TYPE mix);
    
    template <typename FLOAT_TYPE>
    static void Fade(const WDL_TypedBuf<FLOAT_TYPE> &buf0,
                     const WDL_TypedBuf<FLOAT_TYPE> &buf1,
                     FLOAT_TYPE *resultBuf, FLOAT_TYPE fadeStart, FLOAT_TYPE fadeEnd);
    
    template <typename FLOAT_TYPE>
    static void Fade(WDL_TypedBuf<FLOAT_TYPE> *buf,
                     FLOAT_TYPE fadeStart, FLOAT_TYPE fadeEnd, bool fadeIn);
    
    template <typename FLOAT_TYPE>
    static void Fade(FLOAT_TYPE *buf, int bufSize,
                     FLOAT_TYPE fadeStart, FLOAT_TYPE fadeEnd, bool fadeIn);
    
    template <typename FLOAT_TYPE>
    static void Fade(const WDL_TypedBuf<FLOAT_TYPE> &buf0,
                     const WDL_TypedBuf<FLOAT_TYPE> &buf1,
                     FLOAT_TYPE *resultBuf,
                     FLOAT_TYPE fadeStart, FLOAT_TYPE fadeEnd,
                     FLOAT_TYPE startT, FLOAT_TYPE endT);
    
    template <typename FLOAT_TYPE>
    static void Fade(WDL_TypedBuf<FLOAT_TYPE> *ioBuf0,
                     const WDL_TypedBuf<FLOAT_TYPE> &buf1,
                     FLOAT_TYPE fadeStart, FLOAT_TYPE fadeEnd,
                     bool fadeIn,
                     FLOAT_TYPE startPos = 0.0, FLOAT_TYPE endPos = 1.0);
    
    //
    template <typename FLOAT_TYPE>
    static void FadeOut(WDL_TypedBuf<FLOAT_TYPE> *ioBuf,
                        int startSampleId, int endSampleId);
    
    // Cross fade the beginning and the end
    template <typename FLOAT_TYPE>
    static void Fade(WDL_TypedBuf<FLOAT_TYPE> *ioBuf0,
                           const WDL_TypedBuf<FLOAT_TYPE> &buf1,
                           FLOAT_TYPE fadeStart, FLOAT_TYPE fadeEnd);
    
    template <typename FLOAT_TYPE>
    static void Fade(FLOAT_TYPE *ioBuf0Data,
                           const FLOAT_TYPE *buf1Data,
                           int bufSize,
                           FLOAT_TYPE fadeStart, FLOAT_TYPE fadeEnd);
    
    template <typename FLOAT_TYPE>
    static void AntiClipping(WDL_TypedBuf<FLOAT_TYPE> *values, FLOAT_TYPE maxValue);
    
    template <typename FLOAT_TYPE>
    static void SamplesAntiClipping(WDL_TypedBuf<FLOAT_TYPE> *samples, FLOAT_TYPE maxValue);
    
    // Plugin operations
    static void PlugInits();
    
    static void BypassPlug(double **inputs, double **outputs, int nFrames);
    
    // Get all the buffers available to the plugin
    static void GetPlugIOBuffers(Plugin *plug, double **inputs, double **outputs,
                                 double *inp[2], double *scIn[2], double *outp[2]);
    
    static bool GetPlugIOBuffers(Plugin *plug,
                                 double **inputs, double **outputs, int nFrames,
                                 vector<WDL_TypedBuf<BL_FLOAT> > *inp,
                                 vector<WDL_TypedBuf<BL_FLOAT> > *scIn,
                                 vector<WDL_TypedBuf<BL_FLOAT> > *outp);
    
    // Touch plug param for automation to be written
    static void TouchPlugParam(Plugin *plug, int paramIdx);
    
    // For a give index, try to get both in and out buffers
    static bool GetIOBuffers(int index, double *in[2], double *out[2],
                             double **inBuf, double **outBuf);
    
    static bool GetIOBuffers(int index,
                             vector<WDL_TypedBuf<double> > &in,
                             vector<WDL_TypedBuf<double> > &out,
                             double **inBuf, double **outBuf);
    
    static bool GetIOBuffers(int index,
                             vector<WDL_TypedBuf<double> > &in,
                             vector<WDL_TypedBuf<double> > &out,
                             WDL_TypedBuf<double> **inBuf,
                             WDL_TypedBuf<double> **outBuf);
    
    static bool PlugIOAllZero(double *inputs[2], double *outputs[2], int nFrames);
    
    static bool PlugIOAllZero(const vector<WDL_TypedBuf<BL_FLOAT> > &inputs,
                              const vector<WDL_TypedBuf<BL_FLOAT> > &outputs);
    
    static void PlugCopyOutputs(const vector<WDL_TypedBuf<BL_FLOAT> > &outp,
                                double **outputs, int nFrames);
    
    static int PlugComputeBufferSize(int bufferSize, BL_FLOAT sampleRate);
    
    static void PlugUpdateLatency(Plugin *plug,
                                  int nativeBufferSize, int nativeLatency,
                                  BL_FLOAT sampleRate);

    static int PlugComputeLatency(Plugin *plug,
                                  int nativeBufferSize, int nativeLatency,
                                  BL_FLOAT sampleRate);
    
    static BL_FLOAT GetBufferSizeCoeff(Plugin *plug, int nativeBufferSize);

    static bool ChannelAllZero(const vector<WDL_TypedBuf<BL_FLOAT> > &channel);
    
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE ApplyParamShape(FLOAT_TYPE normVal, FLOAT_TYPE shape);

    template <typename FLOAT_TYPE>
    static void ApplyParamShape(WDL_TypedBuf<FLOAT_TYPE> *normVals, FLOAT_TYPE shape);
    
    template <typename FLOAT_TYPE>
    static void ApplyParamShapeWaveform(WDL_TypedBuf<FLOAT_TYPE> *normVals, FLOAT_TYPE shape);
    
    // Compute the shape necessary to apply to the WDL param to have the 0
    // when the knob is at 12h
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE ComputeShapeForCenter0(FLOAT_TYPE minKnobValue, FLOAT_TYPE maxKnobValue);
    
    // Operations on arrays
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE ComputeSquareSum(const FLOAT_TYPE *values, int nFrames);
    
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE ComputeRMSAvg(const FLOAT_TYPE *buf, int nFrames);
    
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE ComputeRMSAvg(const WDL_TypedBuf<FLOAT_TYPE> &values);
    
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE ComputeRMSAvg2(const FLOAT_TYPE *buf, int nFrames);
    
    // Does not give good results with UST
    // (this looks false)
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE ComputeRMSAvg2(const WDL_TypedBuf<FLOAT_TYPE> &buf);
    
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE ComputeAvg(const FLOAT_TYPE *buf, int nFrames);
    
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE ComputeAvg(const WDL_TypedBuf<FLOAT_TYPE> &buf);
    
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE ComputeAvg(const WDL_TypedBuf<FLOAT_TYPE> &buf,
                                 int startIndex, int endIndex);
    
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE ComputeAvg(const vector<WDL_TypedBuf<FLOAT_TYPE> > &image);
    
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE ComputeAvg(const vector<FLOAT_TYPE> &buf);
    
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE ComputeAvgSquare(const FLOAT_TYPE *buf, int nFrames);
    
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE ComputeAvgSquare(const WDL_TypedBuf<FLOAT_TYPE> &buf);
    
    template <typename FLOAT_TYPE>
    static void ComputeSquare(WDL_TypedBuf<FLOAT_TYPE> *buf);

    static void ComputeSquareConjugate(WDL_TypedBuf<WDL_FFT_COMPLEX> *buf);

    template <typename FLOAT_TYPE>
    static void ComputeOpposite(WDL_TypedBuf<FLOAT_TYPE> *buf);
    
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE ComputeAbsAvg(const FLOAT_TYPE *buf, int nFrames);
    
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE ComputeAbsAvg(const WDL_TypedBuf<FLOAT_TYPE> &buf);
    
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE ComputeAbsAvg(const WDL_TypedBuf<FLOAT_TYPE> &buf,
                                int startIndex, int endIndex);
    
    
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE ComputeMax(const WDL_TypedBuf<FLOAT_TYPE> &buf);
    
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE ComputeMax(const FLOAT_TYPE *buf, int nFrames);
    
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE ComputeMax(const vector<WDL_TypedBuf<FLOAT_TYPE> > &values);
    
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE ComputeMaxAbs(const FLOAT_TYPE *buf, int nFrames);
    
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE ComputeMaxAbs(const WDL_TypedBuf<FLOAT_TYPE> &buf);
    
    template <typename FLOAT_TYPE>
    static void ComputeMax(WDL_TypedBuf<FLOAT_TYPE> *max, const WDL_TypedBuf<FLOAT_TYPE> &buf);
    
    template <typename FLOAT_TYPE>
    static void ComputeMax(WDL_TypedBuf<FLOAT_TYPE> *max, const FLOAT_TYPE *buf);
    
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE ComputeMin(const WDL_TypedBuf<FLOAT_TYPE> &buf);
    
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE ComputeMin(const vector<WDL_TypedBuf<FLOAT_TYPE> > &values);
    
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE ComputeSum(const FLOAT_TYPE *buf, int nFrames);

    template <typename FLOAT_TYPE>
    static FLOAT_TYPE ComputeSum(const WDL_TypedBuf<FLOAT_TYPE> &buf);

    template <typename FLOAT_TYPE>
    static FLOAT_TYPE ComputeAbsSum(const FLOAT_TYPE *buf, int nFrames);

    template <typename FLOAT_TYPE>
    static FLOAT_TYPE ComputeAbsSum(const WDL_TypedBuf<FLOAT_TYPE> &buf);

    template <typename FLOAT_TYPE>
    static FLOAT_TYPE ComputeClipSum(const WDL_TypedBuf<FLOAT_TYPE> &buf);
    
    
    template <typename FLOAT_TYPE>
    static void ComputeSum(const WDL_TypedBuf<FLOAT_TYPE> &buf0,
                           const WDL_TypedBuf<FLOAT_TYPE> &buf1,
                           WDL_TypedBuf<FLOAT_TYPE> *result);
    
    template <typename FLOAT_TYPE>
    static void ComputeProduct(const WDL_TypedBuf<FLOAT_TYPE> &buf0,
                               const WDL_TypedBuf<FLOAT_TYPE> &buf1,
                               WDL_TypedBuf<FLOAT_TYPE> *result);
    
    template <typename FLOAT_TYPE>
    static void ComputeAbs(WDL_TypedBuf<FLOAT_TYPE> *values);
    
    template <typename FLOAT_TYPE>
    static void ComputeAvg(WDL_TypedBuf<FLOAT_TYPE> *avg,
                           const WDL_TypedBuf<FLOAT_TYPE> &values0,
                           const WDL_TypedBuf<FLOAT_TYPE> &values1);
    
    template <typename FLOAT_TYPE>
    static void ComputeAvg(FLOAT_TYPE *avg,
                           const FLOAT_TYPE *values0, const FLOAT_TYPE *values1,
                           int nFrames);
    
    template <typename FLOAT_TYPE>
    static void ComplexSum(WDL_TypedBuf<FLOAT_TYPE> *ioMagns,
                           WDL_TypedBuf<FLOAT_TYPE> *ioPhases,
                           const WDL_TypedBuf<FLOAT_TYPE> &magns,
                           WDL_TypedBuf<FLOAT_TYPE> &phases);
    
    template <typename FLOAT_TYPE>
    static bool IsAllZero(const FLOAT_TYPE *buffer, int nFrames);
    
    template <typename FLOAT_TYPE>
    static bool IsAllZero(const WDL_TypedBuf<FLOAT_TYPE> &buffer);
    
    static bool IsAllZeroComp(const WDL_TypedBuf<WDL_FFT_COMPLEX> &buffer);
    
    static bool IsAllZeroComp(const WDL_FFT_COMPLEX *buffer, int bufLen);
    
    template <typename FLOAT_TYPE>
    static bool IsAllSmallerEps(const WDL_TypedBuf<FLOAT_TYPE> &buffer, FLOAT_TYPE eps);
    
    template <typename FLOAT_TYPE>
    static void FillAllZero(WDL_TypedBuf<FLOAT_TYPE> *ioBuf);
    
    static void FillAllZero(WDL_TypedBuf<int> *ioBuf);
    
    static void FillAllZero(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuf);

    template <typename FLOAT_TYPE>
    static void FillAllZero(vector<WDL_TypedBuf<FLOAT_TYPE> > *samples);
    
    template <typename FLOAT_TYPE>
    static void FillAllValue(WDL_TypedBuf<FLOAT_TYPE> *ioBuf, FLOAT_TYPE value);
    
    //static void FillAllValue(WDL_TypedBuf<float> *ioBuf, float value);
    
    template <typename FLOAT_TYPE>
    static void FillAllZero(FLOAT_TYPE *ioBuf, int size);
    
    template <typename FLOAT_TYPE>
    static void FillZero(WDL_TypedBuf<FLOAT_TYPE> *ioBuf, int numZeros);

    template <typename FLOAT_TYPE>
    static void AddZeros(WDL_TypedBuf<FLOAT_TYPE> *ioBuf, int size);
    
    template <typename FLOAT_TYPE>
    static void AddValues(WDL_TypedBuf<FLOAT_TYPE> *ioBuf, const WDL_TypedBuf<FLOAT_TYPE> &addBuf);
   
    template <typename FLOAT_TYPE>
    static void AddValues(FLOAT_TYPE *ioBuf, const FLOAT_TYPE *addBuf, int bufSize);
    
    template <typename FLOAT_TYPE>
    static void AddValues(WDL_TypedBuf<FLOAT_TYPE> *result,
                          const WDL_TypedBuf<FLOAT_TYPE> &buf0,
                          const WDL_TypedBuf<FLOAT_TYPE> &buf1);
    
    template <typename FLOAT_TYPE>
    static void AddValues(WDL_TypedBuf<FLOAT_TYPE> *ioBuf, const FLOAT_TYPE *addBuf);
    
    static void AddValues(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuf,
                          const WDL_TypedBuf<WDL_FFT_COMPLEX> &addBuf);
    
    template <typename FLOAT_TYPE>
    static void SubstractValues(WDL_TypedBuf<FLOAT_TYPE> *ioBuf,
                                const WDL_TypedBuf<FLOAT_TYPE> &subBuf);
    
    static void SubstractValues(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuf,
                                const WDL_TypedBuf<WDL_FFT_COMPLEX> &subBuf);
    
    template <typename FLOAT_TYPE>
    static void ComputeDiff(WDL_TypedBuf<FLOAT_TYPE> *resultDiff,
                            const WDL_TypedBuf<FLOAT_TYPE> &buf0,
                            const WDL_TypedBuf<FLOAT_TYPE> &buf1);
    
    template <typename FLOAT_TYPE>
    static void ComputeDiff(FLOAT_TYPE *resultDiff,
                            const FLOAT_TYPE* buf0, const FLOAT_TYPE* buf1,
                            int bufSize);

    
    static void ComputeDiff(WDL_TypedBuf<WDL_FFT_COMPLEX> *resultDiff,
                            const WDL_TypedBuf<WDL_FFT_COMPLEX> &buf0,
                            const WDL_TypedBuf<WDL_FFT_COMPLEX> &buf1);
    
    static void ComputeDiff(WDL_TypedBuf<WDL_FFT_COMPLEX> *resultDiff,
                            const vector<WDL_FFT_COMPLEX> &buf0,
                            const vector<WDL_FFT_COMPLEX> &buf1);
    
    template <typename FLOAT_TYPE>
    static void Permute(WDL_TypedBuf<FLOAT_TYPE> *values,
                        const WDL_TypedBuf<int> &indices,
                        bool forward);
    
    template <typename FLOAT_TYPE>
    static void ClipMin(WDL_TypedBuf<FLOAT_TYPE> *values, FLOAT_TYPE min);
    
    // clipVal is the clip limit, minVal is the value set when below the limit
    template <typename FLOAT_TYPE>
    static void ClipMin2(WDL_TypedBuf<FLOAT_TYPE> *values, FLOAT_TYPE clipVal, FLOAT_TYPE minVal);
    
    template <typename FLOAT_TYPE>
    static void ClipMax(WDL_TypedBuf<FLOAT_TYPE> *values, FLOAT_TYPE max);
    
    template <typename FLOAT_TYPE>
    static void ClipMinMax(FLOAT_TYPE *value, FLOAT_TYPE min, FLOAT_TYPE max);
    
    template <typename FLOAT_TYPE>
    static void ClipMinMax(WDL_TypedBuf<FLOAT_TYPE> *values, FLOAT_TYPE min, FLOAT_TYPE max);
    
    template <typename FLOAT_TYPE>
    static void Diff(WDL_TypedBuf<FLOAT_TYPE> *diff,
                     const WDL_TypedBuf<FLOAT_TYPE> &prevValues,
                     const WDL_TypedBuf<FLOAT_TYPE> &values);
    
    template <typename FLOAT_TYPE>
    static void ApplyDiff(WDL_TypedBuf<FLOAT_TYPE> *values,
                          const WDL_TypedBuf<FLOAT_TYPE> &diff);
    
    template <typename FLOAT_TYPE>
    static bool IsEqual(const WDL_TypedBuf<FLOAT_TYPE> &values0,
                        const WDL_TypedBuf<FLOAT_TYPE> &values1);
    
    template <typename FLOAT_TYPE>
    static void ReplaceValue(WDL_TypedBuf<FLOAT_TYPE> *values, FLOAT_TYPE srcValue, FLOAT_TYPE dstValue);
    
    // Specific operations on arrays
    template <typename FLOAT_TYPE>
    static void AppendValues(WDL_TypedBuf<FLOAT_TYPE> *ioBuffer, const WDL_TypedBuf<FLOAT_TYPE> &values);
    
    template <typename FLOAT_TYPE>
    static void ConsumeLeft(WDL_TypedBuf<FLOAT_TYPE> *ioBuffer, int numToConsume);
    
    template <typename FLOAT_TYPE>
    static void ConsumeRight(WDL_TypedBuf<FLOAT_TYPE> *ioBuffer, int numToConsume);
    
    // Consume one value of the vector
    template <typename FLOAT_TYPE>
    static void ConsumeLeft(vector<WDL_TypedBuf<FLOAT_TYPE> > *ioBuffer);
    
    template <typename FLOAT_TYPE>
    static void TakeHalf(WDL_TypedBuf<FLOAT_TYPE> *buf);
    
    static void TakeHalf(WDL_TypedBuf<WDL_FFT_COMPLEX> *buf);

    template <typename FLOAT_TYPE>
    static void TakeHalf(const WDL_TypedBuf<FLOAT_TYPE> &inBuf,
                         WDL_TypedBuf<FLOAT_TYPE> *outBuf);
    
    static void TakeHalf(const WDL_TypedBuf<WDL_FFT_COMPLEX> &inBuf,
                         WDL_TypedBuf<WDL_FFT_COMPLEX> *outBuf);
    
    //static void TakeHalf(WDL_TypedBuf<WDL_FFT_COMPLEX> *res, const WDL_TypedBuf<WDL_FFT_COMPLEX> &buf);
    
    template <typename FLOAT_TYPE>
    static void ResizeFillZeros(WDL_TypedBuf<FLOAT_TYPE> *buf, int newSize);
    
    static void ResizeFillZeros(WDL_TypedBuf<WDL_FFT_COMPLEX> *buf, int newSize);
    
    template <typename FLOAT_TYPE>
    static void ResizeFillAllZeros(WDL_TypedBuf<FLOAT_TYPE> *buf, int newSize);
    
    template <typename FLOAT_TYPE>
    static void ResizeFillValue(WDL_TypedBuf<FLOAT_TYPE> *buf, int newSize, FLOAT_TYPE value);
    
    template <typename FLOAT_TYPE>
    static void ResizeFillValue2(WDL_TypedBuf<FLOAT_TYPE> *buf, int newSize, FLOAT_TYPE value);
    
    template <typename FLOAT_TYPE>
    static void ResizeFillRandom(WDL_TypedBuf<FLOAT_TYPE> *buf, int newSize, FLOAT_TYPE coeff);
    
    template <typename FLOAT_TYPE>
    static void GrowFillZeros(WDL_TypedBuf<FLOAT_TYPE> *buf, int numGrow);
    
    template <typename FLOAT_TYPE>
    static void InsertZeros(WDL_TypedBuf<FLOAT_TYPE> *buf, int index, int numZeros);
    
    template <typename FLOAT_TYPE>
    static void InsertValues(WDL_TypedBuf<FLOAT_TYPE> *buf, int index, int numValues, FLOAT_TYPE value);
    
    template <typename FLOAT_TYPE>
    static void RemoveValuesCyclic(WDL_TypedBuf<FLOAT_TYPE> *buf, int index, int numValues);
    
    template <typename FLOAT_TYPE>
    static void RemoveValuesCyclic2(WDL_TypedBuf<FLOAT_TYPE> *buf, int index, int numValues);
    
    template <typename FLOAT_TYPE>
    static void AddValues(WDL_TypedBuf<FLOAT_TYPE> *buf, FLOAT_TYPE value);
    
    template <typename FLOAT_TYPE>
    static void MultValues(WDL_TypedBuf<FLOAT_TYPE> *buf, FLOAT_TYPE value);
    
    template <typename FLOAT_TYPE>
    static void MultValues(vector<WDL_TypedBuf<FLOAT_TYPE> > *buf, FLOAT_TYPE value);
    
    template <typename FLOAT_TYPE>
    static void MultValuesRamp(WDL_TypedBuf<FLOAT_TYPE> *buf, FLOAT_TYPE value0, FLOAT_TYPE value1);
    
    template <typename FLOAT_TYPE>
    static void MultValues(WDL_TypedBuf<FLOAT_TYPE> *buf, const WDL_TypedBuf<FLOAT_TYPE> &values);
    
    template <typename FLOAT_TYPE>
    static void MultValues(FLOAT_TYPE *buf, int size, FLOAT_TYPE value);
    
    template <typename FLOAT_TYPE>
    static void MultValues(WDL_TypedBuf<WDL_FFT_COMPLEX> *buf, FLOAT_TYPE value);
    
    template <typename FLOAT_TYPE>
    static void MultValues(WDL_TypedBuf<WDL_FFT_COMPLEX> *buf, const WDL_TypedBuf<FLOAT_TYPE> &values);
    
    static void MultValues(WDL_TypedBuf<WDL_FFT_COMPLEX> *buf, const WDL_TypedBuf<WDL_FFT_COMPLEX> &values);
    
    template <typename FLOAT_TYPE>
    static void ApplyPow(WDL_TypedBuf<FLOAT_TYPE> *values, FLOAT_TYPE exp);
    
    template <typename FLOAT_TYPE>
    static void ApplyExp(WDL_TypedBuf<FLOAT_TYPE> *values);
    
    template <typename FLOAT_TYPE>
    static void PadZerosLeft(WDL_TypedBuf<FLOAT_TYPE> *buf, int padSize);
   
    template <typename FLOAT_TYPE>
    static void PadZerosRight(WDL_TypedBuf<FLOAT_TYPE> *buf, int padSize);

    template <typename FLOAT_TYPE>
    static void CopyBuf(WDL_TypedBuf<FLOAT_TYPE> *toBuf, const WDL_TypedBuf<FLOAT_TYPE> &fromBuf);
    
    template <typename FLOAT_TYPE>
    static void CopyBuf(WDL_TypedBuf<FLOAT_TYPE> *toBuf, const FLOAT_TYPE *fromData, int fromSize);
    
    template <typename FLOAT_TYPE>
    static void CopyBuf(FLOAT_TYPE *toBuf, const FLOAT_TYPE *fromData, int fromSize);
    
    template <typename FLOAT_TYPE>
    static void CopyBuf(FLOAT_TYPE *toData, const WDL_TypedBuf<FLOAT_TYPE> &fromBuf);
    
    template <typename FLOAT_TYPE>
    static void Replace(WDL_TypedBuf<FLOAT_TYPE> *dst, int startIdx, const WDL_TypedBuf<FLOAT_TYPE> &src);
    
    
    template <typename FLOAT_TYPE>
    static void ComputeAvg(WDL_TypedBuf<FLOAT_TYPE> *result, const vector<WDL_TypedBuf<FLOAT_TYPE> > &bufs);
    
    template <typename FLOAT_TYPE>
    static void MakeSymmetry(WDL_TypedBuf<FLOAT_TYPE> *symBuf, const WDL_TypedBuf<FLOAT_TYPE> &buf);
    
    template <typename FLOAT_TYPE>
    static void Reverse(WDL_TypedBuf<FLOAT_TYPE> *values);
    
    static void Reverse(WDL_TypedBuf<int> *values);
    
    template <typename FLOAT_TYPE>
    static void ApplySqrt(WDL_TypedBuf<FLOAT_TYPE> *values);
    
    // Curves
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE ComputeCurveMatchCoeff(const FLOAT_TYPE *curve0, const FLOAT_TYPE *curve1, int nFrames);
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE ComputeCurveMatchCoeff2(const FLOAT_TYPE *curve0, const FLOAT_TYPE *curve1, int nFrames);
    
    // Fft
    template <typename FLOAT_TYPE>
    static void ComplexToMagn(WDL_TypedBuf<FLOAT_TYPE> *result,
                              const WDL_TypedBuf<WDL_FFT_COMPLEX> &complexBuf);
    
    template <typename FLOAT_TYPE>
    static void ComplexToPhase(WDL_TypedBuf<FLOAT_TYPE> *result,
                               const WDL_TypedBuf<WDL_FFT_COMPLEX> &complexBuf);
    
    template <typename FLOAT_TYPE>
    static void ComplexToMagnPhase(WDL_FFT_COMPLEX comp, FLOAT_TYPE *outMagn, FLOAT_TYPE *outPhase);
    
    template <typename FLOAT_TYPE>
    static void MagnPhaseToComplex(WDL_FFT_COMPLEX *outComp, FLOAT_TYPE magn, FLOAT_TYPE phase);
    
    template <typename FLOAT_TYPE>
    static void ComplexToMagnPhase(WDL_TypedBuf<FLOAT_TYPE> *resultMagn,
                                   WDL_TypedBuf<FLOAT_TYPE> *resultPhase,
                                   const WDL_TypedBuf<WDL_FFT_COMPLEX> &complexBuf);
    
    //static void ComplexToMagnPhase(WDL_TypedBuf<float> *resultMagn,
    //                               WDL_TypedBuf<float> *resultPhase,
    //                               const WDL_TypedBuf<WDL_FFT_COMPLEX> &complexBuf);
    
    template <typename FLOAT_TYPE>
    static void MagnPhaseToComplex(WDL_TypedBuf<WDL_FFT_COMPLEX> *complexBuf,
                                   const WDL_TypedBuf<FLOAT_TYPE> &magns,
                                   const WDL_TypedBuf<FLOAT_TYPE> &phases);
    
    template <typename FLOAT_TYPE>
    static void ComplexToReIm(WDL_TypedBuf<FLOAT_TYPE> *resultRe,
                              WDL_TypedBuf<FLOAT_TYPE> *resultIm,
                              const WDL_TypedBuf<WDL_FFT_COMPLEX> &complexBuf);
    template <typename FLOAT_TYPE>
    static void ReImToComplex(WDL_TypedBuf<WDL_FFT_COMPLEX> *complexBuf,
                              const WDL_TypedBuf<FLOAT_TYPE> &reBuf,
                              const WDL_TypedBuf<FLOAT_TYPE> &imBuf);

    
    template <typename FLOAT_TYPE>
    static void NormalizeFftValues(WDL_TypedBuf<FLOAT_TYPE> *magns);
    
    // Num bins is the number of bins of the full fft
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE FftBinToFreq(int binNum, int numBins, FLOAT_TYPE sampleRate);
    
    // fixed version for stereo (phase correction)
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE FftBinToFreq2(int binNum, int numBins, FLOAT_TYPE sampleRate);
    
    template <typename FLOAT_TYPE>
    static int FreqToFftBin(FLOAT_TYPE freq, int numBins, FLOAT_TYPE sampleRate, FLOAT_TYPE *t = NULL);
    
    template <typename FLOAT_TYPE>
    static void FftFreqs(WDL_TypedBuf<FLOAT_TYPE> *freqs, int numBins, FLOAT_TYPE sampleRate);
    
    template <typename FLOAT_TYPE>
    static void MinMaxFftBinFreq(FLOAT_TYPE *minFreq, FLOAT_TYPE *maxFreq, int numBins, FLOAT_TYPE sampleRate);

    //
    static void FillSecondFftHalf(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer);
    
    // Magns only
    template <typename FLOAT_TYPE>
    static void FillSecondFftHalf(WDL_TypedBuf<FLOAT_TYPE> *ioMagns);

    static void FillSecondFftHalf(const WDL_TypedBuf<WDL_FFT_COMPLEX> &inHalfBuf,
                                  WDL_TypedBuf<WDL_FFT_COMPLEX> *outBuf);
    
    // Magns only
    template <typename FLOAT_TYPE>
    static void FillSecondFftHalf(const WDL_TypedBuf<FLOAT_TYPE> &inHalfMagns,
                                  WDL_TypedBuf<FLOAT_TYPE> *outMagns);

    //
    template <typename FLOAT_TYPE>
    static void FftIdsToSamplesIds(const WDL_TypedBuf<FLOAT_TYPE> &phases,
                                   WDL_TypedBuf<int> *samplesIds);
    
    template <typename FLOAT_TYPE>
    static void FftIdsToSamplesIdsSym(const WDL_TypedBuf<FLOAT_TYPE> &phases,
                                      WDL_TypedBuf<int> *samplesIds);
    
    // More precise.
    // Canbe used to compute accurate time delay
    template <typename FLOAT_TYPE>
    static void FftIdsToSamplesIdsFloat(const WDL_TypedBuf<FLOAT_TYPE> &phases,
                                        WDL_TypedBuf<FLOAT_TYPE> *samplesIds);
    
    // NOT TESTED
    template <typename FLOAT_TYPE>
    static void SamplesIdsToFftIds(const WDL_TypedBuf<FLOAT_TYPE> &phases,
                                   WDL_TypedBuf<int> *fftIds);

    template <typename FLOAT_TYPE>
    static void ComputeTimeDelays(WDL_TypedBuf<FLOAT_TYPE> *timeDelays,
                                  const WDL_TypedBuf<FLOAT_TYPE> &phasesL,
                                  const WDL_TypedBuf<FLOAT_TYPE> &phasesR,
                                  FLOAT_TYPE sampleRate);
    
    // fmod() managing correctly negative numbers
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE fmod_negative(FLOAT_TYPE x, FLOAT_TYPE y);
    
    // Phases
    template <typename FLOAT_TYPE>
    static void FindNextPhase(FLOAT_TYPE *phase, FLOAT_TYPE refPhase);
    
    template <typename FLOAT_TYPE>
    static void UnwrapPhases(WDL_TypedBuf<FLOAT_TYPE> *phases);
    
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE MapToPi(FLOAT_TYPE val);

    template <typename FLOAT_TYPE>
    static void MapToPi(WDL_TypedBuf<FLOAT_TYPE> *values);
    
    template <typename FLOAT_TYPE>
    static void PolarToCartesian(const WDL_TypedBuf<FLOAT_TYPE> &Rs,
                                 const WDL_TypedBuf<FLOAT_TYPE> &thetas,
                                 WDL_TypedBuf<FLOAT_TYPE> *xValues,
                                 WDL_TypedBuf<FLOAT_TYPE> *yValues);
    
    template <typename FLOAT_TYPE>
    static void PhasesPolarToCartesian(const WDL_TypedBuf<FLOAT_TYPE> &phasesDiff,
                                       const WDL_TypedBuf<FLOAT_TYPE> *magns,
                                       WDL_TypedBuf<FLOAT_TYPE> *xValues,
                                       WDL_TypedBuf<FLOAT_TYPE> *yValues);
                                           
    // From (angle, distance) to (normalized angle on x, hight distance on y)
    template <typename FLOAT_TYPE>
    static void CartesianToPolarFlat(WDL_TypedBuf<FLOAT_TYPE> *xVector,
                                     WDL_TypedBuf<FLOAT_TYPE> *yVector);
    
    // Inverse of above
    template <typename FLOAT_TYPE>
    static void PolarToCartesianFlat(WDL_TypedBuf<FLOAT_TYPE> *xVector,
                                     WDL_TypedBuf<FLOAT_TYPE> *yVector);
    
    // Apply the inverse of a window using
    // the correspondance between fft samples and samples indices
    //
    // See: // See: http://werner.yellowcouch.org/Papers/transients12/index.html
    //
    // originEnvelope can be null
    //
    template <typename FLOAT_TYPE>
    static void ApplyInverseWindow(WDL_TypedBuf<WDL_FFT_COMPLEX> *fftSamples,
                                   const WDL_TypedBuf<FLOAT_TYPE> &window,
                                   const WDL_TypedBuf<FLOAT_TYPE> *originEnvelope);
    
    template <typename FLOAT_TYPE>
    static void ApplyInverseWindow(WDL_TypedBuf<FLOAT_TYPE> *magns,
                                   const WDL_TypedBuf<FLOAT_TYPE> &phases,
                                   const WDL_TypedBuf<FLOAT_TYPE> &window,
                                   const WDL_TypedBuf<FLOAT_TYPE> *originEnvelope);
    
    // Arithmetic
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE Round(FLOAT_TYPE val, int precision);
    
    template <typename FLOAT_TYPE>
    static void Round(FLOAT_TYPE *buf, int nFrames, int precision);
    
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE DomainAtan2(FLOAT_TYPE x, FLOAT_TYPE y);
    
    static int NextPowerOfTwo(int value);

    
    // Algorithms
    template <typename FLOAT_TYPE>
    static int FindValueIndex(FLOAT_TYPE val, const WDL_TypedBuf<FLOAT_TYPE> &values, FLOAT_TYPE *outT);
    
    template <typename FLOAT_TYPE>
    static int FindMaxIndex(const WDL_TypedBuf<FLOAT_TYPE> &values);
    
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE FindMaxValue(const WDL_TypedBuf<FLOAT_TYPE> &values);
    
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE FindMaxValue(const vector<WDL_TypedBuf<FLOAT_TYPE> > &values);
    
    // Find the value
    // Sort all the values before
    // (can be time consuming in a loop,
    // see the other eversion below)
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE FindMatchingValue(FLOAT_TYPE srcVal,
                                    const WDL_TypedBuf<FLOAT_TYPE> &srcValues,
                                    const WDL_TypedBuf<FLOAT_TYPE> &dstValues);
    
    // Second version, two methods: sort, then process
    template <typename FLOAT_TYPE>
    static void PrepareMatchingValueSorted(WDL_TypedBuf<FLOAT_TYPE> *srcValues,
                                           WDL_TypedBuf<FLOAT_TYPE> *dstValues);
    
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE FindMatchingValueSorted(FLOAT_TYPE srcVal,
                                          const WDL_TypedBuf<FLOAT_TYPE> &sortedSrcValues,
                                          const WDL_TypedBuf<FLOAT_TYPE> &sortedDstValues);
    
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE FactorToDivFactor(FLOAT_TYPE val, FLOAT_TYPE coeff);
    
    // Scaling
    template <typename FLOAT_TYPE>
    static void ScaleNearest(WDL_TypedBuf<FLOAT_TYPE> *values, int factor);
    
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE LogScaleNorm(FLOAT_TYPE value, FLOAT_TYPE maxValue, FLOAT_TYPE factor = 3.0);
    
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE LogScaleNormInv(FLOAT_TYPE value, FLOAT_TYPE maxValue, FLOAT_TYPE factor = 3.0);
    
    // For MetaSoundViewer
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE LogScaleNorm2(FLOAT_TYPE value, FLOAT_TYPE factor);
    
    template <typename FLOAT_TYPE>
    static void LogScaleNorm2(WDL_TypedBuf<FLOAT_TYPE> *values, FLOAT_TYPE factor);
    
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE LogScaleNormInv2(FLOAT_TYPE value, FLOAT_TYPE factor);
    
    template <typename FLOAT_TYPE>
    static void LogScaleNormInv2(WDL_TypedBuf<FLOAT_TYPE> *values, FLOAT_TYPE factor);
    
    // Convert magns from constant freq scale to log scale
    template <typename FLOAT_TYPE>
    static void FreqsToLogNorm(WDL_TypedBuf<FLOAT_TYPE> *resultMagns,
                               const WDL_TypedBuf<FLOAT_TYPE> &magns,
                               FLOAT_TYPE hzPerBin);
    
    // Inverse of above
    template <typename FLOAT_TYPE>
    static void LogToFreqsNorm(WDL_TypedBuf<FLOAT_TYPE> *resultMagns,
                               const WDL_TypedBuf<FLOAT_TYPE> &magns,
                               FLOAT_TYPE hzPerBin);
    
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE LogNormToFreq(int idx, FLOAT_TYPE hzPerBin, int bufferSize);
    
    template <typename FLOAT_TYPE>
    static int FreqIdToLogNormId(int idx, FLOAT_TYPE hzPerBin, int bufferSize);
    
    template <typename FLOAT_TYPE>
    static void FreqsToDbNorm(WDL_TypedBuf<FLOAT_TYPE> *resultMagns,
                              const WDL_TypedBuf<FLOAT_TYPE> &magns,
                              FLOAT_TYPE hzPerBin,
                              FLOAT_TYPE minValue, FLOAT_TYPE maxValue);
    
    //
    template <typename FLOAT_TYPE>
    static void ApplyWindow(WDL_TypedBuf<FLOAT_TYPE> *values,
                            const WDL_TypedBuf<FLOAT_TYPE> &window);
    
    template <typename FLOAT_TYPE>
    static void ApplyWindowRescale(WDL_TypedBuf<FLOAT_TYPE> *values,
                                   const WDL_TypedBuf<FLOAT_TYPE> &window);
    
    template <typename FLOAT_TYPE>
    static void ApplyWindowFft(WDL_TypedBuf<FLOAT_TYPE> *ioMagns,
                               const WDL_TypedBuf<FLOAT_TYPE> &phases,
                               const WDL_TypedBuf<FLOAT_TYPE> &window);
    
    template <typename FLOAT_TYPE>
    static void UnapplyWindow(WDL_TypedBuf<FLOAT_TYPE> *values,
                              const WDL_TypedBuf<FLOAT_TYPE> &window,
                              int boundSize);
    
    template <typename FLOAT_TYPE>
    static void UnapplyWindowFft(WDL_TypedBuf<FLOAT_TYPE> *ioMagns,
                                 const WDL_TypedBuf<FLOAT_TYPE> &phases,
                                 const WDL_TypedBuf<FLOAT_TYPE> &window,
                                 int boundSize);
    
    template <typename FLOAT_TYPE>
    static void Permute(WDL_TypedBuf<FLOAT_TYPE> *values,
                        const WDL_TypedBuf<int> *indices,
                        bool forward);
    
    static void Permute(vector< vector< int > > *values,
                        const WDL_TypedBuf<int> &indices,
                        bool forward);
    
    template <typename FLOAT_TYPE>
    static void FillMissingValues(WDL_TypedBuf<FLOAT_TYPE> *values,
                                  bool extendBounds,
                                  FLOAT_TYPE undefinedValue = 0.0);

    // Test for DB => default vals to undefined instead of 0.0
    template <typename FLOAT_TYPE>
    static void FillMissingValues2(WDL_TypedBuf<FLOAT_TYPE> *values,
                                  bool extendBounds,
                                  FLOAT_TYPE undefinedValue = 0.0);
    
    template <typename FLOAT_TYPE>
    static void FillMissingValues3(WDL_TypedBuf<FLOAT_TYPE> *values,
                                   bool extendBounds,
                                   FLOAT_TYPE undefinedValue = 0.0);
    
    // Interpolation
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE Interp(FLOAT_TYPE val0, FLOAT_TYPE val1, FLOAT_TYPE t);
    
    template <typename FLOAT_TYPE>
    static void Interp(WDL_TypedBuf<FLOAT_TYPE> *result,
                       const WDL_TypedBuf<FLOAT_TYPE> *buf0,
                       const WDL_TypedBuf<FLOAT_TYPE> *buf1,
                       FLOAT_TYPE t);
    
    template <typename FLOAT_TYPE>
    static void Interp(WDL_TypedBuf<WDL_FFT_COMPLEX> *result,
                       const WDL_TypedBuf<WDL_FFT_COMPLEX> *buf0,
                       const WDL_TypedBuf<WDL_FFT_COMPLEX> *buf1,
                       FLOAT_TYPE t);
    
    template <typename FLOAT_TYPE>
    static void Interp2D(WDL_TypedBuf<FLOAT_TYPE> *result,
                         const WDL_TypedBuf<FLOAT_TYPE> bufs[2][2], FLOAT_TYPE u, FLOAT_TYPE v);
    

    // Samples
    template <typename FLOAT_TYPE>
    static void DecimateValues(WDL_TypedBuf<FLOAT_TYPE> *result,
                               const WDL_TypedBuf<FLOAT_TYPE> &buf,
                               FLOAT_TYPE decFactor);
    
    template <typename FLOAT_TYPE>
    static void DecimateValues(WDL_TypedBuf<FLOAT_TYPE> *ioValues,
                               FLOAT_TYPE decFactor);
    
    template <typename FLOAT_TYPE>
    static void DecimateValuesDb(WDL_TypedBuf<FLOAT_TYPE> *result,
                                 const WDL_TypedBuf<FLOAT_TYPE> &buf,
                                 FLOAT_TYPE decFactor, FLOAT_TYPE minValueDb);
    
    // For samples (i.e preserve waveform)
    template <typename FLOAT_TYPE>
    static void DecimateSamples(WDL_TypedBuf<FLOAT_TYPE> *result,
                                const WDL_TypedBuf<FLOAT_TYPE> &buf,
                                FLOAT_TYPE decFactor);

    template <typename FLOAT_TYPE>
    static void DecimateSamples(WDL_TypedBuf<FLOAT_TYPE> *ioSamples,
                                FLOAT_TYPE decFactor);
    
    // DOESN'T WORK ?
    // Fix for flat sections at 0
    template <typename FLOAT_TYPE>
    static void DecimateSamples2(WDL_TypedBuf<FLOAT_TYPE> *result,
                                 const WDL_TypedBuf<FLOAT_TYPE> &buf,
                                 FLOAT_TYPE decFactor);
    
    // Quick and rought version
    template <typename FLOAT_TYPE>
    static void DecimateSamplesFast(WDL_TypedBuf<FLOAT_TYPE> *result,
                                    const WDL_TypedBuf<FLOAT_TYPE> &buf,
                                    FLOAT_TYPE decFactor);

    template <typename FLOAT_TYPE>
    static void DecimateSamplesFast(WDL_TypedBuf<FLOAT_TYPE> *ioSamples,
                                    FLOAT_TYPE decFactor);
    
    // Very basic
    template <typename FLOAT_TYPE>
    static void DecimateStep(WDL_TypedBuf<FLOAT_TYPE> *ioSamples, int step);

    template <typename FLOAT_TYPE>
    static void DecimateStep(const WDL_TypedBuf<FLOAT_TYPE> &inSamples,
                             WDL_TypedBuf<FLOAT_TYPE> *outSamples,
                             int step);
    
    // Math
    template <typename FLOAT_TYPE>
    static int SecondOrderEqSolve(FLOAT_TYPE a, FLOAT_TYPE b, FLOAT_TYPE c, FLOAT_TYPE res[2]);
    
    // Envelopes
    template <typename FLOAT_TYPE>
    static void ComputeEnvelope(const WDL_TypedBuf<FLOAT_TYPE> &samples,
                                WDL_TypedBuf<FLOAT_TYPE> *envelope,
                                bool extendBoundsValues);
    
    //template <typename FLOAT_TYPE>
    static void ComputeEnvelopeSmooth(CMA2Smoother *smoother,
                                      const WDL_TypedBuf<BL_FLOAT> &samples,
                                      WDL_TypedBuf<BL_FLOAT> *envelope,
                                      BL_FLOAT smoothCoeff,
                                      bool extendBoundsValues);
    
    //template <typename FLOAT_TYPE>
    static void ComputeEnvelopeSmooth2(CMA2Smoother *smoother,
                                       const WDL_TypedBuf<BL_FLOAT> &samples,
                                       WDL_TypedBuf<BL_FLOAT> *envelope,
                                       BL_FLOAT smoothCoeff);
    
    template <typename FLOAT_TYPE>
    static void ZeroBoundEnvelope(WDL_TypedBuf<FLOAT_TYPE> *envelope);
    
    // Correct the samples from a source envelop to fit into a distination envelope
    template <typename FLOAT_TYPE>
    static void CorrectEnvelope(WDL_TypedBuf<FLOAT_TYPE> *samples,
                                const WDL_TypedBuf<FLOAT_TYPE> &envelope0,
                                const WDL_TypedBuf<FLOAT_TYPE> &envelope1);
    
    // Compute the shift in sample necessary to fit the two envelopes
    // (base on max of each envelope).
    //
    // precision is used to shift by larger steps only
    // (in case of origin shifts multiple of BUFFER_SIZE)
    template <typename FLOAT_TYPE>
    static int GetEnvelopeShift(const WDL_TypedBuf<FLOAT_TYPE> &envelope0,
                                const WDL_TypedBuf<FLOAT_TYPE> &envelope1,
                                int precision = 1);
    
    template <typename FLOAT_TYPE>
    static void ShiftSamples(const WDL_TypedBuf<FLOAT_TYPE> *ioSamples, int shiftSize);
    
    // File extension
    static char *GetFileExtension(const char *fileName);

	// Get file name from full path
	static char *GetFileName(const char *path);
    
    template <typename FLOAT_TYPE>
    static void AppendValuesFile(const char *fileName, const WDL_TypedBuf<FLOAT_TYPE> &values,
                                 char delim = ' ');
    
    template <typename FLOAT_TYPE>
    static void AppendValuesFile(const char *fileName, const WDL_TypedBuf<float> &values,
                                 char delim = ' ');
    
    template <typename FLOAT_TYPE>
    static void AppendValuesFileBin(const char *fileName, const WDL_TypedBuf<FLOAT_TYPE> &values);
    
    //static void AppendValuesFileBin(const char *fileName, const WDL_TypedBuf<float> &values);
    
    template <typename FLOAT_TYPE>
    static void AppendValuesFileBinFloat(const char *fileName, const WDL_TypedBuf<FLOAT_TYPE> &values);
    
    // Optimized version (3 methods)
    static void *AppendValuesFileBinFloatInit(const char *fileName);
    
    template <typename FLOAT_TYPE>
    static void AppendValuesFileBinFloat(void *cookie, const WDL_TypedBuf<FLOAT_TYPE> &values);
    
    static void AppendValuesFileBinFloatShutdown(void *cookie);

    // Resize
    template <typename FLOAT_TYPE>
    static void ResizeLinear(WDL_TypedBuf<FLOAT_TYPE> *ioBuffer, int newSize);
    
    template <typename FLOAT_TYPE>
    static void ResizeLinear(WDL_TypedBuf<FLOAT_TYPE> *rescaledBuf,
                             const WDL_TypedBuf<FLOAT_TYPE> &buf,
                             int newSize);
    
    // FIX: last value was sometimes undefined
    template <typename FLOAT_TYPE>
    static void ResizeLinear2(WDL_TypedBuf<FLOAT_TYPE> *ioBuffer, int newSize);
    
    // Mel / Mfcc
    
#if 0 // Not tested
    template <typename FLOAT_TYPE>
    static void FreqsToMfcc(WDL_TypedBuf<FLOAT_TYPE> *result,
                            const WDL_TypedBuf<FLOAT_TYPE> freqs,
                            FLOAT_TYPE sampleRate);
#endif
    
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE FreqToMel(FLOAT_TYPE freq);

    template <typename FLOAT_TYPE>
    static FLOAT_TYPE MelToFreq(FLOAT_TYPE mel);
    
    // Convert magns from constant freq scale to mel scale
    template <typename FLOAT_TYPE>
    static void FreqsToMelNorm(WDL_TypedBuf<FLOAT_TYPE> *resultMagns,
                               const WDL_TypedBuf<FLOAT_TYPE> &magns,
                               FLOAT_TYPE hzPerBin,
                               FLOAT_TYPE zeroValue = 0.0);
    
    // Inverse of above
    template <typename FLOAT_TYPE>
    static void MelToFreqsNorm(WDL_TypedBuf<FLOAT_TYPE> *resultMagns,
                               const WDL_TypedBuf<FLOAT_TYPE> &magns,
                               FLOAT_TYPE hzPerBin,
                               FLOAT_TYPE zeroValue = 0.0);
    
    // Better interpolation
    // => manage if we contract or dilate the data
    // When scaling from mel to freq or reverse, sometimes we contract,
    // sometimes we dilate the data, during a single conversion
    // => so will avoid the fact to miss values when contracting
    //
    // NOTE: maybe need more testing
    //
    template <typename FLOAT_TYPE>
    static void FreqsToMelNorm2(WDL_TypedBuf<FLOAT_TYPE> *resultMagns,
                                const WDL_TypedBuf<FLOAT_TYPE> &magns,
                                FLOAT_TYPE hzPerBin,
                                FLOAT_TYPE zeroValue = 0.0);
    
    template <typename FLOAT_TYPE>
    static void MelToFreqsNorm2(WDL_TypedBuf<FLOAT_TYPE> *resultMagns,
                                const WDL_TypedBuf<FLOAT_TYPE> &magns,
                                FLOAT_TYPE hzPerBin,
                                FLOAT_TYPE zeroValue = 0.0);
    
    // Take FLOAT_TYPE id (when we are between bins)
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE MelNormIdToFreq(FLOAT_TYPE idx, FLOAT_TYPE hzPerBin, int bufferSize);
    
    template <typename FLOAT_TYPE>
    static int FreqIdToMelNormId(int idx, FLOAT_TYPE hzPerBin, int bufferSize);
    
    // Same, with FLOAT_TYPE
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE FreqIdToMelNormIdF(FLOAT_TYPE idx, FLOAT_TYPE hzPerBin, int bufferSize);
    
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE FreqToMelNormId(FLOAT_TYPE freq, FLOAT_TYPE hzPerBin, int bufferSize);
    
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE FreqToMelNorm(FLOAT_TYPE freq, FLOAT_TYPE hzPerBin, int bufferSize);
    
    // Algebra
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE FindNearestHarmonic(FLOAT_TYPE value, FLOAT_TYPE refValue);
    
    // GCD
    // (same as hcf or pgcd in French)
    //
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE gcd(FLOAT_TYPE a, FLOAT_TYPE b);
    
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE gcd(const vector<FLOAT_TYPE> &arr);
    
    // Mix param 0->1 => 2 params from 0->1
    template <typename FLOAT_TYPE>
    static void MixParamToCoeffs(FLOAT_TYPE mix, FLOAT_TYPE *coeff0, FLOAT_TYPE *coeff1);
    
    // Smooth
    // (simple convolution by a window)
    // (may be simple and efficient for smooting fixed size data)
    template <typename FLOAT_TYPE>
    static void SmoothDataWin(WDL_TypedBuf<FLOAT_TYPE> *result,
                              const WDL_TypedBuf<FLOAT_TYPE> &data,
                              const WDL_TypedBuf<FLOAT_TYPE> &win);
    
    template <typename FLOAT_TYPE>
    static void SmoothDataWin(WDL_TypedBuf<FLOAT_TYPE> *ioData,
                              const WDL_TypedBuf<FLOAT_TYPE> &win);

    
    // Compute a lower LOD, then re-upscale it
    template <typename FLOAT_TYPE>
    static void SmoothDataPyramid(WDL_TypedBuf<FLOAT_TYPE> *result,
                                  const WDL_TypedBuf<FLOAT_TYPE> &data,
                                  int maxLevel);
    
    // Kind of erosion / dilation
    //
    
    // Erosion
    template <typename FLOAT_TYPE>
    static void ApplyWindowMin(WDL_TypedBuf<FLOAT_TYPE> *values, int winSize);
    
    // Dilation
    template <typename FLOAT_TYPE>
    static void ApplyWindowMax(WDL_TypedBuf<FLOAT_TYPE> *values, int winSize);
    
    // Statistics
    //
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE ComputeMean(const WDL_TypedBuf<FLOAT_TYPE> &values);
    
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE ComputeSigma(const WDL_TypedBuf<FLOAT_TYPE> &values);
    
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE ComputeMean(const vector<FLOAT_TYPE> &values);
    
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE ComputeSigma(const vector<FLOAT_TYPE> &values);
    
    //
    template <typename FLOAT_TYPE>
    static void GetMinMaxFreqAxisValues(FLOAT_TYPE *minHzValue, FLOAT_TYPE *maxHzValue,
                                        int bufferSize, FLOAT_TYPE sampleRate);
   
    template <typename FLOAT_TYPE>
    static void GetMinMaxFreqAxisValues(float *minHzValue, float *maxHzValue,
                                        int bufferSize, float sampleRate);
    
    // Noise
    template <typename FLOAT_TYPE>
    static void GenNoise(WDL_TypedBuf<FLOAT_TYPE> *ioBuf);
    
    // Point distance
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE ComputeDist(FLOAT_TYPE p0[2], FLOAT_TYPE p1[2]);
    
    template <typename FLOAT_TYPE>
    static void CutHalfSamples(WDL_TypedBuf<FLOAT_TYPE> *samples, bool cutDown);
    
    template <typename FLOAT_TYPE>
    static bool IsMono(const WDL_TypedBuf<FLOAT_TYPE> &leftSamples,
                       const WDL_TypedBuf<FLOAT_TYPE> &rightSamples,
                       FLOAT_TYPE eps);
    
    template <typename FLOAT_TYPE>
    static void Smooth(WDL_TypedBuf<FLOAT_TYPE> *ioCurrentValues,
                       WDL_TypedBuf<FLOAT_TYPE> *ioPrevValues,
                       FLOAT_TYPE smoothFactor);
    
    template <typename FLOAT_TYPE>
    static void Smooth(vector<WDL_TypedBuf<FLOAT_TYPE> > *ioCurrentValues,
                       vector<WDL_TypedBuf<FLOAT_TYPE> > *ioPrevValues,
                       FLOAT_TYPE smoothFactor);
    
    template <typename FLOAT_TYPE>
    static void SmoothMax(WDL_TypedBuf<FLOAT_TYPE> *ioCurrentValues,
                          WDL_TypedBuf<FLOAT_TYPE> *ioPrevValues,
                          FLOAT_TYPE smoothFactor);
    
    template <typename FLOAT_TYPE>
    static void FindMaxima(const WDL_TypedBuf<FLOAT_TYPE> &data,
                           WDL_TypedBuf<FLOAT_TYPE> *result);
    
    template <typename FLOAT_TYPE>
    static void FindMaxima(WDL_TypedBuf<FLOAT_TYPE> *ioData);
    
    template <typename FLOAT_TYPE>
    static void FindMaxima2(const WDL_TypedBuf<FLOAT_TYPE> &data,
                            WDL_TypedBuf<FLOAT_TYPE> *result);
    
    template <typename FLOAT_TYPE>
    static void FindMaxima2D(int width, int height,
                             const WDL_TypedBuf<FLOAT_TYPE> &data,
                             WDL_TypedBuf<FLOAT_TYPE> *result,
                             bool keepMaxValue = false);
    
    template <typename FLOAT_TYPE>
    static void FindMaxima2D(int width, int height,
                             WDL_TypedBuf<FLOAT_TYPE> *ioData,
                             bool keepMaxValue = false);
    
    //
    template <typename FLOAT_TYPE>
    static void FindMinima(const WDL_TypedBuf<FLOAT_TYPE> &data,
                           WDL_TypedBuf<FLOAT_TYPE> *result,
                           FLOAT_TYPE infValue);
    
    // Take 1.0 as the max value
    template <typename FLOAT_TYPE>
    static void FindMinima(const WDL_TypedBuf<FLOAT_TYPE> &data,
                           WDL_TypedBuf<FLOAT_TYPE> *result);
    
    template <typename FLOAT_TYPE>
    static void FindMinima2(const WDL_TypedBuf<FLOAT_TYPE> &data,
                            WDL_TypedBuf<FLOAT_TYPE> *result);
    
    template <typename FLOAT_TYPE>
    static void ThresholdMin(WDL_TypedBuf<FLOAT_TYPE> *values, FLOAT_TYPE min);
    
    // Relative to max
    template <typename FLOAT_TYPE>
    static void ThresholdMinRel(WDL_TypedBuf<FLOAT_TYPE> *values, FLOAT_TYPE min);
    
    template <typename FLOAT_TYPE>
    static void ComputeDerivative(const WDL_TypedBuf<FLOAT_TYPE> &values,
                                  WDL_TypedBuf<FLOAT_TYPE> *result);
    
    template <typename FLOAT_TYPE>
    static void ComputeDerivative(WDL_TypedBuf<FLOAT_TYPE> *ioValues);
    
    // #bl-iplug2: ok
    //static void SwapColors(LICE_MemBitmap *bmp);
    
    // See: http://paulbourke.net/miscellaneous/correlate/
    template <typename FLOAT_TYPE>
    static void CrossCorrelation2D(const vector<WDL_TypedBuf<FLOAT_TYPE> > &image,
                                   const vector<WDL_TypedBuf<FLOAT_TYPE> > &mask,
                                   vector<WDL_TypedBuf<FLOAT_TYPE> > *corr);
    
    // Use two steps method described in https://pdfs.semanticscholar.org/665b/802b0236201adff4df707a26080cf808873e.pdf
    // "A SIMPLE BINARY IMAGE SIMILARITY MATCHING METHOD BASED ON EXACT PIXEL MATCHING" - Mikiyas Teshome.
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE BinaryImageMatch(const vector<WDL_TypedBuf<FLOAT_TYPE> > &image0,
                                   const vector<WDL_TypedBuf<FLOAT_TYPE> > &image1);
    
    // Build min/max map, by finding min and max horizontally only
    template <typename FLOAT_TYPE>
    static void BuildMinMaxMapHoriz(vector<WDL_TypedBuf<FLOAT_TYPE> > *values);
    
    // Build min/max map, by finding min and max vertically only
    template <typename FLOAT_TYPE>
    static void BuildMinMaxMapVert(vector<WDL_TypedBuf<FLOAT_TYPE> > *values);
    
    template <typename FLOAT_TYPE>
    static void Normalize(WDL_TypedBuf<FLOAT_TYPE> *values);
    
    template <typename FLOAT_TYPE>
    static void Normalize(FLOAT_TYPE *values, int numValues);
    
    template <typename FLOAT_TYPE>
    static void Normalize(WDL_TypedBuf<FLOAT_TYPE> *values,
                          FLOAT_TYPE minimum, FLOAT_TYPE maximum);

    template <typename FLOAT_TYPE>
    static FLOAT_TYPE Normalize(FLOAT_TYPE value, FLOAT_TYPE minimum, FLOAT_TYPE maximum);
    
    static void NormalizeMagns(WDL_TypedBuf<WDL_FFT_COMPLEX> *values);
    static void NormalizeMagnsF(WDL_TypedBuf<WDL_FFT_COMPLEX> *values);
    
    template <typename FLOAT_TYPE>
    static void Normalize(deque<WDL_TypedBuf<FLOAT_TYPE> > *values);
    
    template <typename FLOAT_TYPE>
    static void Normalize(WDL_TypedBuf<FLOAT_TYPE> *values, FLOAT_TYPE *minVal, FLOAT_TYPE *maxVal);
    
    template <typename FLOAT_TYPE>
    static void DeNormalize(WDL_TypedBuf<FLOAT_TYPE> *values, FLOAT_TYPE minVal, FLOAT_TYPE maxVal);
    
    template <typename FLOAT_TYPE>
    static void NormalizeFilter(WDL_TypedBuf<FLOAT_TYPE> *values);
    
    // numBins is bufferSize/2
    // Return floating bin num. May be rounded after the function call if you want.
    template <typename FLOAT_TYPE>
    static void BinsToChromaBins(int numBins, WDL_TypedBuf<FLOAT_TYPE> *chromaBins,
                                 FLOAT_TYPE sampleRate, FLOAT_TYPE aTune = 440.0);
    
    // For a given time, compute the waveform value for each frequency
    template <typename FLOAT_TYPE>
    static void ComputeSamplesByFrequency(WDL_TypedBuf<FLOAT_TYPE> *freqSamples,
                                          FLOAT_TYPE sampleRate,
                                          const WDL_TypedBuf<FLOAT_TYPE> &magns,
                                          const WDL_TypedBuf<FLOAT_TYPE> &phases,
                                          int tBinNum = 0);
    
    // Simple version. Some case seems not working
    template <typename FLOAT_TYPE>
    static void SeparatePeaks2D(int width, int height,
                                WDL_TypedBuf<FLOAT_TYPE> *ioData,
                                bool keepMaxValue = false);
    
    // Advanced version
    // Make some "crosses", from the maxima, folowing the lowest slope
    template <typename FLOAT_TYPE>
    static void SeparatePeaks2D2(int width, int height,
                                 WDL_TypedBuf<FLOAT_TYPE> *ioData,
                                bool keepMaxValue = false);
    
    template <typename FLOAT_TYPE>
    static void Reshape(WDL_TypedBuf<FLOAT_TYPE> *ioBuf,
                        int inWidth, int inHeight,
                        int outWidth, int outHeight);
    
    template <typename FLOAT_TYPE>
    static void Transpose(WDL_TypedBuf<FLOAT_TYPE> *ioBuf,
                          int width, int height);
    
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE ComputeVariance(const WDL_TypedBuf<FLOAT_TYPE> &data);
    
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE ComputeVariance(const vector<FLOAT_TYPE> &data);
    
    // Try a fix
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE ComputeVariance2(const vector<FLOAT_TYPE> &data);

    // Lagrange: https://en.wikipedia.org/wiki/Lagrange_polynomial
    // Note: we must not have two identical x
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE LagrangeInterp4(FLOAT_TYPE x,
                                  FLOAT_TYPE p0[2], FLOAT_TYPE p1[2],
                                  FLOAT_TYPE p2[2], FLOAT_TYPE p3[2]);
    
    static void ConvertToGUIFloatType(WDL_TypedBuf<BL_GUI_FLOAT> *dst,
                                      const WDL_TypedBuf<double> &src);
    static void ConvertToGUIFloatType(WDL_TypedBuf<BL_GUI_FLOAT> *dst,
                                      const WDL_TypedBuf<float> &src);

    static void ConvertToFloatType(WDL_TypedBuf<BL_FLOAT> *dst,
                                   const WDL_TypedBuf<double> &src);
    static void ConvertToFloatType(WDL_TypedBuf<BL_FLOAT> *dst,
                                   const WDL_TypedBuf<float> &src);
    
    static void FixDenormal(WDL_TypedBuf<BL_FLOAT> *data);
    
    // See: https://numpy.org/doc/stable/reference/generated/numpy.convolve.html
#define CONVO_MODE_FULL  0
#define CONVO_MODE_SAME  1
#define CONVO_MODE_VALID 2
    static void Convolve(const WDL_TypedBuf<BL_FLOAT> &a,
                         const WDL_TypedBuf<BL_FLOAT> &b,
                         WDL_TypedBuf<BL_FLOAT> *result,
                         int convoMode = CONVO_MODE_FULL);
    
    static bool SegSegIntersect(BL_FLOAT seg0[2][2], BL_FLOAT seg1[2][2]);
    
    static void AddIntermediatePoints(const WDL_TypedBuf<BL_FLOAT> &x,
                                      const WDL_TypedBuf<BL_FLOAT> &y,
                                      WDL_TypedBuf<BL_FLOAT> *newX,
                                      WDL_TypedBuf<BL_FLOAT> *newY);
    
    static long int GetTimeMillis();
    
    static bool GetFullPlugResourcesPath(const IPluginBase &plug,
                                         WDL_String *resPath);

    //
    template <typename FLOAT_TYPE>
    static void FastQueueToBuf(const WDL_TypedFastQueue<FLOAT_TYPE> &q,
                               WDL_TypedBuf<FLOAT_TYPE> *buf,
                               int numToCopy = -1);

    template <typename FLOAT_TYPE>
    static void FastQueueToBuf(const WDL_TypedFastQueue<FLOAT_TYPE> &q,
                               int queueOffset,
                               WDL_TypedBuf<FLOAT_TYPE> *buf,
                               int numToCopy = -1);
    
    template <typename FLOAT_TYPE>
    static void BufToFastQueue(const WDL_TypedBuf<FLOAT_TYPE> &buf,
                               WDL_TypedFastQueue<FLOAT_TYPE> *q);

    template <typename FLOAT_TYPE>
    static void Replace(WDL_TypedFastQueue<FLOAT_TYPE> *dst,
                        int startIdx,
                        const WDL_TypedBuf<FLOAT_TYPE> &src);

    template <typename FLOAT_TYPE>
    static void ConsumeLeft(WDL_TypedFastQueue<FLOAT_TYPE> *ioBuffer,
                            int numToConsume);

    template <typename FLOAT_TYPE>
    static void ResizeFillZeros(WDL_TypedFastQueue<FLOAT_TYPE> *q, int newSize);

    template <typename FLOAT_TYPE>
    static void ConsumeLeft(const WDL_TypedBuf<FLOAT_TYPE> &inBuffer,
                            WDL_TypedBuf<FLOAT_TYPE> *outBuffer,
                            int numToConsume);

    template <typename FLOAT_TYPE>
    static void SetBufResize(WDL_TypedBuf<FLOAT_TYPE> *dstBuffer,
                             const WDL_TypedBuf<FLOAT_TYPE> &srcBuffer,
                             int srcOffset = 0, int numToCopy = -1);

    template <typename FLOAT_TYPE>
    static void SetBuf(WDL_TypedBuf<FLOAT_TYPE> *dstBuffer,
                       const WDL_TypedBuf<FLOAT_TYPE> &srcBuffer);
        
protected:
    template <typename FLOAT_TYPE>
    static FLOAT_TYPE BinaryImageMatchAux(const vector<WDL_TypedBuf<FLOAT_TYPE> > &image0,
                                          const vector<WDL_TypedBuf<FLOAT_TYPE> > &image1);

};
        
#endif /* defined(__Denoiser__BLUtils__) */
