//
//  FftProcessObj13.h
//  Transient
//
//  Created by Apple m'a Tuer on 21/05/17.
//
//

#ifndef __Transient__FftProcessObj13__
#define __Transient__FftProcessObj13__

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
// FftProcessObj12: try to avoid phasing problems in reconstruction for pitch shift
//
// FftProcessObj13: big clean after many experimentations and improvements
//
class FftProcessObj13 //: public BufProcessObj
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
    
    static void Init();
    
    // TODO: remove default arguments
    
    // Set skipFFT to true to skip fft and use only overlapping
    FftProcessObj13(int bufferSize, int overlapping, int freqRes, // default: 2, 1
                    enum AnalysisMethod aMethod, // default: AnalysisMethodWindow
                    enum SynthesisMethod sMethod, // default: SynthesisMethodWindow
                    bool keepSynthesisEnergy, // default: false
                    bool variableHanning, // true
                    // The following flag doesn't work
                    // It makes small oscillations when overlapping
                    bool skipFFT); // false
    
    virtual ~FftProcessObj13();
    
    virtual void Reset(int overlapping, int freqRes);
    
    // Return true if nFrames were provided
    virtual bool Process(BL_FLOAT *input, BL_FLOAT *output, BL_FLOAT *inSc, int nFrames);
    
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
    virtual void ProcessSamplesBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer,
                                      WDL_TypedBuf<BL_FLOAT> *scBuffer);
   
    // Process the sample buffer
    // - before fft
    virtual void PreProcessSamplesBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer);
    
    // Process the sample buffer
    // - just after ifft
    virtual void PostProcessSamplesBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer);
    
    // DEBUG ?
    void UnapplyAnalysisWindowSamples(WDL_TypedBuf<BL_FLOAT> *ioBuffer);
    
    void UnapplyAnalysisWindowFft(WDL_TypedBuf<BL_FLOAT> *magns,
                                  const WDL_TypedBuf<BL_FLOAT> &phases);
    
    
    // Convenient methods
    static void FftToSamples(const WDL_TypedBuf<BL_FLOAT> &fftMagns,
                             const  WDL_TypedBuf<BL_FLOAT> &fftPhases,
                             WDL_TypedBuf<BL_FLOAT> *outSamples);
    
    static void HalfFftToSamples(const WDL_TypedBuf<BL_FLOAT> &fftMagns,
                                 const  WDL_TypedBuf<BL_FLOAT> &fftPhases,
                                 WDL_TypedBuf<BL_FLOAT> *outSamples);
    
    static void SamplesToFft(const WDL_TypedBuf<BL_FLOAT> &inSamples,
                             WDL_TypedBuf<BL_FLOAT> *outFftMagns,
                             WDL_TypedBuf<BL_FLOAT> *outFftPhases);
    
    static void SamplesToFftMagns(const WDL_TypedBuf<BL_FLOAT> &inSamples,
                                  WDL_TypedBuf<BL_FLOAT> *outFftMagns);
    
    static void FftToSamples(const WDL_TypedBuf<WDL_FFT_COMPLEX> &fftBuffer,
                             WDL_TypedBuf<BL_FLOAT> *outSamples);
    
    static void SamplesToFft(const WDL_TypedBuf<BL_FLOAT> &inSamples,
                             WDL_TypedBuf<WDL_FFT_COMPLEX> *outFftBuffer);
    
    // DEBUG
    void DBG_SetCorrectEnvelope1(bool flag);
    
    void DBG_SetEnvelopeAutoAlign(bool flag);
    
    void DBG_SetCorrectEnvelope2(bool flag);
    
    void SetDebugMode(bool flag);
    
    // Set to public for composites (PitchShift)
public:
    virtual void ProcessOneBuffer(const WDL_TypedBuf<BL_FLOAT> &inBuffer,
                                  const WDL_TypedBuf<BL_FLOAT> &inScBuffer,
                                  WDL_TypedBuf<BL_FLOAT> *outBuffer);
    
protected:
    
    // Was public
    // But must not be called directly, it is called by Process()
    void AddSamples(BL_FLOAT *samples, BL_FLOAT *scSamples, int numSamples);
    
    void ProcessOneBuffer();
    
    
    // Shift to next buffer.
    void NextOutBuffer();
    
    // Shift to next buffer.
    void NextSamplesBuffer(WDL_TypedBuf<BL_FLOAT> *samples);
    
    // Get a buffer of the size nFrames, and consume it from the result samples
    void GetResultOutBuffer(BL_FLOAT *output, int nFrames);
    
    // Update the position
    bool Shift();
    
    void ApplyAnalysisWindow(WDL_TypedBuf<BL_FLOAT> *samples);
    
    void ApplyAnalysisWindowInv(WDL_TypedBuf<BL_FLOAT> *samples);
    
    void ApplySynthesisWindow(WDL_TypedBuf<BL_FLOAT> *samples);
    
    void ApplySynthesisWindowNorm(WDL_TypedBuf<BL_FLOAT> *samples);
   
    // Apply the inverse of a window using
    // the correspondance between fft samples and samples indices
    //
    // See: // See: http://werner.yellowcouch.org/Papers/transients12/index.html
    //
    // originEnvelope can be null
    //
    void ApplyInverseWindow(WDL_TypedBuf<WDL_FFT_COMPLEX> *fftSamples,
                            const WDL_TypedBuf<BL_FLOAT> &window,
                            const WDL_TypedBuf<BL_FLOAT> *originEnvelope);
    
    void ApplyInverseWindow(WDL_TypedBuf<BL_FLOAT> *magns,
                            const WDL_TypedBuf<BL_FLOAT> &phases,
                            const WDL_TypedBuf<BL_FLOAT> &window,
                            const WDL_TypedBuf<BL_FLOAT> *originEnvelope);
    
    static void ComputeFft(const WDL_TypedBuf<BL_FLOAT> &samples,
                            WDL_TypedBuf<WDL_FFT_COMPLEX> *fftSamples,
                           int freqRes);
    
    static void ComputeInverseFft(const WDL_TypedBuf<WDL_FFT_COMPLEX> &fftSamples,
                                  WDL_TypedBuf<BL_FLOAT> *samples,
                                  int freqRes);
    
    static void NormalizeFftValues(WDL_TypedBuf<BL_FLOAT> *magns);
    
protected:
    static void MakeWindows(int bufSize, int overlapping,
                            enum AnalysisMethod analysisMethod,
                            enum SynthesisMethod synthesisMethod,
                            bool variableHanning,
                            WDL_TypedBuf<BL_FLOAT> *analysisWindow,
                            WDL_TypedBuf<BL_FLOAT> *synthesisWindow);
    
    // Used for process obj smooth mechanism
    friend class FftProcessObjSmooth5;
    virtual void SetParameter(BL_FLOAT param) {};
    virtual BL_FLOAT GetParameter() { return 0.0; };
    
    int mBufSize;
    int mOverlapping;
    int mShift;
        
    WDL_TypedBuf<BL_FLOAT> mResultChunk;
    WDL_TypedBuf<BL_FLOAT> mResultTmpChunk;
    
    WDL_TypedBuf<BL_FLOAT> mSamplesChunk;
    WDL_TypedBuf<BL_FLOAT> mResultOutChunk;
    WDL_TypedBuf<BL_FLOAT> mScChunk;
    int mBufOffset;
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> mFftBuf;
   
    // Potentially sidechain
    WDL_TypedBuf<WDL_FFT_COMPLEX> mFftBufSc;
    
    // Tweak parameters
    enum AnalysisMethod mAnalysisMethod;
    enum SynthesisMethod mSynthesisMethod;
    
    WDL_TypedBuf<BL_FLOAT> mAnalysisWindow;
    WDL_TypedBuf<BL_FLOAT> mSynthesisWindow;
    
    bool mSkipFFT;
    
    bool mVariableHanning;
    
    // NOTE: tested hanning root: it is not COLA !
    
    
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

    bool mNoOutput;
    
    // DEBUG
    bool mCorrectEnvelope1;
    bool mEnvelopeAutoAlign;
    
    bool mCorrectEnvelope2;
    
    bool mDebugMode;
};

#endif /* defined(__Transient__FftProcessObj13__) */
