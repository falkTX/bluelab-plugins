//
//  FftProcessObj10.cpp
//  Transient
//
//  Created by Apple m'a Tuer on 21/05/17.
//
//

#include <Window.h>
#include <BLUtils.h>

#include "FftProcessObj10.h"


#define NORMALIZE_FFT 1

// WARNING: this is buggy and false, need some fix and reconsidered if want to use it
// When using freq res > 1, add the tail of the fft to the future samples
// (it seems to add aliasing when activated)
#define ADD_TAIL 0

// Useful for debuging
#define DEBUG_DISABLE_OVERLAPPING 0
#define DEBUG_DISABLE_PROCESS 0
#define DEBUG_WINDOWS 0

// Compute "energy" just after analysis windowing
// then just after synthesis windowing
// Compute and apply a coefficient to avoid loosing energy
#define USE_NORM_SYNTHESIS 0

// NOTE: With freqRes == 2, and overlapping = 1 => better
// the transient signal is decreased / the other signal stays constant
// => that's what we want !
// With previous version, the other signal increased
FftProcessObj10::FftProcessObj10(int bufferSize, int overlapping, int freqRes,
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

FftProcessObj10::~FftProcessObj10() {}

void
FftProcessObj10::Reset(int overlapping, int freqRes)
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
    int resultSize = 2*mBufSize*mFreqRes;
    BLUtils::ResizeFillZeros(&mResultChunk, resultSize);
    
    mBufOffset = 0;
    
    int bufSize = mBufSize*mFreqRes;
    mSortedFftBuf.Resize(bufSize);
    mResultTmpChunk.Resize(bufSize);
    
    mSamplesChunk.Resize(0);
    mScChunk.Resize(0);
    
    mResultOutChunk.Resize(0);
    
    mMustPadSamples = true;
    mMustUnpadResult = false;
}

void
FftProcessObj10::AddSamples(BL_FLOAT *samples, BL_FLOAT *scSamples, int numSamples)
{
    mSamplesChunk.Add(samples, numSamples);
    
    if (scSamples != NULL)
        mScChunk.Add(scSamples, numSamples);
}

void
FftProcessObj10::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer) {}

void
FftProcessObj10::ProcessSamplesBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer,
                                     WDL_TypedBuf<BL_FLOAT> *scBuffer) {}

void
FftProcessObj10::SamplesToFftMagns(const WDL_TypedBuf<BL_FLOAT> &inSamples,
                                   WDL_TypedBuf<BL_FLOAT> *outFftMagns)
{
    int numSamples = inSamples.GetSize();
    int numPaddedSamples = BLUtils::NextPowerOfTwo(numSamples);
    
    WDL_TypedBuf<BL_FLOAT> paddedSamples = inSamples;
    BLUtils::ResizeFillZeros(&paddedSamples, numPaddedSamples);
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> fftSamples;
    int freqRes = 1;
    ComputeFft(paddedSamples, &fftSamples, freqRes);
    
    BLUtils::ComplexToMagn(outFftMagns, fftSamples);
}

void
FftProcessObj10::ProcessOneBuffer()
{
    WDL_TypedBuf<BL_FLOAT> copySamplesBuf;
    copySamplesBuf.Resize(mBufSize);
    for (int i = 0; i < mBufSize; i++)
        copySamplesBuf.Get()[i] = mSamplesChunk.Get()[i + mBufOffset];
    
    WDL_TypedBuf<BL_FLOAT> copyScBuf;
    if (mScChunk.GetSize() > 0)
    {
        copyScBuf.Resize(mBufSize);
        for (int i = 0; i < mBufSize; i++)
            copyScBuf.Get()[i] = mScChunk.Get()[i + mBufOffset];
    }
    
    // Apply analysis windows before resizing !
    ApplyAnalysisWindow(&copySamplesBuf);
    
#if USE_NORM_SYNTHESIS
    BL_FLOAT energy0 = BLUtils::ComputeAbsAvg(copySamplesBuf);
#endif
    
    // windowing before zero padding
    // See: http://www.bitweenie.com/listings/fft-zero-padding/
    // (last links of the page)
    if (mFreqRes > 1)
    {
        // Non-cyclic technique, to avoid aliasing
        BLUtils::ResizeFillZeros(&copySamplesBuf, copySamplesBuf.GetSize()*mFreqRes);
        
        if (mScChunk.GetSize() > 0)
            BLUtils::ResizeFillZeros(&copyScBuf, copyScBuf.GetSize()*mFreqRes);
    }
    
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
    BL_FLOAT *result = mResultChunk.Get();
    
    // Apply synthesis window
    ApplySynthesisWindow(&mResultTmpChunk);
    
#if USE_NORM_SYNTHESIS
    BL_FLOAT energy1 = BLUtils::ComputeAbsAvg(mResultTmpChunk);
    
    BL_FLOAT energyCoeff = 1.0;
    if (energy1 > 0)
        energyCoeff = energy0/energy1;

    BL_FLOAT div = std::log(mOverlapping)/std::log(2.0);
    
    energyCoeff /= div;
    
    BLUtils::MultValues(&mResultTmpChunk, energyCoeff);
#endif
    
#if !ADD_TAIL
    // Standard size
    int loopCount = mBufSize;
#else
    // Size to add tail
    int loopCount = copySamplesBuf.GetSize();
#endif
    
    // Make the sum (overlap)
    for (int i = 0; i < loopCount; i++)
    {
        result[i + mBufOffset] += mResultTmpChunk.Get()[i];
    }
}

void
FftProcessObj10::NextSamplesBuffer(WDL_TypedBuf<BL_FLOAT> *samples)
{
    // Reduce the size by mOverlap
    int size = samples->GetSize();
    if (size == mBufSize)
        samples->Resize(0);
    else if (size > mBufSize)
    {
        int newSize = size - mBufSize;
            
        // Resize down, skipping left
        WDL_TypedBuf<BL_FLOAT> tmpChunk;
        tmpChunk.Add(&samples->Get()[mBufSize], newSize);
        *samples = tmpChunk;
    }
}

void
FftProcessObj10::NextOutBuffer()
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
    else if (size > mBufSize)
    {
        int newSize = size - mBufSize;
            
        // Resize down, skipping left
        WDL_TypedBuf<BL_FLOAT> tmpChunk;
        tmpChunk.Add(&mResultChunk.Get()[mBufSize], newSize);
        mResultChunk = tmpChunk;
    }
    
    // Grow the output with zeros
    BLUtils::ResizeFillZeros(&mResultChunk,
                           mResultChunk.GetSize() + mBufSize);
}

void
FftProcessObj10::GetResultOutBuffer(BL_FLOAT *output, int nFrames)
{
    int size = mResultOutChunk.GetSize();
 
    // Should not happen
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
FftProcessObj10::Shift()
{
    mBufOffset += mShift;
    
    if (mBufOffset >= mBufSize)
    {
        mBufOffset = 0;
        
        return true;
    }
    
    return false;
}

bool
FftProcessObj10::Process(BL_FLOAT *input, BL_FLOAT *output,
                         BL_FLOAT *inSc, int nFrames)
{
#if DEBUG_WINDOWS
    for (int i = 0; i < nFrames; i++)
        input[i] = 1.0;
#endif
    
    // Add the new input data
    AddSamples(input, inSc, nFrames);
    
    if (mMustPadSamples)
    {
        BLUtils::PadZerosLeft(&mSamplesChunk, mAnalysisWindow.GetSize());
        
        if (mScChunk.GetSize() > 0)
            BLUtils::PadZerosLeft(&mScChunk, mAnalysisWindow.GetSize());
        
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
FftProcessObj10::NormalizeFftValues(WDL_TypedBuf<BL_FLOAT> *magns)
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
FftProcessObj10::FillSecondHalf(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer)
{
    if (ioBuffer->GetSize() < 2)
        return;
    
    // It is important that the "middle value" (ie index 1023) is duplicated
    // to index 1024. So we have twice the center value
    for (int i = 1; i < ioBuffer->GetSize()/2; i++)
    {
        int id0 = i + ioBuffer->GetSize()/2;
        int id1 = ioBuffer->GetSize()/2 - i;
        
        ioBuffer->Get()[id0].re = ioBuffer->Get()[id1].re;
        
        // Complex conjugate
        ioBuffer->Get()[id0].im = -ioBuffer->Get()[id1].im;
    }
}

void
FftProcessObj10::ApplyAnalysisWindow(WDL_TypedBuf<BL_FLOAT> *samples)
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
FftProcessObj10::ApplySynthesisWindow(WDL_TypedBuf<BL_FLOAT> *samples)
{
    // Multiply the samples by analysis window
    // The window can be identity if necessary
    // (no synthesis window, or no overlap)
    for (int i = 0; i < mSynthesisWindow.GetSize(); i++)
    {
        BL_FLOAT winCoeff = mSynthesisWindow.Get()[i];
        samples->Get()[i] *= winCoeff;
    }
}

void
FftProcessObj10::ApplySynthesisWindowNorm(WDL_TypedBuf<BL_FLOAT> *samples)
{
    BL_FLOAT lostSum = 0.0;
    BL_FLOAT keepSum = 0.0;
    
    // Apply window and compute keep and lost sums
    for (int i = 0; i < mSynthesisWindow.GetSize(); i++)
    {
        BL_FLOAT winCoeff = mSynthesisWindow.Get()[i];
        BL_FLOAT sample = samples->Get()[i];
        
        keepSum += winCoeff*fabs(sample);
        lostSum += (1.0 - winCoeff)*fabs(sample);
        
        sample *= winCoeff;
        
        samples->Get()[i] = sample;
    }
    
    BL_FLOAT coeff = 1.0;
    if (keepSum > 0.0)
        coeff = (keepSum + lostSum)/keepSum;
    
    // WHY ?
    coeff *= 0.5;
    
    for (int i = 0; i < mSynthesisWindow.GetSize(); i++)
    {
        BL_FLOAT sample = samples->Get()[i];
        
        sample *= coeff;
        
        samples->Get()[i] = sample;
    }
}

void
FftProcessObj10::MakeWindows()
{
    // Analysis and synthesis Hann windows
    // With Hanning, the gain is not increased when we increase the overlapping
    
    int anaWindowSize = mBufSize;
    
    int synWindowSize = mBufSize;
    
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
    BL_FLOAT hanningFactor = 1.0;
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
        Window::MakeHanningPow(anaWindowSize, hanningFactor, &mAnalysisWindow);
    else
        Window::MakeSquare(anaWindowSize, 1.0, &mAnalysisWindow);
    
#if DEBUG_WINDOWS
    BL_FLOAT cola0 = Window::CheckCOLA(&mAnalysisWindow, mOverlapping);
   fprintf(stderr, "ANALYSIS - over: %d cola: %g\n", mOverlapping, cola0);
#endif
    
    // Synthesis
    if ((mSynthesisMethod == SynthesisMethodWindow) && (mOverlapping > 1))
        Window::MakeHanningPow(synWindowSize, hanningFactor, &mSynthesisWindow);
    else
        Window::MakeSquare(synWindowSize, 1.0, &mSynthesisWindow);
    
#if DEBUG_WINDOWS
    BL_FLOAT cola1 = Window::CheckCOLA(&mSynthesisWindow, mOverlapping);
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
        
        WDL_TypedBuf<BL_FLOAT> win;
        Window::MakeHanningPow(mAnalysisWindow.GetSize(), hanningFactor*2, &win);
        
        BL_FLOAT cola = Window::CheckCOLA(&win, mOverlapping);
        
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
FftProcessObj10::ComputeFft(const WDL_TypedBuf<BL_FLOAT> &samples,
                            WDL_TypedBuf<WDL_FFT_COMPLEX> *fftSamples,
                            int freqRes)
{
    int bufSize = samples.GetSize();
    
    WDL_TypedBuf<WDL_FFT_COMPLEX> tmpFftBuf;
    tmpFftBuf.Resize(bufSize);

    fftSamples->Resize(bufSize);
    
    BL_FLOAT coeff = 1.0;
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
FftProcessObj10::ComputeInverseFft(const WDL_TypedBuf<WDL_FFT_COMPLEX> &fftSamples,
                                   WDL_TypedBuf<BL_FLOAT> *samples,
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
    BL_FLOAT coeff = 1.0;
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
