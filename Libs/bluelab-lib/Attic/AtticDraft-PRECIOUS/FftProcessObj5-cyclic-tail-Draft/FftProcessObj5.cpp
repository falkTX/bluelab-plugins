//
//  FftProcessObj.cpp
//  Transient
//
//  Created by Apple m'a Tuer on 21/05/17.
//
//

#include <Window.h>
#include <Utils.h>
#include <Debug.h>

#include "FftProcessObj5.h"

// TODO: with NON_CYCLIC_PROCESSING, there can remain values in the second part of the buffer,
// after "reseting", so we must implement Reset() and fill the result buffer with zeros
//
// TODO: For windowing (analysis and synthesis), how do we implement NON_CYCLIC_PROCESSING ?


// We take care to avoid cyclic processing, by growing the
// buffers by 2 and padding with zeros
// And we sum also the second part of the buffers, which contains
// "future" sample contributions
// See: See: http://eeweb.poly.edu/iselesni/EL713/zoom/overlap.pdf
//
// Remark: in all the cases, with NON_CYCLIC_PROCESSING, we increase the resolution
// of the fft, so we decrease the aliasing.
#define NON_CYCLIC_PROCESSING 0
#define ADD_TAIL 0 // Adding tails seems to add aliasing

// Good with
#define DEBUG_DISABLE_OVERSAMPLING 0

#define DEBUG_DISABLE_PROCESS 0
#define DEBUG_FORCE_NO_WINDOW 0
#define DEBUG_EXT_WINDOWING 0 // Buggy

// NOTE: NON_CYCLIC_PROCESSING, DEBUG_DISABLE_OVERSAMPLING => better
// the transient signal is decreased / the other signal stays constant
// => that's what we want !
// With previous version, the other signal increased

FftProcessObj5::FftProcessObj5(int bufferSize, int oversampling, bool normalize,
                               enum AnalysisMethod aMethod,
                               enum SynthesisMethod sMethod, bool skipFFT)
{
    mBufSize = bufferSize;
    mOversampling = oversampling;
    
#if DEBUG_DISABLE_OVERSAMPLING
    mOversampling = 1;
#endif

    mShift = mBufSize/mOversampling;
    mOverlap = mBufSize - mShift;
    
    mNormalize = normalize;
    mAnalysisMethod = aMethod;
    mSynthesisMethod = sMethod;
    
    mSkipFFT = skipFFT;
    
    // Set the buffers.
#if !NON_CYCLIC_PROCESSING //
    int resultSize = 2*mBufSize;
#else
    int resultSize = 4*mBufSize;
#endif
    
    double *zeros = (double *)malloc(resultSize*sizeof(double));
    for (int i = 0; i < resultSize; i++)
        zeros[i] = 0.0;
    
    mBufFftChunk.Add(zeros, mBufSize);
    mBufIfftChunk.Add(zeros, mBufSize);
    
    mResultChunk.Add(zeros, resultSize);
    
    free(zeros);
    
    mBufOffset = 0;
    
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
    
#if !NON_CYCLIC_PROCESSING
    Window::MakeHanning(mBufSize, &mSynthesisWindow);
#else
    
#if DEBUG_EXT_WINDOWING
    Window::MakeHanning(mBufSize*2, &mSynthesisWindow);
#else
    Window::MakeHanning(mBufSize, &mSynthesisWindow);
#endif
    
    // We use a standard synthesis window size
    // For the remaining samples of the tail, do as
    // if it is the next step, and use the same window, but shifted
    //Window::MakeHanning(mBufSize, &mSynthesisWindow);
#endif
    
#if !NON_CYCLIC_PROCESSING
    mFftBuf.Resize(mBufSize);
    mSortedFftBuf.Resize(mBufSize);
    mResultTmpChunk.Resize(mBufSize);
#else
    mFftBuf.Resize(mBufSize*2);
    mSortedFftBuf.Resize(mBufSize*2);
    mResultTmpChunk.Resize(mBufSize*2);
#endif
    
    // Comppute energy factor
    // Useful to keep the same volume even if we have oversampling > 2 with Hanning
    mAnalysisWinFactor = ComputeWinFactor(mAnalysisWindow);
    mSynthesisWinFactor = ComputeWinFactor(mSynthesisWindow);
}

FftProcessObj5::~FftProcessObj5() {}

void
FftProcessObj5::AddSamples(double *samples, int numSamples)
{
    mSamplesChunk.Add(samples, numSamples);
}

void
FftProcessObj5::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer) {}

void
FftProcessObj5::ProcessSamplesBuffer(WDL_TypedBuf<double> *ioBuffer) {}

void
FftProcessObj5::ProcessOneBuffer()
{
    // Get samples
    //WDL_TypedBuf<double> copySamplesBuf(mSamplesChunk);
    
//#if NON_CYCLIC_PROCESSING
    // To avoid crash
    //copySamplesBuf.Resize(mBufSize);
//#endif
    
    WDL_TypedBuf<double> copySamplesBuf;
    copySamplesBuf.Resize(mBufSize);
    for (int i = 0; i < mBufSize; i++)
        copySamplesBuf.Get()[i] = mSamplesChunk.Get()[i + mBufOffset];
    
    double *samples = copySamplesBuf.Get();
    
#if !DEBUG_FORCE_NO_WINDOW
    //if (!mSkipFFT)
        // If we skip the fft, we multiply only once at the end by the overlap window
    {
        if (mAnalysisMethod == AnalysisMethodWindow)
        {
            // Windowing
            if (mOverlap > 0) // then should be half the buffer size
            {
                // Multiply the samples by Hanning window
                for (int i = 0; i < mBufSize; i++)
                {
                    samples[i /*+ mBufOffset*/] *= mAnalysisWindow.Get()[i];
                
                    samples[i /*+ mBufOffset*/] /= mAnalysisWinFactor;
                }
            }
        }
    }
#endif
    
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
#if NON_CYCLIC_PROCESSING
        // Non-cyclic technique, to avoid aliasing
        Utils::ResizeFillZeros(&copySamplesBuf, copySamplesBuf.GetSize()*2);
#endif
        
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
    double *result = mResultChunk.Get();
    
    // Multiply by Hanning root, to avoid "clics", i.e breaks in temporal domain
    // which make blue vertical lines in spectrogram
    // NO NEED !
    
    // We must use the same window for synthesis than for analysis, but for synthesis, we take the root
    // See: https://www.dsprelated.com/freebooks/sasp/overlap_add_ola_stft_processing.html
#if !NON_CYCLIC_PROCESSING
    for (int i = 0; i < mBufSize; i++)
#else
        
#if ADD_TAIL
    for (int i = 0; i < copySamplesBuf.GetSize(); i++)
#else
    for (int i = 0; i < mBufSize; i++)
#endif
        
#endif
        // Finally, no need to multiply by Hanning, because the data is already multiplied (at the beginning)
        // So simply overlap.
    {
        // Default value, for no window (SynthesisMethodNone)
        double winCoeff = 1.0;
        
#if !DEBUG_FORCE_NO_WINDOW
        if (!mSkipFFT)
        {
            if (mOverlap > 0)
            {
                int winIndex = i;
                
#if NON_CYCLIC_PROCESSING
                // We use a standard synthesis window size
                // For the remaining samples of the tail, do as
                // if it is the next step, and use the same window, but shifted
                winIndex = winIndex % mSynthesisWindow.GetSize();
#endif
                if (mSynthesisMethod == SynthesisMethodWindow)
                    winCoeff = mSynthesisWindow.Get()[winIndex];
                else
                    if (mSynthesisMethod == SynthesisMethodInverseWindow)
                    {
                        winCoeff = mAnalysisWindow.Get()[winIndex];
                            
                        winCoeff = 1.0 - winCoeff;
                
                        // Just in case, but should not happen with Hanning
                        if (winCoeff < 0.0)
                            winCoeff = 0.0;
                    }
            
                    winCoeff /= mSynthesisWinFactor;
            }
        }
#endif
        
        // TODO: avoid this hack
        
        // Avoid increasing the gain when increasing the oversampling
        double oversampCoeff = 1.0;
        double skipCoeff = 1.0;
        {
            if (mOversampling > 1)
            {
                oversampCoeff = 1.0/(mOversampling/4.0);
                
                // Hack
#if NON_CYCLIC_PROCESSING
                oversampCoeff *= 0.5;
#endif
            }
            
            // Hack
            double skipCoeff = 1.0;
            if (mSkipFFT)
                // We haven't done analysis...
                skipCoeff = 1.0/4.0;
        }
        
        result[i + mBufOffset] += skipCoeff*oversampCoeff*winCoeff*mResultTmpChunk.Get()[i];
    }
}

void
FftProcessObj5::NextSamplesBuffer()
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
            WDL_TypedBuf<double> tmpChunk;
            tmpChunk.Add(&mSamplesChunk.Get()[mBufSize], newSize);
            mSamplesChunk = tmpChunk;
        }
}

void
FftProcessObj5::NextOutBuffer()
{
    // Reduce the size by mOverlap
    int size = mResultChunk.GetSize();
    
#if !NON_CYCLIC_PROCESSING //
    if (size < mBufSize*2)
        return;
#else
    if (size < mBufSize*4)
        return;
#endif
    
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
        if (size > mBufSize) // TODO: cyclic modif ?
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
FftProcessObj5::GetResultOutBuffer(double *output, int nFrames)
{
    // Should not happen
#if !NON_CYCLIC_PROCESSING //
    if (mResultOutChunk.GetSize() < nFrames)
        return;
#else
    if (mResultOutChunk.GetSize()/2 < nFrames)
        return;
#endif
    
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
FftProcessObj5::Shift()
{
    mBufOffset += mShift;
    
    if (mBufOffset >= mBufSize)
    {
        mBufOffset = 0;
        
        return true;
    }
    
    return false;
}

double
FftProcessObj5::ComputeWinFactor(const WDL_TypedBuf<double> &window)
{
    if (mOversampling == 1)
        // No windowing
        return 1.0;
    
    double factor = 0.0;
    
    // Sum the values of the synthesis windows on the interfval mBufSize
    // For Hanning, this should be 1.0
    
    int start = 0;
    int numValues = 0;
    while(start < mBufSize)
    {
        for (int i = 0; i < window.GetSize(); i++)
        {
            double winCoeff = window.Get()[i];
            
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

bool
FftProcessObj5::Process(double *input, double *output, int nFrames)
{
    // Process the data
    
    // Get new data
    AddSamples(input, nFrames);
    
    int numSamples = mSamplesChunk.GetSize();
    int numOutSamples = mResultOutChunk.GetSize();
    
    if (numSamples >= 2*mBufSize - mShift)
    {
        bool stopFlag = false;
        while(!stopFlag)
        {
            ProcessOneBuffer();
            stopFlag = Shift();
        }
        
        NextSamplesBuffer();
        NextOutBuffer();
    }
    
#if !NON_CYCLIC_PROCESSING //
    if (nFrames <= numOutSamples)
#else
    //if (nFrames <= numOutSamples/2)
    if (nFrames <= numOutSamples)
#endif
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
FftProcessObj5::NormalizeFftValues(WDL_TypedBuf<double> *magns)
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
FftProcessObj5::FillSecondHalf(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer)
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
FftProcessObj5::ComputeFft(const WDL_TypedBuf<double> *samples,
                           WDL_TypedBuf<WDL_FFT_COMPLEX> *fftSamples)
{
    // fft
    double coeff = 1.0;

#if !NON_CYCLIC_PROCESSING
    if (mNormalize)
        coeff = 1.0/mBufSize;
    
    if (mTmpFftBuf.GetSize() != mBufSize)
        mTmpFftBuf.Resize(mBufSize);
#else
    if (mNormalize)
        coeff = 1.0/samples->GetSize();
    
    if (mTmpFftBuf.GetSize() != samples->GetSize())
        mTmpFftBuf.Resize(samples->GetSize());
#endif

    // Fill the fft buf
#if !NON_CYCLIC_PROCESSING
    for (int i = 0; i < mBufSize; i++)
#else
    for (int i = 0; i < samples->GetSize(); i++)
#endif
    {
        // No need to divide by mBufSize if we ponderate analysis hanning window
        mTmpFftBuf.Get()[i].re = samples->Get()[i /*+ mBufOffset*/]*coeff;
        mTmpFftBuf.Get()[i].im = 0.0;
    }

    // Do the fft
    // Do it on the window but also in following the empty space, to capture remaining waves
#if !NON_CYCLIC_PROCESSING
    WDL_fft(mTmpFftBuf.Get(), mBufSize, false);
#else
    WDL_fft(mTmpFftBuf.Get(), samples->GetSize(), false);
#endif
    
    // Sort the fft buffer
#if !NON_CYCLIC_PROCESSING
    for (int i = 0; i < mBufSize; i++)
#else
    for (int i = 0; i < samples->GetSize(); i++)
#endif
    {
#if !NON_CYCLIC_PROCESSING
        int k = WDL_fft_permute(mBufSize, i);
#else
        int k = WDL_fft_permute(samples->GetSize(), i);
#endif
        fftSamples->Get()[i].re = mTmpFftBuf.Get()[k].re;
        fftSamples->Get()[i].im = mTmpFftBuf.Get()[k].im;
    }
}

void
FftProcessObj5::ComputeInverseFft(const WDL_TypedBuf<WDL_FFT_COMPLEX> *fftSamples,
                                  WDL_TypedBuf<double> *samples)
{
#if !NON_CYCLIC_PROCESSING
    if (mTmpFftBuf.GetSize() != mBufSize)
        mTmpFftBuf.Resize(mBufSize);
#else
    if (mTmpFftBuf.GetSize() != fftSamples->GetSize())
        mTmpFftBuf.Resize(fftSamples->GetSize());
#endif

#if !NON_CYCLIC_PROCESSING
    for (int i = 0; i < mBufSize; i++)
#else
    for (int i = 0; i < fftSamples->GetSize(); i++)
#endif
    {
#if !NON_CYCLIC_PROCESSING
        int k = WDL_fft_permute(mBufSize, i);
#else
        int k = WDL_fft_permute(fftSamples->GetSize(), i);
#endif
        
        mTmpFftBuf.Get()[k].re = fftSamples->Get()[i].re;
        mTmpFftBuf.Get()[k].im = fftSamples->Get()[i].im;
    }
    
    // Should not do this step when not necessary (for example for transients)
    
    // Do the ifft
#if !NON_CYCLIC_PROCESSING
    WDL_fft(mTmpFftBuf.Get(), mBufSize, true);
#else
    WDL_fft(mTmpFftBuf.Get(), fftSamples->GetSize(), true);
#endif
    
#if !NON_CYCLIC_PROCESSING
    for (int i = 0; i < mBufSize; i++)
#else
    for (int i = 0; i < fftSamples->GetSize(); i++)
#endif
        samples->Get()[i] = mTmpFftBuf.Get()[i].re;
}
