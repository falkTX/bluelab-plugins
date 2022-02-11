//
//  FftConvolver5.cpp
//  Spatializer
//
//  Created by Pan on 20/11/17.
//
//

#include "Resampler2.h"
#include <Window.h>

#include <BLUtils.h>
#include <BLUtilsMath.h>
#include <BLUtilsFft.h>
#include <BLUtilsComp.h>

#include "FftConvolver5.h"


// On Impulses, doesn't change anything !
// (and most costly when activated)
//
// Warning: this may override the maximum size of fft !
//
#define USE_PAD_FACTOR 0

#if USE_PAD_FACTOR
// PAD_FACTOR == 2: Non-cyclic technique, to avoid aliasing
// and to remove the need of overlap
#define PAD_FACTOR 2
#endif

// Use windowing to avoid ringing artifacts ("echo ghost")
#define USE_WINDOWING 0


FftConvolver5::FftConvolver5(int bufferSize, bool normalize)
{
    mBufSize = bufferSize;
    mShift = mBufSize;
    mNormalize = normalize;
    mNumResultReady = 0;
    
    Reset();
}

FftConvolver5::~FftConvolver5() {}

void
FftConvolver5::Reset()
{
    mSamplesBuf.Resize(0);
    mResultBuf.Resize(0);
    
    //
    mPadFftResponse.Resize(0);
    mPadSampleResponse.Resize(0);;
    
    mOffsetResult = 0;
    
    mInit = true;
    
    mNumResultReady = 0;
}

void
FftConvolver5::SetResponse(const WDL_TypedBuf<BL_FLOAT> *response)
{    
    if (response->GetSize() == 0)
        return;
    
    WDL_TypedBuf<BL_FLOAT> copyResp = *response;
    
    ResizeImpulse(&copyResp);
    
    // As done in image analysis with normalized kernels
    //NormalizeResponse(&copyResp);
    
    mPadSampleResponse = copyResp;
    
#if USE_PAD_FACTOR
    // Pad the response
    BLUtils::ResizeFillZeros(&mPadSampleResponse, mPadSampleResponse.GetSize()*PAD_FACTOR);    
#endif
    
    WDL_TypedBuf<BL_FLOAT> respForFft = mPadSampleResponse;
    
#if USE_WINDOWING
    if (mWindow.GetSize() != respForFft.GetSize())
    {
        BL_FLOAT hanningFactor = 1.0;
        Window::MakeHanningPow(respForFft.GetSize(), hanningFactor, &mWindow);
    }
#endif

    // Compute fft
    ComputeFft(&respForFft, &mPadFftResponse, false);
    
    NormalizeResponseFft(&mPadFftResponse);
    
    // Without that, the volume increases again after 5s with resp = 10s
#if 1
    // Reset the result buffer
    BLUtils::FillAllZero(&mResultBuf);
#endif
}

void
FftConvolver5::GetResponse(WDL_TypedBuf<BL_FLOAT> *response)
{
    *response = mPadSampleResponse;
    
#if USE_PAD_FACTOR
    // "unpad"
    response->Resize(response->GetSize()/PAD_FACTOR);
#endif
}


void
FftConvolver5::ProcessOneBuffer(const WDL_TypedBuf<BL_FLOAT> &samplesBuf,
                                const WDL_TypedBuf<BL_FLOAT> *ioResultBuf,
                                int offsetSamples, int offsetResult)
{
    WDL_TypedBuf<BL_FLOAT> padBuf;
    padBuf.Resize(mBufSize);
    for (int i = 0; i < mBufSize; i++)
    {
        if (i + offsetSamples >= samplesBuf.GetSize())
            break;
        
        padBuf.Get()[i] = samplesBuf.Get()[i + offsetSamples];
    }
    
    // Take the size of the response if greater than buffer size
#if USE_PAD_FACTOR
    int newSize = MAX(mBufSize*PAD_FACTOR, mPadFftResponse.GetSize());
#else
    int newSize = MAX(mBufSize, mPadFftResponse.GetSize());
#endif
    
    BLUtils::ResizeFillZeros(&padBuf, newSize);
    
#define DEBUG_BUFS 0
#if DEBUG_BUFS
    static int count = 0;
    count++;
    
    if (count == 1)
        BLDebug::DumpData("buf0.txt", padBuf);
#endif
    
#if USE_WINDOWING
    Window::Apply(mWindow, &padBuf);
#endif
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> fftBuf;
    ComputeFft(&padBuf, &fftBuf, mNormalize);
    
#if DEBUG_BUFS
    if (count == 1)
        BLDebug::DumpData("resp.txt", mPadSampleResponse);
#endif
    
    // Apply modifications of the buffer
    ProcessFftBuffer(&fftBuf, mPadFftResponse);
    
    ComputeInverseFft(&fftBuf, &padBuf);
    
#if DEBUG_BUFS
    if (count == 1)
        BLDebug::DumpData("buf1.txt", padBuf);
#endif
    
    for (int i = 0; i < padBuf.GetSize(); i++)
    {
        if (i + offsetResult >= ioResultBuf->GetSize())
            break;
        
        ioResultBuf->Get()[i + offsetResult] += padBuf.Get()[i];
    }
}

bool
FftConvolver5::Process(BL_FLOAT *input, BL_FLOAT *output, int nFrames)
{    
    // Add the new samples
    mSamplesBuf.Add(input, nFrames);
    
    // Without that, crashed with nFrames == 512
    if (mSamplesBuf.GetSize() < mBufSize)
        // Not enough samples, do nothing and wait that we have enough samples
    {
        // Test if we have enough result sample to fill nFrames
        if (mNumResultReady >= nFrames)
        {
            if (output != NULL)
            // Copy the result
            {
                if (mResultBuf.GetSize() == nFrames)
                    memcpy(output, mResultBuf.Get(), nFrames*sizeof(BL_FLOAT));
                // else there is an error somewhere !
            }
            
            // Consume the result already managed
            BLUtils::ConsumeLeft(&mResultBuf, nFrames);
            mNumResultReady -= nFrames;
            
            return true;
        }
        else
            // Fill with zeros
        {
            if (output != NULL)
                memset(output, 0, nFrames*sizeof(BL_FLOAT));
            
            return false;
        }
    }
    
    // Fill with zero the future working area
    BLUtils::AddZeros(&mResultBuf, mBufSize);

    int size = (mBufSize > nFrames) ? mBufSize : nFrames;
    
    // Even bigger if the response is bigger than the samples buffer
    int newSize = MAX(size, mPadFftResponse.GetSize());
    
    // Non-cyclic convolution !
    // We need more room !
#if USE_PAD_FACTOR
    if (mResultBuf.GetSize() < newSize*PAD_FACTOR)
    {
        BLUtils::AddZeros(&mResultBuf, newSize*PAD_FACTOR);
    }
#else
    if (mResultBuf.GetSize() < newSize)
    {
        BLUtils::AddZeros(&mResultBuf, newSize);
    }
#endif
    
    int offsetSamples = 0;
    int numProcessed = 0;
    while (offsetSamples + mBufSize <= mSamplesBuf.GetSize())
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
    BLUtils::ConsumeLeft(&mSamplesBuf, numProcessed);
    
    // We wait for having nFrames samples ready
    if (numProcessed < nFrames)
    {
        if (output != NULL)
            memset(output, 0, nFrames*sizeof(BL_FLOAT));
        
        return false;
    }
    
    // If we have enough result...
    
    // Copy the result
    if (output != NULL)
    {
        if (mResultBuf.GetSize() >= nFrames)
            memcpy(output, mResultBuf.Get(), nFrames*sizeof(BL_FLOAT));
        // else there is an error somewhere
    }
    
    // Consume the result already managed
    BLUtils::ConsumeLeft(&mResultBuf, nFrames);
    
    mNumResultReady -= nFrames;
    
    // Offset is in the interval [0, mBufSize]
    mOffsetResult = mResultBuf.GetSize() % mBufSize;
    
    return true;
}

void
FftConvolver5::ResampleImpulse(WDL_TypedBuf<BL_FLOAT> *impulseResponse,
                               BL_FLOAT sampleRate, BL_FLOAT respSampleRate)
{
    if (impulseResponse->GetSize() == 0)
        return;
    
    if (respSampleRate != sampleRate)
        // We have to resample the impulse
    {
        if (impulseResponse->GetSize() > 0)
        {
            Resampler2 resampler(sampleRate, respSampleRate);
            
            WDL_TypedBuf<BL_FLOAT> newImpulse;
            resampler.Resample(impulseResponse, &newImpulse);
            
            ResizeImpulse(&newImpulse);
            
            *impulseResponse = newImpulse;
        }
    }
}

void
FftConvolver5::ResizeImpulse(WDL_TypedBuf<BL_FLOAT> *impulseResponse)
{
    int respSize = impulseResponse->GetSize();
    int newSize = BLUtilsMath::NextPowerOfTwo(respSize);
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
FftConvolver5::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                                const WDL_TypedBuf<WDL_FFT_COMPLEX> &response)
{
    BLUtils::TakeHalf(ioBuffer);
 
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
    
    BLUtils::ResizeFillZeros(ioBuffer, ioBuffer->GetSize()*2);
    BLUtilsFft::FillSecondFftHalf(ioBuffer);
}

void
FftConvolver5::ComputeFft(const WDL_TypedBuf<BL_FLOAT> *samples,
                          WDL_TypedBuf<WDL_FFT_COMPLEX> *fftSamples,
                          bool normalize)
{
    if (fftSamples->GetSize() != samples->GetSize())
        fftSamples->Resize(samples->GetSize());
    
    BL_FLOAT normCoeff = 1.0;
    if (normalize)
        normCoeff = 1.0/fftSamples->GetSize();
    
    // Fill the fft buf
    for (int i = 0; i < fftSamples->GetSize(); i++)
    {
        // No need to divide by mBufSize if we ponderate analysis hanning window
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
FftConvolver5::ComputeInverseFft(const WDL_TypedBuf<WDL_FFT_COMPLEX> *fftSamples,
                                 WDL_TypedBuf<BL_FLOAT> *samples)
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
FftConvolver5::NormalizeResponseFft(WDL_TypedBuf<WDL_FFT_COMPLEX> *fftSamples)
{
    // As done in image analysis with normalized kernels
    // Multiply by a coefficient so the sum becomes one
    
    // We can normalize the response as samples or as fft, this is the same
    // as fft is conservative for multplication by a factor
    
    WDL_TypedBuf<BL_FLOAT> magns;
    WDL_TypedBuf<BL_FLOAT> phases;
    BLUtilsComp::ComplexToMagnPhase(&magns, &phases, *fftSamples);
    
    // Take sum(samples) and not sum(abs(samples)) !
    BL_FLOAT sum = BLUtils::ComputeSum(magns);
    
    int fftSize = fftSamples->GetSize();
    
    BL_FLOAT coeff = 0.0;
    if (sum > 0.0)
        coeff = 0.125*fftSize/sum;
    
    BLUtils::MultValues(&magns, coeff);
    
    BLUtilsComp::MagnPhaseToComplex(fftSamples, magns, phases);
}

