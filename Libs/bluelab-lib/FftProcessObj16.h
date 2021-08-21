//
//  FftProcessObj16.h
//  Transient
//
//  Created by Apple m'a Tuer on 21/05/17.
//
//

#ifndef __Transient__FftProcessObj16__
#define __Transient__FftProcessObj16__

#include <vector>
using namespace std;

#include "IPlug_include_in_plug_hdr.h"

#include "../../WDL/fft.h"
#include "../../WDL/fastqueue.h"

#include <BLTypes.h>

#include <BufProcessObj.h>

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
// FftProcessObj16: Manage buffer size change after instantiation (e.g due to sample rate change)


// Interface for Fft processing
class ProcessObj
{
public:
    ProcessObj(int bufferSize);
    
    virtual ~ProcessObj();
    
    virtual void Reset(int bufferSize, int oversampling,
                       int freqRes, BL_FLOAT sampleRate);
    
    //
    // Callbacks
    //
    
    // Before adding samples
    virtual void ProcessInputSamplesPre(WDL_TypedBuf<BL_FLOAT> *ioBuffer,
                                        const WDL_TypedBuf<BL_FLOAT> *scBuffer);
    
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
    
    // After having processed
    virtual void ProcessSamplesPost(WDL_TypedBuf<BL_FLOAT> *ioBuffer);
    
protected:
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
    virtual void Reset(int bufferSize, int overlapping,
                       int oversampling, BL_FLOAT sampleRate) {}

    virtual void
    ProcessInputSamplesPre(vector<WDL_TypedBuf<BL_FLOAT> * > *ioSamples,
                           const vector<WDL_TypedBuf<BL_FLOAT> > *scBuffer) {}
    
    virtual void
    ProcessInputSamples(vector<WDL_TypedBuf<BL_FLOAT> * > *ioSamples,
                        const vector<WDL_TypedBuf<BL_FLOAT> > *scBuffer) {}

    virtual void
    ProcessInputSamplesWin(vector<WDL_TypedBuf<BL_FLOAT> * > *ioSamples,
                           const vector<WDL_TypedBuf<BL_FLOAT> > *scBuffer) {}

    virtual void
    ProcessInputFft(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > *ioFftSamples,
                    const vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > *scBuffer) {}

    virtual void
    ProcessResultFft(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > *ioFftSamples,
                     const vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > *scBuffer) {}

    virtual void
    ProcessResultSamples(vector<WDL_TypedBuf<BL_FLOAT> * > *ioSamples,
                         const vector<WDL_TypedBuf<BL_FLOAT> > *scBuffer) {}
    
    virtual void
    ProcessResultSamplesWin(vector<WDL_TypedBuf<BL_FLOAT> * > *ioSamples,
                            const vector<WDL_TypedBuf<BL_FLOAT> > *scBuffer) {}
};

//

class ProcessObjChannel;

class FftProcessObj16
{
public:
    enum WindowType
    {
        WindowRectangular,
        WindowHanning,
        WindowVariableHanning,
        WindowGaussian
    };
 
    enum
    {
        ALL_CHANNELS = -1
    };
    
    static void Init();
    
    // Set skipFFT to true to skip fft and use only overlapping
    FftProcessObj16(const vector<ProcessObj *> &processObjs,
                    int numChannels, int numScInputs,
                    int bufferSize, int overlapping, int freqRes,
                    BL_FLOAT sampleRate);
    
    virtual ~FftProcessObj16();

    void SetFreqResImprov(bool flag);
    
    // Set latency, when we want a value that is not the default (buffer size)
    void SetDefaultLatency(int latency);
    
    int ComputeLatency(int blockSize);
    
    virtual void Reset(int bufferSize, int overlapping,
                       int freqRes, BL_FLOAT sampleRate);
    
    virtual void Reset();
    
    // Accessors
    virtual int GetBufferSize();
    
    virtual int GetOverlapping();
    virtual void SetOverlapping(int overlapping);
    
    virtual int GetFreqRes();
    
    virtual BL_FLOAT GetSampleRate();
    
    // Setters
    virtual void SetAnalysisWindow(int channelNum, WindowType type);
    
    virtual void SetSynthesisWindow(int channelNum, WindowType type);
    
    virtual void SetKeepSynthesisEnergy(int channelNum, bool flag);
    
    virtual void SetSkipFft(int channelNum, bool flag);
    
    // Skip fft, but still calls ProcessFftBuffer
    virtual void SetSkipFftProcess(int channelNum, bool flag);
    
    // OPTIM PROF Infra
    virtual void SetSkipIFft(int channelNum, bool flag);
    
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
    
    WDL_TypedFastQueue<BL_FLOAT> *GetChannelSamples(int channelNum);
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> *GetChannelFft(int channelNum);
    
    WDL_TypedBuf<BL_FLOAT> *GetChannelResultSamples(int channelNum);
    
    // Enable or disable channel processing (not well tested)
    void SetChannelEnabled(int channelNum, bool flag);

    void SetOutTimeStretchFactor(BL_FLOAT factor);
    
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
    
    static void SamplesToHalfMagns(const WDL_TypedBuf<BL_FLOAT> &inSamples,
                                   WDL_TypedBuf<BL_FLOAT> *outMagns);
    
    static void FftToSamples(const WDL_TypedBuf<WDL_FFT_COMPLEX> &fftBuffer,
                             WDL_TypedBuf<BL_FLOAT> *outSamples);
    
    static void SamplesToFft(const WDL_TypedBuf<BL_FLOAT> &inSamples,
                             WDL_TypedBuf<WDL_FFT_COMPLEX> *outFftBuffer);
    
    
    static void HalfFftToSamples(const WDL_TypedBuf<WDL_FFT_COMPLEX> &fftBuffer,
                                 WDL_TypedBuf<BL_FLOAT> *outSamples);
    
    // Cepstrum
    static void MagnsToCepstrum(const WDL_TypedBuf<BL_FLOAT> &halfMagns,
                                WDL_TypedBuf<BL_FLOAT> *outCepstrum);
    
    static void ComputeInverseFft(const WDL_TypedBuf<BL_FLOAT> &fftSamplesReal,
                                  WDL_TypedBuf<BL_FLOAT> *ifftSamplesReal,
                                  bool normalize = false,
                                  WDL_TypedBuf<WDL_FFT_COMPLEX> *tmpBuffer = NULL);
    
#if 1 // Moved to public for DUET and phase aliasing correction
    static void ComputeFft(const WDL_TypedBuf<BL_FLOAT> &samples,
                           WDL_TypedBuf<WDL_FFT_COMPLEX> *fftSamples,
                           int freqRes,
                           WDL_TypedBuf<WDL_FFT_COMPLEX> *tmpBuffer = NULL);
    
    static void ComputeInverseFft(const WDL_TypedBuf<WDL_FFT_COMPLEX> &fftSamples,
                                  WDL_TypedBuf<BL_FLOAT> *samples,
                                  int freqRes,
                                  WDL_TypedBuf<WDL_FFT_COMPLEX> *tmpBuffer = NULL);
#endif
    
protected:
    virtual void AddSamples(const vector<WDL_TypedBuf<BL_FLOAT> > &inputs,
                            const vector<WDL_TypedBuf<BL_FLOAT> > &scInputs);
    
    virtual void AddSamples(int startChan,
                            const vector<WDL_TypedBuf<BL_FLOAT> > &inputs);
    
    // Overwritable method (for DENOISE_OPTIM11)
    virtual void ProcessAllFftSteps();
    void ProcessFftStepChannel(int channelNum);

    
    void ProcessSamples();
    
    bool GetResults(vector<WDL_TypedBuf<BL_FLOAT> > *outputs, int numRequested);
    
    void SetupSideChain();
    
    //
    void GetAllSamples(vector<WDL_TypedFastQueue<BL_FLOAT> * > *samples);
    void GetAllScSamples(vector<WDL_TypedBuf<BL_FLOAT> > *scSamples);

    
    void GetAllFftSamples(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > *samples);
    void GetAllFftScSamples(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > *scSamples);

    
    void GetAllResultSamples(vector<WDL_TypedBuf<BL_FLOAT> * > *samples);
    void GetAllResultScSamples(vector<WDL_TypedBuf<BL_FLOAT> > *scSamples);
    
    // Raw
    static void ComputeInverseFft(const WDL_TypedBuf<WDL_FFT_COMPLEX> &fftSamples,
                                  WDL_TypedBuf<WDL_FFT_COMPLEX> *iFftSamples,
                                  WDL_TypedBuf<WDL_FFT_COMPLEX> *tmpBuffer = NULL);
    
    friend class ProcessObjChannel;
    static void MakeWindows(int bufSize, int overlapping,
                            enum WindowType analysisMethod,
                            enum WindowType synthesisMethod,
                            WDL_TypedBuf<BL_FLOAT> *analysisWindow,
                            WDL_TypedBuf<BL_FLOAT> *synthesisWindow,
                            BL_FLOAT outTimeStretchFactor);
    
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

private:
    // Tmp buffers
    vector<WDL_TypedBuf<BL_FLOAT> > mTmpBuf0;
    vector<WDL_TypedBuf<BL_FLOAT> > mTmpBuf1;
    vector<WDL_TypedBuf<BL_FLOAT> > mTmpBuf2;
    vector<WDL_TypedBuf<BL_FLOAT> > mTmpBuf3;
    vector<WDL_TypedBuf<BL_FLOAT> > mTmpBuf4;
    vector<WDL_TypedBuf<BL_FLOAT> *> mTmpBuf5;
    vector<WDL_TypedBuf<BL_FLOAT> > mTmpBuf6;
    vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > mTmpBuf7;
    vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > mTmpBuf8;
    vector<WDL_TypedFastQueue<BL_FLOAT> * > mTmpBuf9;
    vector<WDL_TypedBuf<BL_FLOAT> * > mTmpBuf10;
    vector<WDL_TypedFastQueue<BL_FLOAT> * > mTmpBuf11;
    vector<WDL_TypedBuf<BL_FLOAT> * > mTmpBuf12;
    vector<WDL_TypedBuf<BL_FLOAT> > mTmpBuf13;
    vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > mTmpBuf14;
    vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > mTmpBuf15;
    vector<WDL_TypedBuf<BL_FLOAT> * > mTmpBuf16;
    vector<WDL_TypedBuf<BL_FLOAT> > mTmpBuf17;
    vector<WDL_TypedBuf<BL_FLOAT> * > mTmpBuf18;
    vector<WDL_TypedBuf<BL_FLOAT> > mTmpBuf19;
    vector<WDL_TypedBuf<BL_FLOAT> > mTmpBuf20;
};

#endif /* defined(__Transient__FftProcessObj16__) */
