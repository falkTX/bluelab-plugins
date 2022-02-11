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
    
    static double ComputeAvg(const double *buf, int nFrames);
    
    static double ComputeMax(const double *buf, int nFrames);
    
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
    
    static void StereoToMono(WDL_TypedBuf<double> *monoResult,
                             const double *in0, const double *in1, int nFrames);
    
    static double ComputeCurveMatchCoeff(const double *curve0, const double *curve1, int nFrames);
    
    static void BypassPlug(double **inputs, double **outputs, int nFrames);
    
    static void GetPlugIOBuffers(IPlug *plug, double **inputs, double **outputs,
                                 double *in[2], double *scIn[2], double *out[2]);
    
    static double FftBinToFreq(int binNum, int numBins, int sampleRate);
    
    static int FreqToFftBin(double freq, int numBins, int sampleRate, double *t = NULL);
    
    static void MinMaxFftBinFreq(double *minFreq, double *maxFreq, int numBins, int sampleRate);
    
    static void ComplexToMagn(WDL_TypedBuf<double> *result, const WDL_TypedBuf<WDL_FFT_COMPLEX> &complexBuf);
    
    static void ComplexToMagnPhase(WDL_TypedBuf<double> *resultMagn,
                                   WDL_TypedBuf<double> *resultPhase,
                                   const WDL_TypedBuf<WDL_FFT_COMPLEX> &complexBuf);
    
    static void MagnPhaseToComplex(WDL_TypedBuf<WDL_FFT_COMPLEX> *complexBuf,
                                   const WDL_TypedBuf<double> &magns,
                                   const WDL_TypedBuf<double> &phases);
    
    static double Round(double val, int precision);
    
    static double DomainAtan2(double x, double y);
};

#endif /* defined(__Denoiser__Utils__) */
