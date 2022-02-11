//
//  FftConvolver.cpp
//  Spatializer
//
//  Created by Pan on 20/11/17.
//
//

#include <Window.h>
#include <BLUtils.h>
#include <Resampler2.h>

#include "FftConvolver2.h"

FftConvolver2::FftConvolver2(int bufferSize, int oversampling, bool normalize,
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

FftConvolver2::~FftConvolver2() {}

void
FftConvolver2::Reset()
{
    mSamplesBuf.Resize(0);
    
    mResultBuf.Resize(0);
    
    mOffsetResult = 0;
    
    //mFirstProcess = true;
}

void
FftConvolver2::SetResponse(const WDL_TypedBuf<BL_FLOAT> *response)
{
    if (response->GetSize() == 0)
        return;
    
    WDL_TypedBuf<BL_FLOAT> copyResp = *response;
    ResizeImpulse(&copyResp, mBufSize);
    
    ComputeFft(&copyResp, &mResponse, false);
}

void
FftConvolver2::ProcessOneBuffer(const WDL_TypedBuf<BL_FLOAT> &samplesBuf,
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
                //if (mFirstProcess && (i < mBufSize/2))
                    // Set to 1 instead of windowing
                    // Avoids fade-in at startup
                //    continue;
                
                buf.Get()[i] *= mAnalysisWindow.Get()[i];
                buf.Get()[i] /= mAnalysisWinFactor;
            }
        }
    }
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> fftBuf;
    ComputeFft(&buf, &fftBuf, mNormalize);
    
    // Apply modifications of the buffer
    ProcessFftBuffer(&fftBuf);
    
    ComputeInverseFft(&fftBuf, &buf);
    
    // Multiply by Hanning root, to avoid "clics", i.e breaks in temporal domain
    // which make blue vertical lines in spectrogram
    // NO NEED !
    
    // We must use the same window for synthesis than for analysis, but for synthesis, we take the root
    // See: https://www.dsprelated.com/freebooks/sasp/overlap_add_ola_stft_processing.html
    for (int i = 0; i < mBufSize; i++)
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
        
        //if (mFirstProcess && (i < mBufSize/2))
            // Set to 1 instead of windowing
            // Avoids fade-in at startup
        //    winCoeff = 1.0;
        
        // Avoid increasing the gain when increasing the oversampling
        BL_FLOAT oversampCoeff = 1.0/(4.0*mOversampling);
        
        ioResultBuf->Get()[i + offsetResult] += oversampCoeff*winCoeff*buf.Get()[i];
    }
    
    //mFirstProcess = false;
}

#if 0 // Slow version
// (with impulse 15s, takes about 1mn at startup)
BL_FLOAT
FftConvolver2::ComputeWinFactor(const WDL_TypedBuf<BL_FLOAT> &window)
{
    if (mOversampling == 1)
        // No windowing
        return 1.0;
    
    BL_FLOAT factor = 0.0;
    
    // Sum the values of the synthesis windows on the interval mBufSize
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
#endif

// Optimized version: do not recompute win sum each time
BL_FLOAT
FftConvolver2::ComputeWinFactor(const WDL_TypedBuf<BL_FLOAT> &window)
{
    if (mOversampling == 1)
        // No windowing
        return 1.0;
    
    BL_FLOAT factor = 0.0;
    
    // Sum the values of the synthesis windows on the interval mBufSize
    // For Hanning, this should be 1.0
    
    BL_FLOAT winSum = 0.0;
    for (int i = 0; i < window.GetSize(); i++)
    {
        BL_FLOAT winCoeff = window.Get()[i];
        
        winSum += winCoeff;
    }
    
    int start = 0;
    int numValues = 0;
    while(start < mBufSize)
    {
        factor += winSum;
        
        numValues += window.GetSize();
        
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
FftConvolver2::Process(BL_FLOAT *input, BL_FLOAT *output, int nFrames)
{
#if DEBUG_BUFFERS
    fprintf(stderr, "numResultSamples BEGIN: %d\n", mResultBuf.GetSize());
#endif
    
    // Add the new samples
    mSamplesBuf.Add(input, nFrames);
    
    // Fill with zero the future working area
    BLUtils::AddZeros(&mResultBuf, nFrames);

#if DEBUG_BUFFERS
    fprintf(stderr, "numResultSamples BEGIN 2: %d\n", mResultBuf.GetSize());
#endif
    
    int offsetSamples = 0;
    int numProcessed = 0;
    while (offsetSamples + mBufSize /*<=*/ < mSamplesBuf.GetSize())
    {
        ProcessOneBuffer(mSamplesBuf, &mResultBuf, offsetSamples, mOffsetResult);
        
        numProcessed += mShift;
        
        // Shift the offsets
        offsetSamples += mShift;
        mOffsetResult += mShift;
        
        // Stop if it remains too few samples to process
    }
    
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
FftConvolver2::ResampleImpulse(WDL_TypedBuf<BL_FLOAT> *impulseResponse,
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
FftConvolver2::ResizeImpulse(WDL_TypedBuf<BL_FLOAT> *impulseResponse, int newSize)
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
FftConvolver2::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer)
{
    if (mResponse.GetSize() != ioBuffer->GetSize())
        return;
                           
    for (int i = 0; i < ioBuffer->GetSize(); i++)
    {
        WDL_FFT_COMPLEX sigComp = ioBuffer->Get()[i];
        WDL_FFT_COMPLEX respComp = mResponse.Get()[i];
        
        // Pointwise multiplication of two complex
        WDL_FFT_COMPLEX res;
        res.re = sigComp.re*respComp.re - sigComp.im*respComp.im;
        res.im = sigComp.im*respComp.re + sigComp.re*respComp.im;
        
        ioBuffer->Get()[i] = res;
    }
}

void
FftConvolver2::NormalizeFftValues(WDL_TypedBuf<BL_FLOAT> *magns)
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
FftConvolver2::FillSecondHalf(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer)
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
FftConvolver2::ComputeFft(const WDL_TypedBuf<BL_FLOAT> *samples,
                         WDL_TypedBuf<WDL_FFT_COMPLEX> *fftSamples,
                         bool normalize)
{
    if (fftSamples->GetSize() != samples->GetSize())
        fftSamples->Resize(samples->GetSize());
    
    BL_FLOAT normCoeff = 1.0;
    if (normalize)
        normCoeff = 1.0/mBufSize;
    
    // Fill the fft buf
    for (int i = 0; i < mBufSize; i++)
    {
        // No need to divide by mBufSize if we ponderate analysis hanning window
        fftSamples->Get()[i].re = normCoeff*samples->Get()[i];
        fftSamples->Get()[i].im = 0.0;
    }
    
    // Do the fft
    // Do it on the window but also in following the empty space, to capture remaining waves
    WDL_fft(fftSamples->Get(), mBufSize, false);
    
    // Sort the fft buffer
    WDL_TypedBuf<WDL_FFT_COMPLEX> tmpFftBuf = *fftSamples;
    for (int i = 0; i < mBufSize; i++)
    {
        int k = WDL_fft_permute(mBufSize, i);
        
        fftSamples->Get()[i].re = tmpFftBuf.Get()[k].re;
        fftSamples->Get()[i].im = tmpFftBuf.Get()[k].im;
    }
}

void
FftConvolver2::ComputeInverseFft(const WDL_TypedBuf<WDL_FFT_COMPLEX> *fftSamples,
                                WDL_TypedBuf<BL_FLOAT> *samples)
{
    WDL_TypedBuf<WDL_FFT_COMPLEX> tmpFftBuf = *fftSamples;
    for (int i = 0; i < mBufSize; i++)
    {
        int k = WDL_fft_permute(mBufSize, i);
        fftSamples->Get()[k].re = tmpFftBuf.Get()[i].re;
        fftSamples->Get()[k].im = tmpFftBuf.Get()[i].im;
    }
    
    // Should not do this step when not necessary (for example for transients)
    
    // Do the ifft
    WDL_fft(fftSamples->Get(), mBufSize, true);
    
    for (int i = 0; i < mBufSize; i++)
        samples->Get()[i] = fftSamples->Get()[i].re;
}
