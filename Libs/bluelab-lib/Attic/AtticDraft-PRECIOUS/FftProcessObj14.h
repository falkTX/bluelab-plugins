//
//  FftProcessObj14.h
//  Transient
//
//  Created by Apple m'a Tuer on 21/05/17.
//
//

#ifndef __Transient__FftProcessObj14__
#define __Transient__FftProcessObj14__

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
// FftProcessObj13: Multichannel
//

// Interface for Fft processing
class ProcessObj
{
public:
    ProcessObj() {}
    
    virtual ~ProcessObj() {}
    
    virtual void Reset(int oversampling, int freqRes) {}
    
    // Callbacks
    
    // Before analysis windowing
    virtual void PreProcessSamplesBuffer(WDL_TypedBuf<double> *ioBuffer) {}
    
    // Before fft
    virtual void PreProcessSamplesBufferWin(WDL_TypedBuf<double> *ioBuffer) {}
    
    // After fft
    // Before ifft
    virtual void ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                                  const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer = NULL) {}
    
    // After ifft
    virtual void ProcessSamplesBuffer(WDL_TypedBuf<double> *ioBuffer,
                                      WDL_TypedBuf<double> *scBuffer) {}
    
    // After synthesis windowing
    virtual void ProcessSamplesBufferWin(WDL_TypedBuf<double> *ioBuffer) {}
    
    // After restoring initial energy
    virtual void ProcessSamplesBufferEnergy(WDL_TypedBuf<double> *ioBuffer) {}
};

class ProcessObjChannel;

class FftProcessObj14
{
#define FFT_PROCESS_OBJ_ALL_CHANNELS -1
    
public:
    enum WindowType
    {
        WindowRectangular,
        WindowHanning,
        WindowVariableHanning
    };
    
    static void Init();
    
    // Set skipFFT to true to skip fft and use only overlapping
    FftProcessObj14(ProcessObj *processObj,
                    int numChannels, int numScInputs,
                    int bufferSize, int overlapping, int freqRes);
    
    virtual ~FftProcessObj14();
    
    virtual void Reset(int overlapping, int freqRes);
    
    // Setters
    void SetAnalysisWindow(int channelNum, WindowType type);
    
    void SetSynthesisWindow(int channelNum, WindowType type);
    
    void SetKeepSynthesisEnergy(int channelNum, bool flag);
    
    void SetSkipFft(int channelNum, bool flag);
    
    
    // Return true if nFrames were provided
    virtual bool Process(const WDL_TypedBuf<WDL_TypedBuf<double> > &inputs,
                         WDL_TypedBuf<WDL_TypedBuf<double> > *outputs,
                         const WDL_TypedBuf<WDL_TypedBuf<double> >&scInputs);
    
    // Callbacks
    virtual void InputBuffersReady() {}
    
    virtual void FftsReady() {}
    
    virtual void FftResultsReady() {}
    
    virtual void SamplesResultsReady() {}
    
    // Utility methods
    static void MagnPhasesToSamples(const WDL_TypedBuf<double> &magns,
                                    const  WDL_TypedBuf<double> &phases,
                                    WDL_TypedBuf<double> *outSamples);
    
    static void SamplesToMagnPhases(const WDL_TypedBuf<double> &inSamples,
                                    WDL_TypedBuf<double> *outMagns,
                                    WDL_TypedBuf<double> *outPhases);
    
    static void FftToSamples(const WDL_TypedBuf<WDL_FFT_COMPLEX> &fftBuffer,
                             WDL_TypedBuf<double> *outSamples);
    
    static void SamplesToFft(const WDL_TypedBuf<double> &inSamples,
                             WDL_TypedBuf<WDL_FFT_COMPLEX> *outFftBuffer);
    
    
protected:
    void AddSamples(const WDL_TypedBuf<WDL_TypedBuf<double> > &inputs,
                    const WDL_TypedBuf<WDL_TypedBuf<double> > &scInputs);
    
    void ProcessSamples();
    
    void GetResults(WDL_TypedBuf<WDL_TypedBuf<double> > *outputs, int numRequested);
    
    static void ComputeFft(const WDL_TypedBuf<double> &samples,
                           WDL_TypedBuf<WDL_FFT_COMPLEX> *fftSamples,
                           int freqRes);
    
    static void ComputeInverseFft(const WDL_TypedBuf<WDL_FFT_COMPLEX> &fftSamples,
                                  WDL_TypedBuf<double> *samples,
                                  int freqRes);
    
    friend class ProcessObjChannel;
    static void MakeWindows(int bufSize, int overlapping,
                            enum WindowType analysisMethod,
                            enum WindowType synthesisMethod,
                            WDL_TypedBuf<double> *analysisWindow,
                            WDL_TypedBuf<double> *synthesisWindow);
    
    //
    
    WDL_TypedBuf<ProcessObjChannel *> mChannels;
    
    int mNumScInputs;
};

#endif /* defined(__Transient__FftProcessObj14__) */
