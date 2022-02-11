//
//  FftProcessObj.cpp
//  Transient
//
//  Created by Apple m'a Tuer on 21/05/17.
//
//

#include <Window.h>
#include <Debug.h>

#include "FftProcessObj4.h"


FftProcessObj4::FftProcessObj4(int bufferSize, int oversampling, bool normalize,
                               enum AnalysisMethod aMethod,
                               enum SynthesisMethod sMethod)
{
    mBufSize = bufferSize;
    mOversampling = oversampling;
    mShift = mBufSize/mOversampling;
    mOverlap = mBufSize - mShift;
    
    mNormalize = normalize;
    mAnalysisMethod = aMethod;
    mSynthesisMethod = sMethod;
    
    // Set the buffers.
    int resultSize = 2*mBufSize;
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
    Window::MakeRootHanning2(mBufSize, &mAnalysisWindow);
    Window::MakeRootHanning2(mBufSize, &mSynthesisWindow);
    
    mFftBuf.Resize(mBufSize);
    mSortedFftBuf.Resize(mBufSize);
    mResultTmpChunk.Resize(mBufSize);
    
    // Comppute energy factor
    // Usefull to keep the same volume even if we have oversampling > 2 with Hanning
    mAnalysisWinFactor = ComputeWinFactor(mAnalysisWindow);
    mSynthesisWinFactor = ComputeWinFactor(mSynthesisWindow);
}

FftProcessObj4::~FftProcessObj4() {}

void
FftProcessObj4::AddSamples(double *samples, int numSamples)
{
    mSamplesChunk.Add(samples, numSamples);
}

void
FftProcessObj4::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer) {}

void
FftProcessObj4::ProcessSamplesBuffer(WDL_TypedBuf<double> *ioBuffer) {}

void
FftProcessObj4::ProcessOneBuffer()
{
    // Get samples
    WDL_TypedBuf<double> copySamplesBuf(mSamplesChunk);
    double *samples = copySamplesBuf.Get();
    
    if (mAnalysisMethod == AnalysisMethodWindow)
    {
        // Windowing
        if (mOverlap > 0) // then should be half the buffer size
        {
            // Multiply the samples by Hanning window
            for (int i = 0; i < mBufSize; i++)
            {
                samples[i + mBufOffset] *= mAnalysisWindow.Get()[i];
                
                samples[i + mBufOffset] /= mAnalysisWinFactor;
            }
        }
    }
    
    // fft
    
    double coeff = 1.0;
    
    if (mNormalize)
        coeff = 1.0/mBufSize;
    
    // Fill the fft buf
    for (int i = 0; i < mBufSize; i++)
    {
        // No need to divide by mBufSize if we ponderate analysis hanning window
        mFftBuf.Get()[i].re = samples[i + mBufOffset]*coeff;
        mFftBuf.Get()[i].im = 0.0;
    }
    
    // Do the fft
    // Do it on the window but also in following the empty space, to capture remaining waves
    WDL_fft(mFftBuf.Get(), mBufSize, false);
    
    // Sort the fft buffer
    for (int i = 0; i < mBufSize; i++)
    {
        int k = WDL_fft_permute(mBufSize, i);
        
        mSortedFftBuf.Get()[i].re = mFftBuf.Get()[k].re;
        mSortedFftBuf.Get()[i].im = mFftBuf.Get()[k].im;
    }
    
    // Apply modifications of the buffer
    ProcessFftBuffer(&mSortedFftBuf);
    
    for (int i = 0; i < mBufSize; i++)
    {
        int k = WDL_fft_permute(mBufSize, i);
        mFftBuf.Get()[k].re = mSortedFftBuf.Get()[i].re;
        mFftBuf.Get()[k].im = mSortedFftBuf.Get()[i].im;
    }
    
    // Should not do this step when not necessary (for example for transients)
    
    // Do the ifft
    WDL_fft(mFftBuf.Get(), mBufSize, true);
        
    for (int i = 0; i < mBufSize; i++)
        mResultTmpChunk.Get()[i] = mFftBuf.Get()[i].re;
    
    ProcessSamplesBuffer(&mResultTmpChunk);
    
    // Fill the result
    double *result = mResultChunk.Get();
    
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
        
        result[i + mBufOffset] += winCoeff*mResultTmpChunk.Get()[i];
    }
}

void
FftProcessObj4::NextSamplesBuffer()
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
FftProcessObj4::NextOutBuffer()
{
    // Reduce the size by mOverlap
    int size = mResultChunk.GetSize();
    if (size < mBufSize*2)
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
FftProcessObj4::GetResultOutBuffer(double *output, int nFrames)
{
    // Should not happen
    if (mResultOutChunk.GetSize() < nFrames)
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
FftProcessObj4::Shift()
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
FftProcessObj4::ComputeWinFactor(const WDL_TypedBuf<double> &window)
{
    if (mOversampling == 1)
        // No windowing
        // We multiply two times, and we take the square root (??!!)
        //return 0.25;
        //return 0.5;
        return 1.0;
    
    double factor = 0.0;
    
    // Sum the values of the synthesis windows on the interfval mBufSize
    // For oversampling 2 and Hanning, this should be 1.0
    // Take care, we are using ROOT Hanning
    int start = 0;
    int numValues = 0;
    //while(start < mBufSize)
    {
        for (int i = 0; i < mSynthesisWindow.GetSize(); i++)
        {
            double winCoeff = window.Get()[i];
            
            factor += winCoeff;
            numValues++;
        }
        
    //    start += mShift;
    }
    
    if (numValues > 0)
        factor /= numValues;
    
    return factor;
}

bool
FftProcessObj4::Process(double *input, double *output, int nFrames)
{
    // Process the data
    
    // Get new data
    AddSamples(input, nFrames);
    
    int numSamples = mSamplesChunk.GetSize();
    int numOutSamples = mResultOutChunk.GetSize();
    
    if (numSamples >= 2.0*mBufSize - mShift)
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
    
    if (nFrames <= numOutSamples)
    {
        if (output != NULL)
            GetResultOutBuffer(output, nFrames);
        
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
FftProcessObj4::NormalizeFftValues(WDL_TypedBuf<double> *magns)
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
FftProcessObj4::FillSecondHalf(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer)
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
