//
//  FftProcessObj.h
//  Transient
//
//  Created by Apple m'a Tuer on 21/05/17.
//
//

#ifndef __Transient__FftProcessObj2__
#define __Transient__FftProcessObj2__

#include "../../WDL/fft.h"
#include "../../WDL/IPlug/Containers.h"

// Better reconstruction, do not make low oscillations in the reconstructed signal
class FftProcessObj2
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
    
    FftProcessObj2(int bufferSize, bool normalize = true,
                   enum AnalysisMethod aMethod = AnalysisMethodWindow,
                   enum SynthesisMethod sMethod = SynthesisMethodWindow);
    
    virtual ~FftProcessObj2();
    
    void AddSamples(double *samples, int numSamples);
    
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
    void ProcessOneBuffer();
    
    // Shift to next buffer.
    void NextOutBuffer();
    
    // Shift to next buffer.
    void NextSamplesBuffer();
    
    // Get a buffer of the size nFrames, and consume it from the result samples
    void GetResultOutBuffer(double *output, int nFrames);
    
    // Update the position
    bool Shift();
    
    
    int mBufSize;
    int mOverlap;
    
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
    
    // Tweak parameters
    bool mNormalize;
    
    enum AnalysisMethod mAnalysisMethod;
    enum SynthesisMethod mSynthesisMethod;
};

#endif /* defined(__Transient__FftProcessObj2__) */
