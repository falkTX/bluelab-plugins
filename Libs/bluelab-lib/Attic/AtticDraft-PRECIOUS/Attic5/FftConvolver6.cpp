//
//  FftConvolver6.cpp
//  Spatializer
//
//  Created by Pan on 20/11/17.
//
//

#include "Resampler2.h"
#include <Window.h>
#include <Utils.h>
#include <Debug.h>

#include "FftConvolver6.h"


// On Impulses, doesn't change anything !
// (and most costly when activated)
//
// On Spatializer, avoid aliasing
//
// Warning: this may override the maximum size of fft !
//

// PAD_FACTOR == 2: Non-cyclic technique, to avoid aliasing
// and to remove the need of overlap
#define PAD_FACTOR 2

// [NOT USED]
// Not working, not finished
// Use windowing to avoid ringing artifacts ("echo ghost")
#define USE_WINDOWING 0
#define USE_WINDOWING_FFT 0

FftConvolver6::FftConvolver6(int bufferSize, bool normalize,
                             bool usePadFactor, bool normalizeResponses)
{
    mBufferSize = bufferSize;
    
    mNormalize = normalize;
    mUsePadFactor = usePadFactor;
    mNormalizeResponses = normalizeResponses;

#if !FIX_LATENCY
    mShift = mBufferSize;
    mNumResultReady = 0;
#endif

#if FIX_LATENCY
    // Latency fix
    mBufWasFull = false;
    mTotalInSamples = 0;
#endif
    
    Reset();
}

FftConvolver6::~FftConvolver6() {}

void
FftConvolver6::Reset()
{
    mSamplesBuf.Resize(0);
    mResultBuf.Resize(0);
    
    //
    mPadFftResponse.Resize(0);
    mPadSampleResponse.Resize(0);;
    
    mInit = true;
    
#if !FIX_LATENCY
    mOffsetResult = 0;
    mNumResultReady = 0;
#endif

#if FIX_LATENCY
    // Latency fix
    mBufWasFull = false;
    mTotalInSamples = 0;
    
    mResultOut.Resize(0);
#endif
}

void
FftConvolver6::Flush()
{
    mSamplesBuf.Resize(0);
    mResultBuf.Resize(0);
    
    //Utils::FillAllZero(&mSamplesBuf);
    //Utils::FillAllZero(&mResultBuf);
    
#if !FIX_LATENCY
    mOffsetResult = 0;
    mNumResultReady = 0;
#endif
    
    mInit = true;
}

void
FftConvolver6::SetResponse(const WDL_TypedBuf<double> *response)
{    
    if (response->GetSize() == 0)
        return;
    
    WDL_TypedBuf<double> copyResp = *response;
    
    ResizeImpulse(&copyResp);
    
    // As done in image analysis with normalized kernels
    //NormalizeResponse(&copyResp);
    
    mPadSampleResponse = copyResp;
    
    if (mUsePadFactor)
        // Pad the response
        Utils::ResizeFillZeros(&mPadSampleResponse, mPadSampleResponse.GetSize()*PAD_FACTOR);
    
    WDL_TypedBuf<double> respForFft = mPadSampleResponse;
    
#if USE_WINDOWING
    if (mWindow.GetSize() != respForFft.GetSize())
    {
        double hanningFactor = 1.0;
        Window::MakeHanningPow(respForFft.GetSize(), hanningFactor, &mWindow);
    }
    
    Window::Apply(mWindow, &respForFft);
#endif

    // Compute fft
    ComputeFft(&respForFft, &mPadFftResponse, false);
    
#if USE_WINDOWING_FFT
    if (mWindow.GetSize() != mPadFftResponse.GetSize())
    {
        double hanningFactor = 1.0;
        Window::MakeHanningPow(mPadFftResponse.GetSize(), hanningFactor, &mWindow);
    }
#endif
    
    if (mNormalizeResponses)
        NormalizeResponseFft(&mPadFftResponse);
    
    // Without that, the volume increases again after 5s with resp = 10s
#if 1
    // Reset the result buffer
    Utils::FillAllZero(&mResultBuf);
#endif
}

void
FftConvolver6::GetResponse(WDL_TypedBuf<double> *response)
{
    *response = mPadSampleResponse;
    
if (mUsePadFactor)
    // "unpad"
    response->Resize(response->GetSize()/PAD_FACTOR);
}

#if !FIX_LATENCY 
// Original method
void
FftConvolver6::ProcessOneBuffer(const WDL_TypedBuf<double> &samplesBuf,
                                const WDL_TypedBuf<double> *ioResultBuf,
                                int offsetSamples, int offsetResult)
{
    WDL_TypedBuf<double> padBuf;
    padBuf.Resize(mBufferSize);
    for (int i = 0; i < mBufferSize; i++)
    {
        if (i + offsetSamples >= samplesBuf.GetSize())
            break;
        
        padBuf.Get()[i] = samplesBuf.Get()[i + offsetSamples];
    }
    
    // Take the size of the response if greater than buffer size
    int newSize;
    if (mUsePadFactor)
        newSize = MAX(mBufferSize*PAD_FACTOR, mPadFftResponse.GetSize());
    else
        newSize = MAX(mBufferSize, mPadFftResponse.GetSize());
    
    Utils::ResizeFillZeros(&padBuf, newSize);
    
#define DEBUG_BUFS 0
#if DEBUG_BUFS
    static int count = 0;
    count++;
    
    if (count == 1)
        Debug::DumpData("buf0.txt", padBuf);
#endif
    
#if USE_WINDOWING
    Window::Apply(mWindow, &padBuf);
#endif
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> fftBuf;
    ComputeFft(&padBuf, &fftBuf, mNormalize);
    
#if DEBUG_BUFS
    if (count == 1)
        Debug::DumpData("resp.txt", mPadSampleResponse);
#endif
    
    // Apply modifications of the buffer
    ProcessFftBuffer(&fftBuf, mPadFftResponse);
    
    ComputeInverseFft(&fftBuf, &padBuf);
    
#if DEBUG_BUFS
    if (count == 1)
        Debug::DumpData("buf1.txt", padBuf);
#endif
    
    for (int i = 0; i < padBuf.GetSize(); i++)        
    {
        if (i + offsetResult >= ioResultBuf->GetSize())
            break;
        
        ioResultBuf->Get()[i + offsetResult] += padBuf.Get()[i];
    }
}
#endif

#if FIX_LATENCY
// Method with latency correctly fixed
void
FftConvolver6::ProcessOneBuffer(const WDL_TypedBuf<double> &samplesBuf,
                                WDL_TypedBuf<double> *ioResultBuf)
{
    WDL_TypedBuf<double> padBuf;
    padBuf.Resize(mBufferSize);
    for (int i = 0; i < mBufferSize; i++)
    {
        if (i >= samplesBuf.GetSize())
            break;
        
        padBuf.Get()[i] = samplesBuf.Get()[i];
    }
    
    // Take the size of the response if greater than buffer size
    int newSize;
    if (mUsePadFactor)
        newSize = MAX(mBufferSize*PAD_FACTOR, mPadFftResponse.GetSize());
    else
        newSize = MAX(mBufferSize, mPadFftResponse.GetSize());
    
    Utils::ResizeFillZeros(&padBuf, newSize);
    
#if USE_WINDOWING
    Window::Apply(mWindow, &padBuf);
#endif
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> fftBuf;
    ComputeFft(&padBuf, &fftBuf, mNormalize);
    
    // Apply modifications of the buffer
    ProcessFftBuffer(&fftBuf, mPadFftResponse);
    
    ComputeInverseFft(&fftBuf, &padBuf);
    
    for (int i = 0; i < padBuf.GetSize(); i++)
    {
        if (i >= ioResultBuf->GetSize())
            break;
        
        ioResultBuf->Get()[i] += padBuf.Get()[i];
    }
}
#endif

// Original method (buggy)
// BUG: When buffer size is too small (447), there are crackles in the result.
#if 0
bool
FftConvolver6::Process(double *input, double *output, int nFrames)
{    
    // Add the new samples
    mSamplesBuf.Add(input, nFrames);
    
    // Test if we have enough result sample to fill nFrames
    if (mNumResultReady >= nFrames)
    {
        if (output != NULL)
        // Copy the result
        {
            if (mResultBuf.GetSize() >= nFrames)
                memcpy(output, mResultBuf.Get(), nFrames*sizeof(double));
            // else there is an error somewhere !
        }
            
        // Consume the result already managed
        Utils::ConsumeLeft(&mResultBuf, nFrames);
        mNumResultReady -= nFrames;
            
        return true;
    }

    // Here, we have not yet enough result samples
    
    if (mSamplesBuf.GetSize() < mBufferSize)
    {
        // And here, we have not yet enough samples to compute a result
        
        // Fill with zeros
        if (output != NULL)
            memset(output, 0, nFrames*sizeof(double));
        
        return false;
    }
    
    // Fill with zero the future working area
    Utils::AddZeros(&mResultBuf, mBufferSize);

    int size = (mBufferSize > nFrames) ? mBufferSize : nFrames;
    
    // Even bigger if the response is bigger than the samples buffer
    int newSize = MAX(size, mPadFftResponse.GetSize());
    
    // Non-cyclic convolution !
    // We need more room !
    if (mUsePadFactor)
    {
        if (mResultBuf.GetSize() < newSize*PAD_FACTOR)
            Utils::AddZeros(&mResultBuf, newSize*PAD_FACTOR);
    }
    else
    {
        if (mResultBuf.GetSize() < newSize)
            Utils::AddZeros(&mResultBuf, newSize);
    }
    
    int offsetSamples = 0;
    int numProcessed = 0;
    while (offsetSamples + mBufferSize <= mSamplesBuf.GetSize())
    {
        ProcessOneBuffer(mSamplesBuf, &mResultBuf, offsetSamples, mOffsetResult);
        
        numProcessed += mShift;
        
        // Shift the offsets
        offsetSamples += mShift;
        mOffsetResult += mShift;
        
        // Stop if it remains too few samples to process
    }
    
    mNumResultReady += numProcessed;
    
    // Consume the processed samples
    Utils::ConsumeLeft(&mSamplesBuf, numProcessed);
    
    // We wait for having nFrames samples ready
    if (numProcessed < nFrames)
    {
        if (output != NULL)
            memset(output, 0, nFrames*sizeof(double));
        
        return false;
    }
    
    // If we have enough result...
    
    // Copy the result
    if (output != NULL)
    {
        if (mResultBuf.GetSize() >= nFrames)
            memcpy(output, mResultBuf.Get(), nFrames*sizeof(double));
        // else there is an error somewhere
    }
    
    // Consume the result already managed
    Utils::ConsumeLeft(&mResultBuf, nFrames);
    
    mNumResultReady -= nFrames;
    
    // Offset is in the interval [0, mBufferSize]
    mOffsetResult = mResultBuf.GetSize() % mBufferSize;
    
    return true;
}
#endif

#if !FIX_LATENCY
      // Worked, but made time shift with host buffer size 447 e.g
      // mOffsetResult is hard to understand

// More simple method (and fixed !)
// FIX: when buffer size is too small (447), there were crackles in the result.
bool
FftConvolver6::Process(double *input, double *output, int nFrames)
{
    // Add the new samples
    mSamplesBuf.Add(input, nFrames);
    
    // Fill with zero the future working area
    //Utils::AddZeros(&mResultBuf, mBufferSize);
    
    int size = (mBufferSize > nFrames) ? mBufferSize : nFrames;
    
    // Even bigger if the response is bigger than the samples buffer
    int newSize = MAX(size, mPadFftResponse.GetSize());
    
    // Non-cyclic convolution !
    // We need more room !
    if (mUsePadFactor)
    {
        if (mResultBuf.GetSize() < newSize*PAD_FACTOR)
            Utils::AddZeros(&mResultBuf, newSize*PAD_FACTOR);
    }
    else
    {
        if (mResultBuf.GetSize() < newSize)
            Utils::AddZeros(&mResultBuf, newSize);
    }
    
    int offsetSamples = 0;
    int numProcessed = 0;
    while (offsetSamples + mBufferSize <= mSamplesBuf.GetSize())
    {
        ProcessOneBuffer(mSamplesBuf, &mResultBuf, offsetSamples, mOffsetResult);
        
        numProcessed += mShift;
        
        // Shift the offsets
        offsetSamples += mShift;
        mOffsetResult += mShift;
        
        // Stop if it remains too few samples to process
    }
    
    mNumResultReady += numProcessed;
    
    // Consume the processed samples
    Utils::ConsumeLeft(&mSamplesBuf, numProcessed);
    
    // Test if we have enough result sample to fill nFrames
    if (mNumResultReady < nFrames)
        return false;
    
    // Here, we have enough redy result samples to fill the output buffer
    if (output != NULL)
    // Copy the result
    {
        if (mResultBuf.GetSize() >= nFrames)
            memcpy(output, mResultBuf.Get(), nFrames*sizeof(double));
        // else there is an error somewhere !
    }
        
    // Consume the result already managed
    Utils::ConsumeLeft(&mResultBuf, nFrames);
    mNumResultReady -= nFrames;
    
    // Offset is in the interval [0, mBufferSize]
    mOffsetResult = mResultBuf.GetSize() % mBufferSize;
    
    return true;
}
#endif

#if FIX_LATENCY
// Even more simple method
bool
FftConvolver6::Process(double *input, double *output, int nFrames)
{
    // Add the new samples
    mSamplesBuf.Add(input, nFrames);
    
    // For latency fix
    mTotalInSamples += nFrames;
    
    // Added for FftConvolver6/FIX_LATENCY
    //
    // If mTotalInSamples is exactly mBufferSize, and
    // nFrames is mBufferSize, we must skip once, to take care of
    // declared plugin latency
    //
    // (otherwise, with all at 1024, we would have the result directly,
    // not corresponding to the annouced latency of the plugin)
    //
    if (mTotalInSamples <= mBufferSize)
    {
        // For the moment, keep accumulating, to manage latency well
        if (output != NULL)
        {
            memset(output, 0, nFrames*sizeof(double));
        }
        
        return false;
    }
    
    int size = (mBufferSize > nFrames) ? mBufferSize : nFrames;
    
    // Even bigger if the response is bigger than the samples buffer
    int newSize = MAX(size, mPadFftResponse.GetSize());
    
    // Non-cyclic convolution !
    // We need more room !
    if (mUsePadFactor)
    {
        if (mResultBuf.GetSize() < newSize*PAD_FACTOR)
            Utils::AddZeros(&mResultBuf, newSize*PAD_FACTOR);
    }
    else
    {
        if (mResultBuf.GetSize() < newSize)
            Utils::AddZeros(&mResultBuf, newSize);
    }
    
    // BUG:
    // Crackles: Spatializer, block size 1024, 44100Hz, width=100%
    //while (mSamplesBuf.GetSize() >/*=*/ mBufferSize)
    
    // FIX:
    // No crackles
    while (mSamplesBuf.GetSize() > mBufferSize)
    {
        // Version for FIX_LATENCY
        ProcessOneBuffer(mSamplesBuf, &mResultBuf);
        
        mResultOut.Add(mResultBuf.Get(), mBufferSize);
        
        // Consume the processed samples
        Utils::ConsumeLeft(&mSamplesBuf, mBufferSize);
        
        Utils::ConsumeLeft(&mResultBuf, mBufferSize);
        
        // Stop if it remains too few samples to process
    }
    
    WDL_TypedBuf<double> resultBuf;
    bool res = GetResult(&resultBuf, nFrames);
    
    if (res)
    {
        memcpy(output, resultBuf.Get(), nFrames*sizeof(double));
    }
    else
        // Result is not ready, fill the output with zeros
        // (to avoid undefined numbers sent to the host)
        //
        // NOTE: added for Rebalance
        //
    {
        if (output != NULL)
        {
            Utils::FillAllZero(output, nFrames);
        }
    }
    
    return res;
}
#endif

bool
FftConvolver6::Process(const WDL_TypedBuf<double> &inMagns,
                       const WDL_TypedBuf<double> &inPhases,
                       WDL_TypedBuf<double> *resultMagns,
                       WDL_TypedBuf<double> *resultPhases)
{
    WDL_TypedBuf<WDL_FFT_COMPLEX> ioBuffer;
    Utils::MagnPhaseToComplex(&ioBuffer, inMagns, inPhases);
    
    ProcessFftBuffer2(&ioBuffer, mPadFftResponse);
    
    Utils::ComplexToMagnPhase(resultMagns, resultPhases, ioBuffer);
    
    return true;
}

#if FIX_LATENCY
// From FftProcessObj15 (exactly the same code)
bool
FftConvolver6::GetResult(WDL_TypedBuf<double> *output, int numRequested)
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
        
        // FIX: fixes auval crash
        if (mTotalInSamples < mBufferSize)
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
        WDL_TypedBuf<double> buf;
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

#if FIX_LATENCY
// From FftProcessObj15 (exactly the same code)
void
FftConvolver6::GetResultOutBuffer(WDL_TypedBuf<double> *output,
                                  int numRequested)
{
    int size = mResultOut.GetSize();
    
    // Should not happen
    if (size < numRequested)
        return;
    
    //
    output->Resize(numRequested);
    
    // Copy the result
    memcpy(output->Get(), mResultOut.Get(), numRequested*sizeof(double));
    
    // Resize down, skipping left
    Utils::ConsumeLeft(&mResultOut, numRequested);
}

#endif

void
FftConvolver6::ResampleImpulse(WDL_TypedBuf<double> *impulseResponse,
                               double sampleRate, double respSampleRate)
{
    if (impulseResponse->GetSize() == 0)
        return;
    
    if (respSampleRate != sampleRate)
        // We have to resample the impulse
    {
        if (impulseResponse->GetSize() > 0)
        {
            Resampler2 resampler(sampleRate, respSampleRate);
            
            WDL_TypedBuf<double> newImpulse;
            resampler.Resample(impulseResponse, &newImpulse);
            
            ResizeImpulse(&newImpulse);
            
            *impulseResponse = newImpulse;
        }
    }
}

void
FftConvolver6::ResampleImpulse2(WDL_TypedBuf<double> *impulseResponse,
                                double sampleRate, double respSampleRate,
                                bool resizeToNextPowerOfTwo)
{
    if (impulseResponse->GetSize() == 0)
        return;
    
    if (respSampleRate != sampleRate)
        // We have to resample the impulse
    {
        if (impulseResponse->GetSize() > 0)
        {
            Resampler2 resampler(sampleRate, respSampleRate);
            
            WDL_TypedBuf<double> newImpulse;
            resampler.Resample2(impulseResponse, &newImpulse);
            
            if (resizeToNextPowerOfTwo)
                ResizeImpulse(&newImpulse);
            
            *impulseResponse = newImpulse;
        }
    }
}

void
FftConvolver6::ResizeImpulse(WDL_TypedBuf<double> *impulseResponse)
{
    int respSize = impulseResponse->GetSize();
    int newSize = Utils::NextPowerOfTwo(respSize);
    int diff = newSize - impulseResponse->GetSize();
        
    impulseResponse->Resize(newSize);
        
    // Fill with zeros if we have grown
    for (int j = 0; j < diff; j++)
    {
        int index = newSize - j - 1;
        if (index < 0)
            continue;
        if (index > impulseResponse->GetSize())
            continue;
        
        impulseResponse->Get()[index] = 0.0;
    }
}

void
FftConvolver6::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                                const WDL_TypedBuf<WDL_FFT_COMPLEX> &response)
{
    Utils::TakeHalf(ioBuffer);
 
    for (int i = 0; i < ioBuffer->GetSize(); i++)
    {
        if (i >= ioBuffer->GetSize())
            break;
        WDL_FFT_COMPLEX sigComp = ioBuffer->Get()[i];
        
        if (i >= response.GetSize())
            break;
        WDL_FFT_COMPLEX respComp = response.Get()[i];
        
        // Pointwise multiplication of two complex
        WDL_FFT_COMPLEX res;
        res.re = sigComp.re*respComp.re - sigComp.im*respComp.im;
        res.im = sigComp.im*respComp.re + sigComp.re*respComp.im;
        
        ioBuffer->Get()[i] = res;
    }
    
    Utils::ResizeFillZeros(ioBuffer, ioBuffer->GetSize()*2);
    Utils::FillSecondFftHalf(ioBuffer);
}

// For using with already frequential signal
void
FftConvolver6::ProcessFftBuffer2(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                                 const WDL_TypedBuf<WDL_FFT_COMPLEX> &response)
{
    for (int i = 0; i < ioBuffer->GetSize(); i++)
    {
        if (i >= ioBuffer->GetSize())
            break;
        WDL_FFT_COMPLEX sigComp = ioBuffer->Get()[i];
        
        if (i >= response.GetSize())
            break;
        WDL_FFT_COMPLEX respComp = response.Get()[i];
        
        // Pointwise multiplication of two complex
        WDL_FFT_COMPLEX res;
        res.re = sigComp.re*respComp.re - sigComp.im*respComp.im;
        res.im = sigComp.im*respComp.re + sigComp.re*respComp.im;
        
        ioBuffer->Get()[i] = res;
    }
}

void
FftConvolver6::ComputeFft(const WDL_TypedBuf<double> *samples,
                          WDL_TypedBuf<WDL_FFT_COMPLEX> *fftSamples,
                          bool normalize)
{
    if (fftSamples->GetSize() != samples->GetSize())
        fftSamples->Resize(samples->GetSize());
    
    double normCoeff = 1.0;
    if (normalize)
        normCoeff = 1.0/fftSamples->GetSize();
    
    // Fill the fft buf
    for (int i = 0; i < fftSamples->GetSize(); i++)
    {
        // No need to divide by mBufferSize if we ponderate analysis hanning window
        fftSamples->Get()[i].re = normCoeff*samples->Get()[i];
        fftSamples->Get()[i].im = 0.0;
    }
    
    // Do the fft
    // Do it on the window but also in following the empty space, to capture remaining waves
    WDL_fft(fftSamples->Get(), fftSamples->GetSize(), false);
    
    // Sort the fft buffer
    WDL_TypedBuf<WDL_FFT_COMPLEX> tmpFftBuf = *fftSamples;
    for (int i = 0; i < fftSamples->GetSize(); i++)
    {
        int k = WDL_fft_permute(fftSamples->GetSize(), i);
        
        // Error somewhere...
        if ((k < 0) || (k >= tmpFftBuf.GetSize()))
            break;
        
        fftSamples->Get()[i].re = tmpFftBuf.Get()[k].re;
        fftSamples->Get()[i].im = tmpFftBuf.Get()[k].im;
    }
}

void
FftConvolver6::ComputeInverseFft(const WDL_TypedBuf<WDL_FFT_COMPLEX> *fftSamples,
                                 WDL_TypedBuf<double> *samples)
{
    WDL_TypedBuf<WDL_FFT_COMPLEX> tmpFftBuf = *fftSamples;
    for (int i = 0; i < fftSamples->GetSize(); i++)
    {
        int k = WDL_fft_permute(fftSamples->GetSize(), i);
        
        // Error somewhere...
        if ((k < 0) || (k >= tmpFftBuf.GetSize()))
            break;
        
        fftSamples->Get()[k].re = tmpFftBuf.Get()[i].re;
        fftSamples->Get()[k].im = tmpFftBuf.Get()[i].im;
    }
    
    // Should not do this step when not necessary (for example for transients)
    
    // Do the ifft
    WDL_fft(fftSamples->Get(), fftSamples->GetSize(), true);
    
    for (int i = 0; i < fftSamples->GetSize(); i++)
        samples->Get()[i] = fftSamples->Get()[i].re;
}

void
FftConvolver6::NormalizeResponseFft(WDL_TypedBuf<WDL_FFT_COMPLEX> *fftSamples)
{
    // As done in image analysis with normalized kernels
    // Multiply by a coefficient so the sum becomes one
    
    // We can normalize the response as samples or as fft, this is the same
    // as fft is conservative for multplication by a factor
    
    WDL_TypedBuf<double> magns;
    WDL_TypedBuf<double> phases;
    Utils::ComplexToMagnPhase(&magns, &phases, *fftSamples);
    
    // Take sum(samples) and not sum(abs(samples)) !
    double sum = Utils::ComputeSum(magns);
    
    int fftSize = fftSamples->GetSize();
    
    double coeff = 0.0;
    if (sum > 0.0)
        coeff = 0.125*fftSize/sum;
    
    Utils::MultValues(&magns, coeff);
    
    Utils::MagnPhaseToComplex(fftSamples, magns, phases);
}

