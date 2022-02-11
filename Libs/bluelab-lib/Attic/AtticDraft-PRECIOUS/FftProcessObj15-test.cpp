//
//  FftProcessObj15.cpp
//  Transient
//
//  Created by Apple m'a Tuer on 21/05/17.
//
//

#include <Window.h>
#include <Utils.h>
#include <Debug.h>

#include "FftProcessObj15.h"


#define INF 1e15
#define EPS 1e-15


#define NORMALIZE_FFT 1

// WARNING: this is buggy and false, need some fix and reconsidered if want to use it
// When using freq res > 1, add the tail of the fft to the future samples
// (it seems to add aliasing when activated)
#define ADD_TAIL 0

///

// TEST: quickly tested to remove the DC offset before fft
// and re-add it after ifft
// (changes a little the waveform)
//
// (not integrated because needs more tests)
#define DC_OFFSET_PROCESS 0

//
// Pad samples not used anymore since we manage well all the buffers


ProcessObj::ProcessObj(int bufferSize)
: mInput(true),
  mOutput(true)
{
    mBufferSize = bufferSize;
    
    mTrackInput = false;
    mTrackOutput = false;
}
    
ProcessObj::~ProcessObj() {}
    
void
ProcessObj::Reset(int oversampling, int freqRes, BL_FLOAT sampleRate)
{
    if (oversampling > 0)
        mOverlapping = oversampling;
    if (freqRes > 0)
        mFreqRes = freqRes;
    if (sampleRate > 0)
        mSampleRate = sampleRate;
    
    Reset();
}

void
ProcessObj::Reset()
{
    // Tracking
    mInput.Reset();
    mOutput.Reset();
}

void
ProcessObj::PreProcessSamplesBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer,
                                    const WDL_TypedBuf<BL_FLOAT> *scBuffer)
{
    mInput.AddValues(*ioBuffer);
}
    
void
ProcessObj::PreProcessSamplesBufferWin(WDL_TypedBuf<BL_FLOAT> *ioBuffer,
                                       const WDL_TypedBuf<BL_FLOAT> *scBuffer) {}

void
ProcessObj::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                             const WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuffer) {}

void
ProcessObj::ProcessSamplesBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer,
                                 WDL_TypedBuf<BL_FLOAT> *scBuffer) {}

void
ProcessObj::ProcessSamplesBufferWin(WDL_TypedBuf<BL_FLOAT> *ioBuffer,
                                    const WDL_TypedBuf<BL_FLOAT> *scBuffer) {}

void
ProcessObj::ProcessSamplesBufferEnergy(WDL_TypedBuf<BL_FLOAT> *ioBuffer,
                                       const WDL_TypedBuf<BL_FLOAT> *scBuffer)
{
    mOutput.AddValues(*ioBuffer);
}

void
ProcessObj::SetTrackIO(int maxNumPoints, BL_FLOAT decimFactor,
                       bool trackInput, bool trackOutput)
{
    mTrackInput = trackInput;
    mTrackOutput = trackOutput;
        
    if (mTrackInput)
        mInput.SetParams(maxNumPoints, decimFactor);
        
    if (mTrackOutput)
        mOutput.SetParams(maxNumPoints, decimFactor);
}
    
void
ProcessObj::GetCurrentInput(WDL_TypedBuf<BL_FLOAT> *outInput)
{
    mInput.GetValues(outInput);
}
    
void
ProcessObj::GetCurrentOutput(WDL_TypedBuf<BL_FLOAT> *outOutput)
{
    mOutput.GetValues(outOutput);
}

///

class ProcessObjChannel
{
public:
    ProcessObjChannel(ProcessObj *processObj, int bufferSize);
    
    virtual ~ProcessObjChannel();
    
    virtual void Reset(int overlapping, int freqRes, BL_FLOAT sampleRate);
    
    virtual void Reset();
    
    void SetSideChain(ProcessObjChannel *scChan);
    
    ProcessObj *GetProcessObject();
    void SetProcessObject(ProcessObj *obj);
    
    // Setters
    void SetAnalysisWindow(FftProcessObj15::WindowType type);
    
    void SetSynthesisWindow(FftProcessObj15::WindowType type);
    
    void SetKeepSynthesisEnergy(bool flag);
    
    void SetSkipFft(bool flag);
    
    
    // Processing mechanism
    void AddSamples(const WDL_TypedBuf<BL_FLOAT> &samples);
    
    bool HasSamplesToProcess();
    
    void BufferProcessed();
    
    // Steps
    void PrepareBufferStep();
    
    void MakeAnaWindowStep();
    
    void MakeFftStep();
    
    void ProcessFftStep();
    
    void MakeIFftStep();
    
    void MakeResultSamplesStep();
    
    void CommitResultStep();
    
    bool GetResult(WDL_TypedBuf<BL_FLOAT> *output, int numRequested);
    
    // Accessors for processing horizontally
    WDL_TypedBuf<BL_FLOAT> *GetSamples();
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> *GetFft();
    
    WDL_TypedBuf<BL_FLOAT> *GetResultSamples();
    
protected:
    void ProcessOneBuffer(const WDL_TypedBuf<BL_FLOAT> &inBuffer,
                          const WDL_TypedBuf<BL_FLOAT> &inScBuffer,
                          WDL_TypedBuf<BL_FLOAT> *outBuffer);
    
    
    // Shift to next buffer.
    void NextOutBuffer();
    
    // Shift to next buffer.
    void NextSamplesBuffer();
    
    // Get a buffer and consume it from the result samples
    void GetResultOutBuffer(WDL_TypedBuf<BL_FLOAT> *output, int numRequested);
    
    
    void ApplyAnalysisWindow(WDL_TypedBuf<BL_FLOAT> *samples);
    
    void ApplySynthesisWindow(WDL_TypedBuf<BL_FLOAT> *samples);
    
    //
    ProcessObj *mProcessObj;
    
    int mBufferSize;
    int mOverlapping;
    BL_FLOAT mSampleRate;
    
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
    
    // Windows
    enum FftProcessObj15::WindowType mAnalysisMethod;
    enum FftProcessObj15::WindowType mSynthesisMethod;
    
    WDL_TypedBuf<BL_FLOAT> mAnalysisWindow;
    WDL_TypedBuf<BL_FLOAT> mSynthesisWindow;
    
    
    bool mKeepSynthesisEnergy;
    bool mSkipFFT;
    
    int mShift;
    
    // Input samples (bufferized)
    WDL_TypedBuf<BL_FLOAT> mSamplesIn;
    
    // Current result
    WDL_TypedBuf<BL_FLOAT> mResult;
    
    // Result with overlap add
    WDL_TypedBuf<BL_FLOAT> mResultSum;
    
    // Result output that is totally ready
    WDL_TypedBuf<BL_FLOAT> mResultOut;
    
    
    WDL_TypedBuf<BL_FLOAT> mPreparedBuffer;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mFftBuf;
    BL_FLOAT mEnergy0;
        
    // Lazy evaluation
    bool mMustMakeWindows;
    
#if DC_OFFSET_PROCESS
    BL_FLOAT mDCOffset;
#endif

    // For latency fix
    bool mBufWasFull;
    unsigned long long mTotalInSamples;
    
private:
    ProcessObjChannel *mScChan;
};

ProcessObjChannel::ProcessObjChannel(ProcessObj *processObj, int bufferSize)
{
    mProcessObj = processObj;
    
    mBufferSize = bufferSize;
    
    mOverlapping = 1;
    mFreqRes = 1;
    mSampleRate = 44100.0;
    
    mShift = mBufferSize/mOverlapping;
    
    mAnalysisMethod = FftProcessObj15::WindowRectangular;
    mSynthesisMethod = FftProcessObj15::WindowRectangular;
    
    mKeepSynthesisEnergy = false;
    
    mSkipFFT = false;
    
    mScChan = NULL;
    
    mMustMakeWindows = true;
    
    // Latency fix
    mBufWasFull = false;
    mTotalInSamples = 0;
    
#if DC_OFFSET_PROCESS
    mDCOffset = 0.0;
#endif
}

ProcessObjChannel::~ProcessObjChannel() {}

void
ProcessObjChannel::Reset(int overlapping, int freqRes, BL_FLOAT sampleRate)
{
    mOverlapping = overlapping;
    mFreqRes = freqRes;
    mSampleRate = sampleRate;
    
    Reset();
}

void
ProcessObjChannel::Reset()
{
    mShift = mBufferSize/mOverlapping;
    
    FftProcessObj15::MakeWindows(mBufferSize, mOverlapping,
                                 mAnalysisMethod, mSynthesisMethod,
                                 &mAnalysisWindow, &mSynthesisWindow);
    // Set the buffers
    mSamplesIn.Resize(0);
    
     // TEST
    //Utils::ResizeFillZeros(&mResult, mBufferSize);
    mResult.Resize(mBufferSize);
    Utils::FillAllZero(&mResult);
    
    int resultSize = 2*mBufferSize*mFreqRes;
    //Utils::ResizeFillZeros(&mResultSum, resultSize);
    mResultSum.Resize(resultSize);
    Utils::FillAllZero(&mResultSum);
    
    mResultOut.Resize(0);
    
    int bufSize = mBufferSize*mFreqRes;
    //Utils::ResizeFillZeros(&mFftBuf, bufSize);
    mFftBuf.Resize(bufSize);
    Utils::FillAllZero(&mFftBuf);
    
    if (mProcessObj != NULL)
        mProcessObj->Reset(mOverlapping, mFreqRes, mSampleRate);
    
    // For latency fix
    mBufWasFull = false;
    mTotalInSamples = 0;
}

void
ProcessObjChannel::SetSideChain(ProcessObjChannel *scChan)
{
    mScChan = scChan;
}

ProcessObj *
ProcessObjChannel::GetProcessObject()
{
    return mProcessObj;
}

void
ProcessObjChannel::SetProcessObject(ProcessObj *obj)
{
    mProcessObj = obj;
}

void
ProcessObjChannel::SetAnalysisWindow(FftProcessObj15::WindowType type)
{
    mAnalysisMethod = type;
    
    mMustMakeWindows = true;
}

void
ProcessObjChannel::SetSynthesisWindow(FftProcessObj15::WindowType type)
{
    mSynthesisMethod = type;
    
    mMustMakeWindows = true;
}

void
ProcessObjChannel::SetKeepSynthesisEnergy(bool flag)
{
    mKeepSynthesisEnergy = flag;
}

void
ProcessObjChannel::SetSkipFft(bool flag)
{
    mSkipFFT = flag;
}

bool
ProcessObjChannel::HasSamplesToProcess()
{
    bool result = (mSamplesIn.GetSize() >= mBufferSize);
    
    return result;
}

void
ProcessObjChannel::BufferProcessed()
{
    NextSamplesBuffer();
    NextOutBuffer();
}

void
ProcessObjChannel::AddSamples(const WDL_TypedBuf<BL_FLOAT> &samples)
{
    int samplesSize = samples.GetSize();
    mSamplesIn.Add(samples.Get(), samplesSize);
    
    // For latency fix
    mTotalInSamples += samples.GetSize();
}

#if 0 // ORIGIN
bool
ProcessObjChannel::GetResult(WDL_TypedBuf<BL_FLOAT> *output, int numRequested)
{
    // Get the computed result
    int numOutSamples = mResultOut.GetSize();
    if (numRequested <= numOutSamples)
    {
        if (output != NULL)
        {
            GetResultOutBuffer(output, numRequested);
        }
        
        return true;
    }
    else
        // Fill with zeros
        // (as required when there is latency and data is not yet available)
    {
        if (output != NULL)
        {
            Utils::ResizeFillZeros(output, numRequested);
        }
        
        return false;
    }
}
#endif

#if 1 // GOOD !
// New version, manages latency exactly
// (until nFrames <= 2048)
bool
ProcessObjChannel::GetResult(WDL_TypedBuf<BL_FLOAT> *output, int numRequested)
{
    // Get the computed result
    int numOutSamples = mResultOut.GetSize();
    
    // Latency fix
    //
    // Special case:
    // - first time we have enough data
    // - and nFrames <= BUFFER_SIZE 
    //
    // NOTE: for nFrames > BUFFER_SIZE, we will have latency, impossible to fix
    //
    if (!mBufWasFull &&
        (numRequested < mBufferSize))
    {
        if (numOutSamples == 0)
            // Nothing processed yet
        {
            // For the moment, keep accumulating, to manage latency well
            if (output != NULL)
            {
                Utils::ResizeFillZeros(output, numRequested);
            }
            
            return false;
        }
        
        // We just have computed something
        //
        // This means we have got more than 2048 samples in input
        // We have reached the latency limit (2048), so we need to start outputing
        //
        // Take care of returning only a part of the result
        // to manage latency well
        //
        
        // Compute the number of samples to send to output
        int remaining = mTotalInSamples - mBufferSize;
        if (remaining > numRequested)
            // Just in case
            remaining = numRequested;
        
        // Compute the starting buffer position where to output samples
        int numZeros = numRequested - remaining;
     
        // get only the necessary samples
        WDL_TypedBuf<BL_FLOAT> buf;
        GetResultOutBuffer(&buf, remaining);
            
        if (output != NULL)
        {
            output->Resize(numRequested);
            Utils::FillAllZero(output);
            
            // Write to output, the correct number of samples,
            // and at the correct position
            for (int i = 0; i < remaining; i++)
            {
                output->Get()[numZeros + i] = buf.Get()[i];
            }
        }
        
        mBufWasFull = true;
        
        return true;
    }
    
    // We have enough out data to provide
    //
    if (numRequested <= numOutSamples)
    {
        if (output != NULL)
        {
            GetResultOutBuffer(output, numRequested);
        }
            
        return true;
    }
    
    // We don't have enough data yet
    if (output != NULL)
    {
        Utils::ResizeFillZeros(output, numRequested);
    }
            
    return false;
}
#endif

// Accessors for processing horizontally
WDL_TypedBuf<BL_FLOAT> *
ProcessObjChannel::GetSamples()
{
    return &mSamplesIn;
}

WDL_TypedBuf<WDL_FFT_COMPLEX> *
ProcessObjChannel::GetFft()
{
    return &mFftBuf;
}

WDL_TypedBuf<BL_FLOAT> *
ProcessObjChannel::GetResultSamples()
{
    return &mResult;
}

void
ProcessObjChannel::PrepareBufferStep()
{
    // This can be an empty channel !
    // i.e without samples added
    // (e.g in the case we create 2 channels, and we are in mono)
    if (mSamplesIn.GetSize() < mBufferSize)
        return;
    
    mPreparedBuffer.Resize(mBufferSize);
    for (int i = 0; i < mBufferSize; i++)
        mPreparedBuffer.Get()[i] = mSamplesIn.Get()[i];
    
    if (mProcessObj != NULL)
    {
        WDL_TypedBuf<BL_FLOAT> *scBuf = NULL;
        if (mScChan != NULL)
            scBuf = &mScChan->mPreparedBuffer;
        
        mProcessObj->PreProcessSamplesBuffer(&mPreparedBuffer, scBuf);
    }
}

void
ProcessObjChannel::MakeAnaWindowStep()
{
    if (mMustMakeWindows)
    {
        FftProcessObj15::MakeWindows(mBufferSize, mOverlapping,
                                     mAnalysisMethod, mSynthesisMethod,
                                     &mAnalysisWindow, &mSynthesisWindow);
    }
    mMustMakeWindows = false;
    
    mResult.Resize(mPreparedBuffer.GetSize());
    
    // Apply analysis windows before resizing !
    ApplyAnalysisWindow(&mPreparedBuffer);
    
    if (mProcessObj != NULL)
    {
        WDL_TypedBuf<BL_FLOAT> *scBuf = NULL;
        if (mScChan != NULL)
            scBuf = &mScChan->mPreparedBuffer;
        
        mProcessObj->PreProcessSamplesBufferWin(&mPreparedBuffer, scBuf);
    }
}

void
ProcessObjChannel::MakeFftStep()
{
    // Compute "energy" just after analysis windowing
    // then just after synthesis windowing
    // Compute and apply a coefficient to avoid loosing energy
    mEnergy0 = 0.0;
    if (mKeepSynthesisEnergy)
        mEnergy0 = Utils::ComputeAbsAvg(mPreparedBuffer);
    
    // windowing before zero padding
    // See: http://www.bitweenie.com/listings/fft-zero-padding/
    // (last links of the page)
    if (mFreqRes > 1)
    {
        // Non-cyclic technique, to avoid aliasing
        Utils::ResizeFillZeros(&mPreparedBuffer, mPreparedBuffer.GetSize()*mFreqRes);
    }
    
    if (mSkipFFT)
    {
        // Be careful to not add the beginning of the buffer
        // (otherwise the windows will be applied twice sometimes).
        mResult.Add(mPreparedBuffer.Get(), mBufferSize);
    }
    else
    {
#if DC_OFFSET_PROCESS
        mDCOffset = Utils::ComputeAvg(mPreparedBuffer);
        Utils::AddValues(&mPreparedBuffer, -mDCOffset);
#endif
        
        // NOTE: Here, we have checked that mFreqRes didn't modify the amplitude
        // of the out samples, neither the intermediate magnitude !
        FftProcessObj15::ComputeFft(mPreparedBuffer, &mFftBuf, mFreqRes);
    }
}

void
ProcessObjChannel::ProcessFftStep()
{
    if (!mSkipFFT)
    {
        if (mProcessObj != NULL)
        {
            WDL_TypedBuf<WDL_FFT_COMPLEX> *scBuf = NULL;
            if (mScChan != NULL)
                scBuf = &mScChan->mFftBuf;
            
            mProcessObj->ProcessFftBuffer(&mFftBuf, scBuf);
        }
    }
}

void
ProcessObjChannel::MakeIFftStep()
{
    FftProcessObj15::ComputeInverseFft(mFftBuf, &mResult, mFreqRes);
    
#if DC_OFFSET_PROCESS
    Utils::AddValues(&mResult, mDCOffset);
#endif

    if (mProcessObj != NULL)
    {
        WDL_TypedBuf<BL_FLOAT> *scBuf = NULL;
        if (mScChan != NULL)
            scBuf = &mScChan->mResult;
        
        mProcessObj->ProcessSamplesBuffer(&mResult, scBuf);
    }
}

void
ProcessObjChannel::MakeResultSamplesStep()
{    
    if (mKeepSynthesisEnergy)
    {
        BL_FLOAT energy1 = Utils::ComputeAbsAvg(mResult);
        
        BL_FLOAT energyCoeff = 1.0;
        if (energy1 > 0.0)
            energyCoeff = mEnergy0/energy1;
        
        // If mOverlapping is 1, then div is 0
        if (mOverlapping > 1)
        {
            Utils::MultValues(&mResult, energyCoeff);
        }
    }
    
    if (mProcessObj != NULL)
    {
        WDL_TypedBuf<BL_FLOAT> *scBuf = NULL;
        if (mScChan != NULL)
            scBuf = &mScChan->mResult;
        
        mProcessObj->ProcessSamplesBufferEnergy(&mResult, scBuf);
    }
    
    if (mMustMakeWindows)
    {
        FftProcessObj15::MakeWindows(mBufferSize, mOverlapping,
                                     mAnalysisMethod, mSynthesisMethod,
                                     &mAnalysisWindow, &mSynthesisWindow);
    }
    mMustMakeWindows = false;
    
    // Apply synthesis window
    ApplySynthesisWindow(&mResult);
}

void
ProcessObjChannel::CommitResultStep()
{
    if (mProcessObj != NULL)
    {
        WDL_TypedBuf<BL_FLOAT> *scBuf = NULL;
        if (mScChan != NULL)
            scBuf = &mScChan->mResult;
        
        mProcessObj->ProcessSamplesBufferWin(&mResult, scBuf);
    }
    
#if !ADD_TAIL
    // Standard size
    int loopCount = mBufferSize;
#else
    // Size to add tail
    int loopCount = copySamplesBuf.GetSize();
#endif
    
    // Channel can be empty
    if (mResult.GetSize() < loopCount)
        return;
    
    for (int i = 0; i < loopCount; i++)
    {
        mResultSum.Get()[i] += mResult.Get()[i];
    }
}

void
ProcessObjChannel::NextSamplesBuffer()
{
    // Reduce the size by mOverlap
    int size = mSamplesIn.GetSize();
    
    if (size == mShift)
    {
        mSamplesIn.Resize(0);
    }
    else if (size > mShift)
    {
        // Resize down, skipping left
        
        Utils::ConsumeLeft(&mSamplesIn, mShift);
    }
}

void
ProcessObjChannel::NextOutBuffer()
{
    if (mResultSum.GetSize() < mBufferSize)
        return;
        
    WDL_TypedBuf<BL_FLOAT> newBuffer;
    mResultOut.Add(mResultSum.Get(), mShift);
    
    if (mResultSum.GetSize() == mShift)
    {
        mResultSum.Resize(0);
    }
    else if (mResultSum.GetSize() > mBufferSize)
    {
        // Resize down, skipping left
        Utils::ConsumeLeft(&mResultSum, mShift);
    }
    
    // Grow the output with zeros
    Utils::ResizeFillZeros(&mResultSum,
                           mResultSum.GetSize() + mShift);
}

void
ProcessObjChannel::GetResultOutBuffer(WDL_TypedBuf<BL_FLOAT> *output,
                                      int numRequested)
{
    int size = mResultOut.GetSize();
    
    // Should not happen
    if (size < numRequested)
        return;
    
    //
    output->Resize(numRequested);
    
    // Copy the result
    memcpy(output->Get(), mResultOut.Get(), numRequested*sizeof(BL_FLOAT));
    
    // Resize down, skipping left
    Utils::ConsumeLeft(&mResultOut, numRequested);
}

void
ProcessObjChannel::ApplyAnalysisWindow(WDL_TypedBuf<BL_FLOAT> *samples)
{
    // Multiply the samples by analysis window
    // The window can be identity if necessary (no analysis window, or no overlap)
    for (int i = 0; i < samples->GetSize(); i++)
    {
        BL_FLOAT winCoeff = mAnalysisWindow.Get()[i];
        samples->Get()[i] *= winCoeff;
    }
}

void
ProcessObjChannel::ApplySynthesisWindow(WDL_TypedBuf<BL_FLOAT> *samples)
{
    // Multiply the samples by synthesis window
    // The window can be identity if necessary
    // (no synthesis window, or no overlap)
    for (int i = 0; i < samples->GetSize(); i++)
    {
        BL_FLOAT winCoeff = mSynthesisWindow.Get()[i];
        samples->Get()[i] *= winCoeff;
    }
}

void
FftProcessObj15::Init()
{
    // Init WDL FFT
    WDL_fft_init();
}

// NOTE: With freqRes == 2, and overlapping = 1 => better
// the transient signal is decreased / the other signal stays constant
// => that's what we want !
// With previous version, the other signal increased
FftProcessObj15::FftProcessObj15(const vector<ProcessObj *> &processObjs,
                                 int numChannels, int numScInputs,
                                 int bufferSize, int overlapping, int freqRes,
                                 BL_FLOAT sampleRate)
{
    mBufferSize = bufferSize;
    
    Reset(overlapping, freqRes, sampleRate);
    
    mNumScInputs = numScInputs;
    
    mChannels.resize(numChannels + numScInputs);
    for (int i = 0; i < mChannels.size(); i++)
    {
        ProcessObj *obj = (i < processObjs.size()) ? processObjs[i] : NULL;
        ProcessObjChannel *chan = new ProcessObjChannel(obj, bufferSize);
        
        chan->Reset(mOverlapping, mFreqRes, mSampleRate);
        
        mChannels[i] = chan;
    }
    
    SetupSideChain();
}

FftProcessObj15::~FftProcessObj15()
{
    for (int i = 0; i < mChannels.size(); i++)
    {
        ProcessObjChannel *chan = mChannels[i];
     
        delete chan;
    }
}

void
FftProcessObj15::Reset(int overlapping, int freqRes, BL_FLOAT sampleRate)
{
    mOverlapping = overlapping;
    mFreqRes = freqRes;
    mSampleRate = sampleRate;
    
    Reset();
}

void
FftProcessObj15::Reset()
{
    for (int i = 0; i < mChannels.size(); i++)
    {
        ProcessObjChannel *chan = mChannels[i];
        chan->Reset(mOverlapping, mFreqRes, mSampleRate);
    }
    
    for (int i = 0; i < mMcProcesses.size(); i++)
    {
        MultichannelProcess *mcProcess = mMcProcesses[i];
        mcProcess->Reset(mOverlapping, mFreqRes, mSampleRate);
    }
}

int
FftProcessObj15::GetBufferSize()
{
    return mBufferSize;
}

int
FftProcessObj15::GetOverlapping()
{
    return mOverlapping;
}

int
FftProcessObj15::GetFreqRes()
{
    return mFreqRes;
}

BL_FLOAT
FftProcessObj15::GetSampleRate()
{
    return mSampleRate;
}

void
FftProcessObj15::SetAnalysisWindow(int channelNum, WindowType type)
{
    if ((channelNum != ALL_CHANNELS) &&
        (channelNum < mChannels.size()))
        mChannels[channelNum]->SetAnalysisWindow(type);
    else
    {
        for (int i = 0; i < mChannels.size(); i++)
            mChannels[i]->SetAnalysisWindow(type);
    }
}

void
FftProcessObj15::SetSynthesisWindow(int channelNum, WindowType type)
{
    if ((channelNum != ALL_CHANNELS) &&
        (channelNum < mChannels.size()))
        mChannels[channelNum]->SetSynthesisWindow(type);
    else
    {
        for (int i = 0; i < mChannels.size(); i++)
            mChannels[i]->SetSynthesisWindow(type);
    }
}

void
FftProcessObj15::SetKeepSynthesisEnergy(int channelNum, bool flag)
{
    if ((channelNum != ALL_CHANNELS) &&
        (channelNum < mChannels.size()))
        mChannels[channelNum]->SetKeepSynthesisEnergy(flag);
    else
    {
    for (int i = 0; i < mChannels.size(); i++)
        mChannels[i]->SetKeepSynthesisEnergy(flag);
    }
}

void
FftProcessObj15::SetSkipFft(int channelNum, bool flag)
{
    if ((channelNum != ALL_CHANNELS) &&
        (channelNum < mChannels.size()))
        mChannels[channelNum]->SetSkipFft(flag);
    else
    {
        for (int i = 0; i < mChannels.size(); i++)
            mChannels[i]->SetSkipFft(flag);
    }
}

void
FftProcessObj15::AddMultichannelProcess(MultichannelProcess *mcProcess)
{
    mMcProcesses.push_back(mcProcess);
}

void
FftProcessObj15::InputSamplesReady()
{
    if (mMcProcesses.empty())
        return;
    
    vector<WDL_TypedBuf<BL_FLOAT> * > samples;
    GetAllSamples(&samples);
    
    for (int i = 0; i < mMcProcesses.size(); i++)
    {
        MultichannelProcess *mcProcess = mMcProcesses[i];
        
        mcProcess->ProcessInputSamples(&samples);
    }
}

void
FftProcessObj15::InputSamplesWinReady()
{
    if (mMcProcesses.empty())
        return;
    
    vector<WDL_TypedBuf<BL_FLOAT> * > samples;
    GetAllSamples(&samples);
    
    for (int i = 0; i < mMcProcesses.size(); i++)
    {
        MultichannelProcess *mcProcess = mMcProcesses[i];
        
        mcProcess->ProcessInputSamplesWin(&samples);
    }
}

void
FftProcessObj15::InputFftReady()
{
    if (mMcProcesses.empty())
        return;
    
    vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > samples;
    GetAllFftSamples(&samples);
    
    for (int i = 0; i < mMcProcesses.size(); i++)
    {
        MultichannelProcess *mcProcess = mMcProcesses[i];
        
        mcProcess->ProcessInputFft(&samples);
    }
}

void
FftProcessObj15::ResultFftReady()
{
    if (mMcProcesses.empty())
        return;
    
    vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > samples;
    GetAllFftSamples(&samples);
    
    for (int i = 0; i < mMcProcesses.size(); i++)
    {
        MultichannelProcess *mcProcess = mMcProcesses[i];
        
        mcProcess->ProcessResultFft(&samples);
    }
}

void
FftProcessObj15::ResultSamplesWinReady()
{
    if (mMcProcesses.empty())
        return;
    
    vector<WDL_TypedBuf<BL_FLOAT> * > samples;
    GetAllResultSamples(&samples);
    
    for (int i = 0; i < mMcProcesses.size(); i++)
    {
        MultichannelProcess *mcProcess = mMcProcesses[i];
        
        mcProcess->ProcessResultSamplesWin(&samples);
    }
}

void
FftProcessObj15::ResultSamplesReady()
{
    if (mMcProcesses.empty())
        return;
    
    vector<WDL_TypedBuf<BL_FLOAT> * > samples;
    GetAllResultSamples(&samples);
    
    for (int i = 0; i < mMcProcesses.size(); i++)
    {
        MultichannelProcess *mcProcess = mMcProcesses[i];
        
        mcProcess->ProcessResultSamples(&samples);
    }
}

void
FftProcessObj15::SamplesToMagnPhases(const WDL_TypedBuf<BL_FLOAT> &inSamples,
                                     WDL_TypedBuf<BL_FLOAT> *outFftMagns,
                                     WDL_TypedBuf<BL_FLOAT> *outFftPhases)
{
    int numSamples = inSamples.GetSize();
    int numPaddedSamples = Utils::NextPowerOfTwo(numSamples);
    
    WDL_TypedBuf<BL_FLOAT> paddedSamples = inSamples;
    Utils::ResizeFillZeros(&paddedSamples, numPaddedSamples);
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> fftSamples;
    int freqRes = 1;
    ComputeFft(paddedSamples, &fftSamples, freqRes);
    
    WDL_TypedBuf<BL_FLOAT> magns;
    WDL_TypedBuf<BL_FLOAT> phases;
    Utils::ComplexToMagnPhase(&magns, &phases, fftSamples);
    
    if (outFftMagns != NULL)
        *outFftMagns = magns;
    
    if (outFftPhases != NULL)
        *outFftPhases = phases;
}

void
FftProcessObj15::SamplesToHalfMagnPhases(const WDL_TypedBuf<BL_FLOAT> &inSamples,
                                         WDL_TypedBuf<BL_FLOAT> *outFftMagns,
                                         WDL_TypedBuf<BL_FLOAT> *outFftPhases)
{
    int numSamples = inSamples.GetSize();
    int numPaddedSamples = Utils::NextPowerOfTwo(numSamples);
    
    WDL_TypedBuf<BL_FLOAT> paddedSamples = inSamples;
    Utils::ResizeFillZeros(&paddedSamples, numPaddedSamples);
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> fftSamples;
    int freqRes = 1;
    ComputeFft(paddedSamples, &fftSamples, freqRes);
    
    Utils::TakeHalf(&fftSamples);
    
    WDL_TypedBuf<BL_FLOAT> magns;
    WDL_TypedBuf<BL_FLOAT> phases;
    Utils::ComplexToMagnPhase(&magns, &phases, fftSamples);
    
    if (outFftMagns != NULL)
        *outFftMagns = magns;
    
    if (outFftPhases != NULL)
        *outFftPhases = phases;
}

void
FftProcessObj15::FftToSamples(const WDL_TypedBuf<WDL_FFT_COMPLEX> &fftBuffer,
                              WDL_TypedBuf<BL_FLOAT> *outSamples)
{
    outSamples->Resize(fftBuffer.GetSize());
    
    int freqRes = 1;
    ComputeInverseFft(fftBuffer, outSamples, freqRes);
}

void
FftProcessObj15::SamplesToFft(const WDL_TypedBuf<BL_FLOAT> &inSamples,
                              WDL_TypedBuf<WDL_FFT_COMPLEX> *outFftBuffer)
{
    outFftBuffer->Resize(inSamples.GetSize());
    
    int freqRes = 1;
    ComputeFft(inSamples, outFftBuffer, freqRes);
}

// See: https://en.wikipedia.org/wiki/Cepstrum
void
FftProcessObj15::MagnsToCepstrum(const WDL_TypedBuf<BL_FLOAT> &halfMagns,
                                 WDL_TypedBuf<BL_FLOAT> *outCepstrum)
{
    // Fill second half
    WDL_TypedBuf<BL_FLOAT> magns = halfMagns;
    magns.Resize(magns.GetSize()*2);
                 
    Utils::FillSecondFftHalf(&magns);

#if 1
    // Compute the square
    for (int i = 0; i < magns.GetSize(); i++)
    {
        BL_FLOAT val = magns.Get()[i];
        BL_FLOAT val2 = val*val;
        magns.Get()[i] = val2;
    }
#endif
    
    // Compute the (natural) log
    for (int i = 0; i < magns.GetSize(); i++)
    {
        BL_FLOAT val = magns.Get()[i];
        
        BL_FLOAT logVal = 0.0;
        if (val > EPS)
            logVal = log(val);
        magns.Get()[i] = logVal;
    }
    
    // Compute the inverse fft
    //
    
    // Prepare
    WDL_TypedBuf<WDL_FFT_COMPLEX> fftSamples;
    fftSamples.Resize(magns.GetSize());
    for (int i = 0; i < magns.GetSize(); i++)
    {
        BL_FLOAT val = magns.Get()[i];
        
        fftSamples.Get()[i].re = val;
        fftSamples.Get()[i].im = 0.0;
    }
    
    // Compute
    WDL_TypedBuf<WDL_FFT_COMPLEX> iFftSamples;
    ComputeInverseFft(fftSamples, &iFftSamples);
    
    // Compute the result
    WDL_TypedBuf<BL_FLOAT> result;
    result.Resize(iFftSamples.GetSize());
                  
    Utils::ComplexToMagn(&result, iFftSamples);
        
    // Power Cepstrum => compute the square
    for (int i = 0; i < result.GetSize(); i++)
    {
        BL_FLOAT val = result.Get()[i];
        BL_FLOAT val2 = val*val;
        result.Get()[i] = val2;
    }
    
    Utils::TakeHalf(&result);
    
    *outCepstrum = result;
}

ProcessObj *
FftProcessObj15::GetChannelProcessObject(int channelNum)
{
    if ((channelNum < 0) || (channelNum >= mChannels.size()))
        return NULL;
    
    ProcessObj *result = mChannels[channelNum]->GetProcessObject();
    
    return result;
}

void
FftProcessObj15::SetChannelProcessObject(int channelNum, ProcessObj *obj)
{
    if ((channelNum < 0) || (channelNum >= mChannels.size()))
        return;
    
    mChannels[channelNum]->SetProcessObject(obj);
}


WDL_TypedBuf<BL_FLOAT> *
FftProcessObj15::GetChannelSamples(int channelNum)
{
    if ((channelNum < 0) || (channelNum >= mChannels.size()))
        return NULL;
    
    WDL_TypedBuf<BL_FLOAT> *result = mChannels[channelNum]->GetSamples();
    
    return result;
}

WDL_TypedBuf<WDL_FFT_COMPLEX> *
FftProcessObj15::GetChannelFft(int channelNum)
{
    if ((channelNum < 0) || (channelNum >= mChannels.size()))
        return NULL;
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> *result = mChannels[channelNum]->GetFft();
    
    return result;
}

WDL_TypedBuf<BL_FLOAT> *
FftProcessObj15::GetChannelResultSamples(int channelNum)
{
    if ((channelNum < 0) || (channelNum >= mChannels.size()))
        return NULL;
    
    WDL_TypedBuf<BL_FLOAT> *result = mChannels[channelNum]->GetResultSamples();
    
    return result;
}

void
FftProcessObj15::MagnPhasesToSamples(const WDL_TypedBuf<BL_FLOAT> &fftMagns,
                                     const  WDL_TypedBuf<BL_FLOAT> &fftPhases,
                                     WDL_TypedBuf<BL_FLOAT> *outSamples)
{
    outSamples->Resize(fftMagns.GetSize());
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> fftBuffer;
    Utils::MagnPhaseToComplex(&fftBuffer, fftMagns, fftPhases);
    
    int freqRes = 1;
    ComputeInverseFft(fftBuffer, outSamples, freqRes);
}

void
FftProcessObj15::HalfMagnPhasesToSamples(const WDL_TypedBuf<BL_FLOAT> &fftMagns,
                                         const  WDL_TypedBuf<BL_FLOAT> &fftPhases,
                                         WDL_TypedBuf<BL_FLOAT> *outSamples)
{
    outSamples->Resize(fftMagns.GetSize()*2);
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> fftBuffer;
    Utils::MagnPhaseToComplex(&fftBuffer, fftMagns, fftPhases);
    
    fftBuffer.Resize(fftBuffer.GetSize()*2);
    Utils::FillSecondFftHalf(&fftBuffer);
    
    int freqRes = 1;
    ComputeInverseFft(fftBuffer, outSamples, freqRes);
}


void
FftProcessObj15::AddSamples(const vector<WDL_TypedBuf<BL_FLOAT> > &inputs,
                            const vector<WDL_TypedBuf<BL_FLOAT> > &scInputs)
{
    // Pre-process samples
    
    // Make a copy of the input
    // because it may be modified before adding it
    vector<WDL_TypedBuf<BL_FLOAT> > samplesIn = inputs;
    
    // Convert to the right format for Mc
    vector<WDL_TypedBuf<BL_FLOAT> *> samplesInMc;
    for (int i = 0; i < samplesIn.size(); i++)
    {
        samplesInMc.push_back(&samplesIn[i]);
    }
    
    // Potentially modify the input before adding it
    for (int i = 0; i < mMcProcesses.size(); i++)
    {
        MultichannelProcess *mcProcess = mMcProcesses[i];
        
        mcProcess->ProcessInputSamplesPre(&samplesInMc);
    }
    
    // Add the new input data
    for (int i = 0; i < inputs.size(); i++)
    {
        mChannels[i]->AddSamples(samplesIn[i]);
    }
    
    for (int i = 0; i < scInputs.size(); i++)
    {
        int channelNum = mChannels.size() - mNumScInputs + i;
        mChannels[channelNum]->AddSamples(scInputs[i]);
    }
}

void
FftProcessObj15::AddSamples(int startChan,
                            const vector<WDL_TypedBuf<BL_FLOAT> > &inputs)
{
    int numInputs = inputs.size();
    
    // Add the new input data
    for (int i = 0; i < numInputs; i++)
    {
        mChannels[i + startChan]->AddSamples(inputs[i]);
    }
}

void
FftProcessObj15::ProcessSamples()
{
    if (mChannels.size() == 0)
        return;
    
    // All channels should have the same number of samples
    // and the same state
    
    ProcessObjChannel *chan0 = mChannels[0];
    while(chan0->HasSamplesToProcess())
    {
        // Compute from the available input data
        
        // Count in the reverse order, to be sure to process the side chains first
        // So when we will process the ordinary oblect, they will have their side
        // chain data ready
            
        // Process step by step for all channels
        // So it will be possible to make operations between channels
        for (int i = mChannels.size() - 1; i >= 0 ; i--)
        {
            ProcessObjChannel *chan = mChannels[i];
            chan->PrepareBufferStep();
        }
            
        InputSamplesReady();
            
        for (int i = mChannels.size() - 1; i >= 0 ; i--)
        {
            ProcessObjChannel *chan = mChannels[i];
            chan->MakeAnaWindowStep();
        }
            
        InputSamplesWinReady();
            
        for (int i = mChannels.size() - 1; i >= 0 ; i--)
        {
            ProcessObjChannel *chan = mChannels[i];
            chan->MakeFftStep();
        }
            
        InputFftReady();
            
        for (int i = mChannels.size() - 1; i >= 0 ; i--)
        {
            ProcessObjChannel *chan = mChannels[i];
            chan->ProcessFftStep();
        }
            
        ResultFftReady();
            
        for (int i = mChannels.size() - 1; i >= 0 ; i--)
        {
            ProcessObjChannel *chan = mChannels[i];
            chan->MakeIFftStep();
        }
        
        ResultSamplesReady();
            
        for (int i = mChannels.size() - 1; i >= 0 ; i--)
        {
            ProcessObjChannel *chan = mChannels[i];
            chan->MakeResultSamplesStep();
        }
        
        ResultSamplesWinReady();
            
        for (int i = mChannels.size() - 1; i >= 0 ; i--)
        {
            ProcessObjChannel *chan = mChannels[i];
            chan->CommitResultStep();
        }
        
        //
        for (int i = mChannels.size() - 1; i >= 0 ; i--)
        {
            ProcessObjChannel *chan = mChannels[i];
            chan->BufferProcessed();
        }
    }
}

bool
FftProcessObj15::GetResults(vector<WDL_TypedBuf<BL_FLOAT> > *outputs,
                           int numRequested)
{
    int numInputChannels = mChannels.size() - mNumScInputs;
    
    bool resultReady = true;
    for (int i = 0; i < numInputChannels; i++)
    {
        if (i < outputs->size())
        {
            WDL_TypedBuf<BL_FLOAT> &out = (*outputs)[i];
            bool res = mChannels[i]->GetResult(&out, numRequested);
        
            resultReady = resultReady && res;
        }
    }
    
    return resultReady;
}

void
FftProcessObj15::SetupSideChain()
{
    if (mNumScInputs == 0)
        return;
    
    int numInputs = mChannels.size() - mNumScInputs;
    for (int i = 0; i < numInputs; i++)
    {
        ProcessObjChannel *chan = mChannels[i];
        
        // Setup side chain input
        // If we have stereo side chain, try to assign each side chain channel
        // to each input channel
        if (i < mNumScInputs)
        {
            ProcessObjChannel *sc = mChannels[i + numInputs];
            chan->SetSideChain(sc);
            
            continue;
        }
        
        // Otherwise, take the first side chain input for all
        // the input channels
        
        // Set side chain 0
        ProcessObjChannel *sc = mChannels[numInputs];
        chan->SetSideChain(sc);
    }
}

bool
FftProcessObj15::Process(const vector<WDL_TypedBuf<BL_FLOAT> > &inputs,
                         const vector<WDL_TypedBuf<BL_FLOAT> > &scInputs,
                         vector<WDL_TypedBuf<BL_FLOAT> > *outputs)
{
    vector<WDL_TypedBuf<BL_FLOAT> > inputs0 = inputs;
    
    if (inputs0.size() == 0)
    {
        // Create dummy input samples
        inputs0 = *outputs;
        for (int i = 0; i < inputs0.size(); i++)
        {
            WDL_TypedBuf<BL_FLOAT> &input = inputs0[i];
            Utils::FillAllZero(&input);
        }
    }
    
    AddSamples(inputs0, scInputs);
    
    ProcessSamples();
    
    bool res = true;
    if (outputs != NULL)
    {
        int numRequested = inputs0[0].GetSize();
        res = GetResults(outputs, numRequested);
    }
    
    if (!res)
        // Result is not ready, fill the output with zeros
        // (to avoid undefined numbers sent to the host)
        //
        // NOTE: added for Rebalance
        //
    {
        if (outputs != NULL)
        {
            for (int i = 0; i < outputs->size(); i++)
            {
                Utils::FillAllZero(&(*outputs)[i]);
            }
        }
    }
    
    return res;
}

void
FftProcessObj15::MakeWindows(int bufSize, int overlapping,
                             enum WindowType analysisMethod,
                             enum WindowType synthesisMethod,
                             WDL_TypedBuf<BL_FLOAT> *analysisWindow,
                             WDL_TypedBuf<BL_FLOAT> *synthesisWindow)
{
    // Analysis and synthesis Hann windows
    // With Hanning, the gain is not increased when we increase the overlapping
    
    int anaWindowSize = bufSize;
    int synWindowSize = bufSize;
    
#if ADD_TAIL
    synWindowSize *= mFreqRes;
#endif
    
    bool variableHanning = ((analysisMethod == WindowVariableHanning) ||
                            (synthesisMethod == WindowVariableHanning));
    
    // NOTE: this is a simplificiation
    //
    // To simplify the code above and having too many tests
    //
    // We consider that if we use variable hanning, both windows will
    // be variable hanning
    //
    // But in the future, we may have to separate !
    //
    if (variableHanning)
    {
        if (analysisMethod == WindowVariableHanning)
            analysisMethod = WindowHanning;
        
        if (synthesisMethod == WindowVariableHanning)
            synthesisMethod = WindowHanning;
    }
    
    //
    // Hanning pow is used to narrow the analysis and synthesis windows
    // as much as possible, to have more "horizontal blur" in the spectrograms
    // (this reduces the attachs otherwise)
    //
    // Overlapping must be Hanning pow x 2 at the minimum
    // otherwise, we loose COLA condition
    //
    // Be careful, when using both analysis and synthesis windows,
    // it is as the Hanning pow was twice
    // (we have ana*syn equivalent to han*han).
    //
    //
    // NOTE: in the case of overlapping = 2, and two windows,
    // we will apply automatically the root Hanning, which is logical
    //
    // See: https://www.dsprelated.com/freebooks/sasp/overlap_add_ola_stft_processing.html
    // and
    //  https://www.dsprelated.com/freebooks/sasp/overlap_add_ola_stft_processing.html
    // Section: "Weighted Overlap Add"
    //
    // NOTE: here, we choose the maximum narrow value. Maybe we can narrow less by default
    //
    BL_FLOAT hanningFactor = 1.0;
    if (variableHanning && (overlapping > 1))
    {
        if ((analysisMethod == WindowHanning) ||
            (synthesisMethod == WindowHanning))
            // We happly windows twice (before, and after)
            hanningFactor = overlapping/4.0;
        else
            // We apply only one window, so we can narrow more
            hanningFactor = overlapping/2.0;
    }
    
    // Analysis
    //
    // NOTE: if no overlapping, it is sometimes better to even have an analysis window !
    //
    
    // FIX: if we want to analyse only, with overlapping > 1, we need a window !
    // (otherwise there would be some discontinuities, due to rectangular window)
    // (vertical clear bars in the spectrogram for Ghost for example)
    
    //if ((analysisMethod == WindowHanning) && (overlapping > 1))
    if (analysisMethod == WindowHanning)
        Window::MakeHanningPow(anaWindowSize, hanningFactor, analysisWindow);
    else
        Window::MakeSquare(anaWindowSize, 1.0, analysisWindow);
    
    // Synthesis
    
    // FIX (not tested): it should be logical to have synthesis window
    // for synthesis and overlapping == 1
    
    //if ((synthesisMethod == WindowHanning) && (overlapping > 1))
    if (synthesisMethod == WindowHanning)
        Window::MakeHanningPow(synWindowSize, hanningFactor, synthesisWindow);
    else
        Window::MakeSquare(synWindowSize, 1.0, synthesisWindow);
        
    // Normalize
    if ((analysisMethod == WindowHanning) &&
        (synthesisMethod == WindowRectangular))
    {
        Window::NormalizeWindow(analysisWindow, overlapping);
    }
    else if((analysisMethod == WindowRectangular) &&
            (synthesisMethod == WindowHanning))
    {
        Window::NormalizeWindow(synthesisWindow, overlapping);
    }
    else if((analysisMethod == WindowHanning) &&
            (synthesisMethod == WindowHanning))
    {
        // Must make the test
        // Otherwise, when overlapping is 1 (no overlapping),
        // we have two rectangular windows, and cola returns -1
        if (overlapping > 1)
        {
            // We have both windows
            // We must compute the cola of the next hanning factor,
            // then normalize with it
            //
            
            WDL_TypedBuf<BL_FLOAT> win;
            Window::MakeHanningPow(analysisWindow->GetSize(), hanningFactor*2, &win);
            
            BL_FLOAT cola = Window::CheckCOLA(&win, overlapping);
            
            // Normalize only the synthesis window
            Window::NormalizeWindow(synthesisWindow, cola);
        }
    }
    else if ((analysisMethod == WindowRectangular) &&
             (synthesisMethod == WindowRectangular))
    {
        // Normalize only the synthesis window...
        Window::NormalizeWindow(synthesisWindow, overlapping);
    }
}

void
FftProcessObj15::GetAllSamples(vector<WDL_TypedBuf<BL_FLOAT> * > *samples)
{
    for (int i = 0; i < mChannels.size(); i++)
    {
        ProcessObjChannel *chan = mChannels[i];
    
        WDL_TypedBuf<BL_FLOAT> *s = chan->GetSamples();
    
        samples->push_back(s);
    }
}

void
FftProcessObj15::GetAllFftSamples(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > *samples)
{
    for (int i = 0; i < mChannels.size(); i++)
    {
        ProcessObjChannel *chan = mChannels[i];
        
        WDL_TypedBuf<WDL_FFT_COMPLEX> *s = chan->GetFft();
        
        samples->push_back(s);
    }
}

void
FftProcessObj15::GetAllResultSamples(vector<WDL_TypedBuf<BL_FLOAT> * > *samples)
{
    for (int i = 0; i < mChannels.size(); i++)
    {
        ProcessObjChannel *chan = mChannels[i];
        
        WDL_TypedBuf<BL_FLOAT> *s = chan->GetResultSamples();
        
        samples->push_back(s);
    }
}

void
FftProcessObj15::ComputeFft(const WDL_TypedBuf<BL_FLOAT> &samples,
                            WDL_TypedBuf<WDL_FFT_COMPLEX> *fftSamples,
                            int freqRes)
{
    int bufSize = samples.GetSize();
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> tmpFftBuf;
    tmpFftBuf.Resize(bufSize);

    fftSamples->Resize(bufSize);
    
    // FIX: for FLStudio graphic freeze on AutoGain on Windows
    // This point really fixes the bug !
    //
    // (certainly that the data sent for fft was all zero,
    // and WDL_FFT has strange results when all the input samples are null
    if (Utils::IsAllZero(samples))
    {
          Utils::FillAllZero(fftSamples);
	
          return;
    }

    BL_FLOAT coeff = 1.0;
#if NORMALIZE_FFT
    if (bufSize > 0)
      // Test just in case (added for AutoGain FLStudio Win graphic freeze)
      coeff = 1.0/bufSize;
#endif
    
    // Fill the fft buf
    for (int i = 0; i < bufSize; i++)
    {
        // No need to divide by mBufSize if we ponderate analysis hanning window
        tmpFftBuf.Get()[i].re = samples.Get()[i]*coeff;
        tmpFftBuf.Get()[i].im = 0.0;
    }
    
    // Take FREQ_RES into account
    // Not sure we must do that only when normalizing or not
    for (int i = 0; i < bufSize; i++)
        tmpFftBuf.Get()[i].re *= freqRes;
    
    // Do the fft
    // Do it on the window but also in following the empty space, to capture remaining waves
    WDL_fft(tmpFftBuf.Get(), bufSize, false);
    
    // Sort the fft buffer
    for (int i = 0; i < bufSize; i++)
    {
        int k = WDL_fft_permute(bufSize, i);

        fftSamples->Get()[i].re = tmpFftBuf.Get()[k].re;
        fftSamples->Get()[i].im = tmpFftBuf.Get()[k].im;
    }
}

void
FftProcessObj15::ComputeInverseFft(const WDL_TypedBuf<WDL_FFT_COMPLEX> &fftSamples,
                                   WDL_TypedBuf<BL_FLOAT> *samples,
                                   int freqRes)
{
    int bufSize = fftSamples.GetSize();
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> tmpFftBuf;
    tmpFftBuf.Resize(bufSize);

    samples->Resize(fftSamples.GetSize());
    
    for (int i = 0; i < bufSize; i++)
    {
        int k = WDL_fft_permute(bufSize, i);
        
        tmpFftBuf.Get()[k].re = fftSamples.Get()[i].re;
        tmpFftBuf.Get()[k].im = fftSamples.Get()[i].im;
    }
    
    // Should not do this step when not necessary (for example for transients)
    
    // Do the ifft
    WDL_fft(tmpFftBuf.Get(), bufSize, true);

    for (int i = 0; i < bufSize; i++)
        samples->Get()[i] = tmpFftBuf.Get()[i].re;
    
    // fft
    BL_FLOAT coeff = 1.0;
#if !NORMALIZE_FFT
    // Not sure about that...
    //coeff = bufSize;
#endif
    
    // Take FREQ_RES into account
    // Not sure we must do that only when normalizing or not
    coeff *= 1.0/freqRes;
    
    for (int i = 0; i < bufSize; i++)
        samples->Get()[i] *= coeff;
}

void
FftProcessObj15::ComputeInverseFft(const WDL_TypedBuf<WDL_FFT_COMPLEX> &fftSamples,
                                   WDL_TypedBuf<WDL_FFT_COMPLEX> *iFftSamples)
{
    int bufSize = fftSamples.GetSize();
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> tmpFftBuf;
    tmpFftBuf.Resize(bufSize);
    
    iFftSamples->Resize(fftSamples.GetSize());
    
    for (int i = 0; i < bufSize; i++)
    {
        int k = WDL_fft_permute(bufSize, i);
        
        tmpFftBuf.Get()[k].re = fftSamples.Get()[i].re;
        tmpFftBuf.Get()[k].im = fftSamples.Get()[i].im;
    }
    
    // Should not do this step when not necessary (for example for transients)
    
    // Do the ifft
    WDL_fft(tmpFftBuf.Get(), bufSize, true);
    
    for (int i = 0; i < bufSize; i++)
    {
        iFftSamples->Get()[i] = tmpFftBuf.Get()[i];
    }
}
