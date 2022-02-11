//
//  FftProcessObj.cpp
//  Transient
//
//  Created by Apple m'a Tuer on 21/05/17.
//
//

#include <ffts.h>

#include <Window.h>
#include <Debug.h>

#include "FftProcessObj3.h"

FftProcessObj3::FftProcessObj3(int bufferSize, bool normalize,
                               enum AnalysisMethod aMethod,
                               enum SynthesisMethod sMethod)
{
    mBufSize = bufferSize;
    mOverlap = mBufSize*0.5;
    
    mNormalize = normalize;
    mAnalysisMethod = aMethod;
    mSynthesisMethod = sMethod;
    
    // Set the buffers.
    double *zeros = (double *)malloc(mBufSize*3*sizeof(double));
    for (int i = 0; i < mBufSize*3; i++)
        zeros[i] = 0.0;
    
    mBufFftChunk.Add(zeros, mBufSize);
    mBufIfftChunk.Add(zeros, mBufSize);
    // Keep one more mBufferSize, to store remaining fft waves
    mResultChunk.Add(zeros, mBufSize*2 + mOverlap*2);
    
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
}

FftProcessObj3::~FftProcessObj3() {}

void
FftProcessObj3::AddSamples(double *samples, int numSamples)
{
    mSamplesChunk.Add(samples, numSamples);
}

void
FftProcessObj3::ProcessFftBuffer(WDL_TypedBuf<WDL_FFT_COMPLEX> *ioBuffer) {}

void
FftProcessObj3::ProcessSamplesBuffer(WDL_TypedBuf<double> *ioBuffer) {}

void
FftProcessObj3::ProcessOneBuffer()
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
                samples[i + mBufOffset] *= mAnalysisWindow.Get()[i];
        }
    }
    
    // fft
    
    double coeff = 1.0;
    
    if (mNormalize)
        coeff = 1.0/mBufSize;
    
    // in and out for ffts
    float *input = (float *)valloc(2 * mBufSize * sizeof(float));
    float *output = (float *)valloc(2 * mBufSize * sizeof(float));
    
    // Fill the fft buf
    for (int i = 0; i < mBufSize; i++)
    {
        input[2*i + 0] = (float)samples[i + mBufOffset]*coeff;
        input[2*i + 1] = 0.0f;
    }
    
    ffts_plan_t *p0 = ffts_init_1d(mBufSize, FFTS_FORWARD);
    
    // Do the fft
    ffts_execute(p0, input, output);
    
    //mSortedFftBuf.Resize(0);
    for (int i = 0; i < mBufSize; i++)
    {
        WDL_FFT_COMPLEX comp;
        comp.re = output[i*2];
        comp.im = output[i*2 + 1];
        
        //mSortedFftBuf.Add(comp);
        mSortedFftBuf.Get()[i] = comp;
    }
    
    ffts_free(p0);
    
#if 0
    Debug::DumpComplexData("buf-magn.txt", "buf-phase.txt", mSortedFftBuf.Get(), mSortedFftBuf.GetSize());
    Debug::DumpRawComplexData("buf-re.txt", "buf-imag.txt", mSortedFftBuf.Get(), mSortedFftBuf.GetSize());
    
    Debug::DumpData("input.txt", samples, mBufSize);
    
    exit(0);
#endif
    
    // Apply modifications of the buffer
    ProcessFftBuffer(&mSortedFftBuf);
    
    ffts_plan_t *p1 = ffts_init_1d(mBufSize, FFTS_BACKWARD);
    
    mFftBuf.Resize(0);
    for (int i = 0; i < mBufSize; i++)
    {
        WDL_FFT_COMPLEX comp = mSortedFftBuf.Get()[i];
        
        input[i*2] = comp.re;
        input[i*2 + 1] = comp.im;
    }
    
    ffts_execute(p1, input, output);
    
    for (int i = 0; i < mBufSize; i++)
    {
        // Take only the real part
        mResultTmpChunk.Get()[i] = output[i*2];
    }
    
    ffts_free(p1);
    
    ProcessSamplesBuffer(&mResultTmpChunk);
    
    // Fill the result
    double *result = mResultChunk.Get();
    
    // We must use the same window for synthesis than for analysis, but for synthesis, we take the root
    // See: https://www.dsprelated.com/freebooks/sasp/overlap_add_ola_stft_processing.html
    for (int i = 0; i < mBufSize; i++)
        // Finally, no need to multiply by Hanning, because the data is already multiplied (at the beginning)
        // So simply overlap.
    {
        // Default value, for no window (SynthesisMethodNone)
        double winCoeff = 1.0;
        
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
    
    free(input);
    free(output);
}

void
FftProcessObj3::NextSamplesBuffer()
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
FftProcessObj3::NextOutBuffer()
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
FftProcessObj3::GetResultOutBuffer(double *output, int nFrames)
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
FftProcessObj3::Shift()
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
FftProcessObj3::Process(double *input, double *output, int nFrames)
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
