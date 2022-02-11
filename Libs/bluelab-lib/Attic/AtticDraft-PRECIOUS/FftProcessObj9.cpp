//
//  FftProcessObj9.cpp
//  Transient
//
//  Created by Apple m'a Tuer on 21/05/17.
//
//

#include <Window.h>
#include <Utils.h>
#include <Debug.h>

#include "FftProcessObj9.h"


#define NORMALIZE_FFT 1

// When using freq res > 1, add the tail of the fft to the future samples
// (it seems to add aliasing when activated)
#define ADD_TAIL 0
#define ZERO_PAD_WINDOW_REORDER 0

// Useful for debuging
#define DEBUG_DISABLE_OVERLAPPING 0
#define DEBUG_DISABLE_PROCESS 0
#define DEBUG_WINDOWS 0


// NOTE: With freqRes == 2, and overlapping = 1 => better
// the transient signal is decreased / the other signal stays constant
// => that's what we want !
// With previous version, the other signal increased
FftProcessObj9::FftProcessObj9(int bufferSize, int overlapping, int freqRes,
                               enum AnalysisMethod aMethod,
                               enum SynthesisMethod sMethod, bool skipFFT)
{
    mBufSize = bufferSize;
    
    mAnalysisMethod = aMethod;
    mSynthesisMethod = sMethod;
    
    mSkipFFT = skipFFT;
    
    mMustPadSamples = true;
    mMustUnpadResult = false;
    
    Reset(overlapping, freqRes);
}

FftProcessObj9::~FftProcessObj9() {}

void
FftProcessObj9::Reset(int overlapping, int freqRes)
{
    if (overlapping > 0)
        mOverlapping = overlapping;
    
    if (freqRes > 0)
        mFreqRes = freqRes;
    
#if DEBUG_DISABLE_OVERLAPPING
    mOverlapping = 1;
#endif
    
    mShift = mBufSize/mOverlapping;
    
    MakeWindows();
    
    // Set the buffers.
    int resultSize = 2*mBufSize;
    resultSize *= mFreqRes;
    
    double *zeros = (double *)malloc(resultSize*sizeof(double));
    for (int i = 0; i < resultSize; i++)
        zeros[i] = 0.0;
    
    mResultChunk.Resize(0);
    mResultChunk.Add(zeros, resultSize);
    
    free(zeros);
    
    mBufOffset = 0;
    
    int bufSize = mBufSize*mFreqRes;
    mSortedFftBuf.Resize(bufSize);
    mResultTmpChunk.Resize(bufSize);
    
    mSamplesChunk.Resize(0);
    mScChunk.Resize(0);
    
    mMustPadSamples = true;
    mMustUnpadResult = false;
}

void
FftProcessObj9::AddSamples(double *samples, double *scSamples, int numSamples)
{
    mSamplesChunk.Add(samples, numSamples);
    
    if (scSamples != NULL)
        mScChunk.Add(scSamples, numSamples);
}

void
FftProcessObj9::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer) {}

void
FftProcessObj9::ProcessSamplesBuffer(WDL_TypedBuf<double> *ioBuffer,
                                     WDL_TypedBuf<double> *scBuffer) {}

void
FftProcessObj9::SamplesToFftMagns(const WDL_TypedBuf<double> &inSamples,
                                  WDL_TypedBuf<double> *outFftMagns)
{
    int numSamples = inSamples.GetSize();
    int numPaddedSamples = Utils::NewtPowerOfTwo(numSamples);
    
    WDL_TypedBuf<double> paddedSamples = inSamples;
    Utils::ResizeFillZeros(&paddedSamples, numPaddedSamples);
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> fftSamples;
    int freqRes = 1;
    ComputeFft(paddedSamples, &fftSamples, freqRes);
    
    Utils::ComplexToMagn(outFftMagns, fftSamples);
}

void
FftProcessObj9::ProcessOneBuffer()
{
    WDL_TypedBuf<double> copySamplesBuf;
    copySamplesBuf.Resize(mBufSize);
    for (int i = 0; i < mBufSize; i++)
        copySamplesBuf.Get()[i] = mSamplesChunk.Get()[i + mBufOffset];
    
    WDL_TypedBuf<double> copyScBuf;
    if (mScChunk.GetSize() > 0)
    {
        copyScBuf.Resize(mBufSize);
        for (int i = 0; i < mBufSize; i++)
            copyScBuf.Get()[i] = mScChunk.Get()[i + mBufOffset];
    }
    
#if ZERO_PAD_WINDOW_REORDER
    // Zero padding before windowing
    // See: http://www.bitweenie.com/listings/fft-zero-padding/
    // (last liens of the page)
    if (mFreqRes > 1)
    {
        // Non-cyclic technique, to avoid aliasing
        Utils::ResizeFillZeros(&copySamplesBuf, copySamplesBuf.GetSize()*mFreqRes);
        
        if (mScChunk.GetSize() > 0)
            Utils::ResizeFillZeros(&copyScBuf, copyScBuf.GetSize()*mFreqRes);
    }
#endif
    
    ApplyAnalysisWindow(&copySamplesBuf);
    
    if (mScChunk.GetSize() > 0)
        ApplyAnalysisWindow(&copyScBuf);
    
    if (mSkipFFT)
    {
        if (mScChunk.GetSize() == 0)
            // No side chain, take the input
        {
            // By default, do nothing with the fft
            mResultTmpChunk.Resize(0);
        
            // Be careful to not add the beginning of the buffer
            // (otherwise the windows will be applied twice sometimes).
            mResultTmpChunk.Add(copySamplesBuf.Get(), mBufSize);
        }
        else
            // Side chain: take the side chain buffer
        {
            mResultTmpChunk.Resize(0);
            mResultTmpChunk.Add(copyScBuf.Get(), mBufSize);
        }
    }
    else
    {
#if !ZERO_PAD_WINDOW_REORDER
        if (mFreqRes > 1)
        {
            // Non-cyclic technique, to avoid aliasing
            Utils::ResizeFillZeros(&copySamplesBuf, copySamplesBuf.GetSize()*mFreqRes);
            
            if (mScChunk.GetSize() > 0)
                Utils::ResizeFillZeros(&copyScBuf, copyScBuf.GetSize()*mFreqRes);
        }
#endif
        
        // NOTE: Here, we have checked that mFreqRes didn't modify the amplitude
        // of the out samples, neither the intermediate magnitude !
        if (mScChunk.GetSize() == 0)
            // No side chain, take the input
            ComputeFft(copySamplesBuf, &mSortedFftBuf, mFreqRes);
        else
            // Side chain: process the side chain
            ComputeFft(copyScBuf, &mSortedFftBuf, mFreqRes);
        
#if !DEBUG_DISABLE_PROCESS
        // Apply modifications of the buffer
        ProcessFftBuffer(&mSortedFftBuf);
#endif
        
        ComputeInverseFft(mSortedFftBuf, &mResultTmpChunk, mFreqRes);
    }
    
#if !DEBUG_DISABLE_PROCESS
    if (mScChunk.GetSize() == 0)
        // No side chain
        ProcessSamplesBuffer(&mResultTmpChunk, NULL);
    else
        ProcessSamplesBuffer(&copySamplesBuf, &mResultTmpChunk);
#endif
    
    // Fill the result
    double *result = mResultChunk.Get();

    // Standard loop count
    int loopCount = mBufSize;
    
#if ADD_TAIL
    if (mFreqRes > 1)
        // Grater size
        loopCount = copySamplesBuf.GetSize();
#endif
    
    // Apply synthesis window
    ApplySynthesisWindow(&mResultTmpChunk);
    
    // Make the sum (overlap)
    for (int i = 0; i < loopCount; i++)
    {
        result[i + mBufOffset] += mResultTmpChunk.Get()[i];
    }
}

void
FftProcessObj9::NextSamplesBuffer(WDL_TypedBuf<double> *samples)
{
    // Reduce the size by mOverlap
    int size = samples->GetSize();
    if (size == mBufSize)
        samples->Resize(0);
    else
        if (size > mBufSize)
        {
            int newSize = size - mBufSize;
            
            // Resize down, skipping left
            WDL_TypedBuf<double> tmpChunk;
            tmpChunk.Add(&samples->Get()[mBufSize], newSize);
            *samples = tmpChunk;
        }
}

void
FftProcessObj9::NextOutBuffer()
{
    // Reduce the size by mOverlap
    int size = mResultChunk.GetSize();
    
    int minSize = mBufSize*2*mFreqRes;

    if (size < minSize)
        return;
    
    double *resultBuf = mResultChunk.Get();
    
    WDL_TypedBuf<double> newBuffer;
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
            WDL_TypedBuf<double> tmpChunk;
            tmpChunk.Add(&mResultChunk.Get()[mBufSize], newSize);
            mResultChunk = tmpChunk;
        }
    
    // Grow the output with zeros
    double *zeros = (double *)malloc(mBufSize*sizeof(double));
    for (int i = 0; i < mBufSize; i++)
        zeros[i] = 0.0;
    mResultChunk.Add(zeros, mBufSize);
    free(zeros);
}

void
FftProcessObj9::GetResultOutBuffer(double *output, int nFrames)
{
    // Should not happen
    int size = mResultOutChunk.GetSize()/mFreqRes;
    if (size < nFrames)
        return;
    
    // Copy the result
    memcpy(output, mResultOutChunk.Get(), nFrames*sizeof(double));
    
    // Consume the returned samples
    int newSize = mResultOutChunk.GetSize() - nFrames;
    
    // Resize down, skipping left
    WDL_TypedBuf<double> tmpChunk;
    tmpChunk.Add(&mResultOutChunk.Get()[nFrames], newSize);
    mResultOutChunk = tmpChunk;
}

bool
FftProcessObj9::Shift()
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
FftProcessObj9::Process(double *input, double *output,
                        double *inSc, int nFrames)
{
#if DEBUG_WINDOWS
    for (int i = 0; i < nFrames; i++)
        input[i] = 1.0;
#endif
    
    // Add the new input data
    AddSamples(input, inSc, nFrames);
    
    if (mMustPadSamples)
    {
        Utils::PadZerosLeft(&mSamplesChunk, mAnalysisWindow.GetSize());
        
        if (mScChunk.GetSize() > 0)
            Utils::PadZerosLeft(&mScChunk, mAnalysisWindow.GetSize());
        
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
        
        NextSamplesBuffer(&mSamplesChunk);
        NextSamplesBuffer(&mScChunk);
        
        NextOutBuffer();
        
        numSamples = mSamplesChunk.GetSize();
    }
    
    if (mResultOutChunk.GetSize() >= mAnalysisWindow.GetSize()*1.5)
    {
        if (mMustUnpadResult)
        {
            Utils::ConsumeLeft(&mResultOutChunk, mAnalysisWindow.GetSize());
            
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
FftProcessObj9::NormalizeFftValues(WDL_TypedBuf<double> *magns)
{
    double sum = 0.0f;
    
    for (int i = 1; i < magns->GetSize()/2; i++)
    {
        double magn = magns->Get()[i];
        
        sum += magn;
    }
    
    sum /= magns->GetSize()/2 - 1;
    
    magns->Get()[0] = sum;
    magns->Get()[magns->GetSize() - 1] = sum;
}

void
FftProcessObj9::FillSecondHalf(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer)
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
FftProcessObj9::ApplyAnalysisWindow(WDL_TypedBuf<double> *samples)
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
FftProcessObj9::ApplySynthesisWindow(WDL_TypedBuf<double> *samples)
{
    // Multiply the samples by analysis window
    // The window can be identity if necessary
    // (no synthesis window, or no overlap)
    for (int i = 0; i < samples->GetSize(); i++)
    {
        double winCoeff = mSynthesisWindow.Get()[i];
        samples->Get()[i] *= winCoeff;
    }
}


void
FftProcessObj9::MakeWindows()
{
    // Analysis and synthesis Hann windows
    // With Hanning, the gain is not increased when we increase the overlapping
    
    int windowSize = mBufSize;

#if ZERO_PAD_WINDOW_REORDER
    windowSize = mBufSize*mFreqRes;
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
    if (mOverlapping > 1)
    {
        if ((mAnalysisMethod == AnalysisMethodWindow) || (mSynthesisMethod == SynthesisMethodWindow))
            // We happly windows twice (before, and after)
            hanningFactor = mOverlapping/4.0;
        else
            // We apply only one window, so we can narrow more
            hanningFactor = mOverlapping/2.0;
    }
    
    // Analysis
    if ((mAnalysisMethod == AnalysisMethodWindow) && (mOverlapping > 1))
        Window::MakeHanningPow(windowSize, hanningFactor, &mAnalysisWindow);
    else
        Window::MakeSquare(windowSize, 1.0, &mAnalysisWindow);
    
#if DEBUG_WINDOWS
    double cola0 = Window::CheckCOLA(&mAnalysisWindow, mOverlapping);
   fprintf(stderr, "ANALYSIS - over: %d cola: %g\n", mOverlapping, cola0);
#endif
    
    // Synthesis
    if ((mSynthesisMethod == SynthesisMethodWindow) && (mOverlapping > 1))
        Window::MakeHanningPow(windowSize, hanningFactor, &mSynthesisWindow);
    else
        Window::MakeSquare(windowSize, 1.0, &mSynthesisWindow);
    
#if DEBUG_WINDOWS
    double cola1 = Window::CheckCOLA(&mSynthesisWindow, mOverlapping);
    fprintf(stderr, "SYNTHESIS - over: %d cola: %g\n", mOverlapping, cola1);
    fprintf(stderr, "\n");
#endif
    
    // Normalize
    if ((mAnalysisMethod == AnalysisMethodWindow) &&
        (mSynthesisMethod == SynthesisMethodNone))
    {
        Window::NormalizeWindow(&mAnalysisWindow, mOverlapping);
    }
    else if((mAnalysisMethod == AnalysisMethodNone) &&
            (mSynthesisMethod == SynthesisMethodWindow))
    {
        Window::NormalizeWindow(&mSynthesisWindow, mOverlapping);
    }
    else if((mAnalysisMethod == AnalysisMethodWindow) &&
            (mSynthesisMethod == SynthesisMethodWindow))
    {
        // We have both windows
        // We must compute the cola of the next hanning factor,
        // then normalize with it
        //
        
        WDL_TypedBuf<double> win;
        Window::MakeHanningPow(mAnalysisWindow.GetSize(), hanningFactor*2, &win);
        
        double cola = Window::CheckCOLA(&win, mOverlapping);
        
        // Normalize only the synthesis window
        Window::NormalizeWindow(&mSynthesisWindow, cola);
    }
    else if ((mAnalysisMethod == AnalysisMethodNone) &&
             (mSynthesisMethod == SynthesisMethodNone))
    {
        // Normalize only the synthesis window...
        Window::NormalizeWindow(&mSynthesisWindow, mOverlapping);
    }
}

void
FftProcessObj9::ComputeFft(const WDL_TypedBuf<double> &samples,
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
FftProcessObj9::ComputeInverseFft(const WDL_TypedBuf<WDL_FFT_COMPLEX> &fftSamples,
                                  WDL_TypedBuf<double> *samples,
                                  int freqRes)
{
    int bufSize = fftSamples.GetSize();
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> tmpFftBuf;
    tmpFftBuf.Resize(bufSize);

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
