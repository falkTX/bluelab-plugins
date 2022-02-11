//
//  Utils.h
//  Denoiser
//
//  Created by Apple m'a Tuer on 06/05/17.
//
//

#ifndef __Denoiser__Utils__
#define __Denoiser__Utils__

#include <math.h>

#include "IPlug_include_in_plug_hdr.h"
#include "../../WDL/fft.h"

#ifndef M_PI
#define M_PI 3.1415926535897932384
#endif

#ifndef MIN
#define MIN(__x__, __y__) (__x__ < __y__) ? __x__ : __y__
#endif

#ifndef MAX
#define MAX(__x__, __y__) (__x__ > __y__) ? __x__ : __y__
#endif

#define COMP_MAGN(__x__) (sqrt(__x__.re * __x__.re + __x__.im * __x__.im))

#define COMP_PHASE(__x__) (atan2(__x__.im, __x__.re))

#define SET_COLOR_FROM_INT(__COLOR__, __R__, __G__, __B__, __A__) \
__COLOR__[0] = ((float)__R__)/255.0; \
__COLOR__[1] = ((float)__G__)/255.0; \
__COLOR__[2] = ((float)__B__)/255.0; \
__COLOR__[3] = ((float)__A__)/255.0;

#define BUFFER_SIZE 2048

class Utils
{
public:
    
    static double ampToDB(double amp, double minDB);
    
    static double ComputeRMSAvg(const double *buf, int nFrames);
    
    static double ComputeRMSAvg2(const double *buf, int nFrames);
    
    static double ComputeRMSAvg2(const WDL_TypedBuf<double> &buf);
    
    static double ComputeAvg(const double *buf, int nFrames);
    
    static double ComputeAvg(const WDL_TypedBuf<double> &buf);
    
    static double ComputeAvg(const WDL_TypedBuf<double> &buf,
                             int startIndex, int endIndex);
    
    static double ComputeAvgSquare(const double *buf, int nFrames);
    
    static double ComputeAvgSquare(const WDL_TypedBuf<double> &buf);
    
    static double ComputeAbsAvg(const double *buf, int nFrames);
    
    static double ComputeAbsAvg(const WDL_TypedBuf<double> &buf);
    
    static double ComputeAbsAvg(const WDL_TypedBuf<double> &buf,
                                int startIndex, int endIndex);
    
    static double ComputeMax(const WDL_TypedBuf<double> &buf);
    
    static double ComputeMax(const double *buf, int nFrames);
    
    static double ComputeMaxAbs(const WDL_TypedBuf<double> &buf);
    
    static double ComputeSum(const WDL_TypedBuf<double> &buf);
    
    static double ComputeAbsSum(const WDL_TypedBuf<double> &buf);

    static double ComputeClipSum(const WDL_TypedBuf<double> &buf);
    
    static void ComputeSum(const WDL_TypedBuf<double> &buf0,
                           const WDL_TypedBuf<double> &buf1,
                           WDL_TypedBuf<double> *result);
    
    static void ComputeProduct(const WDL_TypedBuf<double> &buf0,
                               const WDL_TypedBuf<double> &buf1,
                               WDL_TypedBuf<double> *result);
    
    // Convert the normalized x to dB, then renormalize it
    // So it is still normalized, but with a dB progression
    static double NormalizedXTodB(double x, double mindB, double maxdB);
    
    static double NormalizedYTodB(double y, double mindB, double maxdB);
    
    // Variant for vertical graph axis
    static double NormalizedYTodB2(double y, double mindB, double maxdB);
    
    // Copy of NormalizedXTodB.
    static double NormalizedYTodB3(double y, double mindB, double maxdB);
    
    static double NormalizedYTodBInv(double y, double mindB, double maxdB);
    
    static double AverageYDB(double y0, double y1, double mindB, double maxdB);
    
    static bool IsAllZero(const double *buffer, int nFrames);
    
    static bool IsAllZero(const WDL_TypedBuf<double> &buffer);
    
    static void FillAllZero(WDL_TypedBuf<double> *ioBuf);
   
    static void FillAllZero(WDL_TypedBuf<int> *ioBuf);
    
    static void FillAllValue(WDL_TypedBuf<double> *ioBuf, double value);
    
    static void AddZeros(WDL_TypedBuf<double> *ioBuf, int size);
    
    static void StereoToMono(WDL_TypedBuf<double> *monoResult,
                             const double *in0, const double *in1, int nFrames);
    
    static void StereoToMono(WDL_TypedBuf<double> *monoResult,
                             const WDL_TypedBuf<double> &in0,
                             const WDL_TypedBuf<double> &in1);

    
    static double ComputeCurveMatchCoeff(const double *curve0, const double *curve1, int nFrames);
    
    static void BypassPlug(double **inputs, double **outputs, int nFrames);
    
    // Get all the buffers available to the plugin
    static void GetPlugIOBuffers(IPlug *plug, double **inputs, double **outputs,
                                 double *in[2], double *scIn[2], double *out[2]);
    
    // For a give index, try to get both in and out buffers
    static bool GetIOBuffers(int index, double *in[2], double *out[2],
                             double **inBuf, double **outBuf);
    
    static bool PlugIOAllZero(double *inputs[2], double *outputs[2], int nFrames);
    
    // Num bins is the number of bins of the full fft
    static double FftBinToFreq(int binNum, int numBins, double sampleRate);
    
    static int FreqToFftBin(double freq, int numBins, double sampleRate, double *t = NULL);
    
    static void MinMaxFftBinFreq(double *minFreq, double *maxFreq, int numBins, double sampleRate);
    
    static void ComplexToMagn(WDL_TypedBuf<double> *result,
                              const WDL_TypedBuf<WDL_FFT_COMPLEX> &complexBuf);
    
    static void ComplexToMagnPhase(WDL_FFT_COMPLEX comp, double *outMagn, double *outPhase);
    
    static void MagnPhaseToComplex(WDL_FFT_COMPLEX *outComp, double magn, double phase);
    
    static void ComplexToMagnPhase(WDL_TypedBuf<double> *resultMagn,
                                   WDL_TypedBuf<double> *resultPhase,
                                   const WDL_TypedBuf<WDL_FFT_COMPLEX> &complexBuf);
    
    static void MagnPhaseToComplex(WDL_TypedBuf<WDL_FFT_COMPLEX> *complexBuf,
                                   const WDL_TypedBuf<double> &magns,
                                   const WDL_TypedBuf<double> &phases);
    
    static double Round(double val, int precision);
    
    static void Round(double *buf, int nFrames, int precision);
    
    static double DomainAtan2(double x, double y);
    
    static void ConsumeLeft(WDL_TypedBuf<double> *ioBuffer, int numToConsume);
    
    static void TakeHalf(WDL_TypedBuf<double> *buf);
    
    static void TakeHalf(WDL_TypedBuf<WDL_FFT_COMPLEX> *buf);
    
    static void ResizeFillZeros(WDL_TypedBuf<double> *buf, int newSize);

    static void ResizeFillZeros(WDL_TypedBuf<WDL_FFT_COMPLEX> *buf, int newSize);
    
    static void GrowFillZeros(WDL_TypedBuf<double> *buf, int numGrow);
    
    static void AddValues(WDL_TypedBuf<double> *buf, double value);
    
    static void MultValues(WDL_TypedBuf<double> *buf, double value);
    
    static void MultValues(WDL_TypedBuf<double> *buf, const WDL_TypedBuf<double> &values);
    
    static void PadZerosLeft(WDL_TypedBuf<double> *buf, int padSize);
    
    static void Interp(WDL_TypedBuf<double> *result,
                       const WDL_TypedBuf<double> *buf0,
                       const WDL_TypedBuf<double> *buf1,
                       double t);

    
    static void Interp2D(WDL_TypedBuf<double> *result,
                         const WDL_TypedBuf<double> bufs[2][2], double u, double v);
    
    static void ComputeAvg(WDL_TypedBuf<double> *result, const vector<WDL_TypedBuf<double> > &bufs);
    
    static void Mix(double *output, double *buf0, double *buf1, int nFrames, double mix);
    
    static void Fade(const WDL_TypedBuf<double> &buf0,
                     const WDL_TypedBuf<double> &buf1,
                     double *resultBuf, double fadeStart, double fadeEnd);
    
    static void Fade(WDL_TypedBuf<double> *buf,
                     double fadeStart, double fadeEnd, bool fadeIn);
    
    static void Fade(double *buf, int bufSize,
                     double fadeStart, double fadeEnd, bool fadeIn);
    
    static void Fade(const WDL_TypedBuf<double> &buf0,
                     const WDL_TypedBuf<double> &buf1,
                     double *resultBuf,
                     double fadeStart, double fadeEnd,
                     double startT, double endT);
    
    static double AmpToDB(double sampleVal, double eps, double minDB);
    
    static void AmpToDB(WDL_TypedBuf<double> *dBBuf,
                        const WDL_TypedBuf<double> &ampBuf,
                        double eps, double minDB);
    
    static double AmpToDBClip(double sampleVal, double eps, double minDB);
    
    static int NextPowerOfTwo(int value);
    
    static void AddValues(WDL_TypedBuf<double> *ioBuf, const WDL_TypedBuf<double> &addBuf);
    
    static void SubstractValues(WDL_TypedBuf<double> *ioBuf, const WDL_TypedBuf<double> &subBuf);
    
    static void ComputeDiff(WDL_TypedBuf<double> *resultDiff,
                            const WDL_TypedBuf<double> &buf0,
                            const WDL_TypedBuf<double> &buf1);
    
    static void MakeSymmetry(WDL_TypedBuf<double> *symBuf, const WDL_TypedBuf<double> &buf);
    
    static void DecimateSamples(WDL_TypedBuf<double> *result,
                                const WDL_TypedBuf<double> &buf,
                                double decFactor);

    static void DecimateSamples(WDL_TypedBuf<double> *ioSamples,
                                double decFactor);
    
    static void FillSecondFftHalf(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer);
    
    static void CopyBuf(WDL_TypedBuf<double> *toBuf, const WDL_TypedBuf<double> &fromBuf);
    
    static int FindValueIndex(double val, const WDL_TypedBuf<double> &values, double *outT);
    
    static void ShiftSamples(const WDL_TypedBuf<double> *ioSamples, int shiftSize);
    
    static void ComputeEnvelope(const WDL_TypedBuf<double> &samples,
                                WDL_TypedBuf<double> *envelope,
                                bool extendBoundsValues);
    
    static void ComputeEnvelopeSmooth(const WDL_TypedBuf<double> &samples,
                                      WDL_TypedBuf<double> *envelope,
                                      double smoothCoeff,
                                      bool extendBoundsValues);
    
    static void ComputeEnvelopeSmooth2(const WDL_TypedBuf<double> &samples,
                                       WDL_TypedBuf<double> *envelope,
                                       double smoothCoeff);
    
    static void ZeroBoundEnvelope(WDL_TypedBuf<double> *envelope);

    
    static void ScaleNearest(WDL_TypedBuf<double> *values, int factor);
    
    static int FindMaxIndex(const WDL_TypedBuf<double> &values);
    
    static void ComputeAbs(WDL_TypedBuf<double> *values);
    
    static void LogScaleX(WDL_TypedBuf<double> *values, double factor = 3.0);
    
    static void FftIdsToSamplesIds(const WDL_TypedBuf<double> &phases,
                                   WDL_TypedBuf<int> *samplesIds);
    
    // Correct the samples from a source envelop to fit into a distination envelope
    static void CorrectEnvelope(WDL_TypedBuf<double> *samples,
                                const WDL_TypedBuf<double> &envelope0,
                                const WDL_TypedBuf<double> &envelope1);
    
    // Compute the shift in sample necessary to fit the two envelopes
    // (base on max of each envelope).
    //
    // precision is used to shift by larger steps only
    // (in case of origin shifts multiple of BUFFER_SIZE)
    static int GetEnvelopeShift(const WDL_TypedBuf<double> &envelope0,
                                const WDL_TypedBuf<double> &envelope1,
                                int precision = 1);
};

#endif /* defined(__Denoiser__Utils__) */
