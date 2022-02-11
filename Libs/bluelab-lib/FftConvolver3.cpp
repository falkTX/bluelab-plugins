//
//  FftConvolver.cpp
//  Spatializer
//
//  Created by Pan on 20/11/17.
//
//

#include <Window.h>
#include <BLUtils.h>

#include "FftConvolver3.h"

FftConvolver3::FftConvolver3(int bufferSize, bool normalize)
{
    mBufSize = bufferSize;
    mShift = mBufSize;
    mNormalize = normalize;
    
    Reset();
}

FftConvolver3::~FftConvolver3() {}

void
FftConvolver3::Reset()
{
    mSamplesBuf.Resize(0);
    
    mResultBuf.Resize(0);
    
    mOffsetResult = 0;
    
    mInit = true;
}

void
FftConvolver3::SetResponse(const WDL_TypedBuf<BL_FLOAT> *response)
{
    mPadSampleResponse = *response;
    
    // Pad the response
    BLUtils::ResizeFillZeros(&mPadSampleResponse, response->GetSize()*2);
    
    // Compute fft
    ComputeFft(&mPadSampleResponse, &mPadFftResponse, false);
    
    // Reset the result buffer
    // because there can remain future tails of previous computations
    // (see non-cyclic buffers)
    BLUtils::FillAllZero(&mResultBuf);
}

void
FftConvolver3::GetResponse(WDL_TypedBuf<BL_FLOAT> *response)
{
    *response = mPadSampleResponse;
    
    // "unpad"
    response->Resize(response->GetSize()/2);
}


void
FftConvolver3::ProcessOneBuffer(const WDL_TypedBuf<BL_FLOAT> &samplesBuf,
                                const WDL_TypedBuf<BL_FLOAT> *ioResultBuf,
                                int offsetSamples, int offsetResult)
{
    WDL_TypedBuf<BL_FLOAT> padBuf;
    padBuf.Resize(mBufSize);
    for (int i = 0; i < mBufSize; i++)
        padBuf.Get()[i] = samplesBuf.Get()[i + offsetSamples];
    
    // Non-cyclic technique, to avoid aliasing
    // and to remove the need of overlap
    BLUtils::ResizeFillZeros(&padBuf, padBuf.GetSize()*2);
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> fftBuf;
    ComputeFft(&padBuf, &fftBuf, mNormalize);
    
    // Apply modifications of the buffer
    ProcessFftBuffer(&fftBuf, mPadFftResponse);
    
    ComputeInverseFft(&fftBuf, &padBuf);
    
    for (int i = 0; i < padBuf.GetSize(); i++)
        ioResultBuf->Get()[i + offsetResult] += padBuf.Get()[i];
}

bool
FftConvolver3::Process(BL_FLOAT *input, BL_FLOAT *output, int nFrames)
{    
    // Add the new samples
    mSamplesBuf.Add(input, nFrames);
    
    // Fill with zero the future working area
    BLUtils::AddZeros(&mResultBuf, nFrames);

    // Non-cyclic convolution !
    // We need more room !
    if (mResultBuf.GetSize() < mBufSize*2)
    {
        BLUtils::AddZeros(&mResultBuf, mBufSize*2);
    }
    
    // Without that, crashed with nFrames == 512
    if (mSamplesBuf.GetSize() < mBufSize)
    // Not enough samples, do nothing and wait that we have enough samples
    {
        return false;
    }
    
    int offsetSamples = 0;
    int numProcessed = 0;
    do
    {
        ProcessOneBuffer(mSamplesBuf, &mResultBuf, offsetSamples, mOffsetResult);
        
        numProcessed += mShift;
        
        // Shift the offsets
        offsetSamples += mShift;
        mOffsetResult += mShift;
        
        // Stop if it remains too few samples to process
    } while (offsetSamples + mBufSize <= mSamplesBuf.GetSize());
    
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
        memcpy(output, mResultBuf.Get(), nFrames*sizeof(BL_FLOAT));
   
    // Consume the result already managed
    BLUtils::ConsumeLeft(&mResultBuf, nFrames);
    
    // Offset is in the interval [0, mBufSize]
    mOffsetResult = mResultBuf.GetSize() % mBufSize;
    
    return true;
}

void
FftConvolver3::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
                                const WDL_TypedBuf<WDL_FFT_COMPLEX> &response)
{
    if (response.GetSize() != ioBuffer->GetSize())
        return;
                           
    for (int i = 0; i < ioBuffer->GetSize(); i++)
    {
        WDL_FFT_COMPLEX sigComp = ioBuffer->Get()[i];
        WDL_FFT_COMPLEX respComp = response.Get()[i];
        
        // Pointwise multiplication of two complex
        WDL_FFT_COMPLEX res;
        res.re = sigComp.re*respComp.re - sigComp.im*respComp.im;
        res.im = sigComp.im*respComp.re + sigComp.re*respComp.im;
        
        ioBuffer->Get()[i] = res;
    }
}

void
FftConvolver3::ComputeFft(const WDL_TypedBuf<BL_FLOAT> *samples,
                          WDL_TypedBuf<WDL_FFT_COMPLEX> *fftSamples,
                          bool normalize)
{
    if (fftSamples->GetSize() != samples->GetSize())
        fftSamples->Resize(samples->GetSize());
    
    BL_FLOAT normCoeff = 1.0;
    if (normalize)
        normCoeff = 1.0/fftSamples->GetSize()/*mBufSize*/;
    
    // Fill the fft buf
    for (int i = 0; i < fftSamples->GetSize()/*mBufSize*/; i++)
    {
        // No need to divide by mBufSize if we ponderate analysis hanning window
        fftSamples->Get()[i].re = normCoeff*samples->Get()[i];
        fftSamples->Get()[i].im = 0.0;
    }
    
    // Do the fft
    // Do it on the window but also in following the empty space, to capture remaining waves
    WDL_fft(fftSamples->Get(), fftSamples->GetSize()/*mBufSize*/, false);
    
    // Sort the fft buffer
    WDL_TypedBuf<WDL_FFT_COMPLEX> tmpFftBuf = *fftSamples;
    for (int i = 0; i < fftSamples->GetSize()/*mBufSize*/; i++)
    {
        int k = WDL_fft_permute(fftSamples->GetSize()/*mBufSize*/, i);
        
        fftSamples->Get()[i].re = tmpFftBuf.Get()[k].re;
        fftSamples->Get()[i].im = tmpFftBuf.Get()[k].im;
    }
}

void
FftConvolver3::ComputeInverseFft(const WDL_TypedBuf<WDL_FFT_COMPLEX> *fftSamples,
                                 WDL_TypedBuf<BL_FLOAT> *samples)
{
    WDL_TypedBuf<WDL_FFT_COMPLEX> tmpFftBuf = *fftSamples;
    for (int i = 0; i < fftSamples->GetSize() /*mBufSize*/; i++)
    {
        int k = WDL_fft_permute(fftSamples->GetSize()/*mBufSize*/, i);
        fftSamples->Get()[k].re = tmpFftBuf.Get()[i].re;
        fftSamples->Get()[k].im = tmpFftBuf.Get()[i].im;
    }
    
    // Should not do this step when not necessary (for example for transients)
    
    // Do the ifft
    WDL_fft(fftSamples->Get(), fftSamples->GetSize()/*mBufSize*/, true);
    
    for (int i = 0; i < fftSamples->GetSize()/*mBufSize*/; i++)
        samples->Get()[i] = fftSamples->Get()[i].re;
}
