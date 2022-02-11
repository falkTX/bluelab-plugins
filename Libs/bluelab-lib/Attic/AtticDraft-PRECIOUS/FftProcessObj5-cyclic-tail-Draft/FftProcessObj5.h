//
//  FftProcessObj.h
//  Transient
//
//  Created by Apple m'a Tuer on 21/05/17.
//
//

#ifndef __Transient__FftProcessObj5__
#define __Transient__FftProcessObj5__

#include "../../WDL/fft.h"
#include "../../WDL/IPlug/Containers.h"

// Better reconstruction, do not make low oscillations in the reconstructed signal

// Make possible to to have an overlap more than 50%
// The overlap is defined by mOversampling (2 for 50%, 4 and more for more...)
// (usefull for pitch shift for example, where in phase recomputation it is recommended from x4 to x32)

// Code refactoring
class FftProcessObj5
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
        SynthesisMethodWindow,
        SynthesisMethodInverseWindow
    };
    
    // Set skipFFT to true to skip fft and use only overlaping
    FftProcessObj5(int bufferSize, int oversampling = 2, bool normalize = true,
                   enum AnalysisMethod aMethod = AnalysisMethodWindow,
                   enum SynthesisMethod sMethod = SynthesisMethodWindow,
                   // The following flag doesn't work
                   // It makes small oscillations when overlapping
                   bool skipFFT = false);
    
    virtual ~FftProcessObj5();
    
    // Return true if nFrames were provided
    bool Process(double *input, double *output, int nFrames);
    
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
    virtual void ProcessSamplesBuffer(WDL_TypedBuf<double> *ioBuffer);
    
protected:
    // Was public
    // But must not be called directly, it is called by Process()
    void AddSamples(double *samples, int numSamples);
    
    void ProcessOneBuffer();
    
    // Shift to next buffer.
    void NextOutBuffer();
    
    // Shift to next buffer.
    void NextSamplesBuffer();
    
    // Get a buffer of the size nFrames, and consume it from the result samples
    void GetResultOutBuffer(double *output, int nFrames);
    
    // Update the position
    bool Shift();
    
    double ComputeWinFactor(const WDL_TypedBuf<double> &window);
    
    virtual void ComputeFft(const WDL_TypedBuf<double> *samples,
                            WDL_TypedBuf<WDL_FFT_COMPLEX> *fftSamples);
    
    virtual void ComputeInverseFft(const WDL_TypedBuf<WDL_FFT_COMPLEX> *fftSamples,
                                   WDL_TypedBuf<double> *samples);
    
    static void NormalizeFftValues(WDL_TypedBuf<double> *magns);
    
    static void FillSecondHalf(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer);
    
    int mBufSize;
    int mOverlap;
    int mOversampling;
    int mShift;
    
    double mAnalysisWinFactor;
    double mSynthesisWinFactor;
    
    WDL_TypedBuf<double> mBufFftChunk;
    WDL_TypedBuf<double> mBufIfftChunk;
    WDL_TypedBuf<double> mResultChunk;
    WDL_TypedBuf<double> mResultTmpChunk;
    
    WDL_TypedBuf<double> mSamplesChunk;
    WDL_TypedBuf<double> mResultOutChunk;
    
    WDL_TypedBuf<double> mAnalysisWindow;
    WDL_TypedBuf<double> mSynthesisWindow;
    
    int mBufOffset;
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> mFftBuf;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mSortedFftBuf;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpFftBuf;
    
    // Tweak parameters
    bool mNormalize;
    
    enum AnalysisMethod mAnalysisMethod;
    enum SynthesisMethod mSynthesisMethod;
    
    bool mSkipFFT;
};

#endif /* defined(__Transient__FftProcessObj5__) */
