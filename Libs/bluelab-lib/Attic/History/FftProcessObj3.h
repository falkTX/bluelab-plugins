//
//  FftProcessObj.h
//  Transient
//
//  Created by Apple m'a Tuer on 21/05/17.
//
//

#ifndef __Transient__FftProcessObj3__
#define __Transient__FftProcessObj3__

#include "../../WDL/fft.h"
#include "../../WDL/IPlug/Containers.h"

// Better reconstruction, do not make low oscillations in the reconstructed signal
// Using fftw instead of WDL fft
class FftProcessObj3
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
    
    FftProcessObj3(int bufferSize, bool normalize = true,
                   enum AnalysisMethod aMethod = AnalysisMethodWindow,
                   enum SynthesisMethod sMethod = SynthesisMethodWindow);
    
    virtual ~FftProcessObj3();
    
    void AddSamples(BL_FLOAT *samples, int numSamples);
    
    // Return true if nFrames were provided
    bool Process(BL_FLOAT *input, BL_FLOAT *output, int nFrames);
    
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
    virtual void ProcessSamplesBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer);
    
protected:
    void ProcessOneBuffer();
    
    // Shift to next buffer.
    void NextOutBuffer();
    
    // Shift to next buffer.
    void NextSamplesBuffer();
    
    // Get a buffer of the size nFrames, and consume it from the result samples
    void GetResultOutBuffer(BL_FLOAT *output, int nFrames);
    
    // Update the position
    bool Shift();
    
    
    int mBufSize;
    int mOverlap;
    
    WDL_TypedBuf<BL_FLOAT> mBufFftChunk;
    WDL_TypedBuf<BL_FLOAT> mBufIfftChunk;
    WDL_TypedBuf<BL_FLOAT> mResultChunk;
    WDL_TypedBuf<BL_FLOAT> mResultTmpChunk;
    
    WDL_TypedBuf<BL_FLOAT> mSamplesChunk;
    WDL_TypedBuf<BL_FLOAT> mResultOutChunk;
    
    WDL_TypedBuf<BL_FLOAT> mAnalysisWindow;
    WDL_TypedBuf<BL_FLOAT> mSynthesisWindow;
    
    int mBufOffset;
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> mFftBuf;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mSortedFftBuf;
    
    // Tweak parameters
    bool mNormalize;
    
    enum AnalysisMethod mAnalysisMethod;
    enum SynthesisMethod mSynthesisMethod;
};

#endif /* defined(__Transient__FftProcessObj3__) */
