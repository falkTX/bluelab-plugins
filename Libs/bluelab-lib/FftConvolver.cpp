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
//  FftConvolver.cpp
//  Spatializer
//
//  Created by Pan on 20/11/17.
//
//

#include <Window.h>
#include <BLUtils.h>

#include "FftConvolver.h"

FftConvolver::FftConvolver(int bufferSize, int oversampling, bool normalize,
                           enum AnalysisMethod aMethod, enum SynthesisMethod sMethod)
{
    mBufSize = bufferSize;
    mOversampling = oversampling;
    mShift = mBufSize/mOversampling;
    mOverlap = mBufSize - mShift;
    
    mNormalize = normalize;
    mAnalysisMethod = aMethod;
    mSynthesisMethod = sMethod;
    
    // Analysis and synthesis Hann windows
    // We take the root as explained above, otherwise it will make low oscillations
    // To check oscillations:
    // - pass a white noise inside the algorithm, without any additional processing
    // - visualize the result signal from 1 to 5000
    //
    // Explanation of thaking the root, both for analysis and synthesis:
    // https://www.dsprelated.com/freebooks/sasp/overlap_add_ola_stft_processing.html
    // Section: "Weighted Overlap Add"
    //Window::MakeRootHanning2(mBufSize, &mAnalysisWindow);
    //Window::MakeRootHanning2(mBufSize, &mSynthesisWindow);
    
    // Better with Hanning
    // With Hanning, the gain is not increased when we increase the oversampling
    
    Window::MakeHanning(mBufSize, &mAnalysisWindow);
    Window::MakeHanning(mBufSize, &mSynthesisWindow);
    
    // Comppute energy factor
    // Useful to keep the same volume even if we have oversampling > 2 with Hanning
    mAnalysisWinFactor = ComputeWinFactor(mAnalysisWindow);
    mSynthesisWinFactor = ComputeWinFactor(mSynthesisWindow);
    
    Reset();
}

FftConvolver::~FftConvolver() {}

void
FftConvolver::Reset()
{
    mSamplesBuf.Resize(0);
    
    mResultBuf.Resize(0);
    
    mOffsetResult = 0;
    
    mInit = true;
}

void
FftConvolver::SetResponse(const WDL_TypedBuf<BL_FLOAT> *response)
{
    ComputeFft(response, &mResponse, false);
}

void
FftConvolver::ProcessOneBuffer(const WDL_TypedBuf<BL_FLOAT> &samplesBuf,
                               const WDL_TypedBuf<BL_FLOAT> *ioResultBuf,
                               int offsetSamples, int offsetResult)
{
    WDL_TypedBuf<BL_FLOAT> buf;
    buf.Resize(mBufSize);
    for (int i = 0; i < mBufSize; i++)
        buf.Get()[i] = samplesBuf.Get()[i + offsetSamples];
        
    if (mAnalysisMethod == AnalysisMethodWindow) 
    {
        // Windowing
        if (mOverlap > 0) // then should be half the buffer size
        {
            // Multiply the samples by Hanning window
            for (int i = 0; i < mBufSize; i++)
            {
                buf.Get()[i] *= mAnalysisWindow.Get()[i];
                buf.Get()[i] /= mAnalysisWinFactor;
            }
        }
    }
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> fftBuf;
    ComputeFft(&buf, &fftBuf, mNormalize);
    
    // Apply modifications of the buffer
    ProcessFftBuffer(&fftBuf, mResponse);
    
    ComputeInverseFft(&fftBuf, &buf);
    
    // Multiply by Hanning root, to avoid "clics", i.e breaks in temporal domain
    // which make blue vertical lines in spectrogram
    // NO NEED !
    
    // We must use the same window for synthesis than for analysis, but for synthesis, we take the root
    // See: https://www.dsprelated.com/freebooks/sasp/overlap_add_ola_stft_processing.html
    for (int i = 0; i < buf.GetSize()/*mBufSize*/; i++)
        // Finally, no need to multiply by Hanning, because the data is already multiplied (at the beginning)
        // So simply overlap.
    {
        // Default value, for no window (SynthesisMethodNone)
        BL_FLOAT winCoeff = 1.0;
        
        if (mOverlap > 0)
        {
            if (mSynthesisMethod == SynthesisMethodWindow)
                winCoeff = mSynthesisWindow.Get()[i];
            else
                if (mSynthesisMethod == SynthesisMethodInverseWindow)
                {
                    winCoeff = mAnalysisWindow.Get()[i];
                    winCoeff = 1.0 - winCoeff;
                        
                    // Just in case, but should not happen with Hanning
                    if (winCoeff < 0.0)
                        winCoeff = 0.0;
                }
                
            winCoeff /= mSynthesisWinFactor;
        }
        
        // Avoid increasing the gain when increasing the oversampling
        BL_FLOAT oversampCoeff = 1.0/(4.0*mOversampling);
        
        ioResultBuf->Get()[i + offsetResult] += oversampCoeff*winCoeff*buf.Get()[i];
    }
}

BL_FLOAT
FftConvolver::ComputeWinFactor(const WDL_TypedBuf<BL_FLOAT> &window)
{
    if (mOversampling == 1)
        // No windowing
        return 1.0;
    
    BL_FLOAT factor = 0.0;
    
    // Sum the values of the synthesis windows on the interfval mBufSize
    // For Hanning, this should be 1.0
    
    int start = 0;
    int numValues = 0;
    while(start < mBufSize)
    {
        for (int i = 0; i < window.GetSize(); i++)
        {
            BL_FLOAT winCoeff = window.Get()[i];
            
            factor += winCoeff;
            numValues++;
        }
        
        start += mShift;
    }
    
    // WARNING: this is not really trus, since we have overlap
    // This should be less
    // But this works like that... we have a multiplier elsewhere
    if (numValues > 0)
        factor /= numValues;
    
    return factor;
}

#define DEBUG_BUFFERS 0
bool
FftConvolver::Process(BL_FLOAT *input, BL_FLOAT *output, int nFrames)
{
#if DEBUG_BUFFERS
    fprintf(stderr, "numResultSamples BEGIN: %d\n", mResultBuf.GetSize());
#endif
    
    // Add the new samples
    mSamplesBuf.Add(input, nFrames);
    
    // Fill with zero the future working area
    BLUtils::AddZeros(&mResultBuf, nFrames);

#if NON_CYCLIC_CONV
    if (mResultBuf.GetSize() < nFrames*2)
        BLUtils::AddZeros(&mResultBuf, nFrames);
#endif
    
#if DEBUG_BUFFERS
    fprintf(stderr, "numResultSamples BEGIN 2: %d\n", mResultBuf.GetSize());
#endif
    
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
    
#if DEBUG_BUFFERS
    fprintf(stderr, "numSamples: %d\n", mSamplesBuf.GetSize());
    fprintf(stderr, "numProcessed: %d\n", numProcessed);
#endif
    
    // Consume the processed samples
    BLUtils::ConsumeLeft(&mSamplesBuf, numProcessed);
    
#if DEBUG_BUFFERS
    fprintf(stderr, "\nnumSamples END: %d\n", mSamplesBuf.GetSize());
#endif
    
    // We wait for having nFrames samples ready
    if (numProcessed < nFrames)
    {
        memset(output, 0, nFrames*sizeof(BL_FLOAT));
        
        return false;
    }
    
    // If we have enough result...
    
    // Copy the result
    memcpy(output, mResultBuf.Get(), nFrames*sizeof(BL_FLOAT));
   
    // Consume the result already managed
    BLUtils::ConsumeLeft(&mResultBuf, nFrames);
    
    // Offset is in the interval [0, mBufSize]
    mOffsetResult = mResultBuf.GetSize() % mBufSize;
    
#if DEBUG_BUFFERS
    fprintf(stderr, "numResultSamples END: %d\n", mResultBuf.GetSize());
    fprintf(stderr, "offsetResult END: %d\n", mOffsetResult);
#endif
    
    return true;
}

void
FftConvolver::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer,
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
FftConvolver::NormalizeFftValues(WDL_TypedBuf<BL_FLOAT> *magns)
{
    BL_FLOAT sum = 0.0f;
    
    for (int i = 1; i < magns->GetSize()/2; i++)
    {
        BL_FLOAT magn = magns->Get()[i];
        
        sum += magn;
    }
    
    sum /= magns->GetSize()/2 - 1;
    
    magns->Get()[0] = sum;
    magns->Get()[magns->GetSize() - 1] = sum;
}

void
FftConvolver::FillSecondHalf(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer)
{
    // It is important that the "middle value" (ie index 1023) is duplicated
    // to index 1024. So we have twice the center value
    for (int i = 0; i < ioBuffer->GetSize()/2; i++)
    {
        int id0 = i + ioBuffer->GetSize()/2;
        int id1 = ioBuffer->GetSize()/2 - i - 1;
        
        ioBuffer->Get()[id0].re = ioBuffer->Get()[id1].re;
        
        // Complex conjugate
        ioBuffer->Get()[id0].im = -ioBuffer->Get()[id1].im;
    }
}

void
FftConvolver::ComputeFft(const WDL_TypedBuf<BL_FLOAT> *samples,
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
FftConvolver::ComputeInverseFft(const WDL_TypedBuf<WDL_FFT_COMPLEX> *fftSamples,
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
