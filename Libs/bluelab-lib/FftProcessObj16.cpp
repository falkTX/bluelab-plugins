//
//  FftProcessObj16.cpp
//  Transient
//
//  Created by Apple m'a Tuer on 21/05/17.
//
//

#include <Window.h>
#include <BLUtils.h>
#include <BLDebug.h>

#include "FftProcessObj16.h"


//#define INF 1e15
//#define EPS 1e-15


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

#define OPTIM_SIMD 1

// Validated!
#define DENOISER_OPTIM3 1

// If we provided in input more input channels
// than the number declared when creating the FftProcessObj
// and use side chain => the side chain was not correct
// (the analysis window was never applied, or was several times,
// on the side chain channel)
//
// Steps to reproduce: create and FftProcessObj with 1 input channel and 1 sc channel
// Provide 2 input channels and 1 sc channel as input for processing
//
// Done during Rebalance update
#define FIX_TOO_MANY_INPUT_CHANNELS 1

// BUG: when output was null, the internal buffer grown more and more
#define FIX_NULL_OUT_MEMLEAK 1

#define OPTIM_CONSUME_LEFT 1

ProcessObj::ProcessObj(int bufferSize)
//: mInput(true),
//  mOutput(true)
{
    mBufferSize = bufferSize;
    
    //mTrackInput = false;
    //mTrackOutput = false;
}
    
ProcessObj::~ProcessObj() {}
    
void
ProcessObj::Reset(int bufferSize, int oversampling,
                  int freqRes, BL_FLOAT sampleRate)
{
    if (bufferSize > 0)
        mBufferSize = bufferSize;
    if (oversampling > 0)
        mOverlapping = oversampling;
    if (freqRes > 0)
        mFreqRes = freqRes;
    if (sampleRate > 0)
        mSampleRate = sampleRate;
    
    //Reset();
}

/*void
ProcessObj::Reset()
{
    // Tracking
    mInput.Reset();
    mOutput.Reset();
}*/

void
ProcessObj::ProcessInputSamplesPre(WDL_TypedBuf<BL_FLOAT> *ioBuffer,
                                   const WDL_TypedBuf<BL_FLOAT> *scBuffer) {}

void
ProcessObj::PreProcessSamplesBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer,
                                    const WDL_TypedBuf<BL_FLOAT> *scBuffer)
{
    //mInput.AddValues(*ioBuffer);
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
    //mOutput.AddValues(*ioBuffer);
}

void
ProcessObj::ProcessSamplesPost(WDL_TypedBuf<BL_FLOAT> *ioBuffer) {}

/*void
ProcessObj::SetTrackIO(int maxNumPoints, BL_FLOAT decimFactor,
                       bool trackInput, bool trackOutput)
{
    mTrackInput = trackInput;
    mTrackOutput = trackOutput;
        
    //if (mTrackInput)
    //    mInput.SetParams(maxNumPoints, decimFactor);
        
    //if (mTrackOutput)
    //    mOutput.SetParams(maxNumPoints, decimFactor);
}*/

/*
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
*/

///

class ProcessObjChannel
{
public:
    ProcessObjChannel(ProcessObj *processObj, int bufferSize);
    
    virtual ~ProcessObjChannel();
    
    void SetDefaultLatency(int latency);
    
    // For latency fix 2
    //int GetLatency();
    
    int ComputeLatency(int blockSize);
    
    virtual void Reset(int bufferSize, int overlapping, int freqRes, BL_FLOAT sampleRate);
    
    virtual void Reset();
    
    void SetSideChain(ProcessObjChannel *scChan);
    ProcessObjChannel *GetSideChainChannel();
    
    ProcessObj *GetProcessObject();
    void SetProcessObject(ProcessObj *obj);
    
    // Setters
    void SetAnalysisWindow(FftProcessObj16::WindowType type);
    
    void SetSynthesisWindow(FftProcessObj16::WindowType type);
    
    void SetKeepSynthesisEnergy(bool flag);
    
    void SetSkipFft(bool flag);
    
    // Skip Fft, but still process ProcessFftBuffer()
    void SetSkipFftProcess(bool flag);
    
    // OPTIM PROF Infra
    void SetSkipIFft(bool flag);
    
    
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
    //WDL_TypedBuf<BL_FLOAT> *GetSamples();
    WDL_TypedFastQueue<BL_FLOAT> *GetSamples();
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> *GetFft();
    
    WDL_TypedBuf<BL_FLOAT> *GetResultSamples();
    
    // If not data has been sent to the channel
    // (This can be a dummy channel, or unused channel)
    bool IsEmpty();
    
    void SetEnabled(bool flag);
    bool IsEnabled();
    
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
    enum FftProcessObj16::WindowType mAnalysisMethod;
    enum FftProcessObj16::WindowType mSynthesisMethod;
    
    WDL_TypedBuf<BL_FLOAT> mAnalysisWindow;
    WDL_TypedBuf<BL_FLOAT> mSynthesisWindow;
    
    
    bool mKeepSynthesisEnergy;
    
    bool mSkipFFT;
    // Skip fft, but still call ProcessFftBuffer()
    bool mSkipFFTProcess;
    
    bool mSkipIFFT;
    
    int mShift;
    
    bool mIsEnabled;
    
    // Input samples (bufferized)
    //WDL_TypedBuf<BL_FLOAT> mSamplesIn;
    WDL_TypedFastQueue<BL_FLOAT> mSamplesIn;
    
    // Current result
    WDL_TypedBuf<BL_FLOAT> mResult;
    
    // Result with overlap add
    //WDL_TypedBuf<BL_FLOAT> mResultSum;
    WDL_TypedFastQueue<BL_FLOAT> mResultSum;
    
    // Result output that is totally ready
    //WDL_TypedBuf<BL_FLOAT> mResultOut;
    WDL_TypedFastQueue<BL_FLOAT> mResultOut;
    
    
    WDL_TypedBuf<BL_FLOAT> mPreparedBuffer;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mFftBuf;
    BL_FLOAT mEnergy0;
        
    // Lazy evaluation
    bool mMustMakeWindows;
    
#if DC_OFFSET_PROCESS
    BL_FLOAT mDCOffset;
#endif

#if 0
    // For latency fix
    bool mBufWasFull;
    unsigned long long mTotalInSamples;
#endif
    
#if 1
    // For latency fix 2
    int mLatency;
    int mCurrentLatency;
#endif
    
    // Flag set if a channel is empty
    // Avoids processing an empty channel,
    // and moreover, to grow the output buffer indefinitely
    // if the channel is empty (memory leak)
    bool mEmptyChannel;
    
private:
    ProcessObjChannel *mScChan;

    // Tmp buffers
    WDL_TypedBuf<BL_FLOAT> mTmpBuf0;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf1;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf2;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf3;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf4;

    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf5;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mTmpBuf6;

    WDL_TypedBuf<BL_FLOAT> mTmpBuf7;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf8;
    WDL_TypedBuf<BL_FLOAT> mTmpBuf9;
};

ProcessObjChannel::ProcessObjChannel(ProcessObj *processObj, int bufferSize)
{
    mProcessObj = processObj;
    
    mBufferSize = bufferSize;
    
    mOverlapping = 1;
    mFreqRes = 1;
    mSampleRate = 44100.0;
    
    mShift = mBufferSize/mOverlapping;
    
    mAnalysisMethod = FftProcessObj16::WindowRectangular;
    mSynthesisMethod = FftProcessObj16::WindowRectangular;
    
    mKeepSynthesisEnergy = false;
    
    mSkipFFT = false;
    
    mSkipFFTProcess = false;
    
    mSkipIFFT = false;
    
    mScChan = NULL;
    
    mMustMakeWindows = true;
    
#if 0
    // Latency fix
    mBufWasFull = false;
    mTotalInSamples = 0;
#endif
    
#if 1
    // Latency fix 2
    mLatency = mBufferSize;
    mCurrentLatency = mLatency;
#endif
    
    mEmptyChannel = false;
    
    mIsEnabled = true;
    
#if DC_OFFSET_PROCESS
    mDCOffset = 0.0;
#endif
}

ProcessObjChannel::~ProcessObjChannel() {}

void
ProcessObjChannel::SetDefaultLatency(int latency)
{
    mLatency = latency;
    mCurrentLatency = mLatency;
}

//int
//ProcessObjChannel::GetLatency()
//{
//    return mLatency;
//}

int
ProcessObjChannel::ComputeLatency(int blockSize)
{
    int result = 0;
    int currentLatency = mLatency;
    
    // FIX: infinite loop with VST3
    // because block size was 0
    if (blockSize == 0)
        return mLatency;
    
    while(true)
    {
        if (currentLatency >= blockSize)
        {
            currentLatency -= blockSize;
            
            if (currentLatency < 0)
                currentLatency = 0;
            
            result += blockSize;
            
            continue;
        }
        
        if (currentLatency == 0)
        {
            // Nothing to do
            
            break;
        }
        
        int numRequestedLat = blockSize - currentLatency;
        if (numRequestedLat < 0)
            numRequestedLat = 0;
        
        if (numRequestedLat <= blockSize) // Just in case
        {
            result += currentLatency;
            
            break;
        }
    }
    
    return result;
}


void
ProcessObjChannel::Reset(int bufferSize, int overlapping,
                         int freqRes, BL_FLOAT sampleRate)
{
    mBufferSize = bufferSize;
    mOverlapping = overlapping;
    mFreqRes = freqRes;
    mSampleRate = sampleRate;
    
    // NEW (fixes !)
    mLatency = mBufferSize;
    
    Reset();
}

void
ProcessObjChannel::Reset()
{
    mShift = mBufferSize/mOverlapping;
    
    FftProcessObj16::MakeWindows(mBufferSize, mOverlapping,
                                 mAnalysisMethod, mSynthesisMethod,
                                 &mAnalysisWindow, &mSynthesisWindow);
    // Set the buffers
    //mSamplesIn.Resize(0);
    mSamplesIn.Clear();
    
     // TEST
    //BLUtils::ResizeFillZeros(&mResult, mBufferSize);
    mResult.Resize(mBufferSize);
    BLUtils::FillAllZero(&mResult);
    
    int resultSize = 2*mBufferSize*mFreqRes;
    //BLUtils::ResizeFillZeros(&mResultSum, resultSize);

    //mResultSum.Resize(resultSize);
    //BLUtils::FillAllZero(&mResultSum);
    mResultSum.Clear();
    mResultSum.Add(0, resultSize);
    
    //mResultOut.Resize(0);
    mResultOut.Clear();
    
    int bufSize = mBufferSize*mFreqRes;
    //BLUtils::ResizeFillZeros(&mFftBuf, bufSize);
    mFftBuf.Resize(bufSize);
    BLUtils::FillAllZero(&mFftBuf);
    
    if (mProcessObj != NULL)
        mProcessObj->Reset(mBufferSize, mOverlapping, mFreqRes, mSampleRate);
    
#if 0
    // For latency fix
    mBufWasFull = false;
    mTotalInSamples = 0;
#endif
    
#if 1
    // For latency fix 2
    mCurrentLatency = mLatency;
#endif
    
    mEmptyChannel = false;
}

void
ProcessObjChannel::SetSideChain(ProcessObjChannel *scChan)
{
    mScChan = scChan;
}

ProcessObjChannel *
ProcessObjChannel::GetSideChainChannel()
{
    return mScChan;
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
ProcessObjChannel::SetAnalysisWindow(FftProcessObj16::WindowType type)
{
    mAnalysisMethod = type;
    
    mMustMakeWindows = true;
}

void
ProcessObjChannel::SetSynthesisWindow(FftProcessObj16::WindowType type)
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

void
ProcessObjChannel::SetSkipFftProcess(bool flag)
{
    mSkipFFT = flag;
    mSkipFFTProcess = flag;
}

void
ProcessObjChannel::SetSkipIFft(bool flag)
{
    mSkipIFFT = flag;
}

bool
ProcessObjChannel::HasSamplesToProcess()
{
    //bool result = (mSamplesIn.GetSize() >= mBufferSize);
    bool result = (mSamplesIn.Available() >= mBufferSize);
    
    return result;
}

void
ProcessObjChannel::BufferProcessed()
{
    if (mEmptyChannel)
        return;
    
    NextSamplesBuffer();
    NextOutBuffer();
}

void
ProcessObjChannel::AddSamples(const WDL_TypedBuf<BL_FLOAT> &samples)
{
    int samplesSize = samples.GetSize();
    mSamplesIn.Add(samples.Get(), samplesSize);
    
#if 0
    // For latency fix
    mTotalInSamples += samples.GetSize();
#endif
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
            BLUtils::ResizeFillZeros(output, numRequested);
        }
        
        return false;
    }
}
#endif

#if 0 // Latency fix 1: 
      // Fails with Spatializer,
      // buffer size 2000, width=100%, bypass FftProcessObj16::ProcessFftBuffer()
      // => crackles

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
                BLUtils::ResizeFillZeros(output, numRequested);
            }
            
            return false;
        }

        // NEW
        
        // FIX: fixes auval crash
        if (mTotalInSamples < mBufferSize)
        {
            // For the moment, keep accumulating, to manage latency well
            if (output != NULL)
            {
                BLUtils::ResizeFillZeros(output, numRequested);
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
        WDL_TypedBuf<BL_FLOAT> &buf = mTmpBuf7;
        GetResultOutBuffer(&buf, remaining);
            
        if (output != NULL)
        {
            output->Resize(numRequested);
            BLUtils::FillAllZero(output);
            
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
        BLUtils::ResizeFillZeros(output, numRequested);
    }
            
    return false;
}
#endif

#if 1
// Latency fix 2 
//
// More clear fix
// (and manages block size > buffer size)
bool
ProcessObjChannel::GetResult(WDL_TypedBuf<BL_FLOAT> *output, int numRequested)
{
    // Get the computed result
    //int numOutSamples = mResultOut.GetSize();
    int numOutSamples = mResultOut.Available();
    
    // We have big latency, output only zeros
    if (mCurrentLatency >= numRequested)
    {
        // outputs zeros
        if (output != NULL)
        {
            output->Resize(numRequested);
            BLUtils::FillAllZero(output);
        }
        
        mCurrentLatency -= numRequested;
        if (mCurrentLatency < 0)
            mCurrentLatency = 0;
        
        return false;
    }
    
    // We have no latency, output result normally
    if (mCurrentLatency == 0)
    {
        if (numRequested <= numOutSamples) // just in case
        {
            WDL_TypedBuf<BL_FLOAT> &buf = mTmpBuf2;
            GetResultOutBuffer(&buf, numRequested);
            
            if (output != NULL)
            {
                *output = buf;
            }
            
            return true;
        }
    }
    
    // We have medium latency, outputs zeros and a part of the result
    
    int numRequestedLat = numRequested - mCurrentLatency;
    
    // Just in cases
    if (numRequestedLat < 0)
        numRequestedLat = 0;
    
#if !FIX_NULL_OUT_MEMLEAK
    if (output != NULL)
    {
        output->Resize(numRequested);
        BLUtils::FillAllZero(output);
        
        if (numRequestedLat <= numOutSamples) // Just in case
        {
            // get only the necessary samples
            WDL_TypedBuf<BL_FLOAT> &buf = mTmpBuf8;
            GetResultOutBuffer(&buf, numRequestedLat);
        
            if (output != NULL)
            {
                // Write to output, the correct number of samples,
                // and at the correct position
#if !OPTIM_SIMD
                for (int i = 0; i < numRequestedLat; i++)
                {
                    output->Get()[mCurrentLatency + i] = buf.Get()[i];
                }
#else
                memcpy(&output->Get()[mCurrentLatency], buf.Get(),
                       numRequestedLat*sizeof(BL_FLOAT));
#endif
            }
            
            mCurrentLatency = 0;
        
            return true;
        }
    }
#else
    if (output != NULL)
    {
        output->Resize(numRequested);
        BLUtils::FillAllZero(output);
    }
    if (numRequestedLat <= numOutSamples) // Just in case
    {
        // get only the necessary samples
        WDL_TypedBuf<BL_FLOAT> &buf = mTmpBuf9;
        GetResultOutBuffer(&buf, numRequestedLat);
            
        if (output != NULL)
        {
            // Write to output, the correct number of samples,
            // and at the correct position
#if !OPTIM_SIMD
            for (int i = 0; i < numRequestedLat; i++)
            {
                output->Get()[mCurrentLatency + i] = buf.Get()[i];
            }
#else
            memcpy(&output->Get()[mCurrentLatency],
                   buf.Get(),
                   numRequestedLat*sizeof(BL_FLOAT));
#endif
        }
            
        mCurrentLatency = 0;
            
        return true;
    }
#endif
    
    return false;
}
#endif


// Accessors for processing horizontally
//WDL_TypedBuf<BL_FLOAT> *
WDL_TypedFastQueue<BL_FLOAT> *
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

bool
ProcessObjChannel::IsEmpty()
{
    return mEmptyChannel;
}

void
ProcessObjChannel::SetEnabled(bool flag)
{
    mIsEnabled = flag;
}

bool
ProcessObjChannel::IsEnabled()
{
    return mIsEnabled;
}

void
ProcessObjChannel::PrepareBufferStep()
{
    // NEW
  
    // This can be an empty channel !
    // i.e without samples added
    // (e.g in the case we create 2 channels, and we are in mono)
    //if (mSamplesIn.GetSize() < mBufferSize)
    if (mSamplesIn.Available() < mBufferSize)
    {
        mEmptyChannel = true;
        
        return;
    }
    mEmptyChannel = false;
    
    mPreparedBuffer.Resize(mBufferSize);
    
#if !OPTIM_SIMD
    //for (int i = 0; i < mBufferSize; i++)
    //    mPreparedBuffer.Get()[i] = mSamplesIn.Get()[i];
    mSamplesIn.GetToBuf(0, mPreparedBuffer.Get(), mBufferSize);
#else
    //memcpy(mPreparedBuffer.Get(), mSamplesIn.Get(), mBufferSize*sizeof(BL_FLOAT));
    mSamplesIn.GetToBuf(0, mPreparedBuffer.Get(), mBufferSize);
#endif
    
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
    if (mEmptyChannel)
        return;
    
    if (mMustMakeWindows)
    {
        FftProcessObj16::MakeWindows(mBufferSize, mOverlapping,
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
    if (mEmptyChannel)
        return;
    
    // Compute "energy" just after analysis windowing
    // then just after synthesis windowing
    // Compute and apply a coefficient to avoid loosing energy
    mEnergy0 = 0.0;
    if (mKeepSynthesisEnergy)
        mEnergy0 = BLUtils::ComputeAbsAvg(mPreparedBuffer);
    
    // windowing before zero padding
    // See: http://www.bitweenie.com/listings/fft-zero-padding/
    // (last links of the page)
    if (mFreqRes > 1)
    {
        // Non-cyclic technique, to avoid aliasing
        BLUtils::ResizeFillZeros(&mPreparedBuffer,
                                 mPreparedBuffer.GetSize()*mFreqRes);
    }
    
    if (!mIsEnabled || mSkipFFT)
    {
#if 0 // Original version, tested a lot on iPlug1
        // Be careful to not add the beginning of the buffer
        // (otherwise the windows will be applied twice sometimes).
        mResult.Add(mPreparedBuffer.Get(), mBufferSize);
#endif

#if 1 // New version, for iPlug2 and memory optimization
        mResult = mPreparedBuffer;
#endif
    }
    else
    {
#if DC_OFFSET_PROCESS
        mDCOffset = BLUtils::ComputeAvg(mPreparedBuffer);
        BLUtils::AddValues(&mPreparedBuffer, -mDCOffset);
#endif
        
        // NOTE: Here, we have checked that mFreqRes didn't modify the amplitude
        // of the out samples, neither the intermediate magnitude !
        FftProcessObj16::ComputeFft(mPreparedBuffer, &mFftBuf, mFreqRes, &mTmpBuf5);
    }
}

void
ProcessObjChannel::ProcessFftStep()
{
    if (mEmptyChannel)
        return;
    
    if (mIsEnabled && (!mSkipFFT || mSkipFFTProcess))
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
    if (mEmptyChannel)
        return;

    if (mIsEnabled && !mSkipIFFT)
    {
        FftProcessObj16::ComputeInverseFft(mFftBuf, &mResult, mFreqRes, &mTmpBuf6);
    }
    else
    {
        // Solution 1: zero buffer
        //mResult.Resize(mFftBuf.GetSize());
        //BLUtils::FillAllZero(&mResult); // Just in case
        
        // Solution 2: take the previous windowed samples
        mResult = mPreparedBuffer;
    }
    
#if DC_OFFSET_PROCESS
    BLUtils::AddValues(&mResult, mDCOffset);
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
    if (mEmptyChannel)
        return;
    
    if (mKeepSynthesisEnergy)
    {
        BL_FLOAT energy1 = BLUtils::ComputeAbsAvg(mResult);
        
        BL_FLOAT energyCoeff = 1.0;
        if (energy1 > 0.0)
            energyCoeff = mEnergy0/energy1;
        
        // If mOverlapping is 1, then div is 0
        if (mOverlapping > 1)
        {
            BLUtils::MultValues(&mResult, energyCoeff);
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
        FftProcessObj16::MakeWindows(mBufferSize, mOverlapping,
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
    if (mEmptyChannel)
        return;
    
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

#if 0
#if !OPTIM_SIMD
    for (int i = 0; i < loopCount; i++)
    {
        mResultSum.Get()[i] += mResult.Get()[i];
    }
#else
    BLUtils::AddValues(mResultSum.Get(), mResult.Get(), loopCount);
#endif
#endif

    WDL_TypedBuf<BL_FLOAT> &buf = mTmpBuf3;
    buf.Resize(loopCount);
    mResultSum.GetToBuf(0, buf.Get(), loopCount);
    
#if !OPTIM_SIMD
    for (int i = 0; i < loopCount; i++)
    {
        buf.Get()[i] += mResult.Get()[i];
    }
#else
    BLUtils::AddValues(buf.Get(), mResult.Get(), loopCount);
#endif
    
    mResultSum.SetFromBuf(0, buf.Get(), loopCount);
}

void
ProcessObjChannel::NextSamplesBuffer()
{
    // Reduce the size by mOverlap
    //int size = mSamplesIn.GetSize();
    int size = mSamplesIn.Available();
    
    if (size == mShift)
    {
        //mSamplesIn.Resize(0);
        mSamplesIn.Clear();
    }
    else if (size > mShift)
    {
        // Resize down, skipping left
        
        //BLUtils::ConsumeLeft(&mSamplesIn, mShift);
        mSamplesIn.Advance(mShift);
    }
}

#if !OPTIM_CONSUME_LEFT 
// ORIGIN (allocate and free much memory)
void
ProcessObjChannel::NextOutBuffer()
{
    //if (mResultSum.GetSize() < mBufferSize)
    //    return;
    if (mResultSum.Available() < mBufferSize)
        return;
        
    // NEW
    
    // Let the possiblity to modify, or even resample
    // the result, before adding it to the object
    //WDL_TypedBuf<BL_FLOAT> samplesToAdd;
    //samplesToAdd.Add(mResultSum.Get(), mShift);
    WDL_TypedBuf<BL_FLOAT> &samplesToAdd = mTmpBuf4;
    samplesToAdd.Resize(mShift);
    mResultSum.GetToBuf(0, sampleToAdd.Get(), mShift);
 
    if (mProcessObj != NULL)
    {
        mProcessObj->ProcessSamplesPost(&samplesToAdd);
    }
    mResultOut.Add(samplesToAdd.Get(), samplesToAdd.GetSize());
    
    // OLD
    
    //WDL_TypedBuf<BL_FLOAT> newBuffer;
    //mResultOut.Add(mResultSum.Get(), mShift);

#if 0
    if (mResultSum.GetSize() == mShift)
    {
        mResultSum.Resize(0);
    }
    else if (mResultSum.GetSize() > mBufferSize)
    {
        // Resize down, skipping left
        BLUtils::ConsumeLeft(&mResultSum, mShift);
    }
    
    // Grow the output with zeros
    BLUtils::ResizeFillZeros(&mResultSum,
                             mResultSum.GetSize() + mShift);
#endif
    if (mResultSum.Available() == mShift)
    {
        mResultSum.Clear();
    }
    else if (mResultSum.Available() > mBufferSize)
    {
        mResultSum.Advance(mShift);
    }

    mResultSum.Add(0, mShift);
}
#endif

#if OPTIM_CONSUME_LEFT
// NEW: try to optimize memory
void
ProcessObjChannel::NextOutBuffer()
{
    //if (mResultSum.GetSize() < mBufferSize)
    //    return;
    if (mResultSum.Available() < mBufferSize)
        return;
        
    // Let the possiblity to modify, or even resample
    // the result, before adding it to the object
    if (mTmpBuf0.GetSize() != mShift)
        mTmpBuf0.Resize(mShift);
    
    //samplesToAdd.Add(mResultSum.Get(), mShift);
    WDL_TypedBuf<BL_FLOAT> &samplesToAdd = mTmpBuf0;
    //memcpy(samplesToAdd.Get(), mResultSum.Get(), mShift*sizeof(BL_FLOAT));
    mResultSum.GetToBuf(0, samplesToAdd.Get(), mShift);
    
    if (mProcessObj != NULL)
    {
        mProcessObj->ProcessSamplesPost(&samplesToAdd);
    }
    mResultOut.Add(samplesToAdd.Get(), samplesToAdd.GetSize());
    
    //

#if 0
    if (mResultSum.GetSize() == mShift)
    {
        mResultSum.Resize(0);
    
        // Grow the output with zeros
        BLUtils::ResizeFillZeros(&mResultSum,
                                 mResultSum.GetSize() + mShift);
    }
    else if (mResultSum.GetSize() > mBufferSize)
    {
        if (mTmpBuf1.GetSize() != mResultSum.GetSize())
            mTmpBuf1.Resize(mResultSum.GetSize());
        // Copy the intersting data at the beginning
        memcpy(mTmpBuf1.Get(),
               &mResultSum.Get()[mShift],
               (mResultSum.GetSize() - mShift)*sizeof(BL_FLOAT));
       
        // Fill the end with zeros
        memset(&mTmpBuf1.Get()[mTmpBuf1.GetSize() - mShift],
               0, mShift*sizeof(BL_FLOAT));
        
        // Copy the result
        memcpy(mResultSum.Get(),
               mTmpBuf1.Get(),
               mResultSum.GetSize()*sizeof(BL_FLOAT));
    }
#endif

    if (mResultSum.Available() == mShift)
    {
        mResultSum.Clear();
    
        // Grow the output with zeros
        //BLUtils::ResizeFillZeros(&mResultSum,
        //                         mResultSum.GetSize() + mShift);
        mResultSum.Add(0, mShift);
    }
    //else if (mResultSum.GetSize() > mBufferSize)
    else if (mResultSum.Available() > mBufferSize)
    {
        //if (mTmpBuf1.GetSize() != mResultSum.GetSize())
        //    mTmpBuf1.Resize(mResultSum.GetSize());
        if (mTmpBuf1.GetSize() != mResultSum.Available())
            mTmpBuf1.Resize(mResultSum.Available());
    
        // Copy the intersting data at the beginning
        //memcpy(mTmpBuf1.Get(),
        //       &mResultSum.Get()[mShift],
        //       (mResultSum.GetSize() - mShift)*sizeof(BL_FLOAT));
        mResultSum.GetToBuf(mShift, mTmpBuf1.Get(), mResultSum.Available() - mShift);
        
        // Fill the end with zeros
        memset(&mTmpBuf1.Get()[mTmpBuf1.GetSize() - mShift],
               0, mShift*sizeof(BL_FLOAT));
        
        // Copy the result
        //memcpy(mResultSum.Get(),
        //       mTmpBuf1.Get(),
        //       mResultSum.GetSize()*sizeof(BL_FLOAT));
        mResultSum.SetFromBuf(0, mTmpBuf1.Get(), mResultSum.Available());
    }
}
#endif

void
ProcessObjChannel::GetResultOutBuffer(WDL_TypedBuf<BL_FLOAT> *output,
                                      int numRequested)
{
    //int size = mResultOut.GetSize();
    int size = mResultOut.Available();
    
    // Should not happen
    if (size < numRequested)
        return;
    
    //
    output->Resize(numRequested);
    
    // Copy the result
    //memcpy(output->Get(), mResultOut.Get(), numRequested*sizeof(BL_FLOAT));
    mResultOut.GetToBuf(0, output->Get(), numRequested);
    
    // Resize down, skipping left
    //BLUtils::ConsumeLeft(&mResultOut, numRequested);
    mResultOut.Advance(numRequested);
}

void
ProcessObjChannel::ApplyAnalysisWindow(WDL_TypedBuf<BL_FLOAT> *samples)
{
    // Multiply the samples by analysis window
    // The window can be identity if necessary (no analysis window, or no overlap)
#if !OPTIM_SIMD
    for (int i = 0; i < samples->GetSize(); i++)
    {
        BL_FLOAT winCoeff = mAnalysisWindow.Get()[i];
        samples->Get()[i] *= winCoeff;
    }
#else
    BLUtils::MultValues(samples, mAnalysisWindow);
#endif
}

void
ProcessObjChannel::ApplySynthesisWindow(WDL_TypedBuf<BL_FLOAT> *samples)
{
    // Multiply the samples by synthesis window
    // The window can be identity if necessary
    // (no synthesis window, or no overlap)
#if !OPTIM_SIMD
    for (int i = 0; i < samples->GetSize(); i++)
    {
        BL_FLOAT winCoeff = mSynthesisWindow.Get()[i];
        samples->Get()[i] *= winCoeff;
    }
#else
    BLUtils::MultValues(samples, mSynthesisWindow);
#endif
}

void
FftProcessObj16::Init()
{
    // Init WDL FFT
    WDL_fft_init();
}

// NOTE: With freqRes == 2, and overlapping = 1 => better
// the transient signal is decreased / the other signal stays constant
// => that's what we want !
// With previous version, the other signal increased
FftProcessObj16::FftProcessObj16(const vector<ProcessObj *> &processObjs,
                                 int numChannels, int numScInputs,
                                 int bufferSize, int overlapping, int freqRes,
                                 BL_FLOAT sampleRate)
{
    mBufferSize = bufferSize;
    
    Reset(bufferSize, overlapping, freqRes, sampleRate);
    
    mNumScInputs = numScInputs;
    
    mChannels.resize(numChannels + numScInputs);
    for (int i = 0; i < mChannels.size(); i++)
    {
        ProcessObj *obj = (i < processObjs.size()) ? processObjs[i] : NULL;
        ProcessObjChannel *chan = new ProcessObjChannel(obj, bufferSize);
        
        chan->Reset(mBufferSize, mOverlapping, mFreqRes, mSampleRate);
        
        mChannels[i] = chan;
    }
    
    SetupSideChain();
}

FftProcessObj16::~FftProcessObj16()
{
    for (int i = 0; i < mChannels.size(); i++)
    {
        ProcessObjChannel *chan = mChannels[i];
     
        delete chan;
    }
}

void
FftProcessObj16::SetDefaultLatency(int latency)
{
    for (int i = 0; i < mChannels.size(); i++)
        mChannels[i]->SetDefaultLatency(latency);
}

//int
//FftProcessObj16::GetLatency()
//{
//    if (mChannels.empty())
//        return mBufferSize;
//
//    int latency = mChannels[0]->GetLatency();
//
//    return latency;
//}

int
FftProcessObj16::ComputeLatency(int blockSize)
{
    int latency = mBufferSize;
    
    if (!mChannels.empty())
    {
        latency = mChannels[0]->ComputeLatency(blockSize);
    }
    
    return latency;
}

void
FftProcessObj16::Reset(int bufferSize, int overlapping, int freqRes, BL_FLOAT sampleRate)
{
    if (bufferSize > 0) // Tests added for Ghost-X (FIX_FFT_SAMPLERATE)
        mBufferSize = bufferSize;
    
    if (overlapping > 0)
        mOverlapping = overlapping;
    
    if (freqRes > 0)
        mFreqRes = freqRes;
    
    if (sampleRate > 0.0)
        mSampleRate = sampleRate;
    
    Reset();
}

void
FftProcessObj16::Reset()
{
    for (int i = 0; i < mChannels.size(); i++)
    {
        ProcessObjChannel *chan = mChannels[i];
        chan->Reset(mBufferSize, mOverlapping, mFreqRes, mSampleRate);
    }
    
    for (int i = 0; i < mMcProcesses.size(); i++)
    {
        MultichannelProcess *mcProcess = mMcProcesses[i];
        mcProcess->Reset(mBufferSize, mOverlapping, mFreqRes, mSampleRate);
    }
}

int
FftProcessObj16::GetBufferSize()
{
    return mBufferSize;
}

int
FftProcessObj16::GetOverlapping()
{
    return mOverlapping;
}

void
FftProcessObj16::SetOverlapping(int overlapping)
{
    mOverlapping = overlapping;

    Reset(mBufferSize, mOverlapping, mFreqRes, mSampleRate);
}

int
FftProcessObj16::GetFreqRes()
{
    return mFreqRes;
}

BL_FLOAT
FftProcessObj16::GetSampleRate()
{
    return mSampleRate;
}

void
FftProcessObj16::SetAnalysisWindow(int channelNum, WindowType type)
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
FftProcessObj16::SetSynthesisWindow(int channelNum, WindowType type)
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
FftProcessObj16::SetKeepSynthesisEnergy(int channelNum, bool flag)
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
FftProcessObj16::SetSkipFft(int channelNum, bool flag)
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
FftProcessObj16::SetSkipFftProcess(int channelNum, bool flag)
{
    if ((channelNum != ALL_CHANNELS) &&
        (channelNum < mChannels.size()))
        mChannels[channelNum]->SetSkipFftProcess(flag);
    else
    {
        for (int i = 0; i < mChannels.size(); i++)
            mChannels[i]->SetSkipFftProcess(flag);
    }
}

void
FftProcessObj16::SetSkipIFft(int channelNum, bool flag)
{
    if ((channelNum != ALL_CHANNELS) &&
        (channelNum < mChannels.size()))
        mChannels[channelNum]->SetSkipIFft(flag);
    else
    {
        for (int i = 0; i < mChannels.size(); i++)
            mChannels[i]->SetSkipIFft(flag);
    }
}


void
FftProcessObj16::AddMultichannelProcess(MultichannelProcess *mcProcess)
{
    mMcProcesses.push_back(mcProcess);
}

void
FftProcessObj16::InputSamplesReady()
{
    if (mMcProcesses.empty())
        return;
    
    //vector<WDL_TypedBuf<BL_FLOAT> * > samples;
    vector<WDL_TypedFastQueue<BL_FLOAT> * > &samples0 = mTmpBuf9;
    GetAllSamples(&samples0);
    
    // Convert queue to vector (copy)
    vector<WDL_TypedBuf<BL_FLOAT> > &samples1 = mTmpBuf6;
    samples1.resize(samples0.size());
    for (int i = 0; i < samples0.size(); i++)
    {
        samples1[i].Resize(samples0[i]->Available());
        samples0[i]->GetToBuf(0, samples1[i].Get(), samples0[i]->Available());
    }
    
    vector<WDL_TypedBuf<BL_FLOAT> * > &samples = mTmpBuf10;
    samples.resize(samples1.size());
    for (int i = 0; i < samples.size(); i++)
    {
        samples[i] = &(samples1[i]);
    }
 
    //
    vector<WDL_TypedBuf<BL_FLOAT> > &scSamples = mTmpBuf4;
    GetAllScSamples(&scSamples);
    
    for (int i = 0; i < mMcProcesses.size(); i++)
    {
        MultichannelProcess *mcProcess = mMcProcesses[i];
        
        vector<WDL_TypedBuf<BL_FLOAT> > *sc = NULL;
        if (!scSamples.empty())
            sc = &scSamples;
        
        mcProcess->ProcessInputSamples(&samples, sc);
    }

    // Convert back vector to queue (copy)
    for (int i = 0; i < samples0.size(); i++)
    {
        samples0[i]->SetFromBuf(0, samples[i]->Get(), samples0[i]->Available());
    }
}

void
FftProcessObj16::InputSamplesWinReady()
{
    if (mMcProcesses.empty())
        return;
    
    //vector<WDL_TypedBuf<BL_FLOAT> * > samples;
    vector<WDL_TypedFastQueue<BL_FLOAT> * > &samples0 = mTmpBuf11;
    GetAllSamples(&samples0);

    // Convert queue to vector (copy)
    vector<WDL_TypedBuf<BL_FLOAT> > &samples1 = mTmpBuf3;
    samples1.resize(samples0.size());
    for (int i = 0; i < samples0.size(); i++)
    {
        samples1[i].Resize(samples0[i]->Available());
        samples0[i]->GetToBuf(0, samples1[i].Get(), samples0[i]->Available());
    }
    
    vector<WDL_TypedBuf<BL_FLOAT> * > &samples = mTmpBuf12;
    samples.resize(samples1.size());
    for (int i = 0; i < samples.size(); i++)
    {
        samples[i] = &(samples1[i]);
    }
 
    vector<WDL_TypedBuf<BL_FLOAT> > &scSamples = mTmpBuf13;
    GetAllScSamples(&scSamples);
    
    for (int i = 0; i < mMcProcesses.size(); i++)
    {
        MultichannelProcess *mcProcess = mMcProcesses[i];
        
        vector<WDL_TypedBuf<BL_FLOAT> > *sc = NULL;
        if (!scSamples.empty())
            sc = &scSamples;
        
        mcProcess->ProcessInputSamplesWin(&samples, sc);
    }

    // Convert back vector to queue (copy)
    for (int i = 0; i < samples0.size(); i++)
    {
        samples0[i]->SetFromBuf(0, samples[i]->Get(), samples0[i]->Available());
    }
}

void
FftProcessObj16::InputFftReady()
{
    if (mMcProcesses.empty())
        return;
    
    vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > &samples = mTmpBuf14;
    GetAllFftSamples(&samples);
    
    vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > &scSamples = mTmpBuf7;
    GetAllFftScSamples(&scSamples);
    
    for (int i = 0; i < mMcProcesses.size(); i++)
    {
        MultichannelProcess *mcProcess = mMcProcesses[i];
        
        vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > *sc = NULL;
        if (!scSamples.empty())
            sc = &scSamples;
        
        mcProcess->ProcessInputFft(&samples, sc);
    }
}

void
FftProcessObj16::ResultFftReady()
{
    if (mMcProcesses.empty())
        return;
    
    vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > &samples = mTmpBuf15;
    GetAllFftSamples(&samples);
    
    vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > &scSamples = mTmpBuf8;
    GetAllFftScSamples(&scSamples);
    
    for (int i = 0; i < mMcProcesses.size(); i++)
    {
        MultichannelProcess *mcProcess = mMcProcesses[i];
        
        vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > *sc = NULL;
        if (!scSamples.empty())
            sc = &scSamples;

        mcProcess->ProcessResultFft(&samples, sc);
    }
}

void
FftProcessObj16::ResultSamplesWinReady()
{
    if (mMcProcesses.empty())
        return;
    
    vector<WDL_TypedBuf<BL_FLOAT> * > &samples = mTmpBuf16;
    GetAllResultSamples(&samples);
    
    vector<WDL_TypedBuf<BL_FLOAT> > &scSamples = mTmpBuf17;
    GetAllResultScSamples(&scSamples);
    
    for (int i = 0; i < mMcProcesses.size(); i++)
    {
        MultichannelProcess *mcProcess = mMcProcesses[i];
        
        vector<WDL_TypedBuf<BL_FLOAT> > *sc = NULL;
        if (!scSamples.empty())
            sc = &scSamples;
        
        mcProcess->ProcessResultSamplesWin(&samples, sc);
    }
}

void
FftProcessObj16::ResultSamplesReady()
{
    if (mMcProcesses.empty())
        return;
    
    vector<WDL_TypedBuf<BL_FLOAT> * > &samples = mTmpBuf18;
    GetAllResultSamples(&samples);
    
    vector<WDL_TypedBuf<BL_FLOAT> > &scSamples= mTmpBuf19;
    GetAllResultScSamples(&scSamples);
    
    for (int i = 0; i < mMcProcesses.size(); i++)
    {
        MultichannelProcess *mcProcess = mMcProcesses[i];
        
        vector<WDL_TypedBuf<BL_FLOAT> > *sc = NULL;
        if (!scSamples.empty())
            sc = &scSamples;
        
        mcProcess->ProcessResultSamples(&samples, sc);
    }
}

void
FftProcessObj16::SamplesToMagnPhases(const WDL_TypedBuf<BL_FLOAT> &inSamples,
                                     WDL_TypedBuf<BL_FLOAT> *outFftMagns,
                                     WDL_TypedBuf<BL_FLOAT> *outFftPhases)
{
    int numSamples = inSamples.GetSize();
    int numPaddedSamples = BLUtils::NextPowerOfTwo(numSamples);
    
    WDL_TypedBuf<BL_FLOAT> paddedSamples = inSamples;
    BLUtils::ResizeFillZeros(&paddedSamples, numPaddedSamples);
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> fftSamples;
    int freqRes = 1;
    ComputeFft(paddedSamples, &fftSamples, freqRes);
    
    WDL_TypedBuf<BL_FLOAT> magns;
    WDL_TypedBuf<BL_FLOAT> phases;
    BLUtils::ComplexToMagnPhase(&magns, &phases, fftSamples);
    
    if (outFftMagns != NULL)
        *outFftMagns = magns;
    
    if (outFftPhases != NULL)
        *outFftPhases = phases;
}

void
FftProcessObj16::SamplesToHalfMagnPhases(const WDL_TypedBuf<BL_FLOAT> &inSamples,
                                         WDL_TypedBuf<BL_FLOAT> *outFftMagns,
                                         WDL_TypedBuf<BL_FLOAT> *outFftPhases)
{
    int numSamples = inSamples.GetSize();
    int numPaddedSamples = BLUtils::NextPowerOfTwo(numSamples);
    
    WDL_TypedBuf<BL_FLOAT> paddedSamples = inSamples;
    BLUtils::ResizeFillZeros(&paddedSamples, numPaddedSamples);
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> fftSamples;
    int freqRes = 1;
    ComputeFft(paddedSamples, &fftSamples, freqRes);
    
    BLUtils::TakeHalf(&fftSamples);
    
    WDL_TypedBuf<BL_FLOAT> magns;
    WDL_TypedBuf<BL_FLOAT> phases;
    BLUtils::ComplexToMagnPhase(&magns, &phases, fftSamples);
    
    if (outFftMagns != NULL)
        *outFftMagns = magns;
    
    if (outFftPhases != NULL)
        *outFftPhases = phases;
}

void
FftProcessObj16::SamplesToHalfMagns(const WDL_TypedBuf<BL_FLOAT> &inSamples,
                                    WDL_TypedBuf<BL_FLOAT> *outFftMagns)
{
    int numSamples = inSamples.GetSize();
    int numPaddedSamples = BLUtils::NextPowerOfTwo(numSamples);
    
    WDL_TypedBuf<BL_FLOAT> paddedSamples = inSamples;
    BLUtils::ResizeFillZeros(&paddedSamples, numPaddedSamples);
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> fftSamples;
    int freqRes = 1;
    ComputeFft(paddedSamples, &fftSamples, freqRes);
    
    BLUtils::TakeHalf(&fftSamples);
    
    WDL_TypedBuf<BL_FLOAT> magns;
    BLUtils::ComplexToMagn(&magns, fftSamples);
    
    if (outFftMagns != NULL)
        *outFftMagns = magns;
}

/*void
FftProcessObj16::SamplesToHalfMagnPhases(const WDL_TypedBuf<BL_FLOAT> &inSamples,
                                         WDL_TypedBuf<float> *outFftMagns,
                                         WDL_TypedBuf<float> *outFftPhases)
{
    int numSamples = inSamples.GetSize();
    int numPaddedSamples = BLUtils::NextPowerOfTwo(numSamples);
    
    WDL_TypedBuf<BL_FLOAT> paddedSamples = inSamples;
    BLUtils::ResizeFillZeros(&paddedSamples, numPaddedSamples);
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> fftSamples;
    int freqRes = 1;
    ComputeFft(paddedSamples, &fftSamples, freqRes);
    
    BLUtils::TakeHalf(&fftSamples);
    
    WDL_TypedBuf<float> magns;
    WDL_TypedBuf<float> phases;
    BLUtils::ComplexToMagnPhase(&magns, &phases, fftSamples);
    
    if (outFftMagns != NULL)
        *outFftMagns = magns;
    
    if (outFftPhases != NULL)
        *outFftPhases = phases;
}
*/

/*
void
FftProcessObj16::SamplesToHalfMagns(const WDL_TypedBuf<BL_FLOAT> &inSamples,
                                    WDL_TypedBuf<float> *outFftMagns)
{
    int numSamples = inSamples.GetSize();
    int numPaddedSamples = BLUtils::NextPowerOfTwo(numSamples);
    
    WDL_TypedBuf<BL_FLOAT> paddedSamples = inSamples;
    BLUtils::ResizeFillZeros(&paddedSamples, numPaddedSamples);
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> fftSamples;
    int freqRes = 1;
    ComputeFft(paddedSamples, &fftSamples, freqRes);
    
    BLUtils::TakeHalf(&fftSamples);
    
    WDL_TypedBuf<float> magns;
    WDL_TypedBuf<float> phases;
    BLUtils::ComplexToMagn(&magns, fftSamples);
    
    if (outFftMagns != NULL)
        *outFftMagns = magns;
}
*/

void
FftProcessObj16::FftToSamples(const WDL_TypedBuf<WDL_FFT_COMPLEX> &fftBuffer,
                              WDL_TypedBuf<BL_FLOAT> *outSamples)
{
    outSamples->Resize(fftBuffer.GetSize());
    
    int freqRes = 1;
    ComputeInverseFft(fftBuffer, outSamples, freqRes);
}

void
FftProcessObj16::SamplesToFft(const WDL_TypedBuf<BL_FLOAT> &inSamples,
                              WDL_TypedBuf<WDL_FFT_COMPLEX> *outFftBuffer)
{
    outFftBuffer->Resize(inSamples.GetSize());
    
    int freqRes = 1;
    ComputeFft(inSamples, outFftBuffer, freqRes);
}

void
FftProcessObj16::HalfFftToSamples(const WDL_TypedBuf<WDL_FFT_COMPLEX> &fftBuffer,
                                  WDL_TypedBuf<BL_FLOAT> *outSamples)
{
    WDL_TypedBuf<WDL_FFT_COMPLEX> BL_FLOATFftBuffer = fftBuffer;
    BL_FLOATFftBuffer.Resize(fftBuffer.GetSize()*2);
    
    BLUtils::FillSecondFftHalf(&BL_FLOATFftBuffer);
    
    FftToSamples(BL_FLOATFftBuffer, outSamples);
}

// See: https://en.wikipedia.org/wiki/Cepstrum
void
FftProcessObj16::MagnsToCepstrum(const WDL_TypedBuf<BL_FLOAT> &halfMagns,
                                 WDL_TypedBuf<BL_FLOAT> *outCepstrum)
{
    // Fill second half
    WDL_TypedBuf<BL_FLOAT> magns = halfMagns;
    magns.Resize(magns.GetSize()*2);
                 
    BLUtils::FillSecondFftHalf(&magns);

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
        if (val > BL_EPS)
            logVal = std::log(val);
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
                  
    BLUtils::ComplexToMagn(&result, iFftSamples);
        
    // Power Cepstrum => compute the square
    for (int i = 0; i < result.GetSize(); i++)
    {
        BL_FLOAT val = result.Get()[i];
        BL_FLOAT val2 = val*val;
        result.Get()[i] = val2;
    }
    
    BLUtils::TakeHalf(&result);
    
    *outCepstrum = result;
}

ProcessObj *
FftProcessObj16::GetChannelProcessObject(int channelNum)
{
    if ((channelNum < 0) || (channelNum >= mChannels.size()))
        return NULL;
    
    ProcessObj *result = mChannels[channelNum]->GetProcessObject();
    
    return result;
}

void
FftProcessObj16::SetChannelProcessObject(int channelNum, ProcessObj *obj)
{
    if ((channelNum < 0) || (channelNum >= mChannels.size()))
        return;
    
    mChannels[channelNum]->SetProcessObject(obj);
}


//WDL_TypedBuf<BL_FLOAT> *
WDL_TypedFastQueue<BL_FLOAT> *
FftProcessObj16::GetChannelSamples(int channelNum)
{
    if ((channelNum < 0) || (channelNum >= mChannels.size()))
        return NULL;
    
    //WDL_TypedBuf<BL_FLOAT> *result = mChannels[channelNum]->GetSamples();
    WDL_TypedFastQueue<BL_FLOAT> *result = mChannels[channelNum]->GetSamples();
    
    return result;
}

WDL_TypedBuf<WDL_FFT_COMPLEX> *
FftProcessObj16::GetChannelFft(int channelNum)
{
    if ((channelNum < 0) || (channelNum >= mChannels.size()))
        return NULL;
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> *result = mChannels[channelNum]->GetFft();
    
    return result;
}

WDL_TypedBuf<BL_FLOAT> *
FftProcessObj16::GetChannelResultSamples(int channelNum)
{
    if ((channelNum < 0) || (channelNum >= mChannels.size()))
        return NULL;
    
    WDL_TypedBuf<BL_FLOAT> *result = mChannels[channelNum]->GetResultSamples();
    
    return result;
}

void
FftProcessObj16::SetChannelEnabled(int channelNum, bool flag)
{
    if ((channelNum < 0) || (channelNum >= mChannels.size()))
        return;
    
    mChannels[channelNum]->SetEnabled(flag);
}

void
FftProcessObj16::MagnPhasesToSamples(const WDL_TypedBuf<BL_FLOAT> &fftMagns,
                                     const  WDL_TypedBuf<BL_FLOAT> &fftPhases,
                                     WDL_TypedBuf<BL_FLOAT> *outSamples)
{
    outSamples->Resize(fftMagns.GetSize());
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> fftBuffer;
    BLUtils::MagnPhaseToComplex(&fftBuffer, fftMagns, fftPhases);
    
    int freqRes = 1;
    ComputeInverseFft(fftBuffer, outSamples, freqRes);
}

void
FftProcessObj16::HalfMagnPhasesToSamples(const WDL_TypedBuf<BL_FLOAT> &fftMagns,
                                         const  WDL_TypedBuf<BL_FLOAT> &fftPhases,
                                         WDL_TypedBuf<BL_FLOAT> *outSamples)
{
    outSamples->Resize(fftMagns.GetSize()*2);
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> fftBuffer;
    BLUtils::MagnPhaseToComplex(&fftBuffer, fftMagns, fftPhases);
    
    fftBuffer.Resize(fftBuffer.GetSize()*2);
    BLUtils::FillSecondFftHalf(&fftBuffer);
    
    int freqRes = 1;
    ComputeInverseFft(fftBuffer, outSamples, freqRes);
}

void
FftProcessObj16::AddSamples(const vector<WDL_TypedBuf<BL_FLOAT> > &inputs,
                            const vector<WDL_TypedBuf<BL_FLOAT> > &scInputs)
{
    // Pre-process samples
    
    // Make a copy of the input
    // because it may be modified before adding it
    //vector<WDL_TypedBuf<BL_FLOAT> > samplesIn = inputs;
    vector<WDL_TypedBuf<BL_FLOAT> > &samplesIn = mTmpBuf1;
    samplesIn = inputs;
    
    // Convert to the right format for Mc
    /*vector<WDL_TypedBuf<BL_FLOAT> *> samplesInMc;
      for (int i = 0; i < samplesIn.size(); i++)
      {
      samplesInMc.push_back(&samplesIn[i]);
      }*/
    vector<WDL_TypedBuf<BL_FLOAT> *> &samplesInMc = mTmpBuf5;
    samplesInMc.resize(samplesIn.size());
    for (int i = 0; i < samplesIn.size(); i++)
    {
        samplesInMc[i] = &samplesIn[i];
    }
    
    /*vector<WDL_TypedBuf<BL_FLOAT> > samplesInScMc;
      for (int i = 0; i < scInputs.size(); i++)
      {
      samplesInScMc.push_back(scInputs[i]);
      }*/
    vector<WDL_TypedBuf<BL_FLOAT> > &samplesInScMc = mTmpBuf2;
    samplesInScMc.resize(scInputs.size());
    for (int i = 0; i < scInputs.size(); i++)
    {
        samplesInScMc[i] = scInputs[i];
    }
      
    // Potentially modify the input before adding it
    for (int i = 0; i < mMcProcesses.size(); i++)
    {
        MultichannelProcess *mcProcess = mMcProcesses[i];
    
        vector<WDL_TypedBuf<BL_FLOAT> > *sc = NULL;
        if (!samplesInScMc.empty())
            sc = &samplesInScMc;
        
        mcProcess->ProcessInputSamplesPre(&samplesInMc, sc);
    }
    
    // NEW
    for (int i = 0; i < inputs.size(); i++)
    {
        WDL_TypedBuf<BL_FLOAT> *scInput = NULL;
        if (i < scInputs.size())
            scInput = &((vector<WDL_TypedBuf<BL_FLOAT> >)scInputs)[i];
        
        if (i < mChannels.size()) //DENOISER_OPTIM10
        {
            ProcessObj *obj = mChannels[i]->GetProcessObject();
        
            if (obj != NULL)
                obj->ProcessInputSamplesPre(&samplesIn[i], scInput);
        }
    }
    
    // Add the new input data
    for (int i = 0; i < inputs.size(); i++)
    {
        if (i < mChannels.size()) //DENOISER_OPTIM10
            mChannels[i]->AddSamples(samplesIn[i]);
    }
    
    for (int i = 0; i < scInputs.size(); i++)
    {
        int channelNum = mChannels.size() - mNumScInputs + i;
        
        if (channelNum < mChannels.size()) //DENOISER_OPTIM10
            mChannels[channelNum]->AddSamples(scInputs[i]);
    }
}

void
FftProcessObj16::AddSamples(int startChan,
                            const vector<WDL_TypedBuf<BL_FLOAT> > &inputs)
{
    int numInputs = inputs.size();
    
    // Add the new input data
    for (int i = 0; i < numInputs; i++)
    {
        if (i + startChan < mChannels.size())
            mChannels[i + startChan]->AddSamples(inputs[i]);
    }
}

void
FftProcessObj16::ProcessAllFftSteps()
{
    for (int i = mChannels.size() - 1; i >= 0 ; i--)
    {
        ProcessObjChannel *chan = mChannels[i];
        chan->ProcessFftStep();
    }
}

void
FftProcessObj16::ProcessFftStepChannel(int channelNum)
{
    if (channelNum >= mChannels.size())
        return;
    
    ProcessObjChannel *chan = mChannels[channelNum];
    chan->ProcessFftStep();
}

void
FftProcessObj16::ProcessSamples()
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
        
        // Iterate over all channels and call ProcessFftStep() for each channel
        // Put is in a method, so it can be overloaded
        ProcessAllFftSteps();
        
        //for (int i = mChannels.size() - 1; i >= 0 ; i--)
        //{
        //    ProcessObjChannel *chan = mChannels[i];
        //    chan->ProcessFftStep();
        //}
        
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
FftProcessObj16::GetResults(vector<WDL_TypedBuf<BL_FLOAT> > *outputs,
                           int numRequested)
{
    int numInputChannels = (int)mChannels.size() - mNumScInputs;
    
    bool resultReady = true;
    for (int i = 0; i < numInputChannels; i++)
    {
#if FIX_NULL_OUT_MEMLEAK
        if (outputs == NULL)
        {
            bool res = mChannels[i]->GetResult(NULL, numRequested);
            
            // Do not take into account an empty channels
            // when checking if the data is available
            // (this would be false every time)
            if (mChannels[i]->IsEmpty())
                res = true;
            
            resultReady = resultReady && res;
            
            continue;
        }
#endif
        
        if (i < outputs->size())
        {
            WDL_TypedBuf<BL_FLOAT> &out = (*outputs)[i];
            bool res = mChannels[i]->GetResult(&out, numRequested);
        
            // Do not take into account an empty channels
            // when checking if the data is available
            // (this would be false every time)
            if (mChannels[i]->IsEmpty())
                res = true;
            
            resultReady = resultReady && res;
        }
    }
    
    return resultReady;
}

void
FftProcessObj16::SetupSideChain()
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
FftProcessObj16::Process(const vector<WDL_TypedBuf<BL_FLOAT> > &inputs,
                         const vector<WDL_TypedBuf<BL_FLOAT> > &scInputs,
                         vector<WDL_TypedBuf<BL_FLOAT> > *outputs)
{
    //vector<WDL_TypedBuf<BL_FLOAT> > inputs0 = inputs;
    vector<WDL_TypedBuf<BL_FLOAT> > &inputs0 = mTmpBuf0;
    if (inputs.size() > 0)
        inputs0 = inputs;
    else
        //if (inputs0.size() == 0)
    {
        // Create dummy input samples
        inputs0 = *outputs;
        for (int i = 0; i < inputs0.size(); i++)
        {
            WDL_TypedBuf<BL_FLOAT> &input = inputs0[i];
            BLUtils::FillAllZero(&input);
        }
    }
    
#if FIX_TOO_MANY_INPUT_CHANNELS
    int numInputChannels = (int)mChannels.size() - mNumScInputs;
    if (inputs0.size() > numInputChannels)
        inputs0.resize(numInputChannels);
#endif
    
    AddSamples(inputs0, scInputs);
    
    ProcessSamples();
    
    bool res = true;
#if !FIX_NULL_OUT_MEMLEAK
    if (outputs != NULL)
#endif
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
                BLUtils::FillAllZero(&(*outputs)[i]);
            }
        }
    }
    
    return res;
}

void
FftProcessObj16::MakeWindows(int bufSize, int overlapping,
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
//FftProcessObj16::GetAllSamples(vector<WDL_TypedBuf<BL_FLOAT> * > *samples)
FftProcessObj16::GetAllSamples(vector<WDL_TypedFastQueue<BL_FLOAT> * > *samples)
{
    samples->resize(mChannels.size() - mNumScInputs);
    
    for (int i = 0; i < mChannels.size() - mNumScInputs; i++) // NEW
    {
        ProcessObjChannel *chan = mChannels[i];
    
        //WDL_TypedBuf<BL_FLOAT> *s = chan->GetSamples();
        WDL_TypedFastQueue<BL_FLOAT> *s = chan->GetSamples();
    
        //samples->push_back(s);
        (*samples)[i] = s;
    }
}

void
//FftProcessObj16::GetAllScSamples(vector<WDL_TypedBuf<BL_FLOAT> > *scSamples)
FftProcessObj16::GetAllScSamples(vector<WDL_TypedBuf<BL_FLOAT> > *scSamples)
{
    int numScChan = 0;
    for (int i = 0; i < mChannels.size(); i++)
    {
        ProcessObjChannel *chan = mChannels[i];
        ProcessObjChannel *scChan = chan->GetSideChainChannel();
        if (scChan != NULL)
            numScChan++;
    }

    scSamples->resize(numScChan);
    
    for (int i = 0; i < mChannels.size(); i++)
    {
        ProcessObjChannel *chan = mChannels[i];
        ProcessObjChannel *scChan = chan->GetSideChainChannel();
        if (scChan != NULL)
        {
            //WDL_TypedBuf<BL_FLOAT> *s = scChan->GetSamples();
            WDL_TypedFastQueue<BL_FLOAT> *s = scChan->GetSamples();
        
            //scSamples->push_back(*s);
            (*scSamples)[i].Resize(s->Available());
            s->GetToBuf(0, (*scSamples)[i].Get(), s->Available());
        }
    }
}

void
FftProcessObj16::GetAllFftSamples(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> * > *samples)
{
    samples->resize(mChannels.size() - mNumScInputs);
    
    for (int i = 0; i < mChannels.size() - mNumScInputs; i++) // NEW
    {
        ProcessObjChannel *chan = mChannels[i];
        
        WDL_TypedBuf<WDL_FFT_COMPLEX> *s = chan->GetFft();
        
        //samples->push_back(s);
        (*samples)[i] = s;
    }
}

void
FftProcessObj16::GetAllFftScSamples(vector<WDL_TypedBuf<WDL_FFT_COMPLEX> > *scSamples)
{
    int numScChan = 0;
    for (int i = 0; i < mChannels.size(); i++)
    {
        ProcessObjChannel *chan = mChannels[i];
        ProcessObjChannel *scChan = chan->GetSideChainChannel();
        if (scChan != NULL)
            numScChan++;
    }

    scSamples->resize(numScChan);

    int scIdx = 0;
    for (int i = 0; i < mChannels.size(); i++)
    {
        ProcessObjChannel *chan = mChannels[i];
        ProcessObjChannel *scChan = chan->GetSideChainChannel();
        if (scChan != NULL)
        {
            WDL_TypedBuf<WDL_FFT_COMPLEX> *s = scChan->GetFft();
        
            //scSamples->push_back(*s);
            (*scSamples)[scIdx++] = *s;
        }
    }
}

void
FftProcessObj16::GetAllResultSamples(vector<WDL_TypedBuf<BL_FLOAT> * > *samples)
{
    samples->resize(mChannels.size() - mNumScInputs);
    
    for (int i = 0; i < mChannels.size() - mNumScInputs; i++) // NEW
    {
        ProcessObjChannel *chan = mChannels[i];
        
        WDL_TypedBuf<BL_FLOAT> *s = chan->GetResultSamples();
        
        //samples->push_back(s);
        (*samples)[i] = s;
    }
}

void
FftProcessObj16::GetAllResultScSamples(vector<WDL_TypedBuf<BL_FLOAT> > *scSamples)
{
    int numScChan = 0;
    for (int i = 0; i < mChannels.size(); i++)
    {
        ProcessObjChannel *chan = mChannels[i];
        
        ProcessObjChannel *scChan = chan->GetSideChainChannel();
        if (scChan != NULL)
            numScChan++;
    }

    scSamples->resize(numScChan);

    int scIdx = 0;
    for (int i = 0; i < mChannels.size(); i++)
    {
        ProcessObjChannel *chan = mChannels[i];
        
        ProcessObjChannel *scChan = chan->GetSideChainChannel();
        if (scChan != NULL)
        {
            WDL_TypedBuf<BL_FLOAT> *s = scChan->GetResultSamples();
            
            //scSamples->push_back(*s);
            (*scSamples)[scIdx++] = *s;
        }
    }
}

void
FftProcessObj16::ComputeFft(const WDL_TypedBuf<BL_FLOAT> &samples,
                            WDL_TypedBuf<WDL_FFT_COMPLEX> *fftSamples,
                            int freqRes,
                            WDL_TypedBuf<WDL_FFT_COMPLEX> *tmpBuf)
{
    int bufSize = samples.GetSize();

    WDL_TypedBuf<WDL_FFT_COMPLEX> tmpBuf0;
    WDL_TypedBuf<WDL_FFT_COMPLEX> &tmpFftBuf = (tmpBuf != NULL) ? *tmpBuf : tmpBuf0;
    tmpFftBuf.Resize(bufSize);

    fftSamples->Resize(bufSize);
    
    // FIX: for FLStudio graphic freeze on AutoGain on Windows
    // This point really fixes the bug !
    //
    // (certainly that the data sent for fft was all zero,
    // and WDL_FFT has strange results when all the input samples are null
    if (BLUtils::IsAllZero(samples))
    {
          BLUtils::FillAllZero(fftSamples);
	
          return;
    }

    BL_FLOAT coeff = 1.0;
#if NORMALIZE_FFT
    if (bufSize > 0)
      // Test just in case (added for AutoGain FLStudio Win graphic freeze)
      coeff = 1.0/bufSize;
#endif

    WDL_FFT_COMPLEX *tmpFftBufData = tmpFftBuf.Get();
    BL_FLOAT *samplesData = samples.Get();
    
    // Fill the fft buf
    for (int i = 0; i < bufSize; i++)
    {
        // No need to divide by mBufSize if we ponderate analysis hanning window
        //tmpFftBuf.Get()[i].re = samples.Get()[i]*coeff;
        //tmpFftBuf.Get()[i].im = 0.0;
        tmpFftBufData[i].re = samplesData[i]*coeff;
        tmpFftBufData[i].im = 0.0;
    }
    
    // Take FREQ_RES into account
    // Not sure we must do that only when normalizing or not
    for (int i = 0; i < bufSize; i++)
        //tmpFftBuf.Get()[i].re *= freqRes;
        tmpFftBufData[i].re *= freqRes;
    
    // Do the fft
    // Do it on the window but also in following the empty space, to capture remaining waves
    WDL_fft(tmpFftBuf.Get(), bufSize, false);
    
    // Sort the fft buffer
    WDL_FFT_COMPLEX *fftSamplesData = fftSamples->Get();
    for (int i = 0; i < bufSize; i++)
    {
        int k = WDL_fft_permute(bufSize, i);

#if !DENOISER_OPTIM3
        //fftSamples->Get()[i].re = tmpFftBuf.Get()[k].re;
        //fftSamples->Get()[i].im = tmpFftBuf.Get()[k].im;
        fftSamplesData()[i].re = tmpFftBufData[k].re;
        fftSamplesData()[i].im = tmpFftBufData[k].im;
#else
        //fftSamples->Get()[i] = tmpFftBuf.Get()[k];
        fftSamplesData[i] = tmpFftBufData[k];
#endif
    }
}

void
FftProcessObj16::ComputeInverseFft(const WDL_TypedBuf<WDL_FFT_COMPLEX> &fftSamples,
                                   WDL_TypedBuf<BL_FLOAT> *samples,
                                   int freqRes,
                                   WDL_TypedBuf<WDL_FFT_COMPLEX> *tmpBuf)
{
    int bufSize = fftSamples.GetSize();

    WDL_TypedBuf<WDL_FFT_COMPLEX> tmpBuf0;
    WDL_TypedBuf<WDL_FFT_COMPLEX> &tmpFftBuf = (tmpBuf != NULL) ? *tmpBuf : tmpBuf0;
    tmpFftBuf.Resize(bufSize);

    samples->Resize(fftSamples.GetSize());
    
	// FIX crash with StudioOne 4, Windows 7 + debug with ApplicationVerifier
	// Because WDL fft is not designed to process zero data
	if (BLUtils::IsAllZeroComp(fftSamples))
	{
		BLUtils::FillAllZero(samples);

		return;
	}

    WDL_FFT_COMPLEX *tmpFftBufData = tmpFftBuf.Get();
    WDL_FFT_COMPLEX *fftSamplesData = fftSamples.Get();
    for (int i = 0; i < bufSize; i++)
    {
        int k = WDL_fft_permute(bufSize, i);
        
#if !DENOISER_OPTIM3
        //tmpFftBuf.Get()[k].re = fftSamples.Get()[i].re;
        //tmpFftBuf.Get()[k].im = fftSamples.Get()[i].im;
        tmpFftBufData[k].re = fftSamplesData[i].re;
        tmpFftBufData[k].im = fftSamplesData[i].im;
#else
        //tmpFftBuf.Get()[k] = fftSamples.Get()[i];
        tmpFftBufData[k] = fftSamplesData[i];
#endif
    }
    
    // Should not do this step when not necessary (for example for transients)
    
    // Do the ifft
    WDL_fft(tmpFftBuf.Get(), bufSize, true);

    BL_FLOAT *samplesData = samples->Get();
    for (int i = 0; i < bufSize; i++)
        //samples->Get()[i] = tmpFftBuf.Get()[i].re;
        samplesData[i] = tmpFftBufData[i].re;
    
    // fft
    BL_FLOAT coeff = 1.0;
#if !NORMALIZE_FFT
    // Not sure about that...
    //coeff = bufSize;
#endif
    
    // Take FREQ_RES into account
    // Not sure we must do that only when normalizing or not
    coeff *= 1.0/freqRes;
    
#if !OPTIM_SIMD
    for (int i = 0; i < bufSize; i++)
        //samples->Get()[i] *= coeff;
        samplesData[i] *= coeff;
#else
    BLUtils::MultValues(samples, coeff);
#endif
}

void
FftProcessObj16::ComputeInverseFft(const WDL_TypedBuf<WDL_FFT_COMPLEX> &fftSamples,
                                   WDL_TypedBuf<WDL_FFT_COMPLEX> *iFftSamples,
                                   WDL_TypedBuf<WDL_FFT_COMPLEX> *tmpBuf)
{
    int bufSize = fftSamples.GetSize();

    WDL_TypedBuf<WDL_FFT_COMPLEX> tmpBuf0;
    WDL_TypedBuf<WDL_FFT_COMPLEX> &tmpFftBuf = (tmpBuf != NULL) ? *tmpBuf : tmpBuf0;
    tmpFftBuf.Resize(bufSize);
    
    iFftSamples->Resize(fftSamples.GetSize());
    
    for (int i = 0; i < bufSize; i++)
    {
        int k = WDL_fft_permute(bufSize, i);
        
#if !DENOISER_OPTIM3
        tmpFftBuf.Get()[k].re = fftSamples.Get()[i].re;
        tmpFftBuf.Get()[k].im = fftSamples.Get()[i].im;
#else
        tmpFftBuf.Get()[k] = fftSamples.Get()[i];
#endif
    }
    
    // Should not do this step when not necessary (for example for transients)
    
    // Do the ifft
    WDL_fft(tmpFftBuf.Get(), bufSize, true);
    
#if !OPTIM_SIMD
    for (int i = 0; i < bufSize; i++)
    {
        iFftSamples->Get()[i] = tmpFftBuf.Get()[i];
    }
#else
    memcpy(iFftSamples->Get(), tmpFftBuf.Get(), bufSize*2*sizeof(BL_FLOAT));
#endif
}

void
FftProcessObj16::ComputeInverseFft(const WDL_TypedBuf<BL_FLOAT> &fftSamplesReal,
                                   WDL_TypedBuf<BL_FLOAT> *ifftSamplesReal,
                                   bool normalize,
                                   WDL_TypedBuf<WDL_FFT_COMPLEX> *tmpBuffer)
{
    WDL_TypedBuf<WDL_FFT_COMPLEX> tmpBuf0;
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> &fftSamples =
    (tmpBuffer != NULL) ? *tmpBuffer : tmpBuf0;
    
    fftSamples.Resize(fftSamplesReal.GetSize());
    for (int i = 0; i < fftSamplesReal.GetSize(); i++)
    {
        BL_FLOAT val = fftSamplesReal.Get()[i];
        
        WDL_FFT_COMPLEX valCpx;
        valCpx.re = val;
        valCpx.im = 0.0;
     
        int k = WDL_fft_permute(fftSamplesReal.GetSize(), i);
        
        fftSamples.Get()[k] = valCpx;
    }
    
    WDL_fft(fftSamples.Get(), fftSamples.GetSize(), true);
    
    ifftSamplesReal->Resize(fftSamples.GetSize());
    for (int i = 0; i < fftSamples.GetSize(); i++)
    {
        WDL_FFT_COMPLEX valCpx = fftSamples.Get()[i];
        
        ifftSamplesReal->Get()[i] = valCpx.re;
    }
    
    if (normalize && (ifftSamplesReal->GetSize() > 0))
    {
        BL_FLOAT coeff = 1.0/ifftSamplesReal->GetSize();
        
        BLUtils::MultValues(ifftSamplesReal, coeff);
    }
}
