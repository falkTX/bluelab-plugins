//
//  FftProcessObj12.h
//  Transient
//
//  Created by Apple m'a Tuer on 21/05/17.
//
//

#ifndef __Transient__FftProcessObj12__
#define __Transient__FftProcessObj12__

#include "../../WDL/fft.h"
#include "../../WDL/IPlug/Containers.h"

#include "BufProcessObj.h"

// Better reconstruction, do not make low oscillations in the reconstructed signal

// Make possible to to have an overlap more than 50%
// The overlap is defined by mOversampling (2 for 50%, 4 and more for more...)
// (usefull for pitch shift for example, where in phase recomputation it is recommended from x4 to x32)

// Code refactoring

// mFreqRes => increasing the Fft resolution, by padding the input signal wth zeros

// FftProcessObj8:
// - fixed buggy windowing coefficients
// - narrowing the windows when oversampling increases, doing better temporal precision

// Pad the first sample buffer with zeros, and cut after processing, in order to have
// good first result, not affected by windows

// Fixed intensity that changes when increasing the (new) overlap
//
// FftProcessObj9: take into account side chain, if any
//
// FftProcessObj9: tests to fix freqRes > 1 in Zarlino
//
// FftProcessObj11: for Denoiser: argument to activate / deactivate synthesis energy conservation
//
// 2018/03/27
// Modification of sidechain management
// Modification of the prototype of ProcessFftBuffer()
//
// FftProcessObj11: try to avoid phasing problems in reconstruction for pitch shift
//
class FftProcessObj12 //: public BufProcessObj
{
public:
    enum AnalysisMethod
    {
        AnalysisMethodNone,
        AnalysisMethodWindow
    };
    
    enum SynthesisMethod
    {
        SynthesisMethodNone,
        SynthesisMethodWindow
    };
    
    // Set skipFFT to true to skip fft and use only overlaping
    FftProcessObj12(int bufferSize, int overlapping = 2, int freqRes = 1,
                    enum AnalysisMethod aMethod = AnalysisMethodWindow,
                    enum SynthesisMethod sMethod = SynthesisMethodWindow,
                    bool keepSynthesisEnergy = false,
                    // The following flag doesn't work
                    // It makes small oscillations when overlapping
                    bool skipFFT = false,
                    bool variableHanning = true);
    
    virtual ~FftProcessObj12();
    
    void Reset(int overlapping, int freqRes);
    
    // Return true if nFrames were provided
    bool Process(double *input, double *output, double *inSc, int nFrames);
    
    // Must be overloaded
    
    // Process the Fft buffer
    // - after analysis windowing
    // - after fft
    // - before ifft
    virtual void ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                                  const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer = NULL);
    
    // Must be overloaded
    
    // Process the samples buffer
    // - after ifft
    // - before synthesis windowing
    virtual void ProcessSamplesBuffer(WDL_TypedBuf<double> *ioBuffer,
                                      WDL_TypedBuf<double> *scBuffer);
    
    // Convenient methods
    static void SamplesToFft(const WDL_TypedBuf<double> &inSamples,
                             WDL_TypedBuf<double> *outFftMagns,
                             WDL_TypedBuf<double> *outFftPhases);
    
    static void SamplesToFftMagns(const WDL_TypedBuf<double> &inSamples,
                                  WDL_TypedBuf<double> *outFftMagns);
    
    static void FftToSamples(const WDL_TypedBuf<WDL_FFT_COMPLEX> &fftBuffer,
                             WDL_TypedBuf<double> *outSamples);
    
protected:
    // Was public
    // But must not be called directly, it is called by Process()
    void AddSamples(double *samples, double *scSamples, int numSamples);
    
    void ProcessOneBuffer();
    
    // Shift to next buffer.
    void NextOutBuffer();
    
    // Shift to next buffer.
    void NextSamplesBuffer(WDL_TypedBuf<double> *samples);
    
    // Get a buffer of the size nFrames, and consume it from the result samples
    void GetResultOutBuffer(double *output, int nFrames);
    
    // Update the position
    bool Shift();
    
    void ApplyAnalysisWindow(WDL_TypedBuf<double> *samples);
    
    void ApplyAnalysisWindowInv(WDL_TypedBuf<double> *samples);
    
    void ApplySynthesisWindow(WDL_TypedBuf<double> *samples);
    
    void ApplySynthesisWindowNorm(WDL_TypedBuf<double> *samples);
    
    void CorrectEnveloppe(WDL_TypedBuf<double> *samples,
                          const WDL_TypedBuf<double> &envelope0,
                          const WDL_TypedBuf<double> &envelope1);
    
    static void ComputeFft(const WDL_TypedBuf<double> &samples,
                            WDL_TypedBuf<WDL_FFT_COMPLEX> *fftSamples,
                           int freqRes);
    
    static void ComputeInverseFft(const WDL_TypedBuf<WDL_FFT_COMPLEX> &fftSamples,
                                  WDL_TypedBuf<double> *samples,
                                  int freqRes);
    
    static void NormalizeFftValues(WDL_TypedBuf<double> *magns);
    
    static void FillSecondHalf(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer);
    
protected:
    void MakeWindows();
    
    // Used for process obj smooth mechanism
    friend class FftProcessObjSmooth5;
    virtual void SetParameter(double param) {};
    virtual double GetParameter() { return 0.0; };
    
    int mBufSize;
    int mOverlapping;
    int mShift;
        
    WDL_TypedBuf<double> mResultChunk;
    WDL_TypedBuf<double> mResultTmpChunk;
    
    WDL_TypedBuf<double> mSamplesChunk;
    WDL_TypedBuf<double> mResultOutChunk;
    WDL_TypedBuf<double> mScChunk;
    int mBufOffset;
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> mSortedFftBuf;
   
    // Potentially sidechain
    WDL_TypedBuf<WDL_FFT_COMPLEX> mSortedFftBufSc;
    
    // Tweak parameters
    enum AnalysisMethod mAnalysisMethod;
    enum SynthesisMethod mSynthesisMethod;
    
    WDL_TypedBuf<double> mAnalysisWindow;
    WDL_TypedBuf<double> mSynthesisWindow;
    
    bool mSkipFFT;
    
    bool mVariableHanning;
    
    // If mFreqRes == 2, and we are doing convolution (which is not the case...)
    // We take care to avoid cyclic processing, by growing the
    // buffers by 2 and padding with zeros
    // And we sum also the second part of the buffers, which contains
    // "future" sample contributions
    // See: See: http://eeweb.poly.edu/iselesni/EL713/zoom/overlap.pdf
    
    // With mFreqRes > 1, we padd the input signal with zeros increasing
    // the resolution of the fft, so we decreasing the aliasing.
    int mFreqRes;
    
    // Used to pad the sampels with zero on the left,
    // for correct overlapping of the beginning
    bool mMustPadSamples;
    
    bool mMustUnpadResult;
    
    bool mKeepSynthesisEnergy;
};

#endif /* defined(__Transient__FftProcessObj12__) */
