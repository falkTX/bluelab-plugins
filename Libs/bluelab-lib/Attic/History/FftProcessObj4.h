//
//  FftProcessObj.h
//  Transient
//
//  Created by Apple m'a Tuer on 21/05/17.
//
//

#ifndef __Transient__FftProcessObj4__
#define __Transient__FftProcessObj4__

#include "../../WDL/fft.h"
#include "../../WDL/IPlug/Containers.h"

// Better reconstruction, do not make low oscillations in the reconstructed signal

// Make possible to to have an overlap more than 50%
// The overlap is defined by mOversampling (2 for 50%, 4 and more for more...)
// (usefull for pitch shift for example, where in phase recomputation it is recommended from x4 to x32)
class FftProcessObj4
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
    FftProcessObj4(int bufferSize, int oversampling = 2, bool normalize = true,
                   enum AnalysisMethod aMethod = AnalysisMethodWindow,
                   enum SynthesisMethod sMethod = SynthesisMethodWindow,
                   // The following flag doesn't work
                   // It makes small oscillations when overlapping
                   bool skipFFT = false);
    
    virtual ~FftProcessObj4();
    
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
    // Was public
    // But must not be called directly, it is called by Process()
    void AddSamples(BL_FLOAT *samples, int numSamples);
    
    void ProcessOneBuffer();
    
    // Shift to next buffer.
    void NextOutBuffer();
    
    // Shift to next buffer.
    void NextSamplesBuffer();
    
    // Get a buffer of the size nFrames, and consume it from the result samples
    void GetResultOutBuffer(BL_FLOAT *output, int nFrames);
    
    // Update the position
    bool Shift();
    
    BL_FLOAT ComputeWinFactor(const WDL_TypedBuf<BL_FLOAT> &window);
    
    static void NormalizeFftValues(WDL_TypedBuf<BL_FLOAT> *magns);
    
    static void FillSecondHalf(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer);
    
    int mBufSize;
    int mOverlap;
    int mOversampling;
    int mShift;
    
    BL_FLOAT mAnalysisWinFactor;
    BL_FLOAT mSynthesisWinFactor;
    
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
    
    bool mSkipFFT;
};

#endif /* defined(__Transient__FftProcessObj4__) */
