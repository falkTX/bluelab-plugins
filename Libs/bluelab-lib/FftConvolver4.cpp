/* Copyright (C) 2022 Nicolas Dittlo <deadlab.plugins@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this software; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */
 
//
//  FftConvolver4.cpp
//  Spatializer
//
//  Created by Pan on 20/11/17.
//
//

#include "Resampler2.h"
#include <Window.h>
#include <BLUtils.h>

#include "FftConvolver4.h"

#define PAD_FACTOR 2

FftConvolver4::FftConvolver4(int bufferSize, bool normalize)
{
    mBufSize = bufferSize;
    mShift = mBufSize;
    mNormalize = normalize;
    mNumResultReady = 0;
    
    Reset();
}

FftConvolver4::~FftConvolver4() {}

void
FftConvolver4::Reset()
{
    mSamplesBuf.Resize(0);
    
    mResultBuf.Resize(0);
    
    mOffsetResult = 0;
    
    mInit = true;
    
    mNumResultReady = 0;
}

void
FftConvolver4::SetResponse(const WDL_TypedBuf<BL_FLOAT> *response)
{
    if (response->GetSize() == 0)
        return;
    
    WDL_TypedBuf<BL_FLOAT> copyResp = *response;
    ResizeImpulse(&copyResp, mBufSize);
    
    mPadSampleResponse = copyResp;
    
    // Pad the response
    BLUtils::ResizeFillZeros(&mPadSampleResponse, copyResp.GetSize()*PAD_FACTOR);
    
    // Compute fft
    ComputeFft(&mPadSampleResponse, &mPadFftResponse, false);
    
    // We must keep the future tails, for continuity when parameter is changing !!
    // So don't fill with zeros !
#if 0
    // Reset the result buffer
    // because there can remain future tails of previous computations
    // (see non-cyclic buffers)
    BLUtils::FillAllZero(&mResultBuf);
#endif
}

void
FftConvolver4::GetResponse(WDL_TypedBuf<BL_FLOAT> *response)
{
    *response = mPadSampleResponse;
    
    // "unpad"
    response->Resize(response->GetSize()/PAD_FACTOR);
}


void
FftConvolver4::ProcessOneBuffer(const WDL_TypedBuf<BL_FLOAT> &samplesBuf,
                                const WDL_TypedBuf<BL_FLOAT> *ioResultBuf,
                                int offsetSamples, int offsetResult)
{
    WDL_TypedBuf<BL_FLOAT> padBuf;
    padBuf.Resize(mBufSize);
    for (int i = 0; i < mBufSize; i++)
        padBuf.Get()[i] = samplesBuf.Get()[i + offsetSamples];
    
    // Non-cyclic technique, to avoid aliasing
    // and to remove the need of overlap
    BLUtils::ResizeFillZeros(&padBuf, padBuf.GetSize()*PAD_FACTOR);
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> fftBuf;
    ComputeFft(&padBuf, &fftBuf, mNormalize);
    
    // Apply modifications of the buffer
    ProcessFftBuffer(&fftBuf, mPadFftResponse);
    
    ComputeInverseFft(&fftBuf, &padBuf);
    
    for (int i = 0; i < padBuf.GetSize(); i++)
        ioResultBuf->Get()[i + offsetResult] += padBuf.Get()[i];
}

bool
FftConvolver4::Process(BL_FLOAT *input, BL_FLOAT *output, int nFrames)
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
                memcpy(output, mResultBuf.Get(), nFrames*sizeof(BL_FLOAT));
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
    
    // Non-cyclic convolution !
    // We need more room !
    if (mResultBuf.GetSize() < size*PAD_FACTOR)
    {
        BLUtils::AddZeros(&mResultBuf, size*PAD_FACTOR);
    }
    
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
        memcpy(output, mResultBuf.Get(), nFrames*sizeof(BL_FLOAT));
   
    // Consume the result already managed
    BLUtils::ConsumeLeft(&mResultBuf, nFrames);
    
    mNumResultReady -= nFrames;
    
    // Offset is in the interval [0, mBufSize]
    mOffsetResult = mResultBuf.GetSize() % mBufSize;
    
    return true;
}

void
FftConvolver4::ResampleImpulse(WDL_TypedBuf<BL_FLOAT> *impulseResponse,
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
            
            int impSize = impulseResponse->GetSize();
            
            WDL_TypedBuf<BL_FLOAT> newImpulse;
            resampler.Resample(impulseResponse, &newImpulse);
            
            ResizeImpulse(&newImpulse, impSize);
            
            *impulseResponse = newImpulse;
        }
    }
}

void
FftConvolver4::ResizeImpulse(WDL_TypedBuf<BL_FLOAT> *impulseResponse, int newSize)
{
    // Resize the resampled impulse to the size of the loaded impulse
    // (to be sure to keep consitency in sizes)
    if (impulseResponse->GetSize() > newSize)
        // Cut
        impulseResponse->Resize(newSize);
    else
    {
        int diff = newSize - impulseResponse->GetSize();
        
        impulseResponse->Resize(newSize);
        
        // Fill with zeros if we have grown
        for (int j = 0; j < diff; j++)
            impulseResponse->Get()[newSize - j - 1] = 0.0;
    }
}

void
FftConvolver4::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
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
FftConvolver4::ComputeFft(const WDL_TypedBuf<BL_FLOAT> *samples,
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
        
        fftSamples->Get()[i].re = tmpFftBuf.Get()[k].re;
        fftSamples->Get()[i].im = tmpFftBuf.Get()[k].im;
    }
}

void
FftConvolver4::ComputeInverseFft(const WDL_TypedBuf<WDL_FFT_COMPLEX> *fftSamples,
                                 WDL_TypedBuf<BL_FLOAT> *samples)
{
    WDL_TypedBuf<WDL_FFT_COMPLEX> tmpFftBuf = *fftSamples;
    for (int i = 0; i < fftSamples->GetSize(); i++)
    {
        int k = WDL_fft_permute(fftSamples->GetSize(), i);
        fftSamples->Get()[k].re = tmpFftBuf.Get()[i].re;
        fftSamples->Get()[k].im = tmpFftBuf.Get()[i].im;
    }
    
    // Should not do this step when not necessary (for example for transients)
    
    // Do the ifft
    WDL_fft(fftSamples->Get(), fftSamples->GetSize(), true);
    
    for (int i = 0; i < fftSamples->GetSize(); i++)
        samples->Get()[i] = fftSamples->Get()[i].re;
}
