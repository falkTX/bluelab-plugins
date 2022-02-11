//
//  FftProcessObj15.h
//  Transient
//
//  Created by Apple m'a Tuer on 21/05/17.
//
//

#ifndef __Transient__FftProcessObj15__
#define __Transient__FftProcessObj15__

#include <vector>
using namespace std;

#include "../../WDL/fft.h"
#include "../../WDL/IPlug/Containers.h"

#include <FifoDecimator.h>

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
// FftProcessObj14: Multichannel
//
// FftProcessObj15: Fix clicks with buffer size 447, and latency that varied depending on the buffer size
//


// Interface for Fft processing
class ProcessObj
{
public:
    ProcessObj(int bufferSize);
    
    virtual ~ProcessObj();
    
    virtual void Reset(int oversampling, int freqRes, BL_FLOAT sampleRate);

    virtual void Reset();
    
    //
    // Callbacks
    //
    
    // Before analysis windowing
    virtual void PreProcessSamplesBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer,
                                         const WDL_TypedBuf<BL_FLOAT> *scBuffer);
    
    // Before fft
    virtual void PreProcessSamplesBufferWin(WDL_TypedBuf<BL_FLOAT> *ioBuffer,
                                            const WDL_TypedBuf<BL_FLOAT> *scBuffer);
    
    // After fft & before ifft
    virtual void ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                                  const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer);
    
    // After ifft
    virtual void ProcessSamplesBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer,
                                      WDL_TypedBuf<BL_FLOAT> *scBuffer);
    
    // After restoring initial energy
    virtual void ProcessSamplesBufferEnergy(WDL_TypedBuf<BL_FLOAT> *ioBuffer,
                                            const WDL_TypedBuf<BL_FLOAT> *scBuffer);
    
    // After synthesis windowing
    virtual void ProcessSamplesBufferWin(WDL_TypedBuf<BL_FLOAT> *ioBuffer,
                                         const WDL_TypedBuf<BL_FLOAT> *scBuffer);
    
    //
    // Tracking
    //
    
    virtual void SetTrackIO(int maxNumPoints, BL_FLOAT decimFactor,
                            bool trackInput, bool trackOutput);
    
    virtual void GetCurrentInput(WDL_TypedBuf<BL_FLOAT> *outInput);
    
    virtual void GetCurrentOutput(WDL_TypedBuf<BL_FLOAT> *outOutput);
    
protected:
    // For tracking
    FifoDecimator mInput;
    FifoDecimator mOutput;
    
    bool mTrackInput;
    bool mTrackOutput;
    
    int mBufferSize;
    
    int mOverlapping;
    int mFreqRes;
    int mSampleRate;
};

// Visitor to process channels in parallel, at each step
class MultichannelProcess
{
public:
    MultichannelProcess() {}
    
    virtual ~MultichannelProcess() {}

    virtual void Reset() {}
    
    // For PhaseDiff::USE_LINERP_PHASES
    virtual void Reset(int overlapping, int oversampling, BL_FLOAT sampleRate) {}

    virtual void ProcessInputSamplesPre(vector<WDL_TypedBuf<BL_FLOAT> * > *ioSamples) {}
    
    virtual void ProcessInputSamples(vector<WDL_TypedBuf<BL_FLOAT> * > *ioSamples) {}

    virtual void ProcessInputSamplesWin(vector<WDL_TypedBuf<BL_FLOAT> * > *ioSamples) {}

    virtual void ProcessInputFft(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > *ioFftSamples) {}

    virtual void ProcessResultFft(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > *ioFftSamples) {}

    virtual void ProcessResultSamplesWin(vector<WDL_TypedBuf<BL_FLOAT> * > *ioSamples) {}

    virtual void ProcessResultSamples(vector<WDL_TypedBuf<BL_FLOAT> * > *ioSamples) {}
};

//

class ProcessObjChannel;

class FftProcessObj15
{
public:
    enum WindowType
    {
        WindowRectangular,
        WindowHanning,
        WindowVariableHanning
    };
 
    enum
    {
        ALL_CHANNELS = -1
    };
    
    static void Init();
    
    // Set skipFFT to true to skip fft and use only overlapping
    FftProcessObj15(const vector<ProcessObj *> &processObjs,
                    int numChannels, int numScInputs,
                    int bufferSize, int overlapping, int freqRes,
                    BL_FLOAT sampleRate);
    
    virtual ~FftProcessObj15();
    
    virtual void Reset(int overlapping, int freqRes, BL_FLOAT sampleRate);
    
    virtual void Reset();
    
    // Accessors
    virtual int GetBufferSize();
    
    virtual int GetOverlapping();
    
    virtual int GetFreqRes();
    
    virtual BL_FLOAT GetSampleRate();
    
    // Setters
    virtual void SetAnalysisWindow(int channelNum, WindowType type);
    
    virtual void SetSynthesisWindow(int channelNum, WindowType type);
    
    virtual void SetKeepSynthesisEnergy(int channelNum, bool flag);
    
    virtual void SetSkipFft(int channelNum, bool flag);
    
    
    // Return true if nFrames were provided
    virtual bool Process(const vector<WDL_TypedBuf<BL_FLOAT> > &inputs,
                         const vector<WDL_TypedBuf<BL_FLOAT> > &scInputs,
                         vector<WDL_TypedBuf<BL_FLOAT> > *outputs);
    
    
    void AddMultichannelProcess(MultichannelProcess *mcProcess);
    
    // Callbacks
    //
    // It is in charge to  the derived class to extract
    // the data from the channels
    
    // Here, we have the input samples
    virtual void InputSamplesReady();
    
    // Here, we have the windowed imput samples
    virtual void InputSamplesWinReady();
    
    // Here, we have the fft
    virtual void InputFftReady();
    
    // Here, we have the processed fft
    virtual void ResultFftReady();
    
    // Here, we have one chunk of result samples
    virtual void ResultSamplesWinReady();
    
    // Here, we have the sum of the windowed samples
    virtual void ResultSamplesReady();
    
    
    // Accessors for processing horizontally
    ProcessObj *GetChannelProcessObject(int channelNum);
    void SetChannelProcessObject(int channelNum, ProcessObj *obj);
    
    WDL_TypedBuf<BL_FLOAT> *GetChannelSamples(int channelNum);
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> *GetChannelFft(int channelNum);
    
    WDL_TypedBuf<BL_FLOAT> *GetChannelResultSamples(int channelNum);
    
    
    // Utility methods
    static void MagnPhasesToSamples(const WDL_TypedBuf<BL_FLOAT> &magns,
                                    const  WDL_TypedBuf<BL_FLOAT> &phases,
                                    WDL_TypedBuf<BL_FLOAT> *outSamples);
    
    static void SamplesToMagnPhases(const WDL_TypedBuf<BL_FLOAT> &inSamples,
                                    WDL_TypedBuf<BL_FLOAT> *outMagns,
                                    WDL_TypedBuf<BL_FLOAT> *outPhases);
    
    static void HalfMagnPhasesToSamples(const WDL_TypedBuf<BL_FLOAT> &magns,
                                        const  WDL_TypedBuf<BL_FLOAT> &phases,
                                        WDL_TypedBuf<BL_FLOAT> *outSamples);
    
    static void SamplesToHalfMagnPhases(const WDL_TypedBuf<BL_FLOAT> &inSamples,
                                        WDL_TypedBuf<BL_FLOAT> *outMagns,
                                        WDL_TypedBuf<BL_FLOAT> *outPhases);
    
    static void FftToSamples(const WDL_TypedBuf<WDL_FFT_COMPLEX> &fftBuffer,
                             WDL_TypedBuf<BL_FLOAT> *outSamples);
    
    static void SamplesToFft(const WDL_TypedBuf<BL_FLOAT> &inSamples,
                             WDL_TypedBuf<WDL_FFT_COMPLEX> *outFftBuffer);
    
    
    // Cepstrum
    static void MagnsToCepstrum(const WDL_TypedBuf<BL_FLOAT> &halfMagns,
                                WDL_TypedBuf<BL_FLOAT> *outCepstrum);
    
protected:
    virtual void AddSamples(const vector<WDL_TypedBuf<BL_FLOAT> > &inputs,
                            const vector<WDL_TypedBuf<BL_FLOAT> > &scInputs);
    
    virtual void AddSamples(int startChan,
                            const vector<WDL_TypedBuf<BL_FLOAT> > &inputs);
    
    void ProcessSamples();
    
    bool GetResults(vector<WDL_TypedBuf<BL_FLOAT> > *outputs, int numRequested);
    
    void SetupSideChain();
    
    //
    void GetAllSamples(vector<WDL_TypedBuf<BL_FLOAT> * > *samples);
    void GetAllFftSamples(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > *samples);
    void GetAllResultSamples(vector<WDL_TypedBuf<BL_FLOAT> * > *samples);

    //
    static void ComputeFft(const WDL_TypedBuf<BL_FLOAT> &samples,
                           WDL_TypedBuf<WDL_FFT_COMPLEX> *fftSamples,
                           int freqRes);
    
    static void ComputeInverseFft(const WDL_TypedBuf<WDL_FFT_COMPLEX> &fftSamples,
                                  WDL_TypedBuf<BL_FLOAT> *samples,
                                  int freqRes);
    
    // Raw
    static void ComputeInverseFft(const WDL_TypedBuf<WDL_FFT_COMPLEX> &fftSamples,
                                  WDL_TypedBuf<WDL_FFT_COMPLEX> *iFftSamples);

    
    friend class ProcessObjChannel;
    static void MakeWindows(int bufSize, int overlapping,
                            enum WindowType analysisMethod,
                            enum WindowType synthesisMethod,
                            WDL_TypedBuf<BL_FLOAT> *analysisWindow,
                            WDL_TypedBuf<BL_FLOAT> *synthesisWindow);
    
    //
    
    vector<ProcessObjChannel *> mChannels;
    
    int mNumScInputs;
    
    //
    int mBufferSize;
    int mOverlapping;
    int mFreqRes;
    BL_FLOAT mSampleRate;
    
    //
    vector<MultichannelProcess *> mMcProcesses;
};

#endif /* defined(__Transient__FftProcessObj15__) */
