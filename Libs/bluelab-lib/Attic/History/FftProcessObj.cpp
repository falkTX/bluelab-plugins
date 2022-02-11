//
//  FftProcessObj.cpp
//  Transient
//
//  Created by Apple m'a Tuer on 21/05/17.
//
//

#include <Window.h>

#include "FftProcessObj.h"

FftProcessObj::FftProcessObj(int bufferSize, bool normalize,
                             enum AnalysisMethod aMethod,
                             enum SynthesisMethod sMethod)
{
    mBufSize = bufferSize;
    mOverlap = mBufSize*0.5;
    
    mNormalize = normalize;
    mAnalysisMethod = aMethod;
    mSynthesisMethod = sMethod;
    
    // Set the buffers.
    BL_FLOAT *zeros = (BL_FLOAT *)malloc(mBufSize*3*sizeof(BL_FLOAT));
    for (int i = 0; i < mBufSize*3; i++)
        zeros[i] = 0.0;
    
    mBufFftChunk.Add(zeros, mBufSize);
    mBufIfftChunk.Add(zeros, mBufSize);
    // Keep one more mBufferSize, to store remaining fft waves
    mResultChunk.Add(zeros, mBufSize*2 + mOverlap*2);
    
    free(zeros);
    
    mBufOffset = 0;
    
    // Hann Window
    Window::MakeHanning(mBufSize, &mAnalysisWindow);
    Window::MakeRoot2Hanning(mBufSize, &mSynthesisWindow);
    
#if 0
    // Use twice the buffer size, to capture remaining waves outside the window
    mFftBuf.Resize(mBufSize*2);
    mSortedFftBuf.Resize(mBufSize*2);
    mResultTmpChunk.Resize(mBufSize*2);
#endif
    
    mFftBuf.Resize(mBufSize);
    mSortedFftBuf.Resize(mBufSize);
    mResultTmpChunk.Resize(mBufSize);
}

FftProcessObj::~FftProcessObj() {}

void
FftProcessObj::AddSamples(BL_FLOAT *samples, int numSamples)
{
    mSamplesChunk.Add(samples, numSamples);
}

void
FftProcessObj::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer) {}

void
FftProcessObj::ProcessSamplesBuffer(WDL_TypedBuf<BL_FLOAT> *ioBuffer) {}

void
FftProcessObj::ProcessOneBuffer()
{
    // Get samples
    WDL_TypedBuf<BL_FLOAT> copySamplesBuf(mSamplesChunk);
    BL_FLOAT *samples = copySamplesBuf.Get();
    
    if (mAnalysisMethod == AnalysisMethodWindow)
    {
        // Windowing
        if (mOverlap > 0) // then should be half the buffer size
        {
            // Multiply the samples by Hanning window
            for (int i = 0; i < mBufSize; i++)
            samples[i + mBufOffset] *= mAnalysisWindow.Get()[i];
        }
    }
    
    // fft
    
    BL_FLOAT coeff = 1.0;
    if (mNormalize)
        coeff = 1.0/mBufSize;
    
    // Fill the fft buf
    for (int i = 0; i < mBufSize; i++)
    {
        // No need to divide by mBufSize if we ponderate analysis hanning window
        mFftBuf.Get()[i].re = samples[i + mBufOffset]*coeff;
        mFftBuf.Get()[i].im = 0.0;
    }
    
    // Fill the rest with zeros
    for (int i = mBufSize; i < mBufSize/**2*/; i++)
    {
        mFftBuf.Get()[i].re = 0.0;
        mFftBuf.Get()[i].im = 0.0;
    }
    
    // Do the fft
    // Do it on the window but also in following the empty space, to capture remaining waves
    WDL_fft(mFftBuf.Get(), mBufSize, false); //*2
    
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
    WDL_fft(mFftBuf.Get(), mBufSize/**2*/, true);
        
    for (int i = 0; i < mBufSize/**2*/; i++)
        mResultTmpChunk.Get()[i] = mFftBuf.Get()[i].re;
    
    ProcessSamplesBuffer(&mResultTmpChunk);
    
    // Fill the result
    BL_FLOAT *result = mResultChunk.Get();
    
    // Multiply by Hanning root, to avoid "clics", i.e breaks in temporal domain
    // which make blue vertical lines in spectrogram
    // NO NEED !
        
    // We must use the same window for synthesis than for analysis, but for synthesis, we take the root
    // See: https://www.dsprelated.com/freebooks/sasp/overlap_add_ola_stft_processing.html
    // NO NEED !
        
    for (int i = 0; i < mBufSize/**2*/; i++)
        // Finally, no need to multiply by Hanning, because the data is already multiplied (at the beginning)
        // So simply overlap.
    {
        // Default value, for no window (SynthesisMethodNone)
        BL_FLOAT winCoeff = 1.0;
        
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
        
        result[i + mBufOffset] += winCoeff*mResultTmpChunk.Get()[i];
    }
}

void
FftProcessObj::NextSamplesBuffer()
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
FftProcessObj::NextOutBuffer()
{
    // Reduce the size by mOverlap
    int size = mResultChunk.GetSize();
    if (size < mBufSize*2)
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
FftProcessObj::GetResultOutBuffer(BL_FLOAT *output, int nFrames)
{
    // Should not happen
    if (mResultOutChunk.GetSize() < nFrames)
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
FftProcessObj::Shift()
{
    mBufOffset += mBufSize - mOverlap;
    
    if (mBufOffset >= mBufSize)
    {
        mBufOffset = 0;
        
        return true;
    }
    
    return false;
}

bool
FftProcessObj::Process(BL_FLOAT *input, BL_FLOAT *output, int nFrames)
{
    // Process the data
    
    // Get new data
    AddSamples(input, nFrames);
    
    int numSamples = mSamplesChunk.GetSize();
    int numOutSamples = mResultOutChunk.GetSize();
    
    if (numSamples >= mBufSize + mOverlap)
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
