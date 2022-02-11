//
//  FftProcessObj14.cpp
//  Transient
//
//  Created by Apple m'a Tuer on 21/05/17.
//
//

#include <Window.h>
#include <Utils.h>
#include <Debug.h>

#include "FftProcessObj14.h"

#define NORMALIZE_FFT 1

// WARNING: this is buggy and false, need some fix and reconsidered if want to use it
// When using freq res > 1, add the tail of the fft to the future samples
// (it seems to add aliasing when activated)
#define ADD_TAIL 0


class ProcessObjChannel
{
public:
    ProcessObjChannel(ProcessObj *processObj, int bufferSize);
    
    virtual ~ProcessObjChannel();
    
    virtual void Reset(int overlapping, int freqRes);
    
    
    // Setters
    void SetAnalysisWindow(FftProcessObj14::WindowType type);
    
    void SetSynthesisWindow(FftProcessObj14::WindowType type);
    
    void SetKeepSynthesisEnergy(bool flag);
    
    void SetSkipFft(bool flag);
    
    
    // Processing mechanism
    void AddSamples(const WDL_TypedBuf<double> &samples); //,
 //                   const WDL_TypedBuf<double> &scSamples);
    
    bool HasSamplesToProcess();
    
    void BufferProcessed();
    
    // Steps
    void PrepareBufferStep();
    
    void MakeFftStep();
    
    void ProcessFftStep();
    
    void MakeIFftStep();
    
    void MakeResultSamplesStep();
    
    void CommitResultStep();
    
    // Update the position
    bool Shift();
    
    bool GetResult(WDL_TypedBuf<double> *output, int numRequested);
    
protected:
    void ProcessOneBuffer(const WDL_TypedBuf<double> &inBuffer,
                          const WDL_TypedBuf<double> &inScBuffer,
                          WDL_TypedBuf<double> *outBuffer);
    
    
    // Shift to next buffer.
    void NextOutBuffer();
    
    // Shift to next buffer.
    void NextSamplesBuffer(WDL_TypedBuf<double> *samples);
    
    // Get a buffer and consume it from the result samples
    void GetResultOutBuffer(WDL_TypedBuf<double> *output, int numRequested);
    
    
    void ApplyAnalysisWindow(WDL_TypedBuf<double> *samples);
    
    void ApplySynthesisWindow(WDL_TypedBuf<double> *samples);
    
    //
    ProcessObj *mProcessObj;
    
    int mBufferSize;
    int mOverlapping;
    
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
    enum FftProcessObj14::WindowType mAnalysisMethod;
    enum FftProcessObj14::WindowType mSynthesisMethod;
    
    WDL_TypedBuf<double> mAnalysisWindow;
    WDL_TypedBuf<double> mSynthesisWindow;
    
    
    bool mKeepSynthesisEnergy;
    bool mSkipFFT;
    
    int mShift;
    int mBufOffset;
    
    // Input samples (bufferized)
    WDL_TypedBuf<double> mSamplesIn;
    
    // Current result
    WDL_TypedBuf<double> mResult;
    
    // Result with overlap add
    WDL_TypedBuf<double> mResultSum;
    
    // REsult output that is totally ready
    WDL_TypedBuf<double> mResultOut;
    
    
    //WDL_TypedBuf<double> mScChunk;
    
    
    WDL_TypedBuf<double> mPreparedBuffer;
    WDL_TypedBuf<WDL_FFT_COMPLEX> mFftBuf;
    double mEnergy0;
    
    // TODO: erase that !
    
    // Potentially sidechain
    //WDL_TypedBuf<WDL_FFT_COMPLEX> mFftBufSc;
    
    
    // Used to pad the sampels with zero on the left,
    // for correct overlapping of the beginning
    bool mMustPadSamples;
    bool mMustUnpadResult;
};

ProcessObjChannel::ProcessObjChannel(ProcessObj *processObj, int bufferSize)
{
    mProcessObj = processObj;
    
    mBufferSize = bufferSize;
    mOverlapping = 1;
    mFreqRes = 1;
    
    mAnalysisMethod = FftProcessObj14::WindowRectangular;
    mSynthesisMethod = FftProcessObj14::WindowRectangular;
    
    mKeepSynthesisEnergy = false;
    
    mSkipFFT = false;
    
    mMustPadSamples = true;
    mMustUnpadResult = false;
}

ProcessObjChannel::~ProcessObjChannel() {}

void
ProcessObjChannel::Reset(int overlapping, int freqRes)
{
    if (overlapping > 0)
        mOverlapping = overlapping;
    
    if (freqRes > 0)
        mFreqRes = freqRes;
    
    mOverlapping = 1;
    
    mShift = mBufferSize/mOverlapping;
    
    FftProcessObj14::MakeWindows(mBufferSize, mOverlapping,
                                 mAnalysisMethod, mSynthesisMethod,
                                 &mAnalysisWindow, &mSynthesisWindow);
    
    // Set the buffers
    mSamplesIn.Resize(0);
    
    Utils::ResizeFillZeros(&mResult, mBufferSize);
    
    int resultSize = 2*mBufferSize*mFreqRes;
    Utils::ResizeFillZeros(&mResultSum, resultSize);
    
    mResultOut.Resize(0);
    
    int bufSize = mBufferSize*mFreqRes;
    Utils::ResizeFillZeros(&mFftBuf, bufSize);
    
    //Utils::ResizeFillZeros(&mFftBufSc, bufSize);
    
    //mScChunk.Resize(0);
    
    mMustPadSamples = true;
    mMustUnpadResult = false;
    
    mBufOffset = 0;
}

void
ProcessObjChannel::SetAnalysisWindow(FftProcessObj14::WindowType type)
{
    mAnalysisMethod = type;
    
    FftProcessObj14::MakeWindows(mBufferSize, mOverlapping,
                                 mAnalysisMethod, mSynthesisMethod,
                                 &mAnalysisWindow, &mSynthesisWindow);
}

void
ProcessObjChannel::SetSynthesisWindow(FftProcessObj14::WindowType type)
{
    mSynthesisMethod = type;
    
    FftProcessObj14::MakeWindows(mBufferSize, mOverlapping,
                                 mAnalysisMethod, mSynthesisMethod,
                                 &mAnalysisWindow, &mSynthesisWindow);
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
    bool result = (mSamplesIn.GetSize() >= 2*mBufferSize - mShift);
    
    return result;
}

void
ProcessObjChannel::BufferProcessed()
{
    NextSamplesBuffer(&mSamplesIn);
    //NextSamplesBuffer(&mScChunk);
    NextOutBuffer();
}

void
ProcessObjChannel::AddSamples(const WDL_TypedBuf<double> &samples)
{
    mSamplesIn.Add(samples.Get(), samples.GetSize());
    
    //if (scSamples != NULL)
    //    mScChunk.Add(scSamples, numSamples);
    
    if (mMustPadSamples)
    {
        Utils::PadZerosLeft(&mSamplesIn, mAnalysisWindow.GetSize());
        
        //if (mScChunk.GetSize() > 0)
        //    Utils::PadZerosLeft(&mScChunk, mAnalysisWindow.GetSize());
        
        mMustPadSamples = false;
        mMustUnpadResult = true;
    }
}

bool
ProcessObjChannel::GetResult(WDL_TypedBuf<double> *output, int numRequested)
{
    // With that (x1.5), this was impossible to setup the latency correctly
    //if (mResultOut.GetSize() >= mAnalysisWindow.GetSize()*1.5)
    
    // This one (x2) is correct for setting the latency correctly
    if (mResultOut.GetSize() >= mAnalysisWindow.GetSize()*2)
    {
        if (mMustUnpadResult)
        {
            Utils::ConsumeLeft(&mResultOut, mAnalysisWindow.GetSize());
            
            mMustUnpadResult = false;
        }
    }
    
    // Get the computed result
    int numOutSamples = mResultOut.GetSize();
    if ((numRequested <= numOutSamples) && !mMustUnpadResult)
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

void
ProcessObjChannel::PrepareBufferStep()
{
    mPreparedBuffer.Resize(mBufferSize);
    for (int i = 0; i < mBufferSize; i++)
        mPreparedBuffer.Get()[i] = mSamplesIn.Get()[i + mBufOffset];
    
    //WDL_TypedBuf<double> copyScBuf;
    //if (mScChunk.GetSize() > 0)
    //{
    //    copyScBuf.Resize(mBufSize);
    //    for (int i = 0; i < mBufSize; i++)
    //        copyScBuf.Get()[i] = mScChunk.Get()[i + mBufOffset];
    //}
}

void
ProcessObjChannel::MakeFftStep()
{
    //if (outBuffer != NULL)
    mResult.Resize(mPreparedBuffer.GetSize());
    
    WDL_TypedBuf<double> inBuffer = mPreparedBuffer;
//    WDL_TypedBuf<double> inScBufferCopy = inScBuffer;
    
    //PreProcessSamplesBuffer(&copySamplesBuf);
    
    if (mProcessObj != NULL)
        mProcessObj->PreProcessSamplesBuffer(&inBuffer);
    
    // Apply analysis windows before resizing !
    ApplyAnalysisWindow(&inBuffer);
    
    if (mProcessObj != NULL)
        mProcessObj->PreProcessSamplesBufferWin(&inBuffer);
    
    // Compute "energy" just after analysis windowing
    // then just after synthesis windowing
    // Compute and apply a coefficient to avoid loosing energy
    mEnergy0 = 0.0;
    if (mKeepSynthesisEnergy)
        mEnergy0 = Utils::ComputeAbsAvg(inBuffer);
    
    // windowing before zero padding
    // See: http://www.bitweenie.com/listings/fft-zero-padding/
    // (last links of the page)
    if (mFreqRes > 1)
    {
        // Non-cyclic technique, to avoid aliasing
        Utils::ResizeFillZeros(&inBuffer, inBuffer.GetSize()*mFreqRes);
        
        //if (mScChunk.GetSize() > 0)
        //    Utils::ResizeFillZeros(&inScBufferCopy, inScBufferCopy.GetSize()*mFreqRes);
    }
    
    //if (mScChunk.GetSize() > 0)
    //    ApplyAnalysisWindow(&inScBufferCopy);
    
    if (mSkipFFT)
    {
        //if (outBuffer != NULL)
            // By default, do nothing with the fft
            //outBuffer->Resize(0);
        //mResult.Resize(0);
        
        // Be careful to not add the beginning of the buffer
        // (otherwise the windows will be applied twice sometimes).
        mResult.Add(inBuffer.Get(), mBufferSize);
    }
    else
    {
        // NOTE: Here, we have checked that mFreqRes didn't modify the amplitude
        // of the out samples, neither the intermediate magnitude !
        FftProcessObj14::ComputeFft(inBuffer, &mFftBuf, mFreqRes);
        
        //if (mScChunk.GetSize() > 0)
        //    // Side chain: process the side chain
        //    ComputeFft(inScBufferCopy, &mFftBufSc, mFreqRes);
    }
}


void
ProcessObjChannel::ProcessFftStep()
{
    if (!mSkipFFT)
    {
        // Apply modifications of the buffer
        //if (mScChunk.GetSize() == 0)
        //{
        if (mProcessObj != NULL)
            mProcessObj->ProcessFftBuffer(&mFftBuf);
        //}
        //else
        //{
            // It is a good idea to pass the sidechain like that
            // Sidechain is managed as samples buffers
            // So it is buffered exactly the same way (e.g in the case of overlapping)
            // ... which is good !
        //    if (mProcessObj != NULL)
        //        mProcessObj->ProcessFftBuffer(&mFftBuf, &mFftBufSc);
        //}
    }
}

void
ProcessObjChannel::MakeIFftStep()
{
    FftProcessObj14::ComputeInverseFft(mFftBuf, &mResult, mFreqRes);
    
    //if (outBuffer != NULL)
    //{
        //if (mScChunk.GetSize() == 0)
            // No side chain
       // {
            if (mProcessObj != NULL)
                mProcessObj->ProcessSamplesBuffer(&mResult, NULL);
        //}
        //else
        //{
        //    if (mProcessObj != NULL)
        //        mProcessObj->ProcessSamplesBuffer(&inBufferCopy, outBuffer);
        //}
    //}
}

void
ProcessObjChannel::MakeResultSamplesStep()
{
    //if (mNoOutput || (outBuffer == NULL))
        // We have finished
    //    return;
    
    //if (outBuffer != NULL)
    // Apply synthesis window
    ApplySynthesisWindow(&mResult);
    
    //if (outBuffer != NULL)
    //{
    if (mProcessObj != NULL)
        mProcessObj->ProcessSamplesBufferWin(&mResult);
    //}
    
    if (mKeepSynthesisEnergy)
    {
        double energy1 = Utils::ComputeAbsAvg(mResult);
        
        double energyCoeff = 1.0;
        if (energy1 > 0)
            energyCoeff = mEnergy0/energy1;
        
        // If mOverlapping is 1, thn div is 0
        if (mOverlapping > 1)
        {
            double div = log(mOverlapping)/log(2.0);
            energyCoeff /= div;
            
            Utils::MultValues(&mResult, energyCoeff);
        }
        
        if (mProcessObj != NULL)
            mProcessObj->ProcessSamplesBufferEnergy(&mResult);
    }
}

void
ProcessObjChannel::CommitResultStep()
{
#if !ADD_TAIL
    // Standard size
    int loopCount = mBufferSize;
#else
    // Size to add tail
    int loopCount = copySamplesBuf.GetSize();
#endif
    
    // Make the sum (overlap)
    for (int i = 0; i < loopCount; i++)
    {
        mResultSum.Get()[i + mBufOffset] += mResult.Get()[i];
    }
}

void
ProcessObjChannel::NextSamplesBuffer(WDL_TypedBuf<double> *samples)
{
    // Reduce the size by mOverlap
    int size = samples->GetSize();
    if (size == mBufferSize)
    {
        samples->Resize(0);
    }
    else if (size > mBufferSize)
    {
        // Resize down, skipping left
        Utils::ConsumeLeft(samples, mBufferSize);
    }
}

void
ProcessObjChannel::NextOutBuffer()
{
    // Reduce the size by mOverlap
    int minSize = mBufferSize*2*mFreqRes;
    if (mResultSum.GetSize() < minSize)
        return;
    
    WDL_TypedBuf<double> newBuffer;
    mResultOut.Add(mResultSum.Get(), mBufferSize);
    
    if (mResult.GetSize() == mBufferSize)
    {
        mResultSum.Resize(0);
    }
    else if (mResult.GetSize() > mBufferSize)
    {
        // Resize down, skipping left
        Utils::ConsumeLeft(&mResultSum, mBufferSize);
    }
    
    // Grow the output with zeros
    Utils::ResizeFillZeros(&mResultSum,
                           mResultSum.GetSize() + mBufferSize);
}

void
ProcessObjChannel::GetResultOutBuffer(WDL_TypedBuf<double> *output,
                                      int numRequested)
{
    int size = mResultOut.GetSize();
    
    // Should not happen
    if (size < numRequested)
        return;
    
    // Copy the result
    memcpy(output, mResultOut.Get(), numRequested*sizeof(double));
    
    // Resize down, skipping left
    Utils::ConsumeLeft(&mResultOut, numRequested);
}

bool
ProcessObjChannel::Shift()
{
    mBufOffset += mShift;
    
    if (mBufOffset >= mBufferSize)
    {
        mBufOffset = 0;
        
        return true;
    }
    
    return false;
}

void
ProcessObjChannel::ApplyAnalysisWindow(WDL_TypedBuf<double> *samples)
{
    // Multiply the samples by analysis window
    // The window can be identity if necessary (no analysis window, or no overlap)
    for (int i = 0; i < samples->GetSize(); i++)
    {
        double winCoeff = mAnalysisWindow.Get()[i];
        samples->Get()[i] *= winCoeff;
    }
}

void
ProcessObjChannel::ApplySynthesisWindow(WDL_TypedBuf<double> *samples)
{
    // Multiply the samples by analysis window
    // The window can be identity if necessary
    // (no synthesis window, or no overlap)
    for (int i = 0; i < mSynthesisWindow.GetSize(); i++)
    {
        double winCoeff = mSynthesisWindow.Get()[i];
        samples->Get()[i] *= winCoeff;
    }
}

void
FftProcessObj14::Init()
{
    // Init WDL FFT
    WDL_fft_init();
}

// NOTE: With freqRes == 2, and overlapping = 1 => better
// the transient signal is decreased / the other signal stays constant
// => that's what we want !
// With previous version, the other signal increased
FftProcessObj14::FftProcessObj14(ProcessObj *processObj,
                                 int numChannels, int numScInputs,
                                 int bufferSize, int overlapping, int freqRes)
{
    mNumScInputs = numScInputs;
    
    mChannels.Resize(numChannels + numScInputs);
    for (int i = 0; i < mChannels.GetSize(); i++)
    {
        ProcessObjChannel *chan = new ProcessObjChannel(processObj, bufferSize);
        chan->Reset(overlapping, freqRes);
        
        mChannels.Get()[i] = chan;
    }
    
    Reset(overlapping, freqRes);
}

FftProcessObj14::~FftProcessObj14() {}

void
FftProcessObj14::Reset(int overlapping, int freqRes)
{
    for (int i = 0; i < mChannels.GetSize(); i++)
    {
        ProcessObjChannel *chan = mChannels.Get()[i];
        chan->Reset(overlapping, freqRes);
    }
}

void
FftProcessObj14::SetAnalysisWindow(int channelNum, WindowType type)
{
    if ((channelNum != FFT_PROCESS_OBJ_ALL_CHANNELS) &&
        (channelNum < mChannels.GetSize()))
        mChannels.Get()[channelNum]->SetAnalysisWindow(type);
    
    for (int i = 0; i < mChannels.GetSize(); i++)
        mChannels.Get()[i]->SetAnalysisWindow(type);
}

void
FftProcessObj14::SetSynthesisWindow(int channelNum, WindowType type)
{
    if ((channelNum != FFT_PROCESS_OBJ_ALL_CHANNELS) &&
        (channelNum < mChannels.GetSize()))
        mChannels.Get()[channelNum]->SetSynthesisWindow(type);
    
    for (int i = 0; i < mChannels.GetSize(); i++)
        mChannels.Get()[i]->SetSynthesisWindow(type);
}

void
FftProcessObj14::SetKeepSynthesisEnergy(int channelNum, bool flag)
{
    if ((channelNum != FFT_PROCESS_OBJ_ALL_CHANNELS) &&
        (channelNum < mChannels.GetSize()))
        mChannels.Get()[channelNum]->SetKeepSynthesisEnergy(flag);
    
    for (int i = 0; i < mChannels.GetSize(); i++)
        mChannels.Get()[i]->SetKeepSynthesisEnergy(flag);
}

void
FftProcessObj14::SetSkipFft(int channelNum, bool flag)
{
    if ((channelNum != FFT_PROCESS_OBJ_ALL_CHANNELS) &&
        (channelNum < mChannels.GetSize()))
        mChannels.Get()[channelNum]->SetSkipFft(flag);
    
    for (int i = 0; i < mChannels.GetSize(); i++)
        mChannels.Get()[i]->SetSkipFft(flag);
}

void
FftProcessObj14::SamplesToMagnPhases(const WDL_TypedBuf<double> &inSamples,
                                     WDL_TypedBuf<double> *outFftMagns,
                                     WDL_TypedBuf<double> *outFftPhases)
{
    int numSamples = inSamples.GetSize();
    int numPaddedSamples = Utils::NextPowerOfTwo(numSamples);
    
    WDL_TypedBuf<double> paddedSamples = inSamples;
    Utils::ResizeFillZeros(&paddedSamples, numPaddedSamples);
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> fftSamples;
    int freqRes = 1;
    ComputeFft(paddedSamples, &fftSamples, freqRes);
    
    WDL_TypedBuf<double> magns;
    WDL_TypedBuf<double> phases;
    Utils::ComplexToMagnPhase(&magns, &phases, fftSamples);
    
    if (outFftMagns != NULL)
        *outFftMagns = magns;
    
    if (outFftPhases != NULL)
        *outFftPhases = phases;
}

void
FftProcessObj14::FftToSamples(const WDL_TypedBuf<WDL_FFT_COMPLEX> &fftBuffer,
                              WDL_TypedBuf<double> *outSamples)
{
    outSamples->Resize(fftBuffer.GetSize());
    
    int freqRes = 1;
    ComputeInverseFft(fftBuffer, outSamples, freqRes);
}

void
FftProcessObj14::SamplesToFft(const WDL_TypedBuf<double> &inSamples,
                              WDL_TypedBuf<WDL_FFT_COMPLEX> *outFftBuffer)
{
    outFftBuffer->Resize(inSamples.GetSize());
    
    int freqRes = 1;
    ComputeFft(inSamples, outFftBuffer, freqRes);
}

void
FftProcessObj14::MagnPhasesToSamples(const WDL_TypedBuf<double> &fftMagns,
                                     const  WDL_TypedBuf<double> &fftPhases,
                                     WDL_TypedBuf<double> *outSamples)
{
    outSamples->Resize(fftMagns.GetSize());
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> fftBuffer;
    Utils::MagnPhaseToComplex(&fftBuffer, fftMagns, fftPhases);
    
    int freqRes = 1;
    ComputeInverseFft(fftBuffer, outSamples, freqRes);
}

void
FftProcessObj14::AddSamples(const WDL_TypedBuf<WDL_TypedBuf<double> > &inputs,
                            const WDL_TypedBuf<WDL_TypedBuf<double> > &scInputs)
{
    // Add the new input data
    for (int i = 0; i < mChannels.GetSize() - mNumScInputs; i++)
    {
        mChannels.Get()[i]->AddSamples(inputs.Get()[i]);
    }

    for (int i = 0; i < mNumScInputs; i++)
    {
        int channelNum = mChannels.GetSize() - mNumScInputs + i;
        mChannels.Get()[channelNum]->AddSamples(scInputs.Get()[i]);
    }
}

void
FftProcessObj14::ProcessSamples()
{
    if (mChannels.GetSize() == 0)
        return;
    
    // All channels should have the same number of samples
    // and the same state
    
    ProcessObjChannel *chan0 = mChannels.Get()[0];
    while(chan0->HasSamplesToProcess())
    {
        // Compute from the available input data
        while(true)
        {
            
            // Process step by step for all channels
            // So it will be possible to make operations between channels
            for (int i = 0; i < mChannels.GetSize(); i++)
            {
                ProcessObjChannel *chan = mChannels.Get()[i];
                chan->PrepareBufferStep();
            }
            
            InputBuffersReady();
            
            for (int i = 0; i < mChannels.GetSize(); i++)
            {
                ProcessObjChannel *chan = mChannels.Get()[i];
                chan->MakeFftStep();
            }
            
            FftsReady();
            
            for (int i = 0; i < mChannels.GetSize(); i++)
            {
                ProcessObjChannel *chan = mChannels.Get()[i];
                chan->ProcessFftStep();
            }
            
            FftResultsReady();
            
            for (int i = 0; i < mChannels.GetSize(); i++)
            {
                ProcessObjChannel *chan = mChannels.Get()[i];
                chan->MakeIFftStep();
            }
            
            SamplesResultsReady();
            
            for (int i = 0; i < mChannels.GetSize(); i++)
            {
                ProcessObjChannel *chan = mChannels.Get()[i];
                chan->MakeResultSamplesStep();
            }
            
            for (int i = 0; i < mChannels.GetSize(); i++)
            {
                ProcessObjChannel *chan = mChannels.Get()[i];
                chan->CommitResultStep();
            }
                
            bool stopFlag = false;
            for (int i = 0; i < mChannels.GetSize(); i++)
            {
                ProcessObjChannel *chan = mChannels.Get()[i];
                stopFlag = chan->Shift();
            }
                
            if (stopFlag)
                break;
        }
        
        for (int i = 0; i < mChannels.GetSize(); i++)
        {
            ProcessObjChannel *chan = mChannels.Get()[i];
            chan->BufferProcessed();
        }
    }
}

void
FftProcessObj14::GetResults(WDL_TypedBuf<WDL_TypedBuf<double> > *outputs,
                           int numRequested)
{
    int numInputChannels = mChannels.GetSize() - mNumScInputs;
    
    outputs->Resize(numInputChannels);
    
    for (int i = 0; i < numInputChannels; i++)
    {
        mChannels.Get()[i]->GetResult(&outputs->Get()[i], numRequested);
    }
}

bool
FftProcessObj14::Process(const WDL_TypedBuf<WDL_TypedBuf<double> > &inputs,
                         WDL_TypedBuf<WDL_TypedBuf<double> > *outputs,
                         const WDL_TypedBuf<WDL_TypedBuf<double> >&scInputs)
{
    if (inputs.GetSize() == 0)
        return false;
    
    AddSamples(inputs, scInputs);
    
    ProcessSamples();
    
    int numRequested = inputs.Get()[0].GetSize();
    GetResults(outputs, numRequested);
}

void
FftProcessObj14::MakeWindows(int bufSize, int overlapping,
                             enum WindowType analysisMethod,
                             enum WindowType synthesisMethod,
                             WDL_TypedBuf<double> *analysisWindow,
                             WDL_TypedBuf<double> *synthesisWindow)
{
    // Analysis and synthesis Hann windows
    // With Hanning, the gain is not increased when we increase the overlapping
    
    int anaWindowSize = bufSize;
    
    int synWindowSize = bufSize;
    
#if ADD_TAIL
    synWindowSize *= mFreqRes;
#endif
    
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
    double hanningFactor = 1.0;
    bool variableHanning = ((analysisMethod == WindowVariableHanning) ||
                            (synthesisMethod == WindowVariableHanning));
    if (variableHanning && (overlapping > 1))
    {
        if ((analysisMethod == WindowHanning) ||
            (analysisMethod == WindowVariableHanning) ||
            (synthesisMethod == WindowHanning) ||
            (synthesisMethod == WindowVariableHanning))
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
    if (((analysisMethod == WindowHanning) ||
          (analysisMethod == WindowVariableHanning))
         && (overlapping > 1))
        Window::MakeHanningPow(anaWindowSize, hanningFactor, analysisWindow);
    else
        Window::MakeSquare(anaWindowSize, 1.0, analysisWindow);
    
#if DEBUG_WINDOWS
    double cola0 = Window::CheckCOLA(&mAnalysisWindow, mOverlapping);
   fprintf(stderr, "ANALYSIS - over: %d cola: %g\n", mOverlapping, cola0);
#endif
    
    // Synthesis
    if (((synthesisMethod == WindowHanning) ||
         (synthesisMethod == WindowVariableHanning)) &&
         (overlapping > 1))
        Window::MakeHanningPow(synWindowSize, hanningFactor, synthesisWindow);
    else
        Window::MakeSquare(synWindowSize, 1.0, synthesisWindow);
    
#if DEBUG_WINDOWS
    double cola1 = Window::CheckCOLA(synthesisWindow, overlapping);
    fprintf(stderr, "SYNTHESIS - over: %d cola: %g\n", overlapping, cola1);
    fprintf(stderr, "\n");
#endif
    
    // Normalize
    if (((analysisMethod == WindowHanning) ||
         (analysisMethod == WindowVariableHanning)) &&
        (synthesisMethod == WindowRectangular))
    {
        Window::NormalizeWindow(analysisWindow, overlapping);
    }
    else if((analysisMethod == WindowRectangular) &&
            ((synthesisMethod == WindowHanning) ||
             (synthesisMethod == WindowVariableHanning)))
    {
        Window::NormalizeWindow(synthesisWindow, overlapping);
    }
    else if(((analysisMethod == WindowHanning) ||
             (analysisMethod == WindowVariableHanning)) &&
            ((synthesisMethod == WindowHanning) ||
             (synthesisMethod == WindowVariableHanning)))
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
        
            WDL_TypedBuf<double> win;
            Window::MakeHanningPow(analysisWindow->GetSize(), hanningFactor*2, &win);
        
            double cola = Window::CheckCOLA(&win, overlapping);
        
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
FftProcessObj14::ComputeFft(const WDL_TypedBuf<double> &samples,
                            WDL_TypedBuf<WDL_FFT_COMPLEX> *fftSamples,
                            int freqRes)
{
    int bufSize = samples.GetSize();
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> tmpFftBuf;
    tmpFftBuf.Resize(bufSize);

    fftSamples->Resize(bufSize);
    
    double coeff = 1.0;
#if NORMALIZE_FFT
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
FftProcessObj14::ComputeInverseFft(const WDL_TypedBuf<WDL_FFT_COMPLEX> &fftSamples,
                                   WDL_TypedBuf<double> *samples,
                                   int freqRes)
{
    int bufSize = fftSamples.GetSize();
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> tmpFftBuf;
    tmpFftBuf.Resize(bufSize);

    //
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
    double coeff = 1.0;
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
