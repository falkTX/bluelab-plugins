//
//  FftConvolver.h
//  Spatializer
//
//  Created by Pan on 20/11/17.
//
//

#ifndef __Spatializer__FftConvolver__
#define __Spatializer__FftConvolver__

#include "../../WDL/fft.h"
#include "../../WDL/IPlug/Containers.h"

// Inspired by FftprocessObj (5)
class FftConvolver
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
    FftConvolver(int bufferSize, int oversampling = 2, bool normalize = true,
                 enum AnalysisMethod aMethod = AnalysisMethodWindow,
                 enum SynthesisMethod sMethod = SynthesisMethodWindow);
        
    virtual ~FftConvolver();
    
    void Reset();
    
    // Set the response to convolve
    void SetResponse(const WDL_TypedBuf<double> *response);
    
    // Return true if nFrames were provided
    bool Process(double *input, double *output, int nFrames);
    
protected:
    void ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer);
    
    void AddSamples(double *samples, int numSamples);
    
    // Return the number of samples processed
    int ProcessOneBuffer();
        
    void NextOutBuffer();
    
    // Update the position
    void Shift();
    
    // Shift to next buffer.
    void NextSamplesBuffer();
        
    // Get a buffer of the size nFrames, and consume it from the result samples
    bool GetResultOutBuffer(double *output, int nFrames);
        
    double ComputeWinFactor(const WDL_TypedBuf<double> &window);
        
    void ComputeFft(const WDL_TypedBuf<double> *samples,
                            WDL_TypedBuf<WDL_FFT_COMPLEX> *fftSamples);
        
    void ComputeInverseFft(const WDL_TypedBuf<WDL_FFT_COMPLEX> *fftSamples,
                                       WDL_TypedBuf<double> *samples);
        
    static void NormalizeFftValues(WDL_TypedBuf<double> *magns);
        
    static void FillSecondHalf(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer);
        
    int mBufSize;
    int mOverlap;
    int mOversampling;
    int mShift;
        
    double mAnalysisWinFactor;
    double mSynthesisWinFactor;
        
   
    WDL_TypedBuf<double> mSamplesBuf;
    WDL_TypedBuf<double> mResultBuf;
    WDL_TypedBuf<double> mResultOutBuf;
        
    WDL_TypedBuf<double> mAnalysisWindow;
    WDL_TypedBuf<double> mSynthesisWindow;
        
    int mBufOffset;
        
    // Tweak parameters
    bool mNormalize;
        
    enum AnalysisMethod mAnalysisMethod;
    enum SynthesisMethod mSynthesisMethod;
        
    // Response
    WDL_TypedBuf<double> mResponse;
};

#endif /* defined(__Spatializer__FftConvolver__) */
