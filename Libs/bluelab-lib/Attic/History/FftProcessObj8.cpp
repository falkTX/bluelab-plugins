//
//  FftProcessObj.cpp
//  Transient
//
//  Created by Apple m'a Tuer on 21/05/17.
//
//

#include <Window.h>
#include <BLUtils.h>

#include "FftProcessObj8.h"


#define NORMALIZE_FFT 1

// When using freq res > 1, add the tail of the fft to the future samples
// (it seems to add aliasing when activated)
#define ADD_TAIL 0
#define ZERO_PAD_WINDOW_REORDER 0

// Useful for debuging
#define DEBUG_DISABLE_OVERSAMPLING 0
#define DEBUG_DISABLE_PROCESS 0
#define DEBUG_WINDOWS 0


// NOTE: With freqRes == 2, and oversampling = 1 => better
// the transient signal is decreased / the other signal stays constant
// => that's what we want !
// With previous version, the other signal increased
FftProcessObj8::FftProcessObj8(int bufferSize, int oversampling, int freqRes,
                               enum AnalysisMethod aMethod,
                               enum SynthesisMethod sMethod, bool skipFFT)
{
    mBufSize = bufferSize;
    
    mAnalysisMethod = aMethod;
    mSynthesisMethod = sMethod;
    
    mSkipFFT = skipFFT;
    
    mMustPadSamples = true;
    mMustUnpadResult = false;
    
    Reset(oversampling, freqRes);
}

FftProcessObj8::~FftProcessObj8() {}

void
FftProcessObj8::Reset(int oversampling, int freqRes)
{
    if (oversampling > 0)
        mOversampling = oversampling;
    
    if (freqRes > 0)
        mFreqRes = freqRes;
    
#if DEBUG_DISABLE_OVERSAMPLING
    mOversampling = 1;
#endif
    
    mShift = mBufSize/mOversampling;
    
    MakeWindows();
    
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
    
    mMustPadSamples = true;
    mMustUnpadResult = false;
}

void
FftProcessObj8::AddSamples(BL_FLOAT *samples, int numSamples)
{
    mSamplesChunk.Add(samples, numSamples);
}

void
FftProcessObj8::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer) {}

void
FftProcessObj8::ProcessSamplesBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer) {}

void
FftProcessObj8::ProcessOneBuffer()
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
    
    ApplyAnalysisWindow(&copySamplesBuf);
    
    if (mSkipFFT)
    {
        // By default, do nothing with the fft
        mResultTmpChunk.Resize(0);
        
        // Be careful to not add the beginning of the buffer
        // (otherwise the windows will be applied twice sometimes).

        //mResultTmpChunk.Add(&samples[mBufOffset], mBufSize);
        mResultTmpChunk.Add(copySamplesBuf.Get(), mBufSize);
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
        //int winIndex = i % mSynthesisWindow.GetSize(); // buggy

        BL_FLOAT winCoeff = mSynthesisWindow.Get()[i];
        
        result[i + mBufOffset] += winCoeff*mResultTmpChunk.Get()[i];
    }
}

void
FftProcessObj8::NextSamplesBuffer()
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
FftProcessObj8::NextOutBuffer()
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
FftProcessObj8::GetResultOutBuffer(BL_FLOAT *output, int nFrames)
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
FftProcessObj8::Shift()
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
FftProcessObj8::Process(BL_FLOAT *input, BL_FLOAT *output, int nFrames)
{
#if DEBUG_WINDOWS
    for (int i = 0; i < nFrames; i++)
        input[i] = 1.0;
#endif
    
    // Add the new input data
    AddSamples(input, nFrames);
    
    if (mMustPadSamples)
    {
        BLUtils::PadZerosLeft(&mSamplesChunk, mAnalysisWindow.GetSize());
        
        mMustPadSamples = false;
        mMustUnpadResult = true;
    }
    
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
    
    if (mResultOutChunk.GetSize() >= mAnalysisWindow.GetSize()*1.5)
    {
        if (mMustUnpadResult)
        {
            BLUtils::ConsumeLeft(&mResultOutChunk, mAnalysisWindow.GetSize());
            
            mMustUnpadResult = false;
        }
    }
    
    // Get the computed result
    int numOutSamples = mResultOutChunk.GetSize();
    if ((nFrames <= numOutSamples) && !mMustUnpadResult)
    {
        if (output != NULL)
        {
            GetResultOutBuffer(output, nFrames);
        }
        
#if DEBUG_WINDOWS
        Debug::AppendData("output.txt", output, nFrames);
#endif

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
FftProcessObj8::NormalizeFftValues(WDL_TypedBuf<BL_FLOAT> *magns)
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
FftProcessObj8::FillSecondHalf(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer)
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
FftProcessObj8::ApplyAnalysisWindow(WDL_TypedBuf<BL_FLOAT> *samples)
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
FftProcessObj8::MakeWindows()
{
    // Analysis and synthesis Hann windows
    // With Hanning, the gain is not increased when we increase the oversampling
    
    int windowSize = mBufSize;
    
#if ZERO_PAD_WINDOW_REORDER
    windowSize = mBufSize*mFreqRes;
#endif
    
    //
    // Hanning pow is used to narrow the analysis and synthesis windows
    // as much as possible, to have more "horizontal blur" in the spectrograms
    // (this reduces the attachs otherwise)
    //
    // Oversampling must be Hanning pow x 2 at the minimum
    // otherwise, we loose COLA condition
    //
    // Be careful, when using both analysis and synthesis windows,
    // it is as the Hanning pow was twice
    // (we have ana*syn equivalent to han*han).
    //
    //
    // NOTE: in the case of oversampling = 2, and two windows,
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
    if (mOversampling > 1)
    {
        if ((mAnalysisMethod == AnalysisMethodWindow) || (mSynthesisMethod == SynthesisMethodWindow))
            // We happly windows twice (before, and after)
            hanningFactor = mOversampling/4.0;
        else
            // We apply only one window, so we can narrow more
            hanningFactor = mOversampling/2.0;
    }
    
    // Analysis
    if ((mAnalysisMethod == AnalysisMethodWindow) && (mOversampling > 1))
        Window::MakeHanningPow(windowSize, hanningFactor, &mAnalysisWindow);
    else
        Window::MakeSquare(windowSize, 1.0, &mAnalysisWindow);
    
#if DEBUG_WINDOWS
    BL_FLOAT cola0 = Window::CheckCOLA(&mAnalysisWindow, mOversampling);
   fprintf(stderr, "ANALYSIS - over: %d cola: %g\n", mOversampling, cola0);
#endif
    
    // Synthesis
    if ((mSynthesisMethod == SynthesisMethodWindow) && (mOversampling > 1))
        Window::MakeHanningPow(windowSize, hanningFactor, &mSynthesisWindow);
    else
        Window::MakeSquare(windowSize, 1.0, &mSynthesisWindow);
    
#if DEBUG_WINDOWS
    BL_FLOAT cola1 = Window::CheckCOLA(&mSynthesisWindow, mOversampling);
    fprintf(stderr, "SYNTHESIS - over: %d cola: %g\n", mOversampling, cola1);
    fprintf(stderr, "\n");
#endif
    
    // Normalize
    if ((mAnalysisMethod == AnalysisMethodWindow) &&
        (mSynthesisMethod == SynthesisMethodNone))
    {
        Window::NormalizeWindow(&mAnalysisWindow, mOversampling);
    }
    else if((mAnalysisMethod == AnalysisMethodNone) &&
            (mSynthesisMethod == SynthesisMethodWindow))
    {
        Window::NormalizeWindow(&mSynthesisWindow, mOversampling);
    }
    else if((mAnalysisMethod == AnalysisMethodWindow) &&
            (mSynthesisMethod == SynthesisMethodWindow))
    {
        // We have both windows
        // We must compute the cola of the next hanning factor,
        // then normalize with it
        //
        
        WDL_TypedBuf<BL_FLOAT> win;
        Window::MakeHanningPow(mAnalysisWindow.GetSize(), hanningFactor*2, &win);
        
        BL_FLOAT cola = Window::CheckCOLA(&win, mOversampling);
        
        // Normalize only the synthesis window
        Window::NormalizeWindow(&mSynthesisWindow, cola);
    }
    else if ((mAnalysisMethod == AnalysisMethodNone) &&
             (mSynthesisMethod == SynthesisMethodNone))
    {
        // Normalize only the synthesis window...
        Window::NormalizeWindow(&mSynthesisWindow, mOversampling);
    }
}

void
FftProcessObj8::ComputeFft(const WDL_TypedBuf<BL_FLOAT> *samples,
                           WDL_TypedBuf<WDL_FFT_COMPLEX> *fftSamples)
{
    int bufSize = samples->GetSize();
    
    if (mTmpFftBuf.GetSize() != bufSize)
        mTmpFftBuf.Resize(bufSize);

    BL_FLOAT coeff = 1.0;
#if NORMALIZE_FFT
    coeff = 1.0/bufSize;
#endif
    
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
FftProcessObj8::ComputeInverseFft(const WDL_TypedBuf<WDL_FFT_COMPLEX> *fftSamples,
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
    
    // fft
    BL_FLOAT coeff = 1.0;
#if !NORMALIZE_FFT
    // Not sure about that...
    //coeff = bufSize;
#endif
    
    // Take FREQ_RES into account
    // Not sure we must do that only when normalizing or not
    coeff *= 1.0/mFreqRes;
    for (int i = 0; i < bufSize; i++)
        samples->Get()[i] *= coeff;
}
