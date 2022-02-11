//
//  FftProcessObj6.h
//  Transient
//
//  Created by Apple m'a Tuer on 21/05/17.
//
//

#ifndef __Transient__FftProcessObj9__
#define __Transient__FftProcessObj9__

#include "../../WDL/fft.h"
#include "../../WDL/IPlug/Containers.h"

#include "BufProcessObj.h"

// Better reconstruction, do not make low oscillations in the reconstructed signal

// Make possible to to have an overlap more than 50%
// The overlap is defined by mOversampling (2 for 50%, 4 and more for more...)
// (usefull for pitch shift for example, where in phase recomputation it is recommended from x4 to x32)

// Code refactoring

// mFreqRes => incresing the Fft resolution, by padding the input signal wth zeros

// FftProcessObj8:
// - fixed buggy windowing coefficients
// - narrowing the windows when oversampling increases, doing better temporal precision

// Pad the first sample buffer with zeros, and cut after processing, in order to have
// good first result, not affected by windows

// Fixed intensity that changes when increasing the (new) overlap
//
// FftProcessObj9: take into account side chain, if any
//
class FftProcessObj9 //: public BufProcessObj
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
    FftProcessObj9(int bufferSize, int overlapping = 2, int freqRes = 1,
                   enum AnalysisMethod aMethod = AnalysisMethodWindow,
                   enum SynthesisMethod sMethod = SynthesisMethodWindow,
                   // The following flag doesn't work
                   // It makes small oscillations when overlapping
                   bool skipFFT = false);
    
    virtual ~FftProcessObj9();
    
    void Reset(int overlapping, int freqRes);
    
    // Return true if nFrames were provided
    bool Process(double *input, double *output, double *inSc, int nFrames);
    
    // Must be overloaded
    
    // Process the Fft buffer
    // - hafter analysis windowing
    // - after fft
    // - before ifft
    virtual void ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer);
    
    // Must be overloaded
    
    // Process the samples buffer
    // - after ifft
    // - before synthesis windowing
    virtual void ProcessSamplesBuffer(WDL_TypedBuf<double> *ioBuffer,
                                      WDL_TypedBuf<double> *scBuffer);
    
    // Convenient method
    static void SamplesToFftMagns(const WDL_TypedBuf<double> &inSamples,
                                  WDL_TypedBuf<double> *outFftMagns);
    
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
    
    void ApplySynthesisWindow(WDL_TypedBuf<double> *samples);
    
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
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpFftBuf;
    
    // Tweak parameters
    enum AnalysisMethod mAnalysisMethod;
    enum SynthesisMethod mSynthesisMethod;
    
    WDL_TypedBuf<double> mAnalysisWindow;
    WDL_TypedBuf<double> mSynthesisWindow;
    
    bool mSkipFFT;
    
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
};

#endif /* defined(__Transient__FftProcessObj9__) */
