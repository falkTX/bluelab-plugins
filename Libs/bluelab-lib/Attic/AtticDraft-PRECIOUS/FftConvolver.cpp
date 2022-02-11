//
//  FftConvolver.cpp
//  Spatializer
//
//  Created by Pan on 20/11/17.
//
//

#include <Window.h>
#include <Utils.h>
#include <Debug.h>

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
    
    mResultBuf.Resize(mBufSize + mShift);
    
    mResultOutBuf.Resize(0);
}

void
FftConvolver::SetResponse(const WDL_TypedBuf<double> *response)
{
    mResponse = *response;
}

void
FftConvolver::AddSamples(double *samples, int numSamples)
{
    mSamplesBuf.Add(samples, numSamples);
    
    if (mResultBuf.GetSize() == 0)
        // Just when init
    {
        mResultBuf.Resize(numSamples);
        Utils::FillAllZero(&mResultBuf);
    }
}

int
FftConvolver::ProcessOneBuffer()
{
    if (mSamplesBuf.GetSize() < mBufSize + mBufOffset)
        // We don't have enough samples
        return 0;

    WDL_TypedBuf<double> buf;
    buf.Resize(mBufSize);
    for (int i = 0; i < mBufSize; i++)
        buf.Get()[i] = mSamplesBuf.Get()[i + mBufOffset];
        
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
    ComputeFft(&buf, &fftBuf);
    
    // Apply modifications of the buffer
    //ProcessFftBuffer(&fftBuf);
    
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
        double winCoeff = 1.0;
        
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
        double oversampCoeff = 1.0/(mOversampling/4.0);
        
        mResultBuf.Get()[i + mBufOffset] += oversampCoeff*winCoeff*buf.Get()[i];
    }
    
    return mBufSize;
}

void
FftConvolver::NextSamplesBuffer()
{
    // Throw away old samples
    int size = mSamplesBuf.GetSize();
    if (size >= mBufSize + mBufOffset) // ??
    {
        int newSize = size - mShift;
        
        // Resize down, skipping left
        WDL_TypedBuf<double> tmpChunk;
        tmpChunk.Add(&mSamplesBuf.Get()[mShift], newSize);
        mSamplesBuf = tmpChunk;
    }
}

void
FftConvolver::NextOutBuffer()
{
    int size = mResultBuf.GetSize();
    
    if (size < mShift)
        // Should never happen
        return;
        
    for (int i = 0; i < mShift; i++)
        mResultOutBuf.Add(mResultBuf.Get()[i]);
    
    int newSize = size - mShift;
            
    // Resize down, skipping left
    WDL_TypedBuf<double> tmpChunk;
    tmpChunk.Add(&mResultBuf.Get()[mShift], newSize);
    mResultBuf = tmpChunk;
        
    // Fill the rest with zeros
    mResultBuf.Resize(size);
    for (int i = newSize; i < size; i++)
        mResultBuf.Add(0.0);
}

void
FftConvolver::Shift()
{
    mBufOffset += mShift;
    
    if (mBufOffset >= mBufSize)
        mBufOffset = 0;
}

bool
FftConvolver::GetResultOutBuffer(double *output, int nFrames)
{
    printf("mResultOutBuf.GetSize(): %d\n", mResultOutBuf.GetSize());
    
    // We wait for having nFrames samples ready
    if (mResultOutBuf.GetSize() < nFrames)
    {
        memset(output, 0, nFrames*sizeof(double));
        
        return false;
    }
    
    // Copy the result
    memcpy(output, mResultOutBuf.Get(), nFrames*sizeof(double));
    
    // Consume the returned samples
    int newSize = mResultOutBuf.GetSize() - nFrames;
    
    // Resize down, skipping left
    WDL_TypedBuf<double> tmpChunk;
    tmpChunk.Add(&mResultOutBuf.Get()[nFrames], newSize);
    mResultOutBuf = tmpChunk;
    
    return true;
}

double
FftConvolver::ComputeWinFactor(const WDL_TypedBuf<double> &window)
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
FftConvolver::Process(double *input, double *output, int nFrames)
{
    // Process the data
    
    // Get new data
    AddSamples(input, nFrames);
 
    int numToProcess = nFrames;
    printf("numToProcess before: %d\n", numToProcess);
    
    do
    {
        int numProcessed = ProcessOneBuffer();
        
        if (numProcessed <= 0)
            break;
        
        numToProcess -= numProcessed;
        
        NextSamplesBuffer();
        NextOutBuffer();
        
        Shift();
        
    } while(numToProcess > 0);
    
    printf("numToProcess after: %d\n", numToProcess);
    
    bool res = GetResultOutBuffer(output, nFrames);
    printf("res: %d\n", res);
    
    Debug::DumpData("output.txt", output, nFrames);
    
    return res;
}

void
FftConvolver::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer)
{
    if (mResponse.GetSize() != ioBuffer->GetSize())
        return;
    
    for (int i = 0; i < ioBuffer->GetSize(); i++)
    {
        WDL_FFT_COMPLEX sigComp = ioBuffer->Get()[i];
        WDL_FFT_COMPLEX respComp;
        
        respComp.re = mResponse.Get()[i];
        respComp.im = 0.0;
        
        // Pointwise multiplication of two complex
        WDL_FFT_COMPLEX res;
        res.re = sigComp.re*respComp.re - sigComp.im*respComp.im;
        res.im = sigComp.re*sigComp.im + respComp.re*respComp.im;
        
        ioBuffer->Get()[i] = res;
    }
}

void
FftConvolver::NormalizeFftValues(WDL_TypedBuf<double> *magns)
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
FftConvolver::ComputeFft(const WDL_TypedBuf<double> *samples,
                         WDL_TypedBuf<WDL_FFT_COMPLEX> *fftSamples)
{
    if (fftSamples->GetSize() != samples->GetSize())
        fftSamples->Resize(samples->GetSize());
    
    double normCoeff = 1.0;
    if (mNormalize)
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
FftConvolver::ComputeInverseFft(const WDL_TypedBuf<WDL_FFT_COMPLEX> *fftSamples,
                                WDL_TypedBuf<double> *samples)
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

    // TODO: suppress this
    double normCoeff = 1.0;
    //if (mNormalize)
    //    normCoeff = 1.0/mBufSize;
    
    for (int i = 0; i < mBufSize; i++)
        samples->Get()[i] = normCoeff*fftSamples->Get()[i].re;
}
