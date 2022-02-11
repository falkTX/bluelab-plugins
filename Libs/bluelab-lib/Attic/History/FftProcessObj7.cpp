//
//  FftProcessObj.cpp
//  Transient
//
//  Created by Apple m'a Tuer on 21/05/17.
//
//

#include <Window.h>
#include <BLUtils.h>

#include "FftProcessObj7.h"


// When using freq res > 1, add the tail of the fft to the future samples
// (it seems to add aliasing when activated)
#define ADD_TAIL 0
#define ZERO_PAD_WINDOW_REORDER 0

// Useful for debuging
#define DEBUG_DISABLE_OVERSAMPLING 0
#define DEBUG_DISABLE_PROCESS 0


// NOTE: With freqRes == 2, and oversampling = 1 => better
// the transient signal is decreased / the other signal stays constant
// => that's what we want !
// With previous version, the other signal increased
FftProcessObj7::FftProcessObj7(int bufferSize, int oversampling, int freqRes,
                               bool normalize,
                               enum AnalysisMethod aMethod,
                               enum SynthesisMethod sMethod, bool skipFFT)
{
    mBufSize = bufferSize;
    mOversampling = oversampling;
    mFreqRes = freqRes;
    
#if DEBUG_DISABLE_OVERSAMPLING
    mOversampling = 1;
#endif

    mShift = mBufSize/mOversampling;
    mOverlap = mBufSize - mShift;
    
    mNormalize = normalize;
    mAnalysisMethod = aMethod;
    mSynthesisMethod = sMethod;
    
    mSkipFFT = skipFFT;
    
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
    
    int windowSize = mBufSize;
    
#if ZERO_PAD_WINDOW_REORDER
    windowSize = mBufSize*mFreqRes;
#endif
    
#define COLA_DEBUG 0
    BL_FLOAT hanningFactor = 1.0;
    if (mOversampling > 1)
        hanningFactor = mOversampling/2.0;

    // Normalization factor for windows
    // Use it for the last window only
    //
    // Maybe we could have used it for both windows, sqrt(normFactor) ?
    // => may be improved
    BL_FLOAT normFactor = mOversampling/4.0;
    
    // Analysis
    if ((aMethod == AnalysisMethodWindow) && (mOversampling > 1))
    {
        Window::MakeHanningPow(windowSize, hanningFactor, &mAnalysisWindow);
    }
    else
    {
        Window::MakeSquare(windowSize, 1.0, &mAnalysisWindow);
    }
    
#if COLA_DEBUG
    fprintf(stderr, "BEFORE---------\n");
    BL_FLOAT cola0 = Window::CheckCOLA(&mAnalysisWindow, mOversampling);
    fprintf(stderr, "cola0: %g\n", cola0);
#endif
    
    Window::NormalizeWindow(&mAnalysisWindow, mOversampling, 1.0);
    
#if COLA_DEBUG
    BL_FLOAT cola1 = Window::CheckCOLA(&mAnalysisWindow, mOversampling);
    fprintf(stderr, "cola1: %g\n", cola1);
#endif
    
    // Synthesis
    if ((sMethod == SynthesisMethodWindow) && (mOversampling > 1))
    {
        Window::MakeHanningPow(windowSize, hanningFactor, &mSynthesisWindow);
    }
    else
    {
        Window::MakeSquare(windowSize, 1.0, &mSynthesisWindow);
    }
    
#if COLA_DEBUG
    BL_FLOAT cola2 = Window::CheckCOLA(&mSynthesisWindow, mOversampling);
    fprintf(stderr, "cola2: %g\n", cola2);
#endif
    
    Window::NormalizeWindow(&mSynthesisWindow, mOversampling, normFactor);
    
#if COLA_DEBUG
    BL_FLOAT cola3 = Window::CheckCOLA(&mSynthesisWindow, mOversampling);
    fprintf(stderr, "cola0: %g\n", cola3);
    fprintf(stderr, "AFTER---------\n");
#endif
    
    Reset();
}

FftProcessObj7::~FftProcessObj7() {}

void
FftProcessObj7::Reset()
{
    // Set the buffers.
    int resultSize = 2*mBufSize;
    resultSize *= mFreqRes;
    
    BL_FLOAT *zeros = (BL_FLOAT *)malloc(resultSize*sizeof(BL_FLOAT));
    for (int i = 0; i < resultSize; i++)
        zeros[i] = 0.0;
    
    mBufFftChunk.Resize(0);
    mBufFftChunk.Add(zeros, mBufSize);
    
    mBufIfftChunk.Resize(0);
    mBufIfftChunk.Add(zeros, mBufSize);
    
    mResultChunk.Resize(0);
    mResultChunk.Add(zeros, resultSize);
    
    free(zeros);
    
    mBufOffset = 0;
    
    int bufSize = mBufSize*mFreqRes;
    mFftBuf.Resize(bufSize);
    mSortedFftBuf.Resize(bufSize);
    mResultTmpChunk.Resize(bufSize);
}

void
FftProcessObj7::AddSamples(BL_FLOAT *samples, int numSamples)
{
    mSamplesChunk.Add(samples, numSamples);
}

void
FftProcessObj7::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer) {}

void
FftProcessObj7::ProcessSamplesBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer) {}

void
FftProcessObj7::ProcessOneBuffer()
{
    WDL_TypedBuf<BL_FLOAT> copySamplesBuf;
    copySamplesBuf.Resize(mBufSize);
    for (int i = 0; i < mBufSize; i++)
        copySamplesBuf.Get()[i] = mSamplesChunk.Get()[i + mBufOffset];
    
#if ZERO_PAD_WINDOW_REORDER
    // Zero padding before windowing
    // See: http://www.bitweenie.com/listings/fft-zero-padding/
    // (last liens of the page)
    if (mFreqRes > 1)
    {
        // Non-cyclic technique, to avoid aliasing
        BLUtils::ResizeFillZeros(&copySamplesBuf, copySamplesBuf.GetSize()*mFreqRes);
    }
#endif
    
    BL_FLOAT *samples = copySamplesBuf.Get();
    
    
    // Multiply the samples by analysis window
    // The window can be identity if necessary (no analysis window, or no overlap)
    for (int i = 0; i < copySamplesBuf.GetSize(); i++)
    {
        BL_FLOAT winCoeff = mAnalysisWindow.Get()[i];
        samples[i] *= winCoeff;
    }
    
    if (mSkipFFT)
    {
        // By default, do nothing with the fft
        mResultTmpChunk.Resize(0);
        
        // Be careful to not add the beginning of the buffer
        // (otherwise the windows will be applied twice sometimes).

        //mResultTmpChunk.Add(&samples[mBufOffset], mBufSize);
        mResultTmpChunk.Add(samples, mBufSize);
    }
    else
    {
#if !ZERO_PAD_WINDOW_REORDER
        if (mFreqRes > 1)
        {
            // Non-cyclic technique, to avoid aliasing
            BLUtils::ResizeFillZeros(&copySamplesBuf, copySamplesBuf.GetSize()*mFreqRes);
        }
#endif
        
        // NOTE: Here, we have checked that mFreqRes didn't modify the amplitude
        // of the out samples, neither the intermediate magnitude !
        
        ComputeFft(&copySamplesBuf, &mSortedFftBuf);
        
#if !DEBUG_DISABLE_PROCESS
        // Apply modifications of the buffer
        ProcessFftBuffer(&mSortedFftBuf);
#endif
        
        ComputeInverseFft(&mSortedFftBuf, &mResultTmpChunk);
    }
    
#if !DEBUG_DISABLE_PROCESS
    ProcessSamplesBuffer(&mResultTmpChunk);
#endif
    
    // Fill the result
    BL_FLOAT *result = mResultChunk.Get();

    // Standard loop count
    int loopCount = mBufSize;
    
#if ADD_TAIL
    if (mFreqRes > 1)
        // Grater size
        loopCount = copySamplesBuf.GetSize();
#endif
    
    for (int i = 0; i < loopCount; i++)
        // Finally, no need to multiply by Hanning, because the data is already multiplied (at the beginning)
        // So simply overlap.
    {
        // Modulo for the case of adding long tail (and repeating the window...)
        // => This may be buggy
        int winIndex = i % mSynthesisWindow.GetSize();

        BL_FLOAT winCoeff = mSynthesisWindow.Get()[winIndex];
        
        result[i + mBufOffset] += winCoeff*mResultTmpChunk.Get()[i];
    }
}

void
FftProcessObj7::NextSamplesBuffer()
{
    // Reduce the size by mOverlap
    int size = mSamplesChunk.GetSize();
    if (size == mBufSize)
        mSamplesChunk.Resize(0);
    else
        if (size > mBufSize)
        {
            int newSize = size - mBufSize;
            
            // Resize down, skipping left
            WDL_TypedBuf<BL_FLOAT> tmpChunk;
            tmpChunk.Add(&mSamplesChunk.Get()[mBufSize], newSize);
            mSamplesChunk = tmpChunk;
        }
}

void
FftProcessObj7::NextOutBuffer()
{
    // Reduce the size by mOverlap
    int size = mResultChunk.GetSize();
    
    int minSize = mBufSize*2*mFreqRes;

    if (size < minSize)
        return;
    
    BL_FLOAT *resultBuf = mResultChunk.Get();
    
    WDL_TypedBuf<BL_FLOAT> newBuffer;
    for (int i = 0; i < mBufSize; i++)
    {
        newBuffer.Add(resultBuf[i]);
    }
    
    for (int i = 0; i < mBufSize; i++)
        mResultOutChunk.Add(newBuffer.Get()[i]);
    
    if (size == mBufSize)
        mResultChunk.Resize(0);
    else
        if (size > mBufSize)
        {
            int newSize = size - mBufSize;
            
            // Resize down, skipping left
            WDL_TypedBuf<BL_FLOAT> tmpChunk;
            tmpChunk.Add(&mResultChunk.Get()[mBufSize], newSize);
            mResultChunk = tmpChunk;
        }
    
    // Grow the output with zeros
    BL_FLOAT *zeros = (BL_FLOAT *)malloc(mBufSize*sizeof(BL_FLOAT));
    for (int i = 0; i < mBufSize; i++)
        zeros[i] = 0.0;
    mResultChunk.Add(zeros, mBufSize);
    free(zeros);
}

void
FftProcessObj7::GetResultOutBuffer(BL_FLOAT *output, int nFrames)
{
    // Should not happen
    int size = mResultOutChunk.GetSize()/mFreqRes;
    if (size < nFrames)
        return;
    
    // Copy the result
    memcpy(output, mResultOutChunk.Get(), nFrames*sizeof(BL_FLOAT));
    
    // Consume the returned samples
    int newSize = mResultOutChunk.GetSize() - nFrames;
    
    // Resize down, skipping left
    WDL_TypedBuf<BL_FLOAT> tmpChunk;
    tmpChunk.Add(&mResultOutChunk.Get()[nFrames], newSize);
    mResultOutChunk = tmpChunk;
}

bool
FftProcessObj7::Shift()
{
    mBufOffset += mShift;
    
    if (mBufOffset >= mBufSize)
    {
        mBufOffset = 0;
        
        return true;
    }
    
    return false;
}

// NOTE: this code works well for 512, 1024, and 2048
bool
FftProcessObj7::Process(BL_FLOAT *input, BL_FLOAT *output, int nFrames)
{
    // Add the new input data
    AddSamples(input, nFrames);
    
    // Compute from the available input data
    int numSamples = mSamplesChunk.GetSize();
    while (numSamples >= 2*mBufSize - mShift)
    {
        bool stopFlag = false;
        while(!stopFlag)
        {
            ProcessOneBuffer();
            stopFlag = Shift();
        }
        
        NextSamplesBuffer();
        NextOutBuffer();
        
        numSamples = mSamplesChunk.GetSize();
    }
    
    // Get the computed result
    int numOutSamples = mResultOutChunk.GetSize();
    if (nFrames <= numOutSamples)
    {
        if (output != NULL)
        {
            GetResultOutBuffer(output, nFrames);
        }
        
        return true;
    }
    else
        // Fill with zeros
        // (as required when there is latency and data is not yet available)
    {
        if (output != NULL)
        {
            for (int i = 0; i < nFrames; i++)
                output[i] = 0.0;
        }
        
        return false;
    }
}

void
FftProcessObj7::NormalizeFftValues(WDL_TypedBuf<BL_FLOAT> *magns)
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
FftProcessObj7::FillSecondHalf(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer)
{
    // It is important that the "middle value" (ie index 1023) is duplicated
    // to index 1024. So we have twice the center value
    //for (int i = 0; i < ioBuffer->GetSize()/2; i++)
    for (int i = 1; i < ioBuffer->GetSize()/2; i++) // Zarlino
    {
        int id0 = i + ioBuffer->GetSize()/2;
        
        // Zarlino modif
        //int id1 = ioBuffer->GetSize()/2 - i - 1;
        int id1 = ioBuffer->GetSize()/2 - i;
        
        ioBuffer->Get()[id0].re = ioBuffer->Get()[id1].re;
        
        // Complex conjugate
        ioBuffer->Get()[id0].im = -ioBuffer->Get()[id1].im;
    }
}

void
FftProcessObj7::ComputeFft(const WDL_TypedBuf<BL_FLOAT> *samples,
                           WDL_TypedBuf<WDL_FFT_COMPLEX> *fftSamples)
{
    int bufSize = samples->GetSize();
    
    // fft
    BL_FLOAT coeff = 1.0;
    if (mNormalize)
    {
        coeff = 1.0/bufSize;
    }
    
    if (mTmpFftBuf.GetSize() != bufSize)
        mTmpFftBuf.Resize(bufSize);

    // Fill the fft buf
    for (int i = 0; i < bufSize; i++)
    {
        // No need to divide by mBufSize if we ponderate analysis hanning window
        mTmpFftBuf.Get()[i].re = samples->Get()[i]*coeff;
        mTmpFftBuf.Get()[i].im = 0.0;
    }
    
    // Take FREQ_RES into account
    // Not sure we must do that only when normalizing or not
    for (int i = 0; i < bufSize; i++)
        mTmpFftBuf.Get()[i].re *= mFreqRes;
    
    // Do the fft
    // Do it on the window but also in following the empty space, to capture remaining waves
    WDL_fft(mTmpFftBuf.Get(), bufSize, false);
    
    // Sort the fft buffer
    for (int i = 0; i < bufSize; i++)
    {
        int k = WDL_fft_permute(bufSize, i);

        fftSamples->Get()[i].re = mTmpFftBuf.Get()[k].re;
        fftSamples->Get()[i].im = mTmpFftBuf.Get()[k].im;
    }
}

void
FftProcessObj7::ComputeInverseFft(const WDL_TypedBuf<WDL_FFT_COMPLEX> *fftSamples,
                                  WDL_TypedBuf<BL_FLOAT> *samples)
{
    int bufSize = fftSamples->GetSize();
    
    if (mTmpFftBuf.GetSize() != bufSize)
        mTmpFftBuf.Resize(bufSize);

    for (int i = 0; i < bufSize; i++)
    {
        int k = WDL_fft_permute(bufSize, i);
        
        mTmpFftBuf.Get()[k].re = fftSamples->Get()[i].re;
        mTmpFftBuf.Get()[k].im = fftSamples->Get()[i].im;
    }
    
    // Should not do this step when not necessary (for example for transients)
    
    // Do the ifft
    WDL_fft(mTmpFftBuf.Get(), bufSize, true);

    for (int i = 0; i < bufSize; i++)
        samples->Get()[i] = mTmpFftBuf.Get()[i].re;
    
    // Take FREQ_RES into account
    // Not sure we must do that only when normalizing or not
    BL_FLOAT coeff = 1.0/mFreqRes;
    for (int i = 0; i < bufSize; i++)
        samples->Get()[i] *= coeff;
}
